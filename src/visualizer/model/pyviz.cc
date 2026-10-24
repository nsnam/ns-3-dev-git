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
#include "ns3/double.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/simulator.h"

#include <cstdlib>
#include <functional>
#include <sstream>

NS_LOG_COMPONENT_DEFINE("PyViz");

/**
 * @ingroup visualizer
 * Initial playback speed, settable with --PyVizSpeed; read by the Python visualizer.
 */
static ns3::GlobalValue g_pyVizSpeed(
    "PyVizSpeed",
    "Initial visualizer speed, in simulated seconds per wall-clock "
    "second (0 keeps the default)",
    ns3::DoubleValue(0),
    ns3::MakeDoubleChecker<double>(0));

/**
 * @ingroup visualizer
 * Time to skip to, settable with --PyVizStartTime; read by the Python visualizer.
 */
static ns3::GlobalValue g_pyVizStartTime(
    "PyVizStartTime",
    "Simulated seconds to run without animation before the "
    "visualizer shows the simulation (0 starts at the beginning)",
    ns3::DoubleValue(0),
    ns3::MakeDoubleChecker<double>(0));

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

namespace visualizer
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
    static TypeId tid = TypeId("ns3::visualizer::PyVizPacketTag")
                            .AddDeprecatedName("ns3::PyVizPacketTag")
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
}

bool
PyViz::ConnectTrace(const std::string& handler, const std::string& path)
{
    NS_LOG_FUNCTION(this << handler << path);
    // Generic handlers only: Python maps each technology's trace sources to
    // them (and adapts other signatures), so this module does not need to
    // link against any device or protocol module.
    static const std::map<std::string, std::function<CallbackBase(PyViz*)>> handlers = {
        {"Tx", [](PyViz* p) { return MakeCallback(&PyViz::TraceTx, p); }},
        {"Rx", [](PyViz* p) { return MakeCallback(&PyViz::TraceRx, p); }},
        {"Drop", [](PyViz* p) { return MakeCallback(&PyViz::TraceDrop, p); }},
        {"QueueDrop", [](PyViz* p) { return MakeCallback(&PyViz::TraceQueueDrop, p); }},
        {"ChannelDrop", [](PyViz* p) { return MakeCallback(&PyViz::TraceChannelDrop, p); }},
    };
    auto it = handlers.find(handler);
    if (it == handlers.end())
    {
        return false;
    }
    return Config::ConnectFailSafe(path, it->second(this));
}

void
PyViz::RegisterCsmaLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/MacTx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceTx, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceRx, this));
}

void
PyViz::RegisterWifiLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Tx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceTx, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceRx, this));
}

void
PyViz::RegisterPointToPointLikeDevice(const std::string& deviceTypeName)
{
    TypeId::LookupByName(deviceTypeName); // this will assert if the type name is invalid

    std::ostringstream sstream;
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/TxQueue/Dequeue";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceTx, this));

    sstream.str("");
    sstream << "/NodeList/*/DeviceList/*/$" << deviceTypeName << "/Rx";
    Config::Connect(sstream.str(), MakeCallback(&PyViz::TraceRx, this));
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
    Config::Connect(tracePath, MakeCallback(&PyViz::TraceDrop, this));
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

bool
PyViz::SimulatorRunUntil(Time time)
{
    NS_LOG_LOGIC("SimulatorRunUntil " << time << " (now is " << Simulator::Now() << ")");

    m_pauseMessages.clear();
    m_transmissionSamples.clear();
    m_packetDrops.clear();
    m_packetDropCounts.clear();
    m_nodeTxBytes.clear();

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
        return true;
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

    RunSimulator();
    // m_stop is set only by CallbackStopSimulation and Pause(), so it stays
    // false when the run ended because no events remain or the simulation
    // called Simulator::Stop().
    return m_stop;
}

void
PyViz::SimulatorRun()
{
    // Stop callbacks scheduled by earlier steps must not end this run.
    m_runUntil = Time::Max();
    RunSimulator();
}

void
PyViz::RunSimulator()
{
    Ptr<VisualSimulatorImpl> visualImpl =
        DynamicCast<VisualSimulatorImpl>(Simulator::GetImplementation());
    if (visualImpl)
    {
        visualImpl->RunRealSimulator();
    }
    else
    {
        // Simulator::Run() rather than impl->Run(): it also stops Time from
        // tracking every Time object for resolution changes, which otherwise
        // grows a set on each Time construction and slows the run down.
        Simulator::Run();
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

// --------- generic trace handlers -------------------

namespace
{
/**
 * Extract the node and device indices from a trace context.
 *
 * @param context Trace context of the form "/NodeList/N/DeviceList/D/...".
 * @return Pair of node index N and device index D (0 if absent).
 */
std::pair<uint32_t, uint32_t>
ContextIndices(const std::string& context)
{
    std::vector<std::string> splitPath = PathSplit(context);
    uint32_t device =
        splitPath.size() > 3 && splitPath[2] == "DeviceList" ? std::stoi(splitPath[3]) : 0;
    return {static_cast<uint32_t>(std::stoi(splitPath[1])), device};
}
} // namespace

void
PyViz::TraceTx(std::string context, Ptr<const Packet> packet)
{
    auto [node, device] = ContextIndices(context);
    RecordTx(node, device, packet);
}

void
PyViz::TraceRx(std::string context, Ptr<const Packet> packet)
{
    auto [node, device] = ContextIndices(context);
    RecordRx(node, device, packet);
}

void
PyViz::TraceDrop(std::string context, Ptr<const Packet> packet)
{
    RecordDrop(ContextIndices(context).first, packet);
}

void
PyViz::TraceQueueDrop(std::string context, Ptr<const Packet> packet)
{
    RecordDrop(ContextIndices(context).first, packet, "queue");
}

void
PyViz::TraceChannelDrop(std::string context, Ptr<const Packet> packet)
{
    RecordDrop(ContextIndices(context).first, packet, "channel");
}

void
PyViz::RecordDrop(uint32_t nodeIndex, Ptr<const Packet> packet, const std::string& kind)
{
    NS_LOG_FUNCTION(nodeIndex << packet->GetUid());
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

    const auto key = std::make_pair(node, kind);
    m_packetDropCounts[key]++;
    m_packetDrops[key] += packet->GetSize();
}

// --------- TX device tracing -------------------

void
PyViz::RecordTx(uint32_t nodeIndex, uint32_t devIndex, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(nodeIndex << devIndex << packet->GetUid());
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

        last.lastTransmittedPackets.push_back(lastPacket);
        while (last.lastTransmittedPackets.size() > captureOptions->numLastPackets)
        {
            last.lastTransmittedPackets.erase(last.lastTransmittedPackets.begin());
        }
    }

    // ---- transmitters of the last packets samples
    constexpr std::size_t MAX_RECENT_TRANSMISSIONS = 4096;
    TxRecordKey recentKey(device->GetChannel(), packet->GetUid());
    if (m_recentTransmitters.insert_or_assign(recentKey, device).second)
    {
        m_recentTransmissionOrder.push_back(recentKey);
        if (m_recentTransmissionOrder.size() > MAX_RECENT_TRANSMISSIONS)
        {
            m_recentTransmitters.erase(m_recentTransmissionOrder.front());
            m_recentTransmissionOrder.pop_front();
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

    m_nodeTxBytes[node] += packet->GetSize();

    TxRecordValue record = {Simulator::Now(), node, false};
    m_txRecords[TxRecordKey(device->GetChannel(), packet->GetUid())] = record;

    PyVizPacketTag tag;
    // packet->RemovePacketTag (tag);
    tag.m_packetId = packet->GetUid();
    packet->AddByteTag(tag);
}

// --------- RX device tracing -------------------

void
PyViz::RecordRx(uint32_t nodeIndex, uint32_t devIndex, Ptr<const Packet> packet)
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

    NS_LOG_FUNCTION(nodeIndex << devIndex << uid);

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
        if (auto transmitter = m_recentTransmitters.find({device->GetChannel(), packet->GetUid()});
            transmitter != m_recentTransmitters.end())
        {
            lastPacket.from = transmitter->second->GetAddress();
        }

        last.lastReceivedPackets.push_back(lastPacket);
        while (last.lastReceivedPackets.size() > captureOptions->numLastPackets)
        {
            last.lastReceivedPackets.erase(last.lastReceivedPackets.begin());
        }
    }

    // ---- destination of the transmitter's last packets sample
    if (auto transmitter = m_recentTransmitters.find({device->GetChannel(), packet->GetUid()});
        transmitter != m_recentTransmitters.end())
    {
        Ptr<NetDevice> txDevice = transmitter->second;
        auto txLast = m_lastPackets.find(txDevice->GetNode()->GetId());
        if (txLast != m_lastPackets.end())
        {
            for (auto& sample : txLast->second.lastTransmittedPackets)
            {
                if (sample.device != txDevice || sample.packet->GetUid() != packet->GetUid())
                {
                    continue;
                }
                if (sample.to.IsInvalid())
                {
                    sample.to = device->GetAddress();
                }
                else if (sample.to != device->GetAddress())
                {
                    // Received by several devices.
                    sample.to = txDevice->GetBroadcast();
                }
            }
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

    m_packetFlowSamples.push_back(
        {record.time, Simulator::Now(), record.srcNode, node, packet->GetSize(), uid});
    constexpr std::size_t MAX_PACKET_FLOW_SAMPLES = 512;
    if (m_packetFlowSamples.size() > MAX_PACKET_FLOW_SAMPLES)
    {
        m_packetFlowSamples.erase(m_packetFlowSamples.begin());
    }

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

std::vector<PacketFlowSample>
PyViz::GetPacketFlowSamples() const
{
    return m_packetFlowSamples;
}

std::vector<NodeTxSample>
PyViz::GetNodeTxSamples() const
{
    std::vector<NodeTxSample> samples;
    for (const auto& [node, bytes] : m_nodeTxBytes)
    {
        samples.push_back({node, bytes});
    }
    return samples;
}

PacketDropSampleList
PyViz::GetPacketDropSamples() const
{
    NS_LOG_DEBUG("GetPacketDropSamples BEGIN");
    PacketDropSampleList list;
    for (auto iter = m_packetDrops.begin(); iter != m_packetDrops.end(); iter++)
    {
        PacketDropSample sample;
        sample.transmitter = iter->first.first;
        sample.kind = iter->first.second;
        sample.bytes = iter->second;
        sample.packets = m_packetDropCounts.at(iter->first);
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

} // namespace visualizer
} // namespace ns3
