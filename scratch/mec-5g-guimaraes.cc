#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-json.hpp"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traci-module.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Mec5gGuimaraes");

namespace
{

struct GnbSite
{
    std::string openCellId;
    std::string radio;
    uint32_t mcc;
    uint32_t mnc;
    Vector position;
    Ptr<NetDevice> device;
};

struct UeContext
{
    Ptr<Node> node;
    Ptr<NetDevice> device;
    uint64_t imsi{0};
    std::string vehicleId;
    std::string vehicleType;
    uint16_t servingCellId{0};
    bool attached{false};
    ApplicationContainer applications;
};

std::map<uint32_t, UeContext> g_ueContexts;
std::map<uint64_t, uint32_t> g_imsiToNode;
std::map<uint64_t, uint16_t> g_servingCell;
std::map<uint32_t, uint32_t> g_nodeToPoolIndex;
std::vector<uint32_t> g_freeUeIndices;
std::vector<GnbSite> g_gnbSites;
Ptr<TraciClient> g_traciClient;
Ptr<Node> g_mecNode;
Ipv4Address g_mecAddress;
Ptr<UdpServer> g_busServer;
Ptr<UdpServer> g_carServer;
Ptr<UdpServer> g_bicycleServer;
std::ofstream g_vehicleEvents;

std::string
CsvEscape(const std::string& value)
{
    std::string escaped = value;
    size_t position = 0;
    while ((position = escaped.find('"', position)) != std::string::npos)
    {
        escaped.insert(position, 1, '"');
        position += 2;
    }
    return '"' + escaped + '"';
}

std::string
VehicleForImsi(uint64_t imsi)
{
    auto mapping = g_imsiToNode.find(imsi);
    if (mapping == g_imsiToNode.end())
    {
        return {};
    }
    auto context = g_ueContexts.find(mapping->second);
    return context == g_ueContexts.end() ? std::string{} : context->second.vehicleId;
}

void
LogEvent(const std::string& event,
         const std::string& vehicleId,
         const std::string& vehicleType,
         uint32_t nodeId,
         uint64_t imsi,
         uint16_t cellId,
         uint16_t rnti,
         const std::string& detail)
{
    if (!g_vehicleEvents.is_open())
    {
        return;
    }
    g_vehicleEvents << Simulator::Now().GetSeconds() << ',' << CsvEscape(event) << ','
                    << CsvEscape(vehicleId) << ',' << CsvEscape(vehicleType) << ',' << nodeId << ','
                    << imsi << ',' << cellId << ',' << rnti << ',' << CsvEscape(detail) << '\n';
    g_vehicleEvents.flush();
}

void
ConnectionEstablished(std::string tracePath, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    (void)tracePath;
    g_servingCell[imsi] = cellId;
    auto mapping = g_imsiToNode.find(imsi);
    uint32_t nodeId = mapping == g_imsiToNode.end() ? 0 : mapping->second;
    std::string vehicleId = VehicleForImsi(imsi);
    std::string type;
    auto context = g_ueContexts.find(nodeId);
    if (context != g_ueContexts.end())
    {
        context->second.servingCellId = cellId;
        type = context->second.vehicleType;
    }
    LogEvent("RRC_CONNECTED", vehicleId, type, nodeId, imsi, cellId, rnti, "UE attached to gNB");
    NS_LOG_UNCOND("RRC_CONNECTED imsi=" << imsi << " cellId=" << cellId << " rnti=" << rnti
                                          << " vehicle=" << vehicleId);
}

void
HandoverStarted(std::string tracePath,
                uint64_t imsi,
                uint16_t cellId,
                uint16_t rnti,
                uint16_t targetCellId)
{
    (void)tracePath;
    uint32_t nodeId = g_imsiToNode.count(imsi) ? g_imsiToNode.at(imsi) : 0;
    auto context = g_ueContexts.find(nodeId);
    std::string vehicleId = VehicleForImsi(imsi);
    std::string type = context == g_ueContexts.end() ? "" : context->second.vehicleType;
    LogEvent("HANDOVER_START", vehicleId, type, nodeId, imsi, cellId, rnti,
             "target_cell=" + std::to_string(targetCellId));
}

void
HandoverCompleted(std::string tracePath, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    (void)tracePath;
    g_servingCell[imsi] = cellId;
    uint32_t nodeId = g_imsiToNode.count(imsi) ? g_imsiToNode.at(imsi) : 0;
    auto context = g_ueContexts.find(nodeId);
    std::string vehicleId = VehicleForImsi(imsi);
    std::string type = context == g_ueContexts.end() ? "" : context->second.vehicleType;
    if (context != g_ueContexts.end())
    {
        context->second.servingCellId = cellId;
    }
    LogEvent("HANDOVER_END", vehicleId, type, nodeId, imsi, cellId, rnti, "successful");
}

Ptr<NetDevice>
FindClosestGnb(const Vector& uePosition)
{
    double closestDistance = std::numeric_limits<double>::max();
    Ptr<NetDevice> closestDevice;
    for (const auto& site : g_gnbSites)
    {
        Ptr<MobilityModel> mobility = site.device->GetNode()->GetObject<MobilityModel>();
        double distance = CalculateDistance(uePosition, mobility->GetPosition());
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestDevice = site.device;
        }
    }
    return closestDevice;
}

std::string
ClassifyVehicle(const std::string& typeId)
{
    std::string lowered = typeId;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (lowered.find("bus") != std::string::npos || lowered.find("coach") != std::string::npos)
    {
        return "bus";
    }
    if (lowered.find("bicycle") != std::string::npos || lowered.find("bike") != std::string::npos)
    {
        return "bicycle";
    }
    return "car";
}

void
ActivateVehicleTraffic(Ptr<TraciClient> client,
                       Ptr<NrHelper> nrHelper,
                       Ptr<NrPointToPointEpcHelper> epcHelper,
                       Ptr<Node> node,
                       double duration)
{
    const uint32_t nodeId = node->GetId();
    auto contextIterator = g_ueContexts.find(nodeId);
    NS_ABORT_MSG_IF(contextIterator == g_ueContexts.end(), "UE pool returned an unknown node");
    UeContext& context = contextIterator->second;
    context.vehicleId = client->GetVehicleId(node);
    NS_ABORT_MSG_IF(context.vehicleId.empty(), "TraCI has not registered the SUMO vehicle for a new UE");
    context.vehicleType = client->vehicle.getTypeID(context.vehicleId);
    const std::string vehicleClass = ClassifyVehicle(context.vehicleType);

    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Ptr<NetDevice> servingGnb = FindClosestGnb(mobility->GetPosition());
    NS_ABORT_MSG_IF(!servingGnb, "No gNB available for UE attachment");
    const uint16_t targetCellId = DynamicCast<NrGnbNetDevice>(servingGnb)->GetCellId();
    if (!context.attached)
    {
        nrHelper->AttachToGnb(context.device, servingGnb);
        context.attached = true;
        context.servingCellId = targetCellId;
    }
    else if (context.servingCellId != 0 && context.servingCellId != targetCellId)
    {
        auto source = std::find_if(g_gnbSites.begin(), g_gnbSites.end(), [&](const GnbSite& site) {
            return DynamicCast<NrGnbNetDevice>(site.device)->GetCellId() == context.servingCellId;
        });
        if (source != g_gnbSites.end())
        {
            nrHelper->HandoverRequest(MilliSeconds(50), context.device, source->device, targetCellId);
            LogEvent("HANDOVER_REQUEST", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                     context.servingCellId, 0, "target_cell=" + std::to_string(targetCellId));
        }
    }

    const uint16_t port = vehicleClass == "bus" ? 9001 : (vehicleClass == "bicycle" ? 9003 : 9002);
    ApplicationContainer applications;
    if (vehicleClass == "bus")
    {
        OnOffHelper video("ns3::UdpSocketFactory", InetSocketAddress(g_mecAddress, port));
        video.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
        video.SetAttribute("PacketSize", UintegerValue(1200));
        video.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        video.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        applications = video.Install(node);
    }
    else
    {
        UdpClientHelper telemetry(g_mecAddress, port);
        telemetry.SetAttribute("MaxPackets", UintegerValue(static_cast<uint32_t>(duration * 10.0 + 10.0)));
        telemetry.SetAttribute("Interval",
                               TimeValue(vehicleClass == "bicycle" ? Seconds(1.0) : MilliSeconds(100)));
        telemetry.SetAttribute("PacketSize", UintegerValue(vehicleClass == "bicycle" ? 100 : 300));
        applications = telemetry.Install(node);
    }
    applications.Start(Simulator::Now() + MilliSeconds(5));
    applications.Stop(Seconds(duration));
    context.applications = applications;
    if (g_servingCell.count(context.imsi))
    {
        LogEvent("SUMO_BIND", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                 g_servingCell.at(context.imsi), 0, "v2x_profile=" + vehicleClass);
    }
    else
    {
        LogEvent("SUMO_BIND", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                 0, 0, "v2x_profile=" + vehicleClass + "; awaiting RRC connection");
    }
    NS_LOG_UNCOND("UE_BIND vehicle=" << context.vehicleId << " type=" << context.vehicleType
                                     << " class=" << vehicleClass << " node=" << nodeId
                                     << " imsi=" << context.imsi << " target_gnb=" << targetCellId
                                     << " serving_gnb=" << context.servingCellId);
}

void
StopVehicleTraffic(Ptr<Node> node)
{
    auto contextIterator = g_ueContexts.find(node->GetId());
    if (contextIterator == g_ueContexts.end())
    {
        return;
    }
    UeContext& context = contextIterator->second;
    LogEvent("SUMO_ARRIVE", context.vehicleId, context.vehicleType, node->GetId(), context.imsi,
             g_servingCell.count(context.imsi) ? g_servingCell.at(context.imsi) : 0, 0,
             "applications stopped; UE remains in preallocated ns-3 pool");
    if (context.applications.GetN() > 0)
    {
        context.applications.Stop(Simulator::Now());
        context.applications = ApplicationContainer();
    }
    context.vehicleId.clear();
    context.vehicleType.clear();
    node->GetObject<MobilityModel>()->SetPosition(Vector(-10000.0, -10000.0, 1.5));
    auto poolIndex = g_nodeToPoolIndex.find(node->GetId());
    if (poolIndex != g_nodeToPoolIndex.end())
    {
        g_freeUeIndices.push_back(poolIndex->second);
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string sumoConfig = "mobility/guimaraes/guimaraes.sumocfg";
    std::string gnbPositions = "mobility/guimaraes/gnb_positions.json";
    std::string outputDirectory = "results/raw_data";
    double duration = 120.0;
    uint32_t maxUes = 256;
    double sumoStartTime = 25200.0;

    CommandLine command(__FILE__);
    command.AddValue("sumoConfig", "SUMO configuration to run through TraCI", sumoConfig);
    command.AddValue("gnbPositions", "OpenCellID positions projected into SUMO XY", gnbPositions);
    command.AddValue("outputDirectory", "Directory for CSV and native NR trace outputs", outputDirectory);
    command.AddValue("duration", "ns-3 runtime in seconds", duration);
    command.AddValue("maxUes", "Preallocated UE pool capacity for TraCI vehicle mapping", maxUes);
    command.AddValue("sumoStartTime", "SUMO absolute begin time in seconds", sumoStartTime);
    command.Parse(argc, argv);

    NS_ABORT_MSG_IF(duration <= 0.0, "Simulation duration must be positive");
    NS_ABORT_MSG_IF(maxUes == 0, "The UE pool cannot be empty");

    const std::filesystem::path absoluteSumoConfig = std::filesystem::absolute(sumoConfig);
    const std::filesystem::path absoluteGnbPositions = std::filesystem::absolute(gnbPositions);
    const std::filesystem::path absoluteOutputDirectory = std::filesystem::absolute(outputDirectory);
    std::filesystem::create_directories(absoluteOutputDirectory);

    std::ifstream positionsFile(absoluteGnbPositions);
    NS_ABORT_MSG_IF(!positionsFile, "Cannot open OpenCellID positions file: " << absoluteGnbPositions);
    nlohmann::json positionsJson;
    positionsFile >> positionsJson;
    NS_ABORT_MSG_IF(!positionsJson.contains("gnbs") || !positionsJson["gnbs"].is_array(),
                    "gnb_positions.json must contain a 'gnbs' array");
    NS_ABORT_MSG_IF(positionsJson["gnbs"].empty(), "No OpenCellID NOS LTE/NR positions were supplied");

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(100)));
    Config::SetDefault("ns3::NrBearerStatsCalculator::DlRlcOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_rlc_dl.tsv").string()));
    Config::SetDefault("ns3::NrBearerStatsCalculator::UlRlcOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_rlc_ul.tsv").string()));
    Config::SetDefault("ns3::NrBearerStatsCalculator::DlPdcpOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_pdcp_dl.tsv").string()));
    Config::SetDefault("ns3::NrBearerStatsCalculator::UlPdcpOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_pdcp_ul.tsv").string()));
    Config::SetDefault("ns3::NrMacSchedulingStats::DlOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_mac_dl.tsv").string()));
    Config::SetDefault("ns3::NrMacSchedulingStats::UlOutputFilename",
                       StringValue((absoluteOutputDirectory / "nr_mac_ul.tsv").string()));

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    auto mec = epcHelper->SetupRemoteHost(std::optional<std::string>{"10Gbps"},
                                          std::nullopt,
                                          std::optional<Time>{MilliSeconds(1)});
    g_mecNode = mec.first;
    g_mecAddress = mec.second;

    Ptr<IdealBeamformingHelper> beamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    nrHelper->SetBeamformingHelper(beamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);
    beamformingHelper->SetAttribute("BeamformingMethod",
                                    TypeIdValue(DirectPathBeamforming::GetTypeId()));
    nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(46.0));

    channelHelper->ConfigureFactories("UMa", "Default", "ThreeGpp");
    channelHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(100)));
    channelHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(true));
    CcBwpCreator bwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConfiguration(3.5e9, 40e6, 1);
    OperationBandInfo band = bwpCreator.CreateOperationBandContiguousCc(bandConfiguration);
    channelHelper->AssignChannelsToBands({band});
    BandwidthPartInfoPtrVector bwps = CcBwpCreator::GetAllBwps({band});

    NodeContainer gnbNodes;
    for (const auto& record : positionsJson["gnbs"])
    {
        Ptr<Node> gnb = CreateObject<Node>();
        gnbNodes.Add(gnb);
        GnbSite site;
        site.openCellId = record.at("cell_id").get<std::string>();
        site.radio = record.at("radio").get<std::string>();
        site.mcc = record.at("mcc").get<uint32_t>();
        site.mnc = record.at("mnc").get<uint32_t>();
        site.position = Vector(record.at("x").get<double>(),
                               record.at("y").get<double>(),
                               record.at("z").get<double>());
        g_gnbSites.push_back(site);
    }
    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.Install(gnbNodes);
    for (uint32_t index = 0; index < gnbNodes.GetN(); ++index)
    {
        gnbNodes.Get(index)->GetObject<MobilityModel>()->SetPosition(g_gnbSites[index].position);
    }
    NetDeviceContainer gnbDevices = nrHelper->InstallGnbDevice(gnbNodes, bwps);
    nrHelper->AssignStreams(gnbDevices, 1);
    for (uint32_t index = 0; index < gnbDevices.GetN(); ++index)
    {
        g_gnbSites[index].device = gnbDevices.Get(index);
    }

    NodeContainer uePool;
    uePool.Create(maxUes);
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMobility.Install(uePool);
    for (uint32_t index = 0; index < uePool.GetN(); ++index)
    {
        uePool.Get(index)->GetObject<MobilityModel>()->SetPosition(Vector(-10000.0, -10000.0, 1.5));
    }
    InternetStackHelper internet;
    internet.Install(uePool);
    NetDeviceContainer ueDevices = nrHelper->InstallUeDevice(uePool, bwps);
    nrHelper->AssignStreams(ueDevices, 100);
    for (uint32_t index = 0; index < ueDevices.GetN(); ++index)
    {
        NrHelper::GetUePhy(ueDevices.Get(index), 0)->SetNumerology(1);
    }
    epcHelper->AssignUeIpv4Address(ueDevices);

    std::filesystem::create_directories(absoluteOutputDirectory);
    g_vehicleEvents.open(absoluteOutputDirectory / "vehicle_events.csv");
    NS_ABORT_MSG_IF(!g_vehicleEvents.is_open(), "Cannot open vehicle_events.csv in results directory");
    g_vehicleEvents << "time_s,event,vehicle_id,vehicle_type,node_id,imsi,nr_cell_id,rnti,details\n";

    std::ofstream gnbCatalog(absoluteOutputDirectory / "gnb_catalog.csv");
    gnbCatalog << "nr_cell_id,opencellid_cell_id,radio,mcc,mnc,x_m,y_m,z_m\n";
    for (const auto& site : g_gnbSites)
    {
        auto gnbDevice = DynamicCast<NrGnbNetDevice>(site.device);
        gnbCatalog << gnbDevice->GetCellId() << ',' << CsvEscape(site.openCellId) << ',' << site.radio << ','
                   << site.mcc << ',' << site.mnc << ',' << site.position.x << ',' << site.position.y << ','
                   << site.position.z << '\n';
    }

    UdpServerHelper busServerHelper(9001);
    UdpServerHelper carServerHelper(9002);
    UdpServerHelper bicycleServerHelper(9003);
    ApplicationContainer busServerApps = busServerHelper.Install(g_mecNode);
    ApplicationContainer carServerApps = carServerHelper.Install(g_mecNode);
    ApplicationContainer bicycleServerApps = bicycleServerHelper.Install(g_mecNode);
    busServerApps.Start(Seconds(0));
    carServerApps.Start(Seconds(0));
    bicycleServerApps.Start(Seconds(0));
    busServerApps.Stop(Seconds(duration));
    carServerApps.Stop(Seconds(duration));
    bicycleServerApps.Stop(Seconds(duration));
    g_busServer = DynamicCast<UdpServer>(busServerApps.Get(0));
    g_carServer = DynamicCast<UdpServer>(carServerApps.Get(0));
    g_bicycleServer = DynamicCast<UdpServer>(bicycleServerApps.Get(0));

    for (uint32_t index = 0; index < uePool.GetN(); ++index)
    {
        Ptr<Node> node = uePool.Get(index);
        Ptr<NrUeNetDevice> ueDevice = DynamicCast<NrUeNetDevice>(ueDevices.Get(index));
        NS_ABORT_MSG_IF(!ueDevice, "UE pool device is not an NrUeNetDevice");
        UeContext context;
        context.node = node;
        context.device = ueDevice;
        context.imsi = ueDevice->GetImsi();
        g_imsiToNode[context.imsi] = node->GetId();
        g_ueContexts[node->GetId()] = context;
        g_nodeToPoolIndex[node->GetId()] = index;
        g_freeUeIndices.push_back(uePool.GetN() - index - 1);

        std::ostringstream tracePath;
        tracePath << "/NodeList/" << node->GetId() << "/DeviceList/" << ueDevice->GetIfIndex()
                  << "/NrUeRrc/";
        Config::Connect(tracePath.str() + "ConnectionEstablished", MakeCallback(&ConnectionEstablished));
        Config::Connect(tracePath.str() + "HandoverStart", MakeCallback(&HandoverStarted));
        Config::Connect(tracePath.str() + "HandoverEndOk", MakeCallback(&HandoverCompleted));
    }

    Ptr<NrPhyRxTrace> phyTrace = nrHelper->GetPhyRxTrace();
    phyTrace->SetResultsFolder(absoluteOutputDirectory.string() + "/");
    phyTrace->SetSimTag("mec-guimaraes");
    nrHelper->EnableDlDataPhyTraces();
    nrHelper->EnableUlPhyTraces();
    nrHelper->EnableRlcE2eTraces();
    nrHelper->EnableDlMacSchedTraces();
    nrHelper->EnableUlMacSchedTraces();

    g_traciClient = CreateObject<TraciClient>();
    g_traciClient->SetAttribute("SumoConfigPath", StringValue(absoluteSumoConfig.string()));
    g_traciClient->SetAttribute("SumoBinaryPath", StringValue(""));
    g_traciClient->SetAttribute("SynchInterval", TimeValue(MilliSeconds(100)));
    g_traciClient->SetAttribute("StartTime", TimeValue(Seconds(sumoStartTime)));
    g_traciClient->SetAttribute("SumoGUI", BooleanValue(false));
    g_traciClient->SetAttribute("SumoPort", UintegerValue(3400));
    g_traciClient->SetAttribute("PenetrationRate", DoubleValue(1.0));
    g_traciClient->SetAttribute("SumoLogFile", BooleanValue(true));
    g_traciClient->SetAttribute("SumoStepLog", BooleanValue(false));
    g_traciClient->SetAttribute("SumoSeed", IntegerValue(42));
    g_traciClient->SetAttribute("SumoAdditionalCmdOptions", StringValue(""));
    g_traciClient->SetAttribute("SumoWaitForSocket", TimeValue(Seconds(1.0)));

    std::function<Ptr<Node>()> includeNode = [&]() -> Ptr<Node> {
        NS_ABORT_MSG_IF(g_freeUeIndices.empty(),
                        "UE pool exhausted; increase --maxUes or the peak concurrency margin");
        const uint32_t poolIndex = g_freeUeIndices.back();
        g_freeUeIndices.pop_back();
        Ptr<Node> node = uePool.Get(poolIndex);
        Simulator::ScheduleNow(&ActivateVehicleTraffic,
                               g_traciClient,
                               nrHelper,
                               epcHelper,
                               node,
                               duration);
        return node;
    };
    std::function<void(Ptr<Node>)> excludeNode = [](Ptr<Node> node) { StopVehicleTraffic(node); };

    NS_LOG_UNCOND("NR configuration: band n78 3.5GHz, 40MHz, numerology 1, UMa/3GPP TR 38.901, channel update 100ms");
    NS_LOG_UNCOND("MEC server " << g_mecAddress << " via PGW: 10Gbps / 1ms");
    NS_LOG_UNCOND("OpenCellID NOS logical cells loaded as gNB candidates: " << g_gnbSites.size());
    NS_LOG_UNCOND("Starting SUMO/TraCI for " << duration << " s with 100ms synchronization");
    g_traciClient->SumoSetup(includeNode, excludeNode);

    Simulator::Stop(Seconds(duration));
    Simulator::Run();

    Simulator::Destroy();
    // The TraciClient destructor closes the TraCI socket.
    g_traciClient = nullptr;

    NS_LOG_UNCOND("MEC_RX bus_packets=" << g_busServer->GetReceived()
                                         << " car_packets=" << g_carServer->GetReceived()
                                         << " bicycle_packets=" << g_bicycleServer->GetReceived());
    LogEvent("MEC_RX_SUMMARY", "", "", 0, 0, 0, 0,
             "bus=" + std::to_string(g_busServer->GetReceived()) +
                 ";car=" + std::to_string(g_carServer->GetReceived()) +
                 ";bicycle=" + std::to_string(g_bicycleServer->GetReceived()));
    g_vehicleEvents.close();
    return 0;
}