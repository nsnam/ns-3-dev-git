/*
 * Copyright (c) 2008 INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */

#include "pyviz.h"

#include "fast-clipping.h"
#include "visual-simulator-impl.h"

#include "ns3/abort.h"
#include "ns3/config.h"
#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-mac-header.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/node-list.h"
#include "ns3/ppp-header.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"

#include <cstdlib>
#include <sstream>

using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("PyViz");

static std::vector<std::string>
PathSplit(std::string str)
{
    std::vector<std::string> results;
    size_t cutAt;
    while ((cutAt = str.find_first_of('/')) != std::string::npos)
    {
        if (cutAt > 0)
        {
            results.push_back(str.substr(0, cutAt));
        }
        str = str.substr(cutAt + 1);
    }
    if (!str.empty())
    {
        results.push_back(str);
    }
    return results;
}

namespace ns3
{

static PyViz* g_visualizer = nullptr; ///< the visualizer

/**
 * PyVizPacketTag structure
 */
struct PyVizPacketTag : public Tag
{
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer buf) const override;
    void Deserialize(TagBuffer buf) override;
    void Print(std::ostream& os) const override;
    PyVizPacketTag();

    uint32_t m_packetId; ///< packet id
};

/**
 * @brief Get the type ID.
 * @return the object TypeId
 */
TypeId
PyVizPacketTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PyVizPacketTag")
                            .SetParent<Tag>()
                            .SetGroupName("Visualizer")
                            .AddConstructor<PyVizPacketTag>();
    return tid;
}

TypeId
PyVizPacketTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
PyVizPacketTag::GetSerializedSize() const
{
    return 4;
}

void
PyVizPacketTag::Serialize(TagBuffer buf) const
{
    buf.WriteU32(m_packetId);
}

void
PyVizPacketTag::Deserialize(TagBuffer buf)
{
    m_packetId = buf.ReadU32();
}

void
PyVizPacketTag::Print(std::ostream& os) const
{
    os << "PacketId=" << m_packetId;
}

PyVizPacketTag::PyVizPacketTag()
    : Tag()
{
}

PyViz::PyViz()
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(g_visualizer == nullptr);
    g_visualizer = this;

    // WiFi
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                            MakeCallback(&PyViz::TraceNetDevTxWifi, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                            MakeCallback(&PyViz::TraceNetDevRxWifi, this));
    // Lr-Wpan
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::lrwpan::LrWpanNetDevice/Mac/MacTx",
                            MakeCallback(&PyViz::TraceNetDevTxLrWpan, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::lrwpan::LrWpanNetDevice/Mac/MacRx",
                            MakeCallback(&PyViz::TraceNetDevRxLrWpan, this));

    // CSMA
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTx",
                            MakeCallback(&PyViz::TraceNetDevTxCsma, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacRx",
                            MakeCallback(&PyViz::TraceNetDevRxCsma, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacPromiscRx",
                            MakeCallback(&PyViz::TraceNetDevPromiscRxCsma, this));

    // Generic queue drop
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/TxQueue/Drop",
                            MakeCallback(&PyViz::TraceDevQueueDrop, this));
    // IPv4 drop
    Config::ConnectFailSafe("/NodeList/*/$ns3::Ipv4L3Protocol/Drop",
                            MakeCallback(&PyViz::TraceIpv4Drop, this));

    // Point-to-Point
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTx",
                            MakeCallback(&PyViz::TraceNetDevTxPointToPoint, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacRx",
                            MakeCallback(&PyViz::TraceNetDevRxPointToPoint, this));

    // LTE
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/Tx",
                            MakeCallback(&PyViz::TraceNetDevTxLte, this));

    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/Rx",
                            MakeCallback(&PyViz::TraceNetDevRxLte, this));
}

void
PyViz::RegisterCsmaLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/MacTx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevTxCsma, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevRxCsma, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/PromiscRx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevPromiscRxCsma, this));
}

void
PyViz::RegisterWifiLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Tx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevTxWifi, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevRxWifi, this));
}

void
PyViz::RegisterPointToPointLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/TxQueue/Dequeue";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevTxPointToPoint, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceNetDevRxPointToPoint, this));
}

void
PyViz::SetPacketCaptureOptions(uint32_t nodeId, PacketCaptureOptions options)
{
    NS_LOG_DEBUG("  SetPacketCaptureOptions "
                 << nodeId << " PacketCaptureOptions (headers size = " << options.headers.size()
                 << " mode = " << options.mode << " numLastPackets = " << options.numLastPackets
                 << ")");
    m_packetCaptureOptions[nodeId] = options;
}

void
PyViz::RegisterDropTracePath(const std::string& tracePath)
{
    Config::Connect(tracePath, MakeCallback(&PyViz::TraceDevQueueDrop, this));
}

PyViz::~PyViz()
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT(g_visualizer == this);
    g_visualizer = nullptr;
}

void
PyViz::DoPause(const std::string& message)
{
    m_pauseMessages.push_back(message);
    m_stop = true;
    NS_LOG_LOGIC(Simulator::Now().As(Time::S)
                 << ": Have " << g_visualizer->m_pauseMessages.size() << " pause messages");
}

void
PyViz::Pause(const std::string& message)
{
    NS_ASSERT(g_visualizer);
    g_visualizer->DoPause(message);
}

std::vector<std::string>
PyViz::GetPauseMessages() const
{
    NS_LOG_LOGIC(Simulator::Now().As(Time::S)
                 << ": GetPauseMessages: have " << g_visualizer->m_pauseMessages.size()
                 << " pause messages");
    return m_pauseMessages;
}

void
PyViz::CallbackStopSimulation()
{
    NS_LOG_FUNCTION_NOARGS();
    if (m_runUntil <= Simulator::Now())
    {
        Simulator::Stop(); // Stop right now
        m_stop = true;
    }
}

void
PyViz::SimulatorRunUntil(Time time)
{
    NS_LOG_LOGIC("SimulatorRunUntil " << time << " (now is " << Simulator::Now() << ")");

    m_pauseMessages.clear();
    m_transmissionSamples.clear();
    m_packetDrops.clear();

    Time expirationTime = Simulator::Now() - Seconds(10);

    // Clear very old transmission records
    for (auto iter = m_txRecords.begin(); iter != m_txRecords.end();)
    {
        if (iter->second.time < expirationTime)
        {
            m_txRecords.erase(iter++);
        }
        else
        {
            iter++;
        }
    }

    // Clear very old packets of interest
    for (auto iter = m_packetsOfInterest.begin(); iter != m_packetsOfInterest.end();)
    {
        if (iter->second < expirationTime)
        {
            m_packetsOfInterest.erase(iter++);
        }
        else
        {
            iter++;
        }
    }

    if (Simulator::Now() >= time)
    {
        return;
    }

    // Schedule a dummy callback function for the target time, to make
    // sure we stop at the right time.  Otherwise, simulations with few
    // events just appear to "jump" big chunks of time.
    NS_LOG_LOGIC("Schedule dummy callback to be called in " << (time - Simulator::Now()));
    m_runUntil = time;
    m_stop = false;
    Simulator::ScheduleWithContext(Simulator::NO_CONTEXT,
                                   time - Simulator::Now(),
                                   &PyViz::CallbackStopSimulation,
                                   this);

    Ptr<SimulatorImpl> impl = Simulator::GetImplementation();
    Ptr<VisualSimulatorImpl> visualImpl = DynamicCast<VisualSimulatorImpl>(impl);
    if (visualImpl)
    {
        visualImpl->RunRealSimulator();
    }
    else
    {
        impl->Run();
    }
}

Time
PyViz::GetSimulatorStopTime()
{
    Ptr<SimulatorImpl> impl = Simulator::GetImplementation();
    Ptr<VisualSimulatorImpl> visualImpl = DynamicCast<VisualSimulatorImpl>(impl);
    return visualImpl->GetStopTime();
}

bool
PyViz::TransmissionSampleKey::operator<(const PyViz::TransmissionSampleKey& other) const
{
    if (this->transmitter < other.transmitter)
    {
        return true;
    }
    if (this->transmitter != other.transmitter)
    {
        return false;
    }
    if (this->receiver < other.receiver)
    {
        return true;
    }
    if (this->receiver != other.receiver)
    {
        return false;
    }
    return this->channel < other.channel;
}

bool
PyViz::TransmissionSampleKey::operator==(const PyViz::TransmissionSampleKey& other) const
{
    bool retval = (transmitter == other.transmitter) && (receiver == other.receiver) &&
                  (channel == other.channel);
    return retval;
}

NetDeviceStatistics&
PyViz::FindNetDeviceStatistics(int node, int interface)
{
    auto nodeStatsIter = m_nodesStatistics.find(node);
    std::vector<NetDeviceStatistics>* stats;
    if (nodeStatsIter == m_nodesStatistics.end())
    {
        stats = &m_nodesStatistics[node];
        stats->resize(NodeList::GetNode(node)->GetNDevices());
    }
    else
    {
        stats = &(nodeStatsIter->second);
    }
    NetDeviceStatistics& devStats = (*stats)[interface];
    return devStats;
}

bool
PyViz::GetPacketCaptureOptions(uint32_t nodeId, const PacketCaptureOptions** outOptions) const
{
    auto iter = m_packetCaptureOptions.find(nodeId);
    if (iter == m_packetCaptureOptions.end())
    {
        return false;
    }
    else
    {
        *outOptions = &iter->second;
        return true;
    }
}

bool
PyViz::FilterPacket(Ptr<const Packet> packet, const PacketCaptureOptions& options)
{
    switch (options.mode)
    {
    case PACKET_CAPTURE_DISABLED:
        return false;

    case PACKET_CAPTURE_FILTER_HEADERS_OR: {
        PacketMetadata::ItemIterator metadataIterator = packet->BeginItem();
        while (metadataIterator.HasNext())
        {
            PacketMetadata::Item item = metadataIterator.Next();
            if (options.headers.find(item.tid) != options.headers.end())
            {
                return true;
            }
        }
        return false;
    }

    case PACKET_CAPTURE_FILTER_HEADERS_AND: {
        std::set<TypeId> missingHeaders(options.headers);
        PacketMetadata::ItemIterator metadataIterator = packet->BeginItem();
        while (metadataIterator.HasNext())
        {
            PacketMetadata::Item item = metadataIterator.Next();
            auto missingIter = missingHeaders.find(item.tid);
            if (missingIter != missingHeaders.end())
            {
                missingHeaders.erase(missingIter);
            }
        }
        return missingHeaders.empty();
    }

    default:
        NS_FATAL_ERROR("should not be reached");
        return false;
    }
}

void
PyViz::TraceDevQueueDrop(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(context << packet->GetUid());
    std::vector<std::string> splitPath = PathSplit(context);
    int nodeIndex = std::stoi(splitPath[1]);
    Ptr<Node> node = NodeList::GetNode(nodeIndex);

    if (m_nodesOfInterest.find(nodeIndex) == m_nodesOfInterest.end())
    {
        // if the transmitting node is not "of interest", we still
        // record the transmission if it is a packet of interest.
        if (m_packetsOfInterest.find(packet->GetUid()) == m_packetsOfInterest.end())
        {
            NS_LOG_DEBUG("Packet " << packet->GetUid() << " is not of interest");
            return;
        }
    }

    // ---- "last packets"
    const PacketCaptureOptions* captureOptions;
    if (GetPacketCaptureOptions(nodeIndex, &captureOptions) &&
        FilterPacket(packet, *captureOptions))
    {
        LastPacketsSample& last = m_lastPackets[nodeIndex];
        PacketSample lastPacket;
        lastPacket.time = Simulator::Now();
        lastPacket.packet = packet->Copy();
        lastPacket.device = nullptr;
        last.lastDroppedPackets.push_back(lastPacket);
        while (last.lastDroppedPackets.size() > captureOptions->numLastPackets)
        {
            last.lastDroppedPackets.erase(last.lastDroppedPackets.begin());
        }
    }

    auto iter = m_packetDrops.find(node);
    if (iter == m_packetDrops.end())
    {
        m_packetDrops[node] = packet->GetSize();
    }
    else
    {
        iter->second += packet->GetSize();
    }
}

void
PyViz::TraceIpv4Drop(std::string context,
                     const ns3::Ipv4Header& hdr,
                     Ptr<const Packet> packet,
                     ns3::Ipv4L3Protocol::DropReason reason,
                     Ptr<Ipv4> dummy_ipv4,
                     uint32_t interface)
{
    Ptr<Packet> packetCopy = packet->Copy();
    packetCopy->AddHeader(hdr);
    TraceDevQueueDrop(context, packetCopy);
}

// --------- TX device tracing -------------------

void
PyViz::TraceNetDevTxCommon(
    const std::string& context,
    Ptr<const Packet> packet,
    const std::variant<Mac16Address, Mac48Address, Mac64Address>& destination)
{
    NS_LOG_FUNCTION(context << packet->GetUid() << *packet);

    std::vector<std::string> splitPath = PathSplit(context);
    int nodeIndex = std::stoi(splitPath[1]);
    int devIndex = std::stoi(splitPath[3]);
    Ptr<Node> node = NodeList::GetNode(nodeIndex);
    Ptr<NetDevice> device = node->GetDevice(devIndex);

    // ---- statistics
    NetDeviceStatistics& stats = FindNetDeviceStatistics(nodeIndex, devIndex);
    ++stats.transmittedPackets;
    stats.transmittedBytes += packet->GetSize();

    // ---- "last packets"
    const PacketCaptureOptions* captureOptions;
    if (GetPacketCaptureOptions(nodeIndex, &captureOptions) &&
        FilterPacket(packet, *captureOptions))
    {
        LastPacketsSample& last = m_lastPackets[nodeIndex];
        TxPacketSample lastPacket;
        lastPacket.time = Simulator::Now();
        lastPacket.packet = packet->Copy();
        lastPacket.device = device;
        lastPacket.to = destination;

        last.lastTransmittedPackets.push_back(lastPacket);
        while (last.lastTransmittedPackets.size() > captureOptions->numLastPackets)
        {
            last.lastTransmittedPackets.erase(last.lastTransmittedPackets.begin());
        }
    }

    // ---- transmissions records

    if (m_nodesOfInterest.find(nodeIndex) == m_nodesOfInterest.end())
    {
        // if the transmitting node is not "of interest", we still
        // record the transmission if it is a packet of interest.
        if (m_packetsOfInterest.find(packet->GetUid()) == m_packetsOfInterest.end())
        {
            NS_LOG_DEBUG("Packet " << packet->GetUid() << " is not of interest");
            return;
        }
    }
    else
    {
        // We will follow this packet throughout the network.
        m_packetsOfInterest[packet->GetUid()] = Simulator::Now();
    }

    TxRecordValue record = {Simulator::Now(), node, false};

    if (std::holds_alternative<Mac16Address>(destination))
    {
        if (std::get<Mac16Address>(destination) == Mac16Address("FF:FF"))
        {
            record.isBroadcast = true;
        }
    }
    else if (std::holds_alternative<Mac48Address>(destination))
    {
        if (std::get<Mac48Address>(destination) == device->GetBroadcast())
        {
            record.isBroadcast = true;
        }
    }
    else if (std::holds_alternative<Mac64Address>(destination))
    {
        if (std::get<Mac64Address>(destination) == Mac64Address("FF:FF:FF:FF:FF:FF:FF:FF"))
        {
            // Note: A broadcast using th the MAC 64 bit address is not really used in practice.
            // instead the 16 bit MAC address is used.
            record.isBroadcast = true;
        }
    }

    m_txRecords[TxRecordKey(device->GetChannel(), packet->GetUid())] = record;

    PyVizPacketTag tag;
    // packet->RemovePacketTag (tag);
    tag.m_packetId = packet->GetUid();
    packet->AddByteTag(tag);
}

void
PyViz::TraceNetDevTxWifi(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(context << packet->GetUid() << *packet);

    /*
     *  To DS    From DS   Address 1    Address 2    Address 3    Address 4
     *----------------------------------------------------------------------
     *    0        0       Destination  Source        BSSID         N/A
     *    0        1       Destination  BSSID         Source        N/A
     *    1        0       BSSID        Source        Destination   N/A
     *    1        1       Receiver     Transmitter   Destination   Source
     */
    WifiMacHeader hdr;
    NS_ABORT_IF(packet->PeekHeader(hdr) == 0);

    std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;
    if (hdr.IsToDs())
    {
        destinationAddress = hdr.GetAddr3();
    }
    else
    {
        destinationAddress = hdr.GetAddr1();
    }
    TraceNetDevTxCommon(context, packet, destinationAddress);
}

void
PyViz::TraceNetDevTxLrWpan(std::string context, Ptr<const Packet> packet)
{
    LrWpanMacHeader hdr;
    NS_ABORT_IF(packet->PeekHeader(hdr) == 0);
    std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;

    switch (hdr.GetDstAddrMode())
    {
    case lrwpan::AddressMode::EXT_ADDR:
        destinationAddress = hdr.GetExtDstAddr();
        break;
    default:
        destinationAddress = hdr.GetShortDstAddr();
        break;
    }

    TraceNetDevTxCommon(context, packet, destinationAddress);
}

void
PyViz::TraceNetDevTxCsma(std::string context, Ptr<const Packet> packet)
{
    EthernetHeader ethernetHeader;
    NS_ABORT_IF(packet->PeekHeader(ethernetHeader) == 0);
    std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;
    destinationAddress = ethernetHeader.GetDestination();
    TraceNetDevTxCommon(context, packet, destinationAddress);
}

void
PyViz::TraceNetDevTxPointToPoint(std::string context, Ptr<const Packet> packet)
{
    std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;
    destinationAddress = Mac48Address();
    TraceNetDevTxCommon(context, packet, destinationAddress);
}

// --------- RX device tracing -------------------

void
PyViz::TraceNetDevRxCommon(const std::string& context,
                           Ptr<const Packet> packet,
                           const std::variant<Mac16Address, Mac48Address, Mac64Address>& source)
{
    uint32_t uid;
    PyVizPacketTag tag;
    if (packet->FindFirstMatchingByteTag(tag))
    {
        uid = tag.m_packetId;
    }
    else
    {
        // NS_ASSERT (0);
        NS_LOG_WARN("Packet has no byte tag");
        uid = packet->GetUid();
    }

    NS_LOG_FUNCTION(context << uid);
    std::vector<std::string> splitPath = PathSplit(context);
    int nodeIndex = std::stoi(splitPath[1]);
    int devIndex = std::stoi(splitPath[3]);

    // ---- statistics
    NetDeviceStatistics& stats = FindNetDeviceStatistics(nodeIndex, devIndex);
    ++stats.receivedPackets;
    stats.receivedBytes += packet->GetSize();

    Ptr<Node> node = NodeList::GetNode(nodeIndex);
    Ptr<NetDevice> device = node->GetDevice(devIndex);

    // ---- "last packets"
    const PacketCaptureOptions* captureOptions;
    if (GetPacketCaptureOptions(nodeIndex, &captureOptions) &&
        FilterPacket(packet, *captureOptions))
    {
        LastPacketsSample& last = m_lastPackets[nodeIndex];
        RxPacketSample lastPacket;
        lastPacket.time = Simulator::Now();
        lastPacket.packet = packet->Copy();
        lastPacket.device = device;
        lastPacket.from = source;

        last.lastReceivedPackets.push_back(lastPacket);
        while (last.lastReceivedPackets.size() > captureOptions->numLastPackets)
        {
            last.lastReceivedPackets.erase(last.lastReceivedPackets.begin());
        }
    }

    // ---- transmissions
    if (m_packetsOfInterest.find(uid) == m_packetsOfInterest.end())
    {
        NS_LOG_DEBUG("RX Packet " << uid << " is not of interest");
        return;
    }

    Ptr<Channel> channel = device->GetChannel();

    auto recordIter = m_txRecords.find(TxRecordKey(channel, uid));

    if (recordIter == m_txRecords.end())
    {
        NS_LOG_DEBUG("RX Packet " << uid << " was not transmitted?!");
        return;
    }

    TxRecordValue& record = recordIter->second;

    if (record.srcNode == node)
    {
        NS_LOG_WARN("Node " << node->GetId() << " receiving back the same packet (UID=" << uid
                            << ") it had previously transmitted, on the same channel!");
        return;
    }

    TransmissionSampleKey key = {record.srcNode, node, channel};

#ifdef NS3_LOG_ENABLE
    NS_LOG_DEBUG("m_transmissionSamples begin:");
    if (g_log.IsEnabled(ns3::LOG_DEBUG))
    {
        for (auto iter = m_transmissionSamples.begin(); iter != m_transmissionSamples.end(); iter++)
        {
            NS_LOG_DEBUG(iter->first.transmitter
                         << "/" << iter->first.transmitter->GetId() << ", " << iter->first.receiver
                         << "/" << iter->first.receiver->GetId() << ", " << iter->first.channel
                         << " => " << iter->second.bytes << " (@ " << &iter->second << ")");
        }
    }
    NS_LOG_DEBUG("m_transmissionSamples end.");
#endif

    auto iter = m_transmissionSamples.find(key);

    if (iter == m_transmissionSamples.end())
    {
        TransmissionSampleValue sample = {packet->GetSize()};
        NS_LOG_DEBUG("RX: from " << key.transmitter << "/" << key.transmitter->GetId() << " to "
                                 << key.receiver << "/" << key.receiver->GetId() << " channel "
                                 << channel << ": " << packet->GetSize()
                                 << " bytes more. => new sample with " << packet->GetSize()
                                 << " bytes.");
        m_transmissionSamples[key] = sample;
    }
    else
    {
        TransmissionSampleValue& sample = iter->second;
        NS_LOG_DEBUG("RX: from " << key.transmitter << "/" << key.transmitter->GetId() << " to "
                                 << key.receiver << "/" << key.receiver->GetId() << " channel "
                                 << channel << ": " << packet->GetSize()
                                 << " bytes more. => sample " << &sample << " with bytes "
                                 << sample.bytes);

        sample.bytes += packet->GetSize();
    }
}

void
PyViz::TraceNetDevRxWifi(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(context << packet->GetUid());

    /*
     *  To DS    From DS   Address 1    Address 2    Address 3    Address 4
     *----------------------------------------------------------------------
     *    0        0       Destination  Source        BSSID         N/A
     *    0        1       Destination  BSSID         Source        N/A
     *    1        0       BSSID        Source        Destination   N/A
     *    1        1       Receiver     Transmitter   Destination   Source
     */
    WifiMacHeader hdr;
    NS_ABORT_IF(packet->PeekHeader(hdr) == 0);
    std::variant<Mac16Address, Mac48Address, Mac64Address> sourceAddress;
    if (!hdr.IsFromDs())
    {
        sourceAddress = hdr.GetAddr2();
    }
    else if (!hdr.IsToDs())
    {
        sourceAddress = hdr.GetAddr3();
    }
    else // if (hdr.IsToDs())
    {
        sourceAddress = hdr.GetAddr4();
    }

    TraceNetDevRxCommon(context, packet, sourceAddress);
}

void
PyViz::TraceNetDevRxLrWpan(std::string context, Ptr<const Packet> packet)
{
    LrWpanMacHeader hdr;
    NS_ABORT_IF(packet->PeekHeader(hdr) == 0);
    std::variant<Mac16Address, Mac48Address, Mac64Address> sourceAddress;
    switch (hdr.GetSrcAddrMode())
    {
    case lrwpan::AddressMode::EXT_ADDR:
        sourceAddress = hdr.GetExtSrcAddr();
        break;
    default:
        sourceAddress = hdr.GetShortSrcAddr();
        break;
    }

    TraceNetDevRxCommon(context, packet, sourceAddress);
}

void
PyViz::TraceNetDevRxCsma(std::string context, Ptr<const Packet> packet)
{
    EthernetHeader ethernetHeader;
    NS_ABORT_IF(packet->PeekHeader(ethernetHeader) == 0);
    std::variant<Mac16Address, Mac48Address, Mac64Address> sourceAddress;
    sourceAddress = ethernetHeader.GetSource();
    TraceNetDevRxCommon(context, packet, sourceAddress);
}

void
PyViz::TraceNetDevRxPointToPoint(std::string context, Ptr<const Packet> packet)
{
    std::variant<Mac16Address, Mac48Address, Mac64Address> sourceAddress;
    sourceAddress = Mac48Address();
    TraceNetDevRxCommon(context, packet, sourceAddress);
}

void
PyViz::TraceNetDevPromiscRxCsma(std::string context, Ptr<const Packet> packet)
{
    EthernetHeader ethernetHeader;
    NS_ABORT_IF(packet->PeekHeader(ethernetHeader) == 0);

    NetDevice::PacketType packetType = NetDevice::PACKET_OTHERHOST; // FIXME

    // Other packet types are already being received by
    // TraceNetDevRxCsma; we don't want to receive them twice.
    if (packetType == NetDevice::PACKET_OTHERHOST)
    {
        std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;
        destinationAddress = ethernetHeader.GetDestination();
        TraceNetDevRxCommon(context, packet, destinationAddress);
    }
}

void
PyViz::TraceNetDevTxLte(std::string context,
                        Ptr<const Packet> packet,
                        const Mac48Address& destination)
{
    NS_LOG_FUNCTION(context);
    std::variant<Mac16Address, Mac48Address, Mac64Address> destinationAddress;
    destinationAddress = destination;
    TraceNetDevTxCommon(context, packet, destinationAddress);
}

void
PyViz::TraceNetDevRxLte(std::string context, Ptr<const Packet> packet, const Mac48Address& source)
{
    NS_LOG_FUNCTION(context);
    std::variant<Mac16Address, Mac48Address, Mac64Address> sourceAddress;
    sourceAddress = source;
    TraceNetDevRxCommon(context, packet, sourceAddress);
}

// ---------------------

TransmissionSampleList
PyViz::GetTransmissionSamples() const
{
    NS_LOG_DEBUG("GetTransmissionSamples BEGIN");
    TransmissionSampleList list;
    for (auto iter = m_transmissionSamples.begin(); iter != m_transmissionSamples.end(); iter++)
    {
        TransmissionSample sample;
        sample.transmitter = iter->first.transmitter;
        sample.receiver = iter->first.receiver;
        sample.channel = iter->first.channel;
        sample.bytes = iter->second.bytes;
        NS_LOG_DEBUG("from " << sample.transmitter->GetId() << " to " << sample.receiver->GetId()
                             << ": " << sample.bytes << " bytes.");
        list.push_back(sample);
    }
    NS_LOG_DEBUG("GetTransmissionSamples END");
    return list;
}

PacketDropSampleList
PyViz::GetPacketDropSamples() const
{
    NS_LOG_DEBUG("GetPacketDropSamples BEGIN");
    PacketDropSampleList list;
    for (auto iter = m_packetDrops.begin(); iter != m_packetDrops.end(); iter++)
    {
        PacketDropSample sample;
        sample.transmitter = iter->first;
        sample.bytes = iter->second;
        NS_LOG_DEBUG("in " << sample.transmitter->GetId() << ": " << sample.bytes
                           << " bytes dropped.");
        list.push_back(sample);
    }
    NS_LOG_DEBUG("GetPacketDropSamples END");
    return list;
}

void
PyViz::SetNodesOfInterest(std::set<uint32_t> nodes)
{
    m_nodesOfInterest = nodes;
}

std::vector<NodeStatistics>
PyViz::GetNodesStatistics() const
{
    std::vector<NodeStatistics> retval;
    for (auto iter = m_nodesStatistics.begin(); iter != m_nodesStatistics.end(); iter++)
    {
        NodeStatistics stats = {iter->first, iter->second};
        retval.push_back(stats);
    }
    return retval;
}

LastPacketsSample
PyViz::GetLastPackets(uint32_t nodeId) const
{
    NS_LOG_DEBUG("GetLastPackets: " << nodeId);

    auto iter = m_lastPackets.find(nodeId);
    if (iter != m_lastPackets.end())
    {
        return iter->second;
    }
    else
    {
        return LastPacketsSample();
    }
}

void
PyViz::LineClipping(double boundsX1,
                    double boundsY1,
                    double boundsX2,
                    double boundsY2,
                    double& lineX1,
                    double& lineY1,
                    double& lineX2,
                    double& lineY2)
{
    FastClipping::Vector2 clipMin = {boundsX1, boundsY1};
    FastClipping::Vector2 clipMax = {boundsX2, boundsY2};
    FastClipping::Line line = {{lineX1, lineY1},
                               {lineX2, lineY2},
                               (lineX2 - lineX1),
                               (lineY2 - lineY1)};

    FastClipping clipper(clipMin, clipMax);
    clipper.ClipLine(line);
    lineX1 = line.start.x;
    lineX2 = line.end.x;
    lineY1 = line.start.y;
    lineY2 = line.end.y;
}

} // namespace ns3
