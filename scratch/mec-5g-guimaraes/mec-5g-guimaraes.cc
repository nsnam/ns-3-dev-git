#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-json.hpp"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traci-module.h"
#include "mec-orchestrator.h"
#include "simulation-logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
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

struct MecServer
{
    std::string id;
    std::string zone;
    Vector position;
    uint32_t vcpus{0};
    uint32_t ramGb{0};
    uint64_t dataRateBps{0};
    Time backhaulDelay;
    std::string network;
    std::string mask;
    std::string addressBase;
    std::string pgwAddress;
    std::string serverAddress;
    std::vector<std::string> primaryGnbCellIds;
    Ptr<Node> node;
    Ptr<PacketSink> udpSink;
    Ipv4Address ipv4Address;
};

struct UeContext
{
    Ptr<Node> node;
    Ptr<NetDevice> device;
    uint64_t imsi{0};
    Ipv4Address ipv4Address;
    std::string vehicleId;
    std::string vehicleType;
    uint16_t servingCellId{0};
    uint32_t currentMecNodeId{0};
    uint64_t txPackets{0};
    uint64_t rxPackets{0};
    double sinrDb{std::numeric_limits<double>::quiet_NaN()};
    double lastLatencyMs{std::numeric_limits<double>::quiet_NaN()};
    bool attached{false};
    ApplicationContainer applications;
    bool is5gEnabled{true};
};

std::map<uint32_t, UeContext> g_ueContexts;
std::map<uint64_t, uint32_t> g_imsiToNode;
std::map<uint64_t, uint16_t> g_servingCell;
std::map<uint32_t, uint32_t> g_nodeToPoolIndex;
std::vector<uint32_t> g_freeUeIndices;
std::vector<GnbSite> g_gnbSites;
Ptr<TraciClient> g_traciClient;
std::ofstream g_vehicleEvents;
std::ofstream g_liveMetrics;

// Quota tracking
uint32_t g_maxCars = 10000;
uint32_t g_maxBuses = 10000;
uint32_t g_maxBicycles = 10000;
std::map<std::string, uint32_t> g_activeVehiclesByClass;

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

class MecTrafficManager;

void
MecPacketReceived(MecTrafficManager* manager,
                  uint32_t mecNodeId,
                  Ptr<const Packet> packet,
                  const Address& from,
                  const Address& local);

void
TrafficSourceTx(uint32_t nodeId, Ptr<const Packet> packet)
{
    (void)packet;
    auto context = g_ueContexts.find(nodeId);
    if (context != g_ueContexts.end())
    {
        ++context->second.txPackets;
    }
}

std::string ClassifyVehicle(const std::string& typeId);
Ptr<NetDevice> FindClosestGnb(const Vector& uePosition);

class MecTrafficManager
{
  public:
    void
    Configure(std::vector<MecServer> servers, uint16_t port)
    {
        m_servers = std::move(servers);
        m_port = port;
    }

    void
    InstallPacketSinks(double duration, uint16_t tcpPort)
    {
        for (auto& server : m_servers)
        {
            PacketSinkHelper udpSink("ns3::UdpSocketFactory",
                                     InetSocketAddress(Ipv4Address::GetAny(), m_port));
            ApplicationContainer udpApps = udpSink.Install(server.node);
            udpApps.Start(Seconds(0));
            udpApps.Stop(Seconds(duration));
            server.udpSink = DynamicCast<PacketSink>(udpApps.Get(0));
            NS_ABORT_MSG_IF(!server.udpSink, "Failed to create MEC UDP PacketSink");
            server.udpSink->TraceConnectWithoutContext(
                "RxWithAddresses",
                MakeBoundCallback(&MecPacketReceived, this, server.node->GetId()));

            PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
                                     InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
            ApplicationContainer tcpApps = tcpSink.Install(server.node);
            tcpApps.Start(Seconds(0));
            tcpApps.Stop(Seconds(duration));
        }
    }

    uint32_t
    FindClosestMec(const Vector& position) const
    {
        NS_ABORT_MSG_IF(m_servers.empty(), "No MEC servers are configured");
        const MecServer* closest = &m_servers.front();
        double closestDistance = CalculateDistance(position, closest->position);
        for (const auto& server : m_servers)
        {
            const double distance = CalculateDistance(position, server.position);
            if (distance < closestDistance)
            {
                closest = &server;
                closestDistance = distance;
            }
        }
        return closest->node->GetId();
    }

    uint32_t
    FindPrimaryMec(const std::string& gnbCellId, const Vector& gnbPosition) const
    {
        for (const auto& server : m_servers)
        {
            if (std::find(server.primaryGnbCellIds.begin(),
                          server.primaryGnbCellIds.end(),
                          gnbCellId) != server.primaryGnbCellIds.end())
            {
                return server.node->GetId();
            }
        }
        return FindClosestMec(gnbPosition);
    }

    void
    RegisterUe(Ptr<Node> node,
               Ipv4Address ueAddress,
               const std::string& vehicleClass,
               uint32_t initialMecNodeId,
               double duration)
    {
        m_duration = duration;
        const uint32_t nodeId = node->GetId();
        UeContext& context = g_ueContexts.at(nodeId);
        const MecServer& server = GetServer(initialMecNodeId);

        Time startTime = Simulator::Now() + MilliSeconds(5);
        Time stopTime = Seconds(duration);
        ApplicationContainer applications =
            CreateUeApplication(node, vehicleClass, server.ipv4Address, startTime, stopTime);

        context.ipv4Address = ueAddress;
        context.currentMecNodeId = initialMecNodeId;
        context.applications = applications;
        m_ueByAddress[ueAddress.Get()] = nodeId;
    }

    void
    ForgetUe(uint32_t ueNodeId)
    {
        auto context = g_ueContexts.find(ueNodeId);
        if (context != g_ueContexts.end())
        {
            context->second.currentMecNodeId = 0;
        }
    }

    void
    SwitchUeMecDestination(uint32_t ueNodeId, uint32_t targetMecNodeId)
    {
        auto contextIterator = g_ueContexts.find(ueNodeId);
        NS_ABORT_MSG_IF(contextIterator == g_ueContexts.end(),
                        "Cannot migrate a UE without an active context");
        
        UeContext& context = contextIterator->second;
        if (context.currentMecNodeId == targetMecNodeId)
        {
            return;
        }

        const MecServer& target = GetServer(targetMecNodeId);
        const std::string oldMec = GetServer(context.currentMecNodeId).id;

        // 1. Stop existing application
        if (context.applications.GetN() > 0)
        {
            context.applications.Stop(Simulator::Now());
            context.applications = ApplicationContainer();
        }

        // 2. Install new application with StartTime configured on helper
        const std::string vehicleClass = ClassifyVehicle(context.vehicleType);
        Time startTime = Simulator::Now() + MilliSeconds(1);
        Time stopTime = Seconds(m_duration);
        ApplicationContainer newApps =
            CreateUeApplication(context.node, vehicleClass, target.ipv4Address, startTime, stopTime);

        // 3. Update context
        context.applications = newApps;
        context.currentMecNodeId = targetMecNodeId;

        std::ostringstream destination;
        destination << target.ipv4Address;
        const std::string details = "from=" + oldMec + ";to=" + target.id + ";ip=" + destination.str();
        LogEvent("MEC_SWITCH",
                 context.vehicleId,
                 context.vehicleType,
                 ueNodeId,
                 context.imsi,
                 context.servingCellId,
                 0,
                 details);
        NS_LOG_UNCOND("MEC_SWITCH time=" << Simulator::Now().GetSeconds()
                                          << " ue=" << context.vehicleId << " from="
                                          << oldMec << " to=" << target.id << " destination="
                                          << target.ipv4Address);
    }

    uint32_t
    FindMecNodeById(const std::string& mecId) const
    {
        auto server = std::find_if(m_servers.begin(), m_servers.end(), [&](const MecServer& item) {
            return item.id == mecId;
        });
        NS_ABORT_MSG_IF(server == m_servers.end(), "Unknown MEC id " << mecId);
        return server->node->GetId();
    }

    void
    PrintSummary() const
    {
        for (const auto& server : m_servers)
        {
            NS_LOG_UNCOND("MEC_RX server=" << server.id << " bytes="
                                            << server.udpSink->GetTotalRx());
        }
    }

    void
    DemoMigrationFromMec0()
    {
        const uint32_t mec0 = FindMecNodeById("MEC_0");
        const uint32_t mec1 = FindMecNodeById("MEC_1");
        auto ue = std::find_if(g_ueContexts.begin(), g_ueContexts.end(), [&](const auto& entry) {
            return !entry.second.vehicleId.empty() && 
                   entry.second.currentMecNodeId == mec0 &&
                   entry.second.applications.GetN() > 0; // Substituído m_sources por verificação de aplicações ativas
        });
        if (ue == g_ueContexts.end())
        {
            NS_LOG_UNCOND("MEC_DEMO skipped: no active UE is currently routed to MEC_0");
            return;
        }
        SwitchUeMecDestination(ue->first, mec1);
    }

    void
    EvaluateMobility(double hysteresisMeters, Ptr<NrHelper> nrHelper = nullptr, bool doSpatialRouting = true)
    {
        for (auto& [nodeId, context] : g_ueContexts)
        {
            if (context.vehicleId.empty() || context.currentMecNodeId == 0)
            {
                continue;
            }

            const Vector uePosition = context.node->GetObject<MobilityModel>()->GetPosition();

            // 0. gNB Handover Evaluation
            if (nrHelper)
            {
                Ptr<NetDevice> closestGnb = FindClosestGnb(uePosition);
                if (closestGnb)
                {
                    const auto gnbDevice = DynamicCast<NrGnbNetDevice>(closestGnb);
                    const uint16_t targetCellId = gnbDevice->GetCellId();
                    
                    if (context.servingCellId != 0 && context.servingCellId != targetCellId)
                    {
                        auto source = std::find_if(g_gnbSites.begin(), g_gnbSites.end(), [&](const GnbSite& site) {
                            return DynamicCast<NrGnbNetDevice>(site.device)->GetCellId() == context.servingCellId;
                        });
                        
                        if (source != g_gnbSites.end())
                        {
                            nrHelper->HandoverRequest(MilliSeconds(10), context.device, source->device, targetCellId);
                            LogEvent("HANDOVER_REQUEST", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                                     context.servingCellId, 0, "target_cell=" + std::to_string(targetCellId));
                            context.servingCellId = targetCellId;
                        }
                    }
                }
            }

            // 1. MEC IP Switch Evaluation
            if (doSpatialRouting)
            {
                const uint32_t closestNodeId = FindClosestMec(uePosition);
                if (closestNodeId != context.currentMecNodeId)
                {
                    const double currentDistance = CalculateDistance(uePosition, GetServer(context.currentMecNodeId).position);
                    const double candidateDistance = CalculateDistance(uePosition, GetServer(closestNodeId).position);
                    if (candidateDistance + hysteresisMeters < currentDistance)
                    {
                        SwitchUeMecDestination(nodeId, closestNodeId);
                    }
                }
            }
        }
    }

    void
    WriteMetrics()
    {
        if (!g_liveMetrics.is_open())
        {
            return;
        }
        for (const auto& [nodeId, context] : g_ueContexts)
        {
            if (context.vehicleId.empty() || context.currentMecNodeId == 0)
            {
                continue;
            }
            g_liveMetrics << Simulator::Now().GetSeconds() << ',' << CsvEscape(context.vehicleId)
                          << ',' << CsvEscape(context.vehicleType) << ',' << context.servingCellId
                          << ',' << CsvEscape(GetServer(context.currentMecNodeId).id);
            for (const auto& server : m_servers)
            {
                g_liveMetrics << ',' << GetLatencyEstimateMs(context, server);
            }
            const double packetLoss = context.txPackets == 0
                                          ? 0.0
                                          : static_cast<double>(context.txPackets -
                                                                std::min(context.txPackets,
                                                                         context.rxPackets)) /
                                                context.txPackets;
            g_liveMetrics << ',';
            if (std::isfinite(context.sinrDb))
            {
                g_liveMetrics << context.sinrDb;
            }
            g_liveMetrics << ',' << packetLoss << '\n';
        }
        g_liveMetrics.flush();
    }

    void
    OnPacketReceived(uint32_t mecNodeId,
                     Ptr<const Packet> packet,
                     const Address& from,
                     const Address& local)
    {
        (void)mecNodeId;
        (void)local;
        if (!InetSocketAddress::IsMatchingType(from))
        {
            return;
        }
        const Ipv4Address sourceAddress = InetSocketAddress::ConvertFrom(from).GetIpv4();
        auto ue = m_ueByAddress.find(sourceAddress.Get());
        if (ue == m_ueByAddress.end())
        {
            return;
        }
        auto context = g_ueContexts.find(ue->second);
        if (context == g_ueContexts.end())
        {
            return;
        }
        ++context->second.rxPackets;
        if (ClassifyVehicle(context->second.vehicleType) != "bus")
        {
            Ptr<Packet> copy = packet->Copy();
            SeqTsHeader header;
            if (copy->RemoveHeader(header) > 0)
            {
                context->second.lastLatencyMs =
                    (Simulator::Now() - header.GetTs()).GetMilliSeconds();
            }
        }
    }

  private:
    const MecServer&
    GetServer(uint32_t nodeId) const
    {
        auto server = std::find_if(m_servers.begin(), m_servers.end(), [nodeId](const MecServer& item) {
            return item.node->GetId() == nodeId;
        });
        NS_ABORT_MSG_IF(server == m_servers.end(), "Unknown MEC node id " << nodeId);
        return *server;
    }

    double
    GetLatencyEstimateMs(const UeContext& context, const MecServer& target) const
    {
        double commonAccessLatencyMs = 1.0;
        if (std::isfinite(context.lastLatencyMs) && context.currentMecNodeId != 0)
        {
            const double currentBackhaulMs =
                GetServer(context.currentMecNodeId).backhaulDelay.GetSeconds() * 1000.0;
            commonAccessLatencyMs = std::max(0.0, context.lastLatencyMs - currentBackhaulMs);
        }
        return commonAccessLatencyMs + target.backhaulDelay.GetSeconds() * 1000.0;
    }

  public:
    ApplicationContainer
    CreateUeApplication(Ptr<Node> node,
                         const std::string& vehicleClass,
                         Ipv4Address targetMecIp,
                         Time startTime,
                         Time stopTime)
    {
        Address remote = InetSocketAddress(targetMecIp, m_port);
        ApplicationContainer apps;

        if (vehicleClass == "bus")
        {
            OnOffHelper source("ns3::UdpSocketFactory", remote);
            source.SetAttribute("DataRate", DataRateValue(DataRate("2.5Mbps")));
            source.SetAttribute("PacketSize", UintegerValue(1024));
            source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            apps = source.Install(node);
        }
        else
        {
            UdpClientHelper source(targetMecIp, m_port);
            source.SetAttribute("MaxPackets", UintegerValue(0));
            source.SetAttribute("PacketSize", UintegerValue(vehicleClass == "bicycle" ? 100 : 300));
            source.SetAttribute("Interval",
                                TimeValue(vehicleClass == "bicycle" ? Seconds(1.0)
                                                                       : MilliSeconds(100)));
            apps = source.Install(node);
        }

        for (uint32_t i = 0; i < apps.GetN(); ++i)
        {
            apps.Get(i)->Initialize();
        }

        apps.Start(startTime);
        apps.Stop(stopTime);

        apps.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TrafficSourceTx, node->GetId()));
        return apps;
    }

    std::vector<MecServer> m_servers;
    uint16_t m_port{0};
    double m_duration{0.0};
    std::unordered_map<uint32_t, uint32_t> m_ueByAddress;
};    

void
MecPacketReceived(MecTrafficManager* manager,
                  uint32_t mecNodeId,
                  Ptr<const Packet> packet,
                  const Address& from,
                  const Address& local)
{
    manager->OnPacketReceived(mecNodeId, packet, from, local);
    InetSocketAddress srcAddr = InetSocketAddress::ConvertFrom(from);
    InetSocketAddress dstAddr = InetSocketAddress::ConvertFrom(local);
    uint32_t srcNodeId = SimulationLogger::GetNodeIdFromIp(srcAddr.GetIpv4());
    
    std::ostringstream details;
    details << "src_ip=" << srcAddr.GetIpv4() << ", src_node=" << srcNodeId << ", size=" << packet->GetSize() << "_bytes";
    std::ostringstream dstIpStream;
    dstIpStream << dstAddr.GetIpv4();
    SimulationLogger::Log("MEC_APP", "MEC_TRAFFIC_RECEIVED", mecNodeId, dstIpStream.str(), details.str());
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
    
    std::ostringstream details;
    details << "gnb_cell_id=" << cellId << ", rnti=" << rnti;
    SimulationLogger::Log("RAN_5G", "5G_CONNECTED", nodeId, "imsi_" + std::to_string(imsi), details.str());
    
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
             
    std::ostringstream details;
    details << "src_cell=" << cellId << ", tgt_cell=" << targetCellId;
    SimulationLogger::Log("RAN_5G", "5G_HANDOVER_START", nodeId, "imsi_" + std::to_string(imsi), details.str());
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
    
    std::ostringstream details;
    details << "new_cell=" << cellId << ", rnti=" << rnti;
    SimulationLogger::Log("RAN_5G", "5G_HANDOVER_COMPLETED", nodeId, "imsi_" + std::to_string(imsi), details.str());
    
    NS_LOG_UNCOND("GNB_CONNECTED event=handover imsi=" << imsi << " cellId=" << cellId
                                                        << " rnti=" << rnti
                                                        << " vehicle=" << vehicleId);
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
                       MecTrafficManager* mecTrafficManager,
                       Ptr<MecOrchestrator> orchestrator,
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

    // Enforce quotas
    uint32_t currentCount = g_activeVehiclesByClass[vehicleClass];
    bool quotaExceeded = false;
    if (vehicleClass == "car" && currentCount >= g_maxCars) quotaExceeded = true;
    else if (vehicleClass == "bus" && currentCount >= g_maxBuses) quotaExceeded = true;
    else if (vehicleClass == "bicycle" && currentCount >= g_maxBicycles) quotaExceeded = true;

    if (quotaExceeded)
    {
        context.is5gEnabled = false;
        return;
    }
    
    g_activeVehiclesByClass[vehicleClass]++;
    context.is5gEnabled = true;

    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();
    std::ostringstream posDetails;
    posDetails << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z;
    SimulationLogger::Log("MOBILITY", "NODE_CREATED", nodeId, context.vehicleId, posDetails.str());

    Ptr<NetDevice> servingGnb = FindClosestGnb(pos);
    NS_ABORT_MSG_IF(!servingGnb, "No gNB available for UE attachment");
    const auto gnbDevice = DynamicCast<NrGnbNetDevice>(servingGnb);
    const uint16_t targetCellId = gnbDevice->GetCellId();
    if (!context.attached)
    {
        nrHelper->AttachToGnb(context.device, servingGnb);
        context.attached = true;
        context.servingCellId = targetCellId;
    }

    auto entrySite = std::find_if(g_gnbSites.begin(), g_gnbSites.end(), [&](const GnbSite& site) {
        return site.device == servingGnb;
    });
    NS_ABORT_MSG_IF(entrySite == g_gnbSites.end(), "Serving gNB is missing from the site catalog");
    const uint32_t initialMecNodeId =
        mecTrafficManager->FindPrimaryMec(entrySite->openCellId, entrySite->position);
    context.txPackets = 0;
    context.rxPackets = 0;
    context.lastLatencyMs = std::numeric_limits<double>::quiet_NaN();
    mecTrafficManager->RegisterUe(node,
                                 context.ipv4Address,
                                 vehicleClass,
                                 initialMecNodeId,
                                 duration);
    // Providing generic demands: 4 Cores, 8GB RAM, 20Mbps BW, 5MB State Size
    orchestrator->RegisterUe(
        nodeId, 
        context.ipv4Address, 
        4.0, 8.0, 20.0, 
        5 * 1024 * 1024, // 5MB state size 
        node, 
        initialMecNodeId, 
        vehicleClass
    );                                 

    if (g_servingCell.count(context.imsi))
    {
        LogEvent("SUMO_BIND", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                 g_servingCell.at(context.imsi), 0, "v2x_profile=" + vehicleClass + ";initial_mec=" +
                                                       std::to_string(initialMecNodeId));
    }
    else
    {
        LogEvent("SUMO_BIND", context.vehicleId, context.vehicleType, nodeId, context.imsi,
                 0, 0, "v2x_profile=" + vehicleClass + ";initial_mec=" +
                           std::to_string(initialMecNodeId) + ";awaiting RRC connection");
    }
    NS_LOG_UNCOND("UE_BIND vehicle=" << context.vehicleId << " type=" << context.vehicleType
                                     << " class=" << vehicleClass << " node=" << nodeId
                                     << " imsi=" << context.imsi << " target_gnb=" << targetCellId
                                     << " serving_gnb=" << context.servingCellId << " initial_mec_node="
                                     << initialMecNodeId);
}

void
StopVehicleTraffic(Ptr<Node> node, 
                   MecTrafficManager* mecTrafficManager,
                   Ptr<MecOrchestrator> orchestrator)
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
             
    SimulationLogger::Log("MOBILITY", "NODE_DESTROYED", node->GetId(), context.vehicleId, "UE departed");
    
    if (context.applications.GetN() > 0)
    {
        context.applications.Stop(Simulator::Now());
        context.applications = ApplicationContainer();
    }
    orchestrator->RemoveUe(node->GetId());
    if (context.is5gEnabled)
    {
        const std::string vehicleClass = ClassifyVehicle(context.vehicleType);
        if (g_activeVehiclesByClass[vehicleClass] > 0)
        {
            g_activeVehiclesByClass[vehicleClass]--;
        }
    }
    
    context.vehicleId.clear();
    context.vehicleType.clear();
    context.is5gEnabled = true;
    node->GetObject<MobilityModel>()->SetPosition(Vector(-10000.0, -10000.0, 1.5));
    auto poolIndex = g_nodeToPoolIndex.find(node->GetId());
    if (poolIndex != g_nodeToPoolIndex.end())
    {
        g_freeUeIndices.push_back(poolIndex->second);
    }
}

void
UeDlSinr(uint32_t nodeId, uint16_t cellId, uint16_t rnti, double sinr, uint16_t bwpId)
{
    (void)cellId;
    (void)rnti;
    (void)bwpId;
    auto context = g_ueContexts.find(nodeId);
    if (context != g_ueContexts.end() && sinr > 0.0 && std::isfinite(sinr))
    {
        context->second.sinrDb = 10.0 * std::log10(sinr);
    }
}

void
PeriodicMecUpdate(MecTrafficManager* manager,
                  Ptr<NrHelper> nrHelper,
                  double intervalSeconds,
                  double hysteresisMeters,
                  double durationSeconds,
                  bool doSpatialRouting)
{
    manager->EvaluateMobility(hysteresisMeters, nrHelper, doSpatialRouting);
    manager->WriteMetrics();
    
    if (Simulator::Now().GetSeconds() + intervalSeconds <= durationSeconds)
    {
        Simulator::Schedule(Seconds(intervalSeconds),
                            &PeriodicMecUpdate,
                            manager,
                            nrHelper,
                            intervalSeconds,
                            hysteresisMeters,
                            durationSeconds,
                            doSpatialRouting);
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string sumoConfig = "mobility/guimaraes/guimaraes.sumocfg";
    std::string gnbPositions = "mobility/guimaraes/gnb_positions.json";
    std::string mecTopologyPath = "mobility/guimaraes/mec_topology.json";
    std::string outputDirectory = "results/raw_data";
    double duration = 120.0;
    uint32_t maxUes = 256;
    double sumoStartTime = 25200.0;
    double demoMigrationAt = -1.0;
    std::string mecStrategy = "spatial";
    double mdmkpInterval = 1.0;

    CommandLine command(__FILE__);
    command.AddValue("sumoConfig", "SUMO configuration to run through TraCI", sumoConfig);
    command.AddValue("gnbPositions", "OpenCellID positions projected into SUMO XY", gnbPositions);
    command.AddValue("mecTopology", "JSON topology and backhaul configuration for MEC servers", mecTopologyPath);
    command.AddValue("outputDirectory", "Directory for CSV and native NR trace outputs", outputDirectory);
    command.AddValue("duration", "ns-3 runtime in seconds", duration);
    command.AddValue("maxUes", "Preallocated UE pool capacity for TraCI vehicle mapping", maxUes);
    command.AddValue("maxCars", "Maximum simultaneous 5G connected cars", g_maxCars);
    command.AddValue("maxBuses", "Maximum simultaneous 5G connected buses", g_maxBuses);
    command.AddValue("maxBicycles", "Maximum simultaneous 5G connected bicycles", g_maxBicycles);
    command.AddValue("sumoStartTime", "SUMO absolute begin time in seconds", sumoStartTime);
    command.AddValue("demoMigrationAt", "Optional time in seconds to demonstrate MEC_0 -> MEC_1 redirect", demoMigrationAt);
    command.AddValue("mecStrategy", "MEC allocation/migration strategy: 'spatial' or 'mdmkp'", mecStrategy);
    command.AddValue("mdmkpInterval", "Optimization interval in seconds for MDMKP orchestrator", mdmkpInterval);
    command.Parse(argc, argv);

    NS_ABORT_MSG_IF(mecStrategy != "spatial" && mecStrategy != "mdmkp", "mecStrategy must be 'spatial' or 'mdmkp'");
    bool doSpatialRouting = (mecStrategy == "spatial");

    NS_ABORT_MSG_IF(duration <= 0.0, "Simulation duration must be positive");
    NS_ABORT_MSG_IF(maxUes == 0, "The UE pool cannot be empty");
    NS_ABORT_MSG_IF(demoMigrationAt >= duration,
                    "demoMigrationAt must be negative (disabled) or less than duration");

    const std::filesystem::path absoluteSumoConfig = std::filesystem::absolute(sumoConfig);
    const std::filesystem::path absoluteGnbPositions = std::filesystem::absolute(gnbPositions);
    const std::filesystem::path absoluteMecTopology = std::filesystem::absolute(mecTopologyPath);
    const std::filesystem::path absoluteOutputDirectory = std::filesystem::absolute(outputDirectory);
    std::filesystem::create_directories(absoluteOutputDirectory);

    std::ifstream positionsFile(absoluteGnbPositions);
    NS_ABORT_MSG_IF(!positionsFile, "Cannot open OpenCellID positions file: " << absoluteGnbPositions);
    nlohmann::json positionsJson;
    positionsFile >> positionsJson;
    NS_ABORT_MSG_IF(!positionsJson.contains("gnbs") || !positionsJson["gnbs"].is_array(),
                    "gnb_positions.json must contain a 'gnbs' array");
    NS_ABORT_MSG_IF(positionsJson["gnbs"].empty(), "No OpenCellID NOS LTE/NR positions were supplied");

    std::ifstream mecTopologyFile(absoluteMecTopology);
    NS_ABORT_MSG_IF(!mecTopologyFile, "Cannot open MEC topology file: " << absoluteMecTopology);
    nlohmann::json mecTopologyJson;
    mecTopologyFile >> mecTopologyJson;
    NS_ABORT_MSG_IF(!mecTopologyJson.contains("mecs") || !mecTopologyJson["mecs"].is_array(),
                    "mec_topology.json must contain a 'mecs' array");
    NS_ABORT_MSG_IF(mecTopologyJson["mecs"].size() != 3,
                    "mec_topology.json must define exactly MEC_0, MEC_1 and MEC_2");
    const uint16_t mecUdpPort = mecTopologyJson.value("udp_port", 8080);
    const uint16_t mecTcpPort = mecTopologyJson.value("tcp_port", 8080);
    const double metricsInterval = mecTopologyJson.value("metrics_interval_s", 1.0);
    const double migrationInterval = mecTopologyJson.value("migration_interval_s", 1.0);
    const double migrationHysteresis = mecTopologyJson.value("migration_hysteresis_m", 150.0);
    NS_ABORT_MSG_IF(metricsInterval <= 0.0 || migrationInterval <= 0.0,
                    "MEC metric and migration intervals must be positive");

    std::vector<MecServer> mecServers;
    for (const auto& record : mecTopologyJson["mecs"])
    {
        MecServer server;
        server.id = record.at("id").get<std::string>();
        server.zone = record.at("zone").get<std::string>();
        server.position = Vector(record.at("position").at("x").get<double>(),
                                 record.at("position").at("y").get<double>(),
                                 record.at("position").at("z").get<double>());
        server.vcpus = record.at("capacity").at("vcpus").get<uint32_t>();
        server.ramGb = record.at("capacity").at("ram_gb").get<uint32_t>();
        const auto& backhaul = record.at("backhaul");
        server.dataRateBps = backhaul.at("data_rate_bps").get<uint64_t>();
        server.backhaulDelay = MilliSeconds(backhaul.at("delay_ms").get<double>());
        server.network = backhaul.at("network").get<std::string>();
        server.mask = backhaul.at("mask").get<std::string>();
        server.pgwAddress = backhaul.at("pgw_address").get<std::string>();
        server.serverAddress = backhaul.at("server_address").get<std::string>();
        server.primaryGnbCellIds = record.at("primary_gnb_cell_ids").get<std::vector<std::string>>();
        NS_ABORT_MSG_IF(server.dataRateBps == 0 || server.vcpus == 0 || server.ramGb == 0,
                        "MEC capacity and backhaul rate must be positive for " << server.id);
        mecServers.push_back(std::move(server));
    }

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
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer mecNodes;
    for (auto& server : mecServers)
    {
        server.node = CreateObject<Node>();
        mecNodes.Add(server.node);
    }
    InternetStackHelper mecInternet;
    mecInternet.Install(mecNodes);
    Ipv4StaticRoutingHelper staticRoutingHelper;

    Ptr<Ipv4> pgwIpv4 = pgw->GetObject<Ipv4>();
    pgwIpv4->SetAttribute("IpForward", BooleanValue(true));
    std::set<int32_t> pgwBackhaulInterfaces;
    std::set<std::string> mecSubnets;

    for (auto& server : mecServers)
    {
        PointToPointHelper backhaul;
        backhaul.SetDeviceAttribute("DataRate", DataRateValue(DataRate(server.dataRateBps)));
        backhaul.SetChannelAttribute("Delay", TimeValue(server.backhaulDelay));
        NetDeviceContainer devices = backhaul.Install(pgw, server.node);

        Ipv4Address pgwIp(server.pgwAddress.c_str());
        Ipv4Mask mask(server.mask.c_str());
        Ipv4Address baseAddress(pgwIp.Get() & ~mask.Get());

        Ipv4AddressHelper address;
        address.SetBase(Ipv4Address(server.network.c_str()), mask, baseAddress);
        Ipv4InterfaceContainer interfaces = address.Assign(devices);
        server.ipv4Address = interfaces.GetAddress(1);

        backhaul.EnablePcap("mec-backhaul-pgw", devices.Get(0), false);
        backhaul.EnablePcap("mec-backhaul-server", devices.Get(1), false);

        const int32_t pgwInterface = pgwIpv4->GetInterfaceForDevice(devices.Get(0));
        NS_ABORT_MSG_IF(pgwInterface < 0, "MEC backhaul device is not attached to the PGW");
        NS_ABORT_MSG_IF(!pgwBackhaulInterfaces.insert(pgwInterface).second,
                        "Two MEC backhauls reused the same PGW NetDevice/interface");
        NS_ABORT_MSG_IF(!mecSubnets.insert(server.network + "/" + server.mask).second,
                        "MEC backhaul subnets must be unique");

        const int32_t mecInterface = server.node->GetObject<Ipv4>()->GetInterfaceForDevice(devices.Get(1));
        NS_ABORT_MSG_IF(mecInterface < 0, "MEC backhaul device is not attached to " << server.id);

        // AddressHelper::Assign installs the connected PGW route for this P2P subnet.
        // Each MEC gets an explicit next hop back to the EPC/UE address space.
        Ptr<Ipv4StaticRouting> mecRouting =
            staticRoutingHelper.GetStaticRouting(server.node->GetObject<Ipv4>());
        mecRouting->SetDefaultRoute(interfaces.GetAddress(0), static_cast<uint32_t>(mecInterface));
        mecRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), interfaces.GetAddress(0), static_cast<uint32_t>(mecInterface));
    }
    MecTrafficManager mecTrafficManager;
    mecTrafficManager.Configure(std::move(mecServers), mecUdpPort);
    mecTrafficManager.InstallPacketSinks(duration, mecTcpPort);

    // 1. Instantiate the MDMKP Orchestrator
    Ptr<MecOrchestrator> orchestrator = CreateObject<MecOrchestrator>();
    orchestrator->SetOptimizationInterval(mdmkpInterval);
    // 2. Register MEC Servers with static capacities (e.g., 32 Cores, 64GB RAM, 10Gbps Bandwidth)
    for (auto& server : mecServers) {
        orchestrator->RegisterMecServer(
            server.node->GetId(), 
            server.ipv4Address, 
            32.0, 64.0, 10000.0, 
            server.node
        );
    }

    // 3. Callback setup for downtime resumption
    orchestrator->SetAppMigrationCallback([&mecTrafficManager, duration](Ptr<Node> node, Ipv4Address targetIp, Time resumeTime) {
        return mecTrafficManager.CreateUeApplication(node, "bus", targetIp, resumeTime, Seconds(duration));
    });

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
    nrHelper->SetHandoverAlgorithmType ("ns3::NrA3RsrpHandoverAlgorithm");
    nrHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                              DoubleValue (3.0));
    nrHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                              TimeValue (MilliSeconds (100)));
    NetDeviceContainer gnbDevices = nrHelper->InstallGnbDevice(gnbNodes, bwps);
    nrHelper->AssignStreams(gnbDevices, 1);
    for (uint32_t index = 0; index < gnbDevices.GetN(); ++index)
    {
        g_gnbSites[index].device = gnbDevices.Get(index);
    }

    nrHelper->AddX2Interface(gnbNodes);

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
    Ipv4InterfaceContainer ueInterfaces = epcHelper->AssignUeIpv4Address(ueDevices);
    for (uint32_t i = 0; i < ueInterfaces.GetN(); ++i) {
        SimulationLogger::RegisterIp(ueInterfaces.GetAddress(i), uePool.Get(i)->GetId());
    }

    std::filesystem::create_directories(absoluteOutputDirectory);
    
    SimulationLogger::Init(absoluteOutputDirectory.string() + "/simulation_event_trace.csv");
    g_vehicleEvents.open(absoluteOutputDirectory / "vehicle_events.csv");
    NS_ABORT_MSG_IF(!g_vehicleEvents.is_open(), "Cannot open vehicle_events.csv in results directory");
    g_vehicleEvents << "time_s,event,vehicle_id,vehicle_type,node_id,imsi,nr_cell_id,rnti,details\n";
    g_liveMetrics.open(absoluteOutputDirectory / "live_metrics.csv");
    NS_ABORT_MSG_IF(!g_liveMetrics.is_open(), "Cannot open live_metrics.csv in results directory");
    g_liveMetrics << "Timestamp,UE_ID,VehicleType,Connected_gNB,Current_MEC,Latency_to_MEC0,"
                     "Latency_to_MEC1,Latency_to_MEC2,SINR,PacketLossRate\n";

    std::ofstream gnbCatalog(absoluteOutputDirectory / "gnb_catalog.csv");
    gnbCatalog << "nr_cell_id,opencellid_cell_id,radio,mcc,mnc,x_m,y_m,z_m\n";
    for (const auto& site : g_gnbSites)
    {
        auto gnbDevice = DynamicCast<NrGnbNetDevice>(site.device);
        gnbCatalog << gnbDevice->GetCellId() << ',' << CsvEscape(site.openCellId) << ',' << site.radio << ','
                   << site.mcc << ',' << site.mnc << ',' << site.position.x << ',' << site.position.y << ','
                   << site.position.z << '\n';
    }

    for (uint32_t index = 0; index < uePool.GetN(); ++index)
    {
        Ptr<Node> node = uePool.Get(index);
        Ptr<NrUeNetDevice> ueDevice = DynamicCast<NrUeNetDevice>(ueDevices.Get(index));
        NS_ABORT_MSG_IF(!ueDevice, "UE pool device is not an NrUeNetDevice");
        UeContext context;
        context.node = node;
        context.device = ueDevice;
        context.imsi = ueDevice->GetImsi();
        context.ipv4Address = ueInterfaces.GetAddress(index);
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

        NrHelper::GetUePhy(ueDevice, 0)->TraceConnectWithoutContext(
            "DlDataSinr",
            MakeBoundCallback(&UeDlSinr, node->GetId()));
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
    g_traciClient->SetAttribute("SumoWaitForSocket", TimeValue(Seconds(10.0)));

    std::function<Ptr<Node>()> includeNode = [&]() -> Ptr<Node> {
        NS_ABORT_MSG_IF(g_freeUeIndices.empty(),
                        "UE pool exhausted; increase --maxUes or the peak concurrency margin");
        const uint32_t poolIndex = g_freeUeIndices.back();
        g_freeUeIndices.pop_back();
        Ptr<Node> node = uePool.Get(poolIndex);
        Simulator::ScheduleNow(&ActivateVehicleTraffic,
                               g_traciClient,
                               nrHelper,
                               &mecTrafficManager,
                               orchestrator,
                               node,
                               duration);
        return node;
    };
    std::function<void(Ptr<Node>)> excludeNode = [&](Ptr<Node> node) {
        StopVehicleTraffic(node, &mecTrafficManager, orchestrator);
    };

    NS_LOG_UNCOND("NR configuration: band n78 3.5GHz, 40MHz, numerology 1, UMa/3GPP TR 38.901, channel update 100ms");
    NS_LOG_UNCOND("Multi-MEC topology loaded with " << mecNodes.GetN()
                                                      << " servers; UDP/TCP port " << mecUdpPort
                                                      << "; PGW backhaul 10Gbps with per-MEC delay");
    NS_LOG_UNCOND("OpenCellID NOS logical cells loaded as gNB candidates: " << g_gnbSites.size());
    NS_LOG_UNCOND("Starting SUMO/TraCI for " << duration << " s with 100ms synchronization");
    g_traciClient->SumoSetup(includeNode, excludeNode);

    const double updateInterval = std::min(metricsInterval, migrationInterval);
    Simulator::Schedule(Seconds(updateInterval),
                        &PeriodicMecUpdate,
                        &mecTrafficManager,
                        nrHelper,
                        updateInterval,
                        migrationHysteresis,
                        duration,
                        doSpatialRouting);
    if (demoMigrationAt >= 0.0)
    {
        Simulator::Schedule(Seconds(demoMigrationAt),
                            &MecTrafficManager::DemoMigrationFromMec0,
                            &mecTrafficManager);
    }

    AsciiTraceHelper asciiTraceHelper;
    const auto routingTableStream = asciiTraceHelper.CreateFileStream(
        (absoluteOutputDirectory / "routing-tables.txt").string());
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(0.1), routingTableStream);

    if (!doSpatialRouting)
    {
        Simulator::Schedule(Seconds(mdmkpInterval), &MecOrchestrator::RunOptimizationCycle, orchestrator);
    }
    
    Simulator::Stop(Seconds(duration));
    Simulator::Run();

    Simulator::Destroy();
    // The TraciClient destructor closes the TraCI socket.
    g_traciClient = nullptr;

    mecTrafficManager.PrintSummary();
    LogEvent("MEC_RX_SUMMARY", "", "", 0, 0, 0, 0,
             "servers=" + std::to_string(mecTopologyJson["mecs"].size()));
    g_vehicleEvents.close();
    g_liveMetrics.close();
    return 0;
}