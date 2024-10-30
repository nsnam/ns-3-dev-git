/*
 * Copyright (c) 2023 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "wifi-phy-rx-trace-helper.h"

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/fatal-error.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/yans-wifi-phy.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPhyRxTraceHelper");

/**
 * Number of places to shift WifiPpdu UID values, when generating unique IDs
 */
const uint32_t SHIFT = 16;

WifiPhyRxTraceHelper::WifiPhyRxTraceHelper()
{
    NS_LOG_FUNCTION(this);
}

void
WifiPhyRxTraceHelper::Enable(NodeContainer nodes)
{
    NS_LOG_FUNCTION(this << nodes.GetN());
    Enable(nodes, MapMacAddressesToNodeIds(nodes));
}

void
WifiPhyRxTraceHelper::Enable(NetDeviceContainer netDevices)
{
    NS_LOG_FUNCTION(this << netDevices.GetN());
    NodeContainer nodes;
    // filter netDevice entries through a set to prevent duplicate entries in the NodeContainer
    std::set<Ptr<Node>> nodesSeen;
    for (uint32_t i = 0; i < netDevices.GetN(); i++)
    {
        auto nodePtr = netDevices.Get(i)->GetNode();
        if (nodesSeen.count(nodePtr) == 0)
        {
            nodesSeen.insert(nodePtr);
            nodes.Add(nodePtr);
        }
    }
    Enable(nodes, MapMacAddressesToNodeIds(nodes));
}

void
WifiPhyRxTraceHelper::Enable(NodeContainer nodes,
                             const std::map<Mac48Address, uint32_t>& macToNodeMap)
{
    NS_LOG_FUNCTION(this << nodes.GetN() << macToNodeMap.size());
    NS_ABORT_MSG_IF(m_traceSink, "A trace sink is already configured for this helper");
    m_traceSink = CreateObject<WifiPhyRxTraceSink>();
    m_traceSink->SetMapMacAddressToNodeId(macToNodeMap);
    for (uint32_t nodeId = 0; nodeId < nodes.GetN(); nodeId++)
    {
        for (uint32_t deviceId = 0; deviceId < nodes.Get(nodeId)->GetNDevices(); deviceId++)
        {
            auto wifiDevice = DynamicCast<WifiNetDevice>(nodes.Get(nodeId)->GetDevice(deviceId));
            if (wifiDevice)
            {
                for (uint8_t i = 0; i < wifiDevice->GetNPhys(); i++)
                {
                    auto phy = wifiDevice->GetPhy(i);
                    auto yansPhy = DynamicCast<YansWifiPhy>(phy);
                    auto spectrumPhy = DynamicCast<SpectrumWifiPhy>(phy);
                    NS_ASSERT_MSG(yansPhy || spectrumPhy, "Phy type not found");
                    const std::string context =
                        "/NodeList/" + std::to_string(wifiDevice->GetNode()->GetId()) +
                        "/DeviceList/" + std::to_string(wifiDevice->GetIfIndex()) + "/Phys/" +
                        std::to_string(i);
                    auto connected = false;
                    if (yansPhy)
                    {
                        connected = yansPhy->TraceConnect(
                            "SignalArrival",
                            context,
                            MakeCallback(&WifiPhyRxTraceSink::PhySignalArrival, m_traceSink));
                    }
                    else if (spectrumPhy)
                    {
                        connected = spectrumPhy->TraceConnect(
                            "SignalArrival",
                            context,
                            MakeCallback(&WifiPhyRxTraceSink::SpectrumPhySignalArrival,
                                         m_traceSink));
                    }
                    else
                    {
                        NS_FATAL_ERROR("Phy type not found");
                    }
                    NS_ASSERT_MSG(connected, "Trace source not connected");
                    connected = phy->TraceConnect(
                        "SignalTransmission",
                        context,
                        MakeCallback(&WifiPhyRxTraceSink::PhySignalTransmission, m_traceSink));
                    NS_ASSERT_MSG(connected, "Trace source not connected");
                    // Log packet drops
                    connected = phy->TraceConnect(
                        "PhyRxPpduDrop",
                        context,
                        MakeCallback(&WifiPhyRxTraceSink::PhyPpduDrop, m_traceSink));
                    NS_ASSERT_MSG(connected, "Trace source not connected");
                    // Trace PHY Outcome events
                    connected = phy->GetState()->TraceConnect(
                        "RxOutcome",
                        context,
                        MakeCallback(&WifiPhyRxTraceSink::PpduOutcome, m_traceSink));
                    NS_ASSERT_MSG(connected, "Trace source not connected");
                }
            }
        }
    }

    // If a map is provided use map if not populate it yourself
    if (!macToNodeMap.empty())
    {
        m_traceSink->SetMapMacAddressToNodeId(macToNodeMap);
    }
    // A linkID in one device might not be the same link on another device, need to create a mapping
    // from node to link
    m_traceSink->MapNodeToLinkToChannel(nodes);

    if (!m_traceSink->IsCollectionPeriodActive())
    {
        NS_LOG_DEBUG("Connected traces but collection period is not active");
    }
}

void
WifiPhyRxTraceHelper::PrintStatistics() const
{
    m_traceSink->PrintStatistics();
}

void
WifiPhyRxTraceHelper::PrintStatistics(Ptr<Node> node, uint32_t deviceId, uint8_t linkId) const
{
    m_traceSink->PrintStatistics(node->GetId(), deviceId, linkId);
}

void
WifiPhyRxTraceHelper::PrintStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    m_traceSink->PrintStatistics(nodeId, deviceId, linkId);
}

void
WifiPhyRxTraceHelper::Start(Time startTime)
{
    NS_LOG_FUNCTION(this << startTime.As(Time::S));
    Simulator::Schedule(startTime, &WifiPhyRxTraceSink::Start, m_traceSink);
}

void
WifiPhyRxTraceHelper::Stop(Time stopTime)
{
    NS_LOG_FUNCTION(this << stopTime.As(Time::S));
    Simulator::Schedule(stopTime, &WifiPhyRxTraceSink::Stop, m_traceSink);
}

const std::vector<WifiPpduRxRecord>&
WifiPhyRxTraceHelper::GetPpduRecords() const
{
    m_traceSink->CreateVectorFromRecords();
    return m_traceSink->GetPpduRecords();
}

std::optional<std::reference_wrapper<const std::vector<WifiPpduRxRecord>>>
WifiPhyRxTraceHelper::GetPpduRecords(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    return m_traceSink->GetPpduRecords(nodeId, deviceId, linkId);
}

void
WifiPhyRxTraceHelper::Reset()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Reset WifiPhyRxTraceHelper");
    m_traceSink->Reset();
}

WifiPhyTraceStatistics
WifiPhyRxTraceHelper::GetStatistics() const
{
    return m_traceSink->GetStatistics();
}

WifiPhyTraceStatistics
WifiPhyRxTraceHelper::GetStatistics(Ptr<Node> node, uint32_t deviceId, uint8_t linkId) const
{
    return m_traceSink->GetStatistics(node->GetId(), deviceId, linkId);
}

WifiPhyTraceStatistics
WifiPhyRxTraceHelper::GetStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    return m_traceSink->GetStatistics(nodeId, deviceId, linkId);
}

std::map<Mac48Address, uint32_t>
WifiPhyRxTraceHelper::MapMacAddressesToNodeIds(NodeContainer nodes) const
{
    std::map<Mac48Address, uint32_t> macAddressToNodeId;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        for (uint32_t device = 0; device < nodes.Get(i)->GetNDevices(); device++)
        {
            if (auto wdev = DynamicCast<WifiNetDevice>(nodes.Get(i)->GetDevice(device)))
            {
                for (uint32_t link = 0; link < wdev->GetNPhys(); link++)
                {
                    macAddressToNodeId[Mac48Address::ConvertFrom(
                        wdev->GetMac()->GetFrameExchangeManager(link)->GetAddress())] =
                        nodes.Get(i)->GetId();
                    NS_LOG_DEBUG("NodeID: "
                                 << nodes.Get(i)->GetId() << " DeviceID: " << device
                                 << " LinkId: " << link << " MAC: "
                                 << wdev->GetMac()->GetFrameExchangeManager(link)->GetAddress());
                }
            }
        }
    }
    return macAddressToNodeId;
}

NS_OBJECT_ENSURE_REGISTERED(WifiPhyRxTraceSink);

TypeId
WifiPhyRxTraceSink::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiPhyRxTraceSink")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiPhyRxTraceSink>();
    return tid;
}

WifiPhyRxTraceSink::WifiPhyRxTraceSink()
{
    NS_LOG_FUNCTION(this);
}

void
WifiPhyRxTraceSink::SetMapMacAddressToNodeId(
    const std::map<Mac48Address, uint32_t>& MacAddressToNodeIdMap)
{
    m_macAddressToNodeId = MacAddressToNodeIdMap;
}

uint32_t
WifiPhyRxTraceSink::ContextToNodeId(std::string context) const
{
    const auto sub = context.substr(10);
    const auto pos = sub.find("/Device");
    return std::stoi(sub.substr(0, pos));
}

int
WifiPhyRxTraceSink::ContextToLinkId(std::string context) const
{
    const auto pos = context.find("/Phys/");
    const auto sub = context.substr(pos + 6);
    const auto nextSlashPos = sub.find('/');
    if (nextSlashPos == std::string::npos)
    {
        return std::stoi(sub);
    }
    const auto physNumberStr = sub.substr(0, nextSlashPos);
    return std::stoi(physNumberStr);
}

int
WifiPhyRxTraceSink::ContextToDeviceId(std::string context) const
{
    auto pos = context.find("/DeviceList/");
    pos += 12;
    const auto sub = context.substr(pos);
    const auto nextSlashPos = sub.find('/');
    const auto deviceIdStr = sub.substr(0, nextSlashPos);
    return std::stoi(deviceIdStr);
}

std::string
WifiPhyRxTraceSink::ContextToTuple(std::string context) const
{
    const auto str = std::to_string(ContextToNodeId(context)) + ":" +
                     std::to_string(ContextToDeviceId(context)) + ":" +
                     std::to_string(ContextToLinkId(context));
    return str;
}

// Each WifiPpdu has a UID, but more than one trace record can exist for a unique WifiPpdu
// This generator shifts the WifiPpdu UID leftward and combines it with a member variable
// counter to generate more than one unique ID for each WifiPpdu UID
uint64_t
WifiPhyRxTraceSink::UniqueTagGenerator::GenerateUniqueTag(uint64_t ppduUid)
{
    uint64_t tag;
    do
    {
        tag = (ppduUid << SHIFT) | counter++;
    } while (usedTags.find(tag) != usedTags.end());
    usedTags.insert(tag);
    NS_LOG_DEBUG("Generating tag " << tag << " from ppdu UID " << ppduUid);
    return tag;
}

bool
operator==(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs)
{
    return lhs.m_rxTag == rhs.m_rxTag;
}

bool
operator!=(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs)
{
    return !(lhs == rhs);
}

bool
operator<(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs)
{
    return lhs.m_rxTag < rhs.m_rxTag;
}

WifiPhyTraceStatistics
operator+(const WifiPhyTraceStatistics& lhs, const WifiPhyTraceStatistics& rhs)
{
    WifiPhyTraceStatistics result;
    result.m_overlappingPpdus = lhs.m_overlappingPpdus + rhs.m_overlappingPpdus;
    result.m_nonOverlappingPpdus = lhs.m_nonOverlappingPpdus + rhs.m_nonOverlappingPpdus;
    result.m_failedPpdus = lhs.m_failedPpdus + rhs.m_failedPpdus;
    result.m_receivedPpdus = lhs.m_receivedPpdus + rhs.m_receivedPpdus;
    result.m_receivedMpdus = lhs.m_receivedMpdus + rhs.m_receivedMpdus;
    result.m_failedMpdus = lhs.m_failedMpdus + rhs.m_failedMpdus;

    // Merge map fields
    for (const auto& pair : lhs.m_ppduDropReasons)
    {
        result.m_ppduDropReasons[pair.first] += pair.second;
    }
    for (const auto& pair : rhs.m_ppduDropReasons)
    {
        result.m_ppduDropReasons[pair.first] += pair.second;
    }
    return result;
}

bool
operator==(const WifiPhyTraceStatistics& lhs, const WifiPhyTraceStatistics& rhs)
{
    return lhs.m_overlappingPpdus == rhs.m_overlappingPpdus &&
           lhs.m_nonOverlappingPpdus == rhs.m_nonOverlappingPpdus &&
           lhs.m_failedPpdus == rhs.m_failedPpdus && lhs.m_receivedPpdus == rhs.m_receivedPpdus &&
           lhs.m_receivedMpdus == rhs.m_receivedMpdus && lhs.m_failedMpdus == rhs.m_failedMpdus &&
           lhs.m_ppduDropReasons == rhs.m_ppduDropReasons;
}

bool
operator!=(const WifiPhyTraceStatistics& lhs, const WifiPhyTraceStatistics& rhs)
{
    return !(lhs == rhs);
}

void
WifiPhyRxTraceSink::MapNodeToLinkToChannel(NodeContainer nodes)
{
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        for (uint32_t device = 0; device < nodes.Get(i)->GetNDevices(); device++)
        {
            auto wdev = DynamicCast<WifiNetDevice>(nodes.Get(i)->GetDevice(device));
            if (wdev)
            {
                for (uint32_t link = 0; link < wdev->GetNPhys(); link++)
                {
                    m_nodeToDeviceToLinkToChannelInfo[nodes.Get(i)->GetId()][wdev->GetIfIndex()]
                                                     [link] = std::make_pair(
                                                         wdev->GetPhy(link)->GetChannelNumber(),
                                                         wdev->GetPhy(link)->GetFrequency());

                    NS_LOG_DEBUG("NodeId: "
                                 << nodes.Get(i)->GetId() << " DeviceID: " << wdev->GetIfIndex()
                                 << " LinkId: " << link
                                 << " freq: " << wdev->GetPhy(link)->GetFrequency()
                                 << " ch#: " << int(wdev->GetPhy(link)->GetChannelNumber()));
                }
            }
        }
    }
}

// channel number, frequency
std::optional<std::pair<uint8_t, uint16_t>>
WifiPhyRxTraceSink::GetChannelInfo(uint32_t nodeId, uint32_t deviceId, int link) const
{
    auto nodeIter = m_nodeToDeviceToLinkToChannelInfo.find(nodeId);
    if (nodeIter != m_nodeToDeviceToLinkToChannelInfo.end())
    {
        auto& deviceMap = nodeIter->second;
        auto deviceIter = deviceMap.find(deviceId);
        if (deviceIter != deviceMap.end())
        {
            auto& linkMap = deviceIter->second;
            auto linkIter = linkMap.find(link);
            if (linkIter != linkMap.end())
            {
                return std::optional<std::pair<uint8_t, uint16_t>>(linkIter->second);
            }
        }
    }
    return std::nullopt;
}

// TODO: Profile this and possibly refactor to avoid duplicates
void
WifiPhyRxTraceSink::UpdateCurrentlyReceivedSignal(uint32_t nodeId,
                                                  uint32_t deviceId,
                                                  uint8_t linkId)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << linkId);
    // Add all overlapping packets to the list of overlapping packets for this frame arrival
    for (const auto& it : m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(linkId))
    {
        for (const auto& iter : m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(linkId))
        {
            if (it != iter)
            {
                m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].emplace_back(iter);
            }
        }
    }

    // Remove duplicates
    for (const auto& it : m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(linkId))
    {
        // First, sort the vector to bring duplicates together
        std::sort(m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].begin(),
                  m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].end());
        // Use std::unique to remove duplicates (adjacent identical elements)
        m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].erase(
            std::unique(m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].begin(),
                        m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].end()),
            m_rxTagToListOfOverlappingPpduRecords[it.m_rxTag].end());
    }
    NS_LOG_INFO(
        "Map of overlapping PPDU records size: " << m_rxTagToListOfOverlappingPpduRecords.size());
}

void
WifiPhyRxTraceSink::EndTx(uint32_t nodeId, uint32_t deviceId, WifiPpduRxRecord ppduRecord)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId);
    // remove from currently received (in this case transmitted) packets
    NS_LOG_INFO("Remove transmit record at " << nodeId << ":" << deviceId << ":"
                                             << +ppduRecord.m_linkId << " UID "
                                             << ppduRecord.m_rxTag);
    m_nodeDeviceLinkRxRecords.at(nodeId)
        .at(deviceId)
        .at(ppduRecord.m_linkId)
        .erase(
            std::remove(
                m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).begin(),
                m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).end(),
                ppduRecord),
            m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).end());
    NS_LOG_INFO("Size of m_nodeDeviceLinkRxRecords: " << m_nodeDeviceLinkRxRecords.size());

    // Erase the item from rxTagToPpduRecord
    m_rxTagToPpduRecord.erase(ppduRecord.m_rxTag);
    NS_LOG_INFO("Size of m_rxTagToPpduRecord: " << m_rxTagToPpduRecord.size());
}

void
WifiPhyRxTraceSink::PhyRxEnd(uint32_t nodeId, uint32_t deviceId, uint64_t rxTag, uint64_t ppduUid)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << rxTag << ppduUid);
    NS_ASSERT_MSG(m_rxTagToPpduRecord.contains(rxTag), "Missing PPDU at PhyRxEnd");
    // Update the end time on the record, and reinsert to the map
    auto ppduRecord = m_rxTagToPpduRecord[rxTag];
    ppduRecord.m_endTime = Simulator::Now();
    auto [it, inserted] = m_rxTagToPpduRecord.insert_or_assign(rxTag, ppduRecord);
    NS_ASSERT_MSG(!inserted, "Did not assign successfully");

    // Update the lists of overlapping PPDUs
    UpdateCurrentlyReceivedSignal(nodeId, deviceId, ppduRecord.m_linkId);
    for (const auto& it : m_rxTagToListOfOverlappingPpduRecords[rxTag])
    {
        ppduRecord.m_overlappingPpdu.emplace_back(it);
    }

    NS_LOG_INFO("Remove reception record at " << nodeId << ":" << deviceId << ":"
                                              << +ppduRecord.m_linkId << " UID " << rxTag);
    // Remove from map of active records
    m_nodeDeviceLinkRxRecords.at(nodeId)
        .at(deviceId)
        .at(ppduRecord.m_linkId)
        .erase(
            std::remove(
                m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).begin(),
                m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).end(),
                ppduRecord),
            m_nodeDeviceLinkRxRecords.at(nodeId).at(deviceId).at(ppduRecord.m_linkId).end());
    NS_LOG_INFO("Size of m_nodeDeviceLinkRxRecords: " << m_nodeDeviceLinkRxRecords.size());

    // Erase the item from rxTagToPpduRecord
    m_rxTagToPpduRecord.erase(rxTag);
    NS_LOG_INFO("Size of m_rxTagToPpduRecord: " << m_rxTagToPpduRecord.size());

    // checks if statistics period has been started before adding to the list of completed records
    if (m_statisticsCollectionPeriodStarted)
    {
        NS_LOG_INFO("Adding PPDU record for " << ppduRecord.m_receiverId << " " << deviceId << " "
                                              << +ppduRecord.m_linkId);
        m_completedRecords[ppduRecord.m_receiverId][deviceId][ppduRecord.m_linkId].emplace_back(
            ppduRecord);
    }
    else
    {
        NS_LOG_DEBUG("Not adding PPDU record (statistics not started) for "
                     << ppduRecord.m_receiverId << " " << deviceId << " " << ppduRecord.m_linkId);
    }
}

bool
WifiPhyRxTraceSink::IsCollectionPeriodActive() const
{
    return m_statisticsCollectionPeriodStarted;
}

void
WifiPhyRxTraceSink::Start()
{
    m_statisticsCollectionPeriodStarted = true;
}

void
WifiPhyRxTraceSink::Stop()
{
    m_statisticsCollectionPeriodStarted = false;
}

void
WifiPhyRxTraceSink::PhySignalTransmission(std::string context,
                                          Ptr<const WifiPpdu> ppdu,
                                          const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(context << ppdu << txVector);
    uint32_t nodeId = ContextToNodeId(context);
    uint32_t deviceId = ContextToDeviceId(context);
    uint8_t linkId = ContextToLinkId(context);

    WifiPpduRxRecord ppduRecord;
    ppduRecord.m_startTime = Simulator::Now();
    ppduRecord.m_endTime = Simulator::Now() + ppdu->GetTxDuration();
    ppduRecord.m_ppdu = ppdu;
    ppduRecord.m_senderId = nodeId;
    ppduRecord.m_linkId = linkId;
    ppduRecord.m_senderDeviceId = deviceId;

    // Tag used to determine which packet needs to be removed from currently received packets
    ppduRecord.m_rxTag = m_tagGenerator.GenerateUniqueTag(ppdu->GetUid());

    NS_LOG_INFO("Transmit at " << ContextToTuple(context) << " insert to tagToRecord map for UID "
                               << ppduRecord.m_rxTag);
    auto [it, inserted] = m_rxTagToPpduRecord.insert_or_assign(ppduRecord.m_rxTag, ppduRecord);
    NS_ASSERT_MSG(inserted, "Did not insert successfully");
    NS_LOG_INFO("Size of m_rxTagToPpduRecord: " << m_rxTagToPpduRecord.size());

    /*
        m_rxTagToPpduRecord[ppduRecord.m_rxTag] = ppduRecord;
    */

    // Use this to find who is the sender in the reception side
    m_ppduUidToTxTag[ppduRecord.m_ppdu->GetUid()] = ppduRecord.m_rxTag;

    // store record in a map, indexed by nodeId, deviceId, linkId
    NS_LOG_INFO("Transmit at " << ContextToTuple(context)
                               << " insert to active records map for UID " << ppduRecord.m_rxTag);
    m_nodeDeviceLinkRxRecords[nodeId][deviceId][linkId].emplace_back(ppduRecord);
    NS_LOG_INFO("Size of active records: " << m_nodeDeviceLinkRxRecords.size());

    Simulator::Schedule(ppduRecord.m_endTime - ppduRecord.m_startTime,
                        &WifiPhyRxTraceSink::EndTx,
                        this,
                        nodeId,
                        deviceId,
                        ppduRecord);
}

void
WifiPhyRxTraceSink::SpectrumPhySignalArrival(std::string context,
                                             Ptr<const SpectrumSignalParameters> signal,
                                             uint32_t senderNodeId,
                                             double rxPower,
                                             Time duration)
{
    NS_LOG_FUNCTION(context << signal << senderNodeId << rxPower << duration);
    const auto wifiSignal = DynamicCast<const WifiSpectrumSignalParameters>(signal);
    const auto ppdu = wifiSignal->ppdu;

    if (!wifiSignal)
    {
        NS_LOG_DEBUG("Non-WiFi Signal Received");
        return;
    }
    PhySignalArrival(context, ppdu, rxPower, duration);
}

void
WifiPhyRxTraceSink::PhySignalArrival(std::string context,
                                     Ptr<const WifiPpdu> ppdu,
                                     double rxPower,
                                     Time duration)
{
    NS_LOG_FUNCTION(context << ppdu << rxPower << duration);
    uint32_t nodeId = ContextToNodeId(context);
    uint32_t deviceId = ContextToDeviceId(context);
    uint8_t linkId = ContextToLinkId(context);

    // Associate this received PPDU with a record previously stored on the transmit side, if present
    bool hasTxTag = false;
    WifiPpduRxRecord txPpduRecord;
    if (m_ppduUidToTxTag.contains(ppdu->GetUid()))
    {
        hasTxTag = true;
        txPpduRecord = m_rxTagToPpduRecord[m_ppduUidToTxTag[ppdu->GetUid()]];
        NS_LOG_DEBUG("Arrival RxNodeID: " << nodeId << " SenderID: " << txPpduRecord.m_senderId
                                          << " Received on LinkID: " << +linkId
                                          << " Frame Sent on LinkId: " << +txPpduRecord.m_linkId);
    }
    else
    {
        NS_LOG_DEBUG("Arrival RxNodeID: " << nodeId << " Received on LinkID: " << +linkId
                                          << "; no sender info");
    }
    if (hasTxTag)
    {
        auto rxInfo = GetChannelInfo(nodeId, deviceId, linkId);
        auto txInfo = GetChannelInfo(txPpduRecord.m_senderId,
                                     txPpduRecord.m_senderDeviceId,
                                     txPpduRecord.m_linkId);
        if (!txInfo.has_value())
        {
            NS_LOG_DEBUG(
                "Didn't find TX record for transmission; possibly from an untraced sender");
        }
        else
        {
            if ((int(txInfo->second) != int(rxInfo->second)) ||
                (int(txInfo->first) != int(rxInfo->first)))
            {
                NS_LOG_DEBUG(
                    "Received Signal on a different frequency or channel number than what was "
                    "configured for this PHY or link");
                return;
            }
        }
    }

    WifiPpduRxRecord ppduRecord;
    ppduRecord.m_startTime = Simulator::Now();
    ppduRecord.m_endTime = Simulator::Now() + duration;
    ppduRecord.m_ppdu = ppdu;
    ppduRecord.m_rssi = rxPower;
    ppduRecord.m_rxTag = m_tagGenerator.GenerateUniqueTag(ppdu->GetUid());
    ppduRecord.m_receiverId = nodeId;
    ppduRecord.m_linkId = linkId;
    if (hasTxTag)
    {
        ppduRecord.m_senderId = txPpduRecord.m_senderId;
        ppduRecord.m_senderDeviceId = txPpduRecord.m_senderDeviceId;
        NS_LOG_INFO(
            "Receive at " << ContextToTuple(context) << " from node " << txPpduRecord.m_senderId
                          << "; insert to active records map for UID " << ppduRecord.m_rxTag);
    }
    else
    {
        NS_LOG_INFO("Receive at " << ContextToTuple(context)
                                  << "; insert to active records map for UID "
                                  << ppduRecord.m_rxTag);
    }

    // Add to list of currently received frames on this node, device and specific link
    m_nodeDeviceLinkRxRecords[nodeId][deviceId][linkId].emplace_back(ppduRecord);
    NS_LOG_INFO("Size of active records " << m_nodeDeviceLinkRxRecords.size());

    UpdateCurrentlyReceivedSignal(nodeId, deviceId, linkId);

    NS_LOG_INFO("Receive at " << ContextToTuple(context) << " insert to tagToRecord map for UID "
                              << ppduRecord.m_rxTag);
    auto [it, inserted] = m_rxTagToPpduRecord.insert_or_assign(ppduRecord.m_rxTag, ppduRecord);
    NS_ASSERT_MSG(inserted, "Did not insert successfully");
    NS_LOG_INFO("Size of m_rxTagToPpduRecord " << m_rxTagToPpduRecord.size());

    NS_LOG_INFO("Receive at " << ContextToTuple(context) << " insert to highly nested map for UID "
                              << ppduRecord.m_rxTag);
    m_nodeDeviceLinkPidToRxId[nodeId][deviceId][linkId][ppduRecord.m_ppdu->GetUid()] =
        ppduRecord.m_rxTag;
    NS_LOG_INFO("Size of highly nested map " << m_nodeDeviceLinkPidToRxId.size());
}

void
WifiPhyRxTraceSink::PhyPpduDrop(std::string context,
                                Ptr<const WifiPpdu> ppdu,
                                WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << context << ppdu << reason);
    uint32_t nodeId = ContextToNodeId(context);
    uint32_t deviceId = ContextToDeviceId(context);
    uint8_t linkId = ContextToLinkId(context);
    WifiPpduRxRecord ppduRecord =
        m_rxTagToPpduRecord[m_nodeDeviceLinkPidToRxId[nodeId][deviceId][linkId][ppdu->GetUid()]];

    if (ppduRecord.m_ppdu == nullptr)
    {
        NS_LOG_DEBUG("Frame being dropped was not observed on SignalArrival trace. Means it was "
                     "received on a wrong link configuration");
        return;
    }
    ppduRecord.m_reason = reason;

    std::vector<bool> outcome;
    for (const auto& mpdu : *PeekPointer(ppdu->GetPsdu()))
    {
        mpdu->GetPacket();
        outcome.emplace_back(false);
    }
    ppduRecord.m_statusPerMpdu = outcome;
    m_rxTagToPpduRecord[ppduRecord.m_rxTag] = ppduRecord;

    Simulator::Schedule(ppduRecord.m_endTime - Simulator::Now(),
                        &WifiPhyRxTraceSink::PhyRxEnd,
                        this,
                        nodeId,
                        deviceId,
                        ppduRecord.m_rxTag,
                        ppdu->GetUid());
}

void
WifiPhyRxTraceSink::PpduOutcome(std::string context,
                                Ptr<const WifiPpdu> ppdu,
                                RxSignalInfo signal,
                                const WifiTxVector& txVector,
                                const std::vector<bool>& statusMpdu)
{
    NS_LOG_FUNCTION(context << ppdu << signal << txVector);
    uint32_t nodeId = ContextToNodeId(context);
    uint32_t deviceId = ContextToDeviceId(context);
    uint8_t linkId = ContextToLinkId(context);

    WifiPpduRxRecord ppduRecord =
        m_rxTagToPpduRecord[m_nodeDeviceLinkPidToRxId[nodeId][deviceId][linkId][ppdu->GetUid()]];

    if (ppduRecord.m_ppdu)
    {
        NS_LOG_DEBUG("Found an expected frame in the outcome");
    }
    else
    {
        NS_LOG_DEBUG("Frame to be processed was not observed on SignalArrival trace");
        return;
    }
    // Save the reception status per MPDU in the PPDU record
    ppduRecord.m_statusPerMpdu = statusMpdu;

    auto [it, inserted] = m_rxTagToPpduRecord.insert_or_assign(ppduRecord.m_rxTag, ppduRecord);
    NS_ASSERT_MSG(!inserted, "Did not assign successfully");
    PhyRxEnd(nodeId, deviceId, ppduRecord.m_rxTag, ppdu->GetUid());
}

void
WifiPhyRxTraceSink::CountStatisticsForRecord(WifiPhyTraceStatistics& statistics,
                                             const WifiPpduRxRecord& record) const
{
    // Check if PPDU was dropped and if any MPDU was addressed to the receiver
    if (record.m_reason)
    {
        bool mpduToReceiver = false;
        bool shouldCount = true;
        Ptr<const Packet> p;
        auto it = record.m_statusPerMpdu.begin();
        for (const auto& mpdu : *PeekPointer(record.m_ppdu->GetPsdu()))
        {
            p = mpdu->GetProtocolDataUnit();
            WifiMacHeader hdr;
            p->PeekHeader(hdr);
            if (!(hdr.IsData() &&
                  (hdr.GetType() == WIFI_MAC_DATA || hdr.GetType() == WIFI_MAC_QOSDATA)))
            {
                shouldCount = false;
                break;
            }
            if (!(*it) && (record.m_receiverId == m_macAddressToNodeId.at(hdr.GetAddr1())))
            {
                // Failed MPDU
                statistics.m_failedMpdus++;
                mpduToReceiver = true;
            }
            ++it;
        }
        // At least one MPDU addressed to the receiver was dropped, mark PPDU as
        // failed
        if (mpduToReceiver && shouldCount)
        {
            statistics.m_failedPpdus++;
            // Check if PPDU Overlap
            if (!record.m_overlappingPpdu.empty())
            {
                // Yes Overlap
                statistics.m_overlappingPpdus++;
            }
            else
            {
                // No Overlap
                statistics.m_nonOverlappingPpdus++;
            }
            // It is due to a drop, clarify and count the reason
            statistics.m_ppduDropReasons[record.m_reason]++;
        }
    }
    else
    {
        // Payload Decode Attempt (No Drop but still not certain of MPDU outcome)
        // MPDU Level

        Ptr<const Packet> p;
        auto it = record.m_statusPerMpdu.begin();
        bool mpduFail = false;
        bool shouldCount = true;
        bool mpduToReceiver = false;
        for (const auto& mpdu : *PeekPointer(record.m_ppdu->GetPsdu()))
        {
            p = mpdu->GetProtocolDataUnit();
            WifiMacHeader hdr;
            p->PeekHeader(hdr);
            if (!(hdr.IsData() &&
                  (hdr.GetType() == WIFI_MAC_DATA || hdr.GetType() == WIFI_MAC_QOSDATA)))
            {
                shouldCount = false;
                break;
            }
            if (*it && (m_macAddressToNodeId.contains(hdr.GetAddr1())) &&
                (record.m_receiverId == m_macAddressToNodeId.at(hdr.GetAddr1())))
            {
                // Successful MPDU
                statistics.m_receivedMpdus++;
                mpduToReceiver = true;
            }
            else if (!(*it) && (m_macAddressToNodeId.contains(hdr.GetAddr1())) &&
                     (record.m_receiverId == m_macAddressToNodeId.at(hdr.GetAddr1())))
            {
                // Failed MPDU
                statistics.m_failedMpdus++;
                mpduFail = true;
                mpduToReceiver = true;
            }
            ++it;
        }
        if (!mpduFail && shouldCount && mpduToReceiver)
        {
            // No drops or payload decode errors for all MPDUs addressed to the
            // receiver
            statistics.m_receivedPpdus++;

            // Check if PPDU Overlap
            if (!record.m_overlappingPpdu.empty())
            {
                // Yes Overlap
                statistics.m_overlappingPpdus++;
            }
            else
            {
                // No Overlap
                statistics.m_nonOverlappingPpdus++;
            }
        }
        else if (shouldCount && mpduToReceiver)
        {
            // At least one MPDU addressed to the receiver failed.
            statistics.m_failedPpdus++;
            // Check if PPDU Overlap
            if (!record.m_overlappingPpdu.empty())
            {
                // Yes Overlap
                statistics.m_overlappingPpdus++;
            }
            else
            {
                // No Overlap
                statistics.m_nonOverlappingPpdus++;
            }
        }
    }
}

WifiPhyTraceStatistics
WifiPhyRxTraceSink::CountStatistics() const
{
    NS_LOG_FUNCTION(this);
    WifiPhyTraceStatistics statistics;
    for (const auto& nodeMap : m_completedRecords)
    {
        for (const auto& deviceMap : nodeMap.second)
        {
            for (const auto& linkMap : deviceMap.second)
            {
                for (const auto& record : linkMap.second)
                {
                    CountStatisticsForRecord(statistics, record);
                }
            }
        }
    }
    return statistics;
}

WifiPhyTraceStatistics
WifiPhyRxTraceSink::CountStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << linkId);
    WifiPhyTraceStatistics statistics;
    if (m_completedRecords.contains(nodeId))
    {
        auto mapOfDevices = m_completedRecords.at(nodeId);
        if (mapOfDevices.contains(deviceId))
        {
            const auto& mapOfLinks = mapOfDevices.at(deviceId);
            if (mapOfLinks.contains(linkId))
            {
                auto vectorOfRecords = mapOfLinks.at(linkId);
                for (const auto& record : vectorOfRecords)
                {
                    CountStatisticsForRecord(statistics, record);
                }
            }
        }
    }
    return statistics;
}

WifiPhyTraceStatistics
WifiPhyRxTraceSink::GetStatistics() const
{
    WifiPhyTraceStatistics stats = CountStatistics();
    return stats;
}

WifiPhyTraceStatistics
WifiPhyRxTraceSink::GetStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    WifiPhyTraceStatistics stats = CountStatistics(nodeId, deviceId, linkId);
    return stats;
}

void
WifiPhyRxTraceSink::PrintStatistics() const
{
    WifiPhyTraceStatistics statistics = CountStatistics();

    std::cout << "Total PPDUs Received: " << statistics.m_receivedPpdus + statistics.m_failedPpdus
              << std::endl;
    std::cout << "Total Non-Overlapping PPDUs Received: " << statistics.m_nonOverlappingPpdus
              << std::endl;
    std::cout << "Total Overlapping PPDUs Received: " << statistics.m_overlappingPpdus << std::endl;

    std::cout << "\nSuccessful PPDUs: " << statistics.m_receivedPpdus << std::endl;
    std::cout << "Failed PPDUs: " << statistics.m_failedPpdus << std::endl;
    for (auto reason : statistics.m_ppduDropReasons)
    {
        std::cout << "PPDU Dropped due to " << reason.first << ": " << reason.second << std::endl;
    }
    std::cout << "\nTotal MPDUs: " << statistics.m_failedMpdus + statistics.m_receivedMpdus
              << std::endl;
    std::cout << "Total Successful MPDUs: " << statistics.m_receivedMpdus << std::endl;
    std::cout << "Total Failed MPDUs: " << statistics.m_failedMpdus << std::endl;
}

void
WifiPhyRxTraceSink::PrintStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    WifiPhyTraceStatistics statistics = CountStatistics(nodeId, deviceId, linkId);
    std::cout << "Total PPDUs Received: " << statistics.m_receivedPpdus + statistics.m_failedPpdus
              << std::endl;
    std::cout << "Total Non-Overlapping PPDUs Received: " << statistics.m_nonOverlappingPpdus
              << std::endl;
    std::cout << "Total Overlapping PPDUs Received: " << statistics.m_overlappingPpdus << std::endl;

    std::cout << "\nSuccessful PPDUs: " << statistics.m_receivedPpdus << std::endl;
    std::cout << "Failed PPDUs: " << statistics.m_failedPpdus << std::endl;
    for (auto reason : statistics.m_ppduDropReasons)
    {
        std::cout << "PPDU Dropped due to " << reason.first << ": " << reason.second << std::endl;
    }
    std::cout << "\nTotal MPDUs: " << statistics.m_failedMpdus + statistics.m_receivedMpdus
              << std::endl;
    std::cout << "Total Successful MPDUs: " << statistics.m_receivedMpdus << std::endl;
    std::cout << "Total Failed MPDUs: " << statistics.m_failedMpdus << std::endl;
}

void
WifiPhyRxTraceSink::Reset()
{
    m_completedRecords.clear();
    m_records.clear();
}

// Return a single vector with all completed records
void
WifiPhyRxTraceSink::CreateVectorFromRecords()
{
    m_records.clear();
    for (const auto& nodeMap : m_completedRecords)
    {
        for (const auto& deviceMap : nodeMap.second)
        {
            for (const auto& linkMap : deviceMap.second)
            {
                for (const auto& wifiRecord : linkMap.second)
                {
                    m_records.emplace_back(wifiRecord);
                }
            }
        }
    }
}

const std::vector<WifiPpduRxRecord>&
WifiPhyRxTraceSink::GetPpduRecords() const
{
    return m_records;
}

std::optional<std::reference_wrapper<const std::vector<WifiPpduRxRecord>>>
WifiPhyRxTraceSink::GetPpduRecords(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const
{
    if (m_completedRecords.contains(nodeId))
    {
        if (m_completedRecords.at(nodeId).contains(deviceId))
        {
            if (m_completedRecords.at(nodeId).at(deviceId).contains(linkId))
            {
                return std::optional<std::reference_wrapper<const std::vector<WifiPpduRxRecord>>>(
                    m_completedRecords.at(nodeId).at(deviceId).at(linkId));
            }
        }
    }
    return std::nullopt;
}

} // namespace ns3
