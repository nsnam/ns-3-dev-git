/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/eht-configuration.h"
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/on-off-helper.h"
#include "ns3/p2p-cache-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiP2pExample");

namespace
{

/**
 * Helper function to convert a given type string (Ht, Vht, He or Eht) to the corresponding standard
 * @param type the type to convert
 * @return the converted standard
 */
WifiStandard
GetStandardForType(const std::string& type)
{
    if (type == "Eht")
    {
        return WIFI_STANDARD_80211be;
    }
    else if (type == "He")
    {
        return WIFI_STANDARD_80211ax;
    }
    else if (type == "Vht")
    {
        return WIFI_STANDARD_80211ac;
    }
    else if (type == "Ht")
    {
        return WIFI_STANDARD_80211n;
    }
    else
    {
        NS_FATAL_ERROR("Wrong type!");
    }
    return WIFI_STANDARD_UNSPECIFIED;
}

/// Access category to TOS mapping
const std::map<std::string, uint8_t> AcToTos{{"BE", 0x00 /* CS0 */},
                                             {"BK", 0x28 /* AF11 */},
                                             {"VI", 0xb8 /* EF */},
                                             {"VO", 0xc0 /* CS7 */}};

/// Access category to human readable string mapping
const std::map<AcIndex, std::string> AcToString{
    {AC_BE, "BE"},
    {AC_BK, "BK"},
    {AC_VI, "VI"},
    {AC_VO, "VO"},
};

/// Traffic type to Type ID
const std::map<std::string, std::string> trafficToTypeId{{"Video", "ns3::VideoTraffic"},
                                                         {"Gaming", "ns3::MobileGaming"},
                                                         {"Voip", "ns3::VoipTraffic"},
                                                         {"VDI", "ns3::VirtualDesktop"}};

} // namespace

/**
 * Wi-Fi P2P example class.
 *
 * It handles the creation and run of a Wi-Fi P2P simulation.
 * At the end, statistics are calculated and verified.
 *
 * Scenario: 2 STAs and 1 AP, STAs are either sending traffic to each others
 * via the AP, or directly to each others if P2P is used. Traffic in 4 directions
 * is considered: from STA1 to STA2, from STA2 to STA1, from STA1 to AP and from
 * AP to STA1. If P2P is enabled, STA2 is a adhoc STA, otherwise it is a regular STA.
 */
class WifiP2pExample
{
  public:
    /**
     * traffic direction
     */
    enum Direction
    {
        AP_TO_STA = 0,
        STA_TO_AP,
        STA_TO_ADHOC,
        ADHOC_TO_STA
    };

    /// Information for a given direction
    struct PerDirectionInfo
    {
        std::string l4Protocol{"Udp"};   ///< L4 protocol
        std::string trafficType{"Cbr"};  ///< traffic type
        std::string ac{"BE"};            ///< access category
        uint32_t cbrPayloadSize{1000};   ///< payload size in bytes if traffic type is Cbr
        DataRate cbrDataRate{"100Mb/s"}; ///< data rate if traffic type is Cbr
        std::string videoTrafficModelClassIdentifier{
            "BV1"}; ///< model class identifier if traffic type is video
        std::string gamingSyncMechanism{
            "StatusSync"}; ///< synchronization mechanism to use if traffic type is gaming
        DataRate
            minExpectedThroughput; ///< minimum expected average throughput at the end of the run
        DataRate
            maxExpectedThroughput; ///< maximum expected average throughput at the end of the run
        Time minExpectedLatency{}; ///< minimum expected average end-to-end latency at the end of
                                   ///< the run
        Time maxExpectedLatency{}; ///< maximum expected average end-to-end latency at the end of
                                   ///< the run
        double maxExpectedLoss{
            0.0}; ///< maximum expected packet loss rate in % at the end of the run
        ApplicationContainer sinkApp{}; ///< the sink application
        uint64_t txBytes{0};            ///< count number of transmitted bytes
        std::unordered_map<
            uint64_t /* packet UID (UDP) or total amount of transmitted bytes so far (TCP) */,
            Time>
            packets{};                 ///< store info about transmitted packets
        std::vector<Time> latencies{}; ///< store end-to-end latencies
        DataRate throughput;           ///< store average throughput
        Time latency{};                ///< store average end-to-end latency
        double lossRate{0.0};          ///< store packet loss rate in %
    };

    /**
     * Create an example instance
     */
    WifiP2pExample() = default;
    virtual ~WifiP2pExample() = default;

    /**
     * Parse the options provided through command line
     * @param argc the command line argument count
     * @param argv the command line arguments
     */
    void Config(int argc, char* argv[]);

    /**
     * Setup nodes, devices and internet stacks
     */
    void Setup();

    /**
     * Run simulation
     */
    void Run();

    /**
     * Print results
     */
    void PrintResults();

    /**
     * Check results
     */
    void CheckResults();

  private:
    /**
     * Function invoked when a packet is generated by the source application
     * @param dir the direction of the generated packet
     * @param packet the generated packet
     */
    void NotifyAppTx(Direction dir, Ptr<const Packet> packet);

    /**
     * Function invoked when a packet is received by the sink application
     * @param dir the direction of the received packet
     * @param packet the received packet
     * @param address the address of the source
     */
    void NotifyAppRx(Direction dir, Ptr<const Packet> packet, const Address& address);

    /**
     * Function invoked when a packet is enqueued
     * @param ac the access category of the queue
     * @param mac the MAC of the device
     * @param mpdu the MPDU that is enqueued
     */
    void PacketEnqueued(const std::string& ac, Ptr<const WifiMac> mac, Ptr<const WifiMpdu> mpdu);

    /**
     * Function invoked when a new backoff is generated
     * @param ac the access category of the queue
     * @param backoff the generated backoff
     * @param linkId the link ID
     */
    void BackoffTrace(AcIndex ac, uint32_t backoff, uint8_t linkId);

    /**
     * Get the string corresponding to the source device for a given direction
     * @param dir the direction
     * @param p2p flag whether the source device is a P2P peer
     * @return the string corresponding to the source device
     */
    static std::string GetFromString(Direction dir, bool p2p = true);

    /**
     * Get the string corresponding to the destination device for a given direction
     * @param dir the direction
     * @param p2p flag whether the destination device is a P2P peer
     * @return the string corresponding to the destination device
     */
    static std::string GetToString(Direction dir, bool p2p = true);

    /**
     * Get the string for a given direction
     * @param dir the direction
     * @return the string
     */
    static std::string GetDirectionString(Direction dir);

    /**
     * Get the source node for a given direction
     * @param dir the direction
     * @return the source node
     */
    Ptr<Node> GetFrom(Direction dir);

    /**
     * Get the destination node for a given direction
     * @param dir the direction
     * @return the destination node
     */
    Ptr<Node> GetTo(Direction dir);

    /**
     * Get the IPv4 address of the destination for a given direction
     * @param dir the direction
     * @return the IPv4 address of the destination
     */
    Ipv4Address GetDestinationIp(Direction dir);

    bool m_pcap{false};               //!< flag whether PCAP files should be generated
    bool m_p2p{true};                 //!< flag whether P2P is enabled
    bool m_linksOverlap{true};        //!< flag whether to consider links overlapping for P2P
    bool m_emlsr{false};              //!< flag whether the non-AP STA is an EMLSR client
    uint16_t paddingDelayUsec{32};    //!< padding delay advertised by EMLSR client
    uint16_t transitionDelayUsec{32}; //!< transition delay advertised by EMLSR client
    uint16_t m_numLinksAp{1};         //!< amount of links for the AP
    uint16_t m_numLinksSta{1};        //!< amount of links for the non-AP STA
    uint16_t m_numLinksP2p{1};        //!< amount of links to use for P2P
    uint16_t m_p2pLinkId{0};          //!< dedicated link for P2P operation
    Time m_simulationTime{"10s"};     //!< simulation time
    std::string m_apType{"Eht"};      //!< type of the AP
    std::string m_staType{"Eht"};     //!< type of the non-AP STAs
    std::string m_adhocType{"Eht"};   //!< type of the adhoc STA
    std::array<double, 3> m_frequencies{5,
                                        2.4,
                                        6}; //!< the order of the frequencies to use for the links
    uint32_t m_rtsThreshold{std::numeric_limits<uint16_t>::max()}; //!< the RTS threshold
    uint16_t m_maxAmpduLength{65535};                              //!< the maximum A-MPDU length
    std::map<Direction, PerDirectionInfo> m_infos{
        {AP_TO_STA, {}},
        {STA_TO_AP, {}},
        {STA_TO_ADHOC, {.cbrPayloadSize = 500, .cbrDataRate = DataRate("50Mb/s")}},
        {ADHOC_TO_STA,
         {.cbrPayloadSize = 500,
          .cbrDataRate =
              DataRate("50Mb/s")}}}; //!< the traffic directions to use for the simulation together
                                     //!< with the associated parameters

    NodeContainer m_wifiApNode{};             //!< the AP node
    NodeContainer m_wifiStaNodes{};           //!< the non-AP STA nodes
    Ipv4InterfaceContainer m_apInterface{};   //!< the IPv4 interface of the AP
    Ipv4InterfaceContainer m_staInterfaces{}; //!< the IPv4 interfaces of the non-AP STAs
    Ipv4InterfaceContainer m_p2pInterfaces{}; //!< the IPv4 interfaces of the P2P peers

    std::map<AcIndex, std::vector<uint32_t>>
        m_backoffs{}; //!< the generated backoff values, indexed per AC
};

std::string
WifiP2pExample::GetFromString(Direction dir, bool p2p)
{
    switch (dir)
    {
    case AP_TO_STA:
        return "Ap";
    case STA_TO_AP:
    case STA_TO_ADHOC:
        return p2p ? "Sta" : "Sta1";
    case ADHOC_TO_STA:
        return p2p ? "Adhoc" : "Sta2";
    }
    return "";
}

std::string
WifiP2pExample::GetToString(Direction dir, bool p2p)
{
    switch (dir)
    {
    case AP_TO_STA:
    case ADHOC_TO_STA:
        return p2p ? "Sta" : "Sta1";
    case STA_TO_AP:
        return "Ap";
    case STA_TO_ADHOC:
        return p2p ? "Adhoc" : "Sta2";
    }
    return "";
}

std::string
WifiP2pExample::GetDirectionString(Direction dir)
{
    const auto fromString{GetFromString(dir)};
    const auto toString{GetToString(dir)};
    return fromString + " to " + toString;
}

Ptr<Node>
WifiP2pExample::GetFrom(Direction dir)
{
    switch (dir)
    {
    case AP_TO_STA:
        return m_wifiApNode.Get(0);
    case STA_TO_AP:
    case STA_TO_ADHOC:
        return m_wifiStaNodes.Get(0);
    case ADHOC_TO_STA:
        return m_wifiStaNodes.Get(1);
    }
    return nullptr;
}

Ptr<Node>
WifiP2pExample::GetTo(Direction dir)
{
    switch (dir)
    {
    case AP_TO_STA:
    case ADHOC_TO_STA:
        return m_wifiStaNodes.Get(0);
    case STA_TO_AP:
        return m_wifiApNode.Get(0);
    case STA_TO_ADHOC:
        return m_wifiStaNodes.Get(1);
    }
    return nullptr;
}

Ipv4Address
WifiP2pExample::GetDestinationIp(Direction dir)
{
    switch (dir)
    {
    case AP_TO_STA:
        return m_staInterfaces.GetAddress(0);
    case ADHOC_TO_STA:
        return m_p2p ? m_p2pInterfaces.GetAddress(0, 1) : m_staInterfaces.GetAddress(0);
    case STA_TO_AP:
        return m_apInterface.GetAddress(0);
    case STA_TO_ADHOC:
        return m_p2p ? m_p2pInterfaces.GetAddress(1) : m_staInterfaces.GetAddress(1);
    }
    return Ipv4Address{};
}

void
WifiP2pExample::Config(int argc, char* argv[])
{
    NS_LOG_FUNCTION(this);

    bool logging{false};
    bool verbose{false};

    CommandLine cmd(__FILE__);
    cmd.AddValue("logging", "turn on example log components", logging);
    cmd.AddValue("verbose", "turn on all wifi log components", verbose);
    cmd.AddValue("pcap", "turn on pcap file output", m_pcap);
    cmd.AddValue("p2p",
                 "turn on P2P capability, if enabled one STA is an ADHOC STA and traffic flows "
                 "directly between the two STAs without going through the AP. Otherwise, the two "
                 "STAs are regular STAs associated to the AP and all traffic goes through the AP.",
                 m_p2p);
    cmd.AddValue("simulationTime", "Simulation time", m_simulationTime);
    cmd.AddValue("apType", "AP type: Ht, Vht, He or Eht", m_apType);
    cmd.AddValue("staType", "STA type: Ht, Vht, He or Eht", m_staType);
    cmd.AddValue("adhocType", "ADHOC type: Ht, Vht, He or Eht", m_adhocType);
    cmd.AddValue("numLinksAp", "The number of links to setup for the AP", m_numLinksAp);
    cmd.AddValue("numLinksSta", "The number of links to setup for the STA", m_numLinksSta);
    cmd.AddValue("numLinksP2p", "The number of links to use for P2P", m_numLinksP2p);
    cmd.AddValue("frequency1",
                 "Specify whether the first link operates in the 5, 2.4 or 6 GHz band (other "
                 "values gets rejected)",
                 m_frequencies.at(0));
    cmd.AddValue("frequency2",
                 "Specify whether the second link operates in the 5, 2.4 or 6 GHz band (other "
                 "values gets rejected)",
                 m_frequencies.at(1));
    cmd.AddValue("frequency3",
                 "Specify whether the third link operates in the 5, 2.4 or 6 GHz band (other "
                 "values gets rejected)",
                 m_frequencies.at(2));
    cmd.AddValue("linksOverlap",
                 "Flag whether P2P links overlap with links used for the infrastructure mode."
                 "If set to false, it picks the first unused band for the link to operate on.",
                 m_linksOverlap);
    cmd.AddValue("p2pLinkId",
                 "Select link (starting from 0) for P2P operation by configuring TID-to-link "
                 "mapping on P2P STA. This is used if links overlapping is disabled and non-AP MLD "
                 "does not have more links than AP MLD.",
                 m_p2pLinkId);
    cmd.AddValue("emlsr",
                 "Specify whether the non-AP MLD is an EMLSR client (in which case, EMLSR mode "
                 "is enabled on all the links)",
                 m_emlsr);
    cmd.AddValue("emlsrPaddingDelay",
                 "The EMLSR padding delay in microseconds (0, 32, 64, 128 or 256)",
                 paddingDelayUsec);
    cmd.AddValue("emlsrTransitionDelay",
                 "The EMLSR transition delay in microseconds (0, 16, 32, 64, 128 or 256)",
                 transitionDelayUsec);
    cmd.AddValue("rtsThreshold", "RTS threshold", m_rtsThreshold);
    cmd.AddValue("maxAmpduLength", "maximum length in bytes of an A-MPDU", m_maxAmpduLength);
    for (auto& [direction, dirInfo] : m_infos)
    {
        const auto directionStr{GetDirectionString(direction)};
        const auto fromString{GetFromString(direction)};
        const auto toString{GetToString(direction)};
        auto prepend{fromString};
        std::transform(prepend.cbegin(), prepend.cend(), prepend.begin(), ::tolower);
        prepend += "To" + toString;

        auto paramName = prepend + "TrafficType";
        auto description =
            "The traffic type from " + directionStr + ": Cbr, Video, Gaming, Voip or VDI";
        cmd.AddValue(paramName, description, dirInfo.trafficType);

        paramName = prepend + "L4Protocol";
        description = "The L4 protocol for traffic from " + directionStr + ": Udp or Tcp";
        cmd.AddValue(paramName, description, dirInfo.l4Protocol);

        paramName = prepend + "AccessCategory";
        description = "The AC for traffic from " + directionStr + ": BE, BK, VO or VI";
        cmd.AddValue(paramName, description, dirInfo.ac);

        paramName = prepend + "CbrPayloadSize";
        description =
            "if CBR traffic type is selected, this configures the payload size (in bytes) from " +
            directionStr;
        cmd.AddValue(paramName, description, dirInfo.cbrPayloadSize);

        paramName = prepend + "CbrDataRate";
        description =
            "if CBR traffic type is selected, this configures the data rate from " + directionStr;
        cmd.AddValue(paramName, description, dirInfo.cbrDataRate);

        paramName = prepend + "VideoTrafficModelClassIdentifier";
        description = "if video traffic type is selected, this configures the Traffic Model Class "
                      "Identifier to use from " +
                      directionStr + ": BV1-6";
        cmd.AddValue(paramName, description, dirInfo.videoTrafficModelClassIdentifier);

        paramName = prepend + "GamingSyncMechanism";
        description = "if gaming traffic type is selected, this configures the synchronization "
                      "mechanism to use from " +
                      directionStr + ": StatusSync or FrameLockstep";
        cmd.AddValue(paramName, description, dirInfo.gamingSyncMechanism);

        paramName = prepend + "MinExpectedThroughput";
        description =
            "if set, simulation fails if the " + directionStr + " throughput is below this value";
        cmd.AddValue(paramName, description, dirInfo.minExpectedThroughput);

        paramName = prepend + "MaxExpectedThroughput";
        description =
            "if set, simulation fails if the " + directionStr + " throughput is above this value";
        cmd.AddValue(paramName, description, dirInfo.maxExpectedThroughput);

        paramName = prepend + "MinExpectedLatency";
        description = "if set, simulation fails if the " + directionStr +
                      " latency (end-to-end) is below this value";
        cmd.AddValue(paramName, description, dirInfo.minExpectedLatency);

        paramName = prepend + "MaxExpectedLatency";
        description = "if set, simulation fails if the " + directionStr +
                      " latency (end-to-end) is above this value";
        cmd.AddValue(paramName, description, dirInfo.maxExpectedLatency);

        paramName = prepend + "MaxExpectedLoss";
        description = "if set, simulation fails if the " + directionStr +
                      " packet loss rate is above this value (in %)";
        cmd.AddValue(paramName, description, dirInfo.maxExpectedLoss);
    }
    cmd.Parse(argc, argv);

    for (auto& [direction, dirInfo] : m_infos)
    {
        NS_ABORT_MSG_IF(!AcToTos.contains(dirInfo.ac),
                        "Invalid access category for " << GetDirectionString(direction));
    }

    if (verbose)
    {
        WifiHelper::EnableLogComponents();
    }
    if (logging)
    {
        LogComponentEnable("WifiP2pExample", LOG_LEVEL_ALL);
    }
}

void
WifiP2pExample::Setup()
{
    NS_LOG_FUNCTION(this);

    int64_t streamNumber = 500;
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                       UintegerValue(m_rtsThreshold));

    // create nodes
    m_wifiApNode.Create(1);
    m_wifiStaNodes.Create(2); // one may be adhoc STA

    if (m_p2p && (m_numLinksP2p > 1))
    {
        NS_FATAL_ERROR("MLD for P2P is not supported yet!");
    }

    if (m_frequencies.at(1) == m_frequencies.at(0) || m_frequencies.at(2) == m_frequencies.at(0) ||
        (m_frequencies.at(2) != 0 && m_frequencies.at(2) == m_frequencies.at(1)))
    {
        NS_FATAL_ERROR("Frequency values must be unique!");
    }

    std::vector<std::pair<std::string, FrequencyRange>> allChannelStrs{};
    std::vector<std::pair<std::string, FrequencyRange>> apChannelStrs{};
    std::vector<std::pair<std::string, FrequencyRange>> staChannelStrs{};
    std::pair<std::string, FrequencyRange> p2pChannelStr{};

    for (auto freq : m_frequencies)
    {
        if (freq == 0)
        {
            break;
        }
        if (freq == 6)
        {
            allChannelStrs.emplace_back("{0, 0, BAND_6GHZ, 0}", WIFI_SPECTRUM_6_GHZ);
        }
        else if (freq == 5)
        {
            allChannelStrs.emplace_back("{0, 0, BAND_5GHZ, 0}", WIFI_SPECTRUM_5_GHZ);
        }
        else if (freq == 2.4)
        {
            allChannelStrs.emplace_back("{0, 0, BAND_2_4GHZ, 0}", WIFI_SPECTRUM_2_4_GHZ);
        }
        else
        {
            NS_FATAL_ERROR("Wrong frequency value!");
        }
    }
    std::copy(allChannelStrs.cbegin(),
              allChannelStrs.cbegin() + m_numLinksAp,
              std::back_inserter(apChannelStrs));
    std::cout << "AP link" << (apChannelStrs.size() > 1 ? "s" : "") << ": ";
    for (const auto& apChannelStr : apChannelStrs)
    {
        std::cout << "" << apChannelStr.first << " ";
    }
    std::cout << std::endl;
    std::copy(allChannelStrs.cbegin(),
              allChannelStrs.cbegin() + m_numLinksSta,
              std::back_inserter(staChannelStrs));
    std::cout << "STA link" << (staChannelStrs.size() > 1 ? "s" : "") << ": ";
    for (const auto& staChannelStr : staChannelStrs)
    {
        std::cout << "" << staChannelStr.first << " ";
    }
    std::cout << std::endl;
    if (m_p2p)
    {
        if (m_linksOverlap)
        {
            p2pChannelStr = allChannelStrs.front();
        }
        else
        {
            if (m_numLinksSta > m_numLinksAp)
            {
                p2pChannelStr = allChannelStrs.at(m_numLinksAp);
            }
            else
            {
                p2pChannelStr = allChannelStrs.at(m_p2pLinkId);
            }
        }
        std::cout << "P2P link: " << p2pChannelStr.first << std::endl;
    }

    // configure PHY and MAC
    SpectrumWifiPhyHelper apPhyHelper(m_numLinksAp);
    SpectrumWifiPhyHelper staPhyHelper(m_numLinksSta);
    SpectrumWifiPhyHelper adhocPhyHelper(m_numLinksP2p);
    apPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    staPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    adhocPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    std::size_t apLinkId{0};
    std::size_t staLinkId{0};
    std::string staLinkIdsStr;
    for (const auto& channelInfo : allChannelStrs)
    {
        auto [channelStr, freqRange] = channelInfo;
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
        auto lossModel = CreateObject<LogDistancePropagationLossModel>();
        spectrumChannel->AddPropagationLossModel(lossModel);
        if ((std::find(staChannelStrs.cbegin(), staChannelStrs.cend(), channelInfo) !=
             staChannelStrs.cend()) ||
            (p2pChannelStr == channelInfo))
        {
            if (!staLinkIdsStr.empty())
            {
                staLinkIdsStr += ",";
            }
            staLinkIdsStr += std::to_string(staLinkId);
            staPhyHelper.Set(staLinkId++, "ChannelSettings", StringValue(channelStr));
            staPhyHelper.AddChannel(spectrumChannel, freqRange);
        }
        if (std::find(apChannelStrs.cbegin(), apChannelStrs.cend(), channelInfo) !=
            apChannelStrs.cend())
        {
            apPhyHelper.Set(apLinkId++, "ChannelSettings", StringValue(channelStr));
            apPhyHelper.AddChannel(spectrumChannel, freqRange);
        }
        if (p2pChannelStr == channelInfo)
        {
            adhocPhyHelper.Set("ChannelSettings", StringValue(channelStr));
            adhocPhyHelper.AddChannel(spectrumChannel, freqRange);
        }
    }

    WifiMacHelper wifiMac;
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    Ssid ssid = Ssid("wifi-p2p");

    if (m_numLinksSta > 1 && m_emlsr)
    {
        wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
        staPhyHelper.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(paddingDelayUsec)));
    }

    // setup AP
    wifi.SetStandard(GetStandardForType(m_apType));
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "BE_MaxAmpduSize",
                    UintegerValue(m_maxAmpduLength));
    auto apDevice = wifi.Install(apPhyHelper, wifiMac, m_wifiApNode);
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);

    NetDeviceContainer allDevices;
    allDevices.Add(apDevice);

    // setup STAs (including adhoc station)
    NetDeviceContainer staDevices;
    if (m_p2p)
    {
        // regular STA
        wifi.SetStandard(GetStandardForType(m_staType));
        wifiMac.SetType("ns3::StaWifiMac",
                        "Ssid",
                        SsidValue(ssid),
                        "EnableP2pLinks",
                        BooleanValue(true),
                        "BE_MaxAmpduSize",
                        UintegerValue(m_maxAmpduLength));
        if (m_numLinksSta > 1 && m_emlsr)
        {
            std::cout << "EMLSR links " << staLinkIdsStr << "\n";
            wifiMac.SetEmlsrManager("ns3::AdvancedEmlsrManager",
                                    "EmlsrLinkSet",
                                    StringValue(staLinkIdsStr),
                                    "EmlsrPaddingDelay",
                                    TimeValue(MicroSeconds(paddingDelayUsec)),
                                    "EmlsrTransitionDelay",
                                    TimeValue(MicroSeconds(transitionDelayUsec)));
        }
        staDevices.Add(wifi.Install(staPhyHelper, wifiMac, m_wifiStaNodes.Get(0)));

        if ((!m_linksOverlap && (m_numLinksSta <= m_numLinksAp)) || m_emlsr)
        {
            auto p2pSta =
                DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(0))->GetMac());
            std::stringstream ss;
            ss << "0 ";
            for (std::size_t id = 0; id < allChannelStrs.size(); ++id)
            {
                if (allChannelStrs.at(id) == p2pChannelStr)
                {
                    continue;
                }
                ss << +id << ",";
            }
            auto staEhtConfig = p2pSta->GetEhtConfiguration();
            staEhtConfig->SetAttribute("TidToLinkMappingNegSupport",
                                       EnumValue(WifiTidToLinkMappingNegSupport::ANY_LINK_SET));
            staEhtConfig->SetAttribute("TidToLinkMappingDl", StringValue(ss.str()));
            staEhtConfig->SetAttribute("TidToLinkMappingUl", StringValue(ss.str()));
        }

        // adhoc STA
        wifi.SetStandard(GetStandardForType(m_adhocType));
        wifiMac.SetType("ns3::AdhocWifiMac",
                        "BeaconGeneration",
                        BooleanValue(true),
                        "BE_MaxAmpduSize",
                        UintegerValue(m_maxAmpduLength),
                        "EmlsrPeer",
                        BooleanValue(m_emlsr),
                        "EmlsrPeerPaddingDelay",
                        TimeValue(MicroSeconds(paddingDelayUsec)),
                        "EmlsrPeerTransitionDelay",
                        TimeValue(MicroSeconds(transitionDelayUsec)));
        staDevices.Add(wifi.Install(adhocPhyHelper, wifiMac, m_wifiStaNodes.Get(1)));
    }
    else
    {
        wifiMac.SetType("ns3::StaWifiMac",
                        "Ssid",
                        SsidValue(ssid),
                        "EnableP2pLinks",
                        BooleanValue(false),
                        "BE_MaxAmpduSize",
                        UintegerValue(m_maxAmpduLength));

        wifi.SetStandard(GetStandardForType(m_staType));
        staDevices.Add(wifi.Install(staPhyHelper, wifiMac, m_wifiStaNodes.Get(0)));

        wifi.SetStandard(GetStandardForType(m_adhocType));
        staDevices.Add(wifi.Install(staPhyHelper, wifiMac, m_wifiStaNodes.Get(1)));
    }
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);
    allDevices.Add(staDevices);

    for (auto it = allDevices.Begin(); it != allDevices.End(); ++it)
    {
        auto mac = DynamicCast<WifiNetDevice>(*it)->GetMac();
        for (const auto& [ac, acStr] : AcToString)
        {
            auto qosTxop = mac->GetQosTxop(ac);
            qosTxop->GetWifiMacQueue()->TraceConnectWithoutContext(
                "Enqueue",
                MakeCallback(&WifiP2pExample::PacketEnqueued, this).Bind(acStr, mac));
            m_backoffs[ac] = {};
            qosTxop->TraceConnectWithoutContext(
                "BackoffTrace",
                MakeCallback(&WifiP2pExample::BackoffTrace, this).Bind(ac));
        }
    }

    // mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(2.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_wifiApNode);
    mobility.Install(m_wifiStaNodes);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(m_wifiApNode);
    stack.Install(m_wifiStaNodes);
    streamNumber += stack.AssignStreams(m_wifiApNode, streamNumber);
    streamNumber += stack.AssignStreams(m_wifiStaNodes, streamNumber);

    Ipv4AddressHelper infraAddress;
    infraAddress.SetBase("192.168.1.0", "255.255.255.0");
    m_apInterface = infraAddress.Assign(apDevice);
    m_staInterfaces = infraAddress.Assign(m_p2p ? staDevices.Get(0) : staDevices);

    if (m_p2p)
    {
        // use a different subnet for P2P
        Ipv4AddressHelper p2pAddress;
        p2pAddress.SetBase("10.1.1.0", "255.255.255.0");
        m_p2pInterfaces = p2pAddress.Assign(staDevices);

        // populate ARP cache for DUT
        P2pCacheHelper cacheHelper{};
        cacheHelper.PopulateP2pCache(staDevices.Get(0), staDevices.Get(1));
    }

    // traffic
    uint16_t port = 100;
    for (auto& [direction, dirInfo] : m_infos)
    {
        InetSocketAddress socket(GetDestinationIp(direction), port);
        const auto l4ProtocolId = "ns3::" + dirInfo.l4Protocol + "SocketFactory";
        Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper packetSinkHelper(l4ProtocolId, localAddress);
        auto sinkNode = GetTo(direction);
        dirInfo.sinkApp = packetSinkHelper.Install(sinkNode);
        streamNumber += packetSinkHelper.AssignStreams(sinkNode, streamNumber);
        dirInfo.sinkApp.Start(Seconds(0.0));
        dirInfo.sinkApp.Stop(m_simulationTime + Seconds(2.0));
        dirInfo.sinkApp.Get(0)->TraceConnectWithoutContext(
            "Rx",
            MakeCallback(&WifiP2pExample::NotifyAppRx, this).Bind(direction));
        if ((dirInfo.trafficType == "Cbr") && (dirInfo.cbrDataRate > 0))
        {
            OnOffHelper cbrSource(l4ProtocolId, socket);
            cbrSource.SetAttribute("DataRate", DataRateValue(dirInfo.cbrDataRate));
            cbrSource.SetAttribute("PacketSize", UintegerValue(dirInfo.cbrPayloadSize));
            cbrSource.SetAttribute("Tos", UintegerValue(AcToTos.at(dirInfo.ac)));
            cbrSource.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            cbrSource.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            auto sourceNode = GetFrom(direction);
            auto source = cbrSource.Install(sourceNode);
            streamNumber += cbrSource.AssignStreams(sourceNode, streamNumber);
            source.Start(Seconds(1.0));
            source.Stop(m_simulationTime + Seconds(1.0));
            source.Get(0)->TraceConnectWithoutContext(
                "Tx",
                MakeCallback(&WifiP2pExample::NotifyAppTx, this).Bind(direction));
        }
        else if (trafficToTypeId.contains(dirInfo.trafficType))
        {
            ApplicationHelper sourceHelper(trafficToTypeId.at(dirInfo.trafficType));
            sourceHelper.SetAttribute("Protocol", StringValue(l4ProtocolId));
            sourceHelper.SetAttribute("Remote", AddressValue(socket));
            sourceHelper.SetAttribute("Tos", UintegerValue(AcToTos.at(dirInfo.ac)));
            if (dirInfo.trafficType == "Video")
            {
                sourceHelper.SetAttribute("TrafficModelClassIdentifier",
                                          StringValue(dirInfo.videoTrafficModelClassIdentifier));
            }
            if (dirInfo.trafficType == "Gaming")
            {
                if (dirInfo.gamingSyncMechanism == "StatusSync")
                {
                    if ((direction == AP_TO_STA) ||
                        (direction == STA_TO_ADHOC)) // assume DL parameters
                    {
                        auto ips =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(0),
                                                                              "Max",
                                                                              DoubleValue(20));
                        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

                        auto eps =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(500),
                                                                              "Max",
                                                                              DoubleValue(600));
                        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

                        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(50),
                            "Scale",
                            DoubleValue(11));
                        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

                        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(13000),
                            "Scale",
                            DoubleValue(3700));
                        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));
                    }
                    else // assume UL parameters
                    {
                        auto ips =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(0),
                                                                              "Max",
                                                                              DoubleValue(20));
                        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

                        auto eps =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(400),
                                                                              "Max",
                                                                              DoubleValue(550));
                        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

                        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(38),
                            "Scale",
                            DoubleValue(3.7));
                        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

                        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(15000),
                            "Scale",
                            DoubleValue(5700));
                        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));
                    }
                }
                else if (dirInfo.gamingSyncMechanism == "FrameLockstep")
                {
                    if ((direction == AP_TO_STA) ||
                        (direction == STA_TO_ADHOC)) // assume DL parameters
                    {
                        auto ips =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(0),
                                                                              "Max",
                                                                              DoubleValue(80));
                        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

                        auto eps =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(140),
                                                                              "Max",
                                                                              DoubleValue(150));
                        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

                        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(210),
                            "Scale",
                            DoubleValue(35));
                        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

                        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(28000),
                            "Scale",
                            DoubleValue(4200));
                        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));
                    }
                    else // assume UL parameters
                    {
                        auto ips =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(0),
                                                                              "Max",
                                                                              DoubleValue(80));
                        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

                        auto eps =
                            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                              DoubleValue(50),
                                                                              "Max",
                                                                              DoubleValue(60));
                        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

                        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(92),
                            "Scale",
                            DoubleValue(38));
                        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

                        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
                            "Location",
                            DoubleValue(22000),
                            "Scale",
                            DoubleValue(3400));
                        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));
                    }
                }
                else
                {
                    NS_FATAL_ERROR("Incorrect gaming synchronization mechanism");
                }
            }
            else if (dirInfo.trafficType == "VDI")
            {
                if ((direction == STA_TO_AP) || (direction == ADHOC_TO_STA)) // assume UL parameters
                {
                    auto ipa = CreateObjectWithAttributes<ExponentialRandomVariable>(
                        "Mean",
                        DoubleValue(48287000));
                    sourceHelper.SetAttribute("InterPacketArrivals", PointerValue(ipa));
                    sourceHelper.SetAttribute("ParametersPacketSize", StringValue("50.598 5.0753"));
                }
                // no else, defaults for VDI are DL
            }
            auto sourceNode = GetFrom(direction);
            auto source = sourceHelper.Install(sourceNode);
            streamNumber += sourceHelper.AssignStreams(sourceNode, streamNumber);
            source.Start(Seconds(1.0));
            source.Stop(m_simulationTime + Seconds(1.0));
            source.Get(0)->TraceConnectWithoutContext(
                "Tx",
                MakeCallback(&WifiP2pExample::NotifyAppTx, this).Bind(direction));
        }
        else
        {
            NS_ABORT_MSG("Invalid traffic type for " << GetDirectionString(direction));
        }
        ++port;
    }

    // pcap
    if (m_pcap)
    {
        apPhyHelper.EnablePcap("wifi-p2p-AP", apDevice.Get(0));
        if (m_p2p)
        {
            staPhyHelper.EnablePcap("wifi-p2p-STA", staDevices.Get(0));
            adhocPhyHelper.EnablePcap("wifi-p2p-ADHOC", staDevices.Get(1));
        }
        else
        {
            staPhyHelper.EnablePcap("wifi-p2p-STA1", staDevices.Get(0));
            staPhyHelper.EnablePcap("wifi-p2p-STA2", staDevices.Get(1));
        }
    }
}

void
WifiP2pExample::Run()
{
    NS_LOG_FUNCTION(this);
    Simulator::Stop(m_simulationTime + Seconds(2.0));
    Simulator::Run();
}

void
WifiP2pExample::PrintResults()
{
    NS_LOG_FUNCTION(this);
    std::ostringstream oss{};

    oss << std::left << std::setw(16) << "Direction" << std::setw(24) << "Throughput [Mbit/s]"
        << std::setw(24) << "E2E latency [ms]" << std::setw(16) << "TX [packets]" << std::setw(16)
        << "RX [packets]" << std::setw(16) << "Packet loss rate [%]" << std::endl;
    uint64_t totalBytesInfrastructure{0};
    uint64_t totalTxPacketInfrastructure{0};
    uint64_t totalRxPacketInfrastructure{0};
    std::vector<Time> totalE2eLatenciesInfrastructure{};
    uint64_t totalBytesP2p{0};
    uint64_t totalTxPacketP2p{0};
    uint64_t totalRxPacketP2p{0};
    std::vector<Time> totalE2eLatenciesP2p{};
    uint64_t totalBytes{0};
    uint64_t totalTxPacket{0};
    uint64_t totalRxPacket{0};
    std::vector<Time> totalE2eLatencies{};
    for (auto& [direction, dirInfo] : m_infos)
    {
        const auto rxBytes = (dirInfo.sinkApp.GetN() != 0)
                                 ? dirInfo.sinkApp.Get(0)->GetObject<PacketSink>()->GetTotalRx()
                                 : 0;
        totalBytes += rxBytes;
        dirInfo.throughput =
            m_simulationTime.IsStrictlyPositive()
                ? DataRate((rxBytes * 8.0 * 1e6) / m_simulationTime.GetMicroSeconds())
                : DataRate();
        std::copy(dirInfo.latencies.cbegin(),
                  dirInfo.latencies.cend(),
                  std::back_inserter(totalE2eLatencies));
        const auto totalLatency = std::accumulate(dirInfo.latencies.cbegin(),
                                                  dirInfo.latencies.cend(),
                                                  Time(),
                                                  [](auto sum, const auto t) { return sum + t; });
        const auto latency = dirInfo.latencies.empty()
                                 ? 0
                                 : totalLatency.ToDouble(Time::MS) / dirInfo.latencies.size();
        dirInfo.latency = Seconds(latency / 1e3);
        auto fromString{GetFromString(direction, m_p2p)};
        auto toString{GetToString(direction, m_p2p)};
        std::transform(fromString.cbegin(), fromString.cend(), fromString.begin(), ::toupper);
        std::transform(toString.cbegin(), toString.cend(), toString.begin(), ::toupper);
        const auto directionStr = fromString + " -> " + toString;
        const auto txPackets = dirInfo.packets.size();
        totalTxPacket += txPackets;
        const auto rxPackets = dirInfo.latencies.size();
        totalRxPacket += rxPackets;
        if ((direction == AP_TO_STA) || (direction == STA_TO_AP))
        {
            totalBytesInfrastructure += rxBytes;
            totalTxPacketInfrastructure += txPackets;
            totalRxPacketInfrastructure += rxPackets;
            std::copy(dirInfo.latencies.cbegin(),
                      dirInfo.latencies.cend(),
                      std::back_inserter(totalE2eLatenciesInfrastructure));
        }
        else
        {
            totalBytesP2p += rxBytes;
            totalTxPacketP2p += txPackets;
            totalRxPacketP2p += rxPackets;
            std::copy(dirInfo.latencies.cbegin(),
                      dirInfo.latencies.cend(),
                      std::back_inserter(totalE2eLatenciesP2p));
        }
        dirInfo.lossRate = (txPackets > 0) ? (100.0 * (txPackets - rxPackets) / txPackets) : 0;
        oss << std::setw(16) << directionStr << std::setw(24)
            << dirInfo.throughput.GetBitRate() * 1e-6 << std::setw(24) << latency << std::setw(16)
            << txPackets << std::setw(16) << rxPackets << std::setw(16) << dirInfo.lossRate
            << std::endl;
    }

    const auto aggregateThroughputInfrastructure =
        m_simulationTime.IsStrictlyPositive()
            ? ((totalBytesInfrastructure * 8.0) / m_simulationTime.GetMicroSeconds())
            : 0.0;
    const auto totalE2eLatencyInfrastructure =
        std::accumulate(totalE2eLatenciesInfrastructure.cbegin(),
                        totalE2eLatenciesInfrastructure.cend(),
                        Time(),
                        [](auto sum, const auto t) { return sum + t; });
    const auto averageE2eLatencyInfrastructure =
        totalE2eLatenciesInfrastructure.empty() ? 0
                                                : totalE2eLatencyInfrastructure.ToDouble(Time::MS) /
                                                      totalE2eLatenciesInfrastructure.size();
    const auto totalossRateInfrastructure =
        (totalTxPacketInfrastructure > 0)
            ? (100.0 * (totalTxPacketInfrastructure - totalRxPacketInfrastructure) /
               totalTxPacketInfrastructure)
            : 0;
    oss << std::setw(16) << "STA <-> AP" << std::setw(24) << aggregateThroughputInfrastructure
        << std::setw(24) << averageE2eLatencyInfrastructure << std::setw(16)
        << totalTxPacketInfrastructure << std::setw(16) << totalRxPacketInfrastructure
        << std::setw(16) << totalossRateInfrastructure << std::endl;

    const auto aggregateThroughputP2p =
        m_simulationTime.IsStrictlyPositive()
            ? ((totalBytesP2p * 8.0) / m_simulationTime.GetMicroSeconds())
            : 0.0;
    const auto totalE2eLatencyP2p = std::accumulate(totalE2eLatenciesP2p.cbegin(),
                                                    totalE2eLatenciesP2p.cend(),
                                                    Time(),
                                                    [](auto sum, const auto t) { return sum + t; });
    const auto averageE2eLatencyP2p =
        totalE2eLatenciesP2p.empty()
            ? 0
            : totalE2eLatencyP2p.ToDouble(Time::MS) / totalE2eLatenciesP2p.size();
    const auto totalossRateP2p =
        (totalTxPacketP2p > 0) ? (100.0 * (totalTxPacketP2p - totalRxPacketP2p) / totalTxPacketP2p)
                               : 0;
    oss << std::setw(16) << "STA <-> STA" << std::setw(24) << aggregateThroughputP2p
        << std::setw(24) << averageE2eLatencyP2p << std::setw(16) << totalTxPacketP2p
        << std::setw(16) << totalRxPacketP2p << std::setw(16) << totalossRateP2p << std::endl;

    const auto aggregateThroughput = m_simulationTime.IsStrictlyPositive()
                                         ? ((totalBytes * 8.0) / m_simulationTime.GetMicroSeconds())
                                         : 0.0;
    const auto totalE2eLatency = std::accumulate(totalE2eLatencies.cbegin(),
                                                 totalE2eLatencies.cend(),
                                                 Time(),
                                                 [](auto sum, const auto t) { return sum + t; });
    const auto averageE2eLatency =
        totalE2eLatencies.empty() ? 0
                                  : totalE2eLatency.ToDouble(Time::MS) / totalE2eLatencies.size();
    const auto totalossRate =
        (totalTxPacket > 0) ? (100.0 * (totalTxPacket - totalRxPacket) / totalTxPacket) : 0;
    oss << std::setw(16) << "Total" << std::setw(24) << aggregateThroughput << std::setw(24)
        << averageE2eLatency << std::setw(16) << totalTxPacket << std::setw(16) << totalRxPacket
        << std::setw(16) << totalossRate << std::endl;

    for (const auto& [ac, acStr] : AcToString)
    {
        const auto totalBackoff =
            std::accumulate(m_backoffs.at(ac).cbegin(), m_backoffs.at(ac).cend(), 0.0);
        if (m_simulationTime.IsStrictlyPositive() && (totalTxPacket > 0))
        {
            const auto averageBackoff =
                m_backoffs.at(ac).empty() ? 0.0 : (totalBackoff / m_backoffs.size());
            oss << "Average backoff [" << acStr << "]: " << averageBackoff << std::endl;
        }
    }

    std::cout << oss.str();
}

void
WifiP2pExample::CheckResults()
{
    NS_LOG_FUNCTION(this);

    for (const auto& [direction, dirInfo] : m_infos)
    {
        if (dirInfo.throughput < dirInfo.minExpectedThroughput)
        {
            NS_LOG_ERROR("Obtained throughput " << dirInfo.throughput << "Mbit/s is not expected!");
            exit(1);
        }
        if (dirInfo.maxExpectedThroughput > 0 &&
            (dirInfo.throughput > dirInfo.maxExpectedThroughput))
        {
            NS_LOG_ERROR("Obtained throughput " << dirInfo.throughput << "Mbit/s is not expected!");
            exit(1);
        }
        if (dirInfo.latency < dirInfo.minExpectedLatency)
        {
            NS_LOG_ERROR("Obtained latency " << dirInfo.latency.ToDouble(Time::MS)
                                             << "ms is not expected!");
            exit(1);
        }
        if (dirInfo.maxExpectedLatency.IsStrictlyPositive() &&
            (dirInfo.latency > dirInfo.maxExpectedLatency))
        {
            NS_LOG_ERROR("Obtained latency " << dirInfo.latency.ToDouble(Time::MS)
                                             << "ms is not expected!");
            exit(1);
        }
        if (dirInfo.maxExpectedLoss > 0 && (dirInfo.lossRate > dirInfo.maxExpectedLoss))
        {
            NS_LOG_ERROR("Obtained packet loss rate " << dirInfo.lossRate << "% is not expected!");
            exit(1);
        }
    }
}

void
WifiP2pExample::NotifyAppTx(Direction dir, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << dir << packet);
    m_infos.at(dir).txBytes += packet->GetSize();
    if (m_infos.at(dir).l4Protocol == "Tcp")
    {
        const auto inserted =
            m_infos.at(dir).packets.insert({m_infos.at(dir).txBytes, Simulator::Now()}).second;
        NS_ABORT_MSG_IF(!inserted,
                        "Duplicate packet for total TX bytes " << m_infos.at(dir).txBytes);
    }
    else
    {
        const auto uid = packet->GetUid();
        const auto inserted = m_infos.at(dir).packets.insert({uid, Simulator::Now()}).second;
        NS_ABORT_MSG_IF(!inserted, "Duplicate packet UID " << uid);
    }
}

void
WifiP2pExample::NotifyAppRx(Direction dir, Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_FUNCTION(this << dir << packet << address);
    if (m_infos.at(dir).l4Protocol == "Tcp")
    {
        uint64_t totalBytesReceived =
            DynamicCast<PacketSink>(m_infos.at(dir).sinkApp.Get(0))->GetTotalRx();
        for (const auto& [totalTxBytes, txTime] : m_infos.at(dir).packets)
        {
            if (totalTxBytes == totalBytesReceived)
            {
                m_infos.at(dir).latencies.push_back(Simulator::Now() - txTime);
                break;
            }
        }
    }
    else
    {
        const auto uid = packet->GetUid();
        auto it = m_infos.at(dir).packets.find(uid);
        NS_ABORT_MSG_IF(it == m_infos.at(dir).packets.end(),
                        "Packet with UID " << uid << " not found");
        m_infos.at(dir).latencies.push_back(Simulator::Now() - it->second);
    }
}

void
WifiP2pExample::PacketEnqueued(const std::string& ac,
                               Ptr<const WifiMac> mac,
                               Ptr<const WifiMpdu> mpdu)
{
    if (!mpdu->GetHeader().IsQosData() || mpdu->GetPacketSize() < 50)
    {
        // ignore non-QoS data frames and small packets (ARP requests, ARP responses, ...)
        return;
    }
    Direction dir;
    if (mac->GetTypeOfStation() == AP)
    {
        dir = AP_TO_STA;
    }
    else
    {
        auto adhocMac = DynamicCast<WifiNetDevice>(m_wifiStaNodes.Get(1)->GetDevice(0))->GetMac();
        if (mac == adhocMac)
        {
            dir = ADHOC_TO_STA;
        }
        else if (mpdu->GetHeader().GetAddr1() == adhocMac->GetAddress())
        {
            dir = STA_TO_ADHOC;
        }
        else
        {
            dir = STA_TO_AP;
        }
    }
    NS_LOG_FUNCTION(this << ac << dir << *mpdu);
    NS_ABORT_MSG_IF(m_infos.at(dir).ac != ac,
                    "Unexpected access category for traffic sent from "
                        << GetFromString(dir) << " to " << GetToString(dir, m_p2p));
}

void
WifiP2pExample::BackoffTrace(AcIndex ac, uint32_t backoff, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << ac << linkId << backoff);
    m_backoffs.at(ac).push_back(backoff);
}

int
main(int argc, char* argv[])
{
    WifiP2pExample example;
    example.Config(argc, argv);
    example.Setup();
    example.Run();
    example.PrintResults();
    example.CheckResults();

    Simulator::Destroy();
    return 0;
}
