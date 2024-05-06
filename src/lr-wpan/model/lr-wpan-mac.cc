/*
 * Copyright (c) 2011 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Erwan Livolant <erwan.livolant@inria.fr>
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */
#include "lr-wpan-mac.h"

#include "lr-wpan-constants.h"
#include "lr-wpan-csmaca.h"
#include "lr-wpan-mac-header.h"
#include "lr-wpan-mac-pl-headers.h"
#include "lr-wpan-mac-trailer.h"

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/random-variable-stream.h>
#include <ns3/simulator.h>
#include <ns3/uinteger.h>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    std::clog << "[address " << m_shortAddress << " | " << m_macExtendedAddress << "] ";

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("LrWpanMac");
NS_OBJECT_ENSURE_REGISTERED(LrWpanMac);

std::ostream&
operator<<(std::ostream& os, const MacState& state)
{
    switch (state)
    {
    case MacState::MAC_IDLE:
        os << "MAC IDLE";
        break;
    case MacState::MAC_CSMA:
        os << "CSMA";
        break;
    case MacState::MAC_SENDING:
        os << "SENDING";
        break;
    case MacState::MAC_ACK_PENDING:
        os << "ACK PENDING";
        break;
    case MacState::CHANNEL_ACCESS_FAILURE:
        os << "CHANNEL_ACCESS_FAILURE";
        break;
    case MacState::CHANNEL_IDLE:
        os << "CHANNEL IDLE";
        break;
    case MacState::SET_PHY_TX_ON:
        os << "SET PHY to TX ON";
        break;
    case MacState::MAC_GTS:
        os << "MAC GTS PERIOD";
        break;
    case MacState::MAC_INACTIVE:
        os << "SUPERFRAME INACTIVE PERIOD";
        break;
    case MacState::MAC_CSMA_DEFERRED:
        os << "CSMA DEFERRED TO NEXT PERIOD";
        break;
    }
    return os;
};

TypeId
LrWpanMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LrWpanMac")
            .SetParent<LrWpanMacBase>()
            .SetGroupName("LrWpan")
            .AddConstructor<LrWpanMac>()
            .AddAttribute("PanId",
                          "16-bit identifier of the associated PAN",
                          UintegerValue(),
                          MakeUintegerAccessor(&LrWpanMac::m_macPanId),
                          MakeUintegerChecker<uint16_t>())
            .AddTraceSource("MacTxEnqueue",
                            "Trace source indicating a packet has been "
                            "enqueued in the transaction queue",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macTxEnqueueTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxDequeue",
                            "Trace source indicating a packet has was "
                            "dequeued from the transaction queue",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macTxDequeueTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacIndTxEnqueue",
                            "Trace source indicating a packet has been "
                            "enqueued in the indirect transaction queue",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macIndTxEnqueueTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacIndTxDequeue",
                            "Trace source indicating a packet has was "
                            "dequeued from the indirect transaction queue",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macIndTxDequeueTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTx",
                            "Trace source indicating a packet has "
                            "arrived for transmission by this device",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macTxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxOk",
                            "Trace source indicating a packet has been "
                            "successfully sent",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macTxOkTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacTxDrop",
                            "Trace source indicating a packet has been "
                            "dropped during transmission",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacIndTxDrop",
                            "Trace source indicating a packet has been "
                            "dropped from the indirect transaction queue"
                            "(The pending transaction list)",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macIndTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacPromiscRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a promiscuous trace,",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macPromiscRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRx",
                            "A packet has been received by this device, "
                            "has been passed up from the physical layer "
                            "and is being forwarded up the local protocol stack.  "
                            "This is a non-promiscuous trace,",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macRxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacRxDrop",
                            "Trace source indicating a packet was received, "
                            "but dropped before being forwarded up the stack",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macRxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("Sniffer",
                            "Trace source simulating a non-promiscuous "
                            "packet sniffer attached to the device",
                            MakeTraceSourceAccessor(&LrWpanMac::m_snifferTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PromiscSniffer",
                            "Trace source simulating a promiscuous "
                            "packet sniffer attached to the device",
                            MakeTraceSourceAccessor(&LrWpanMac::m_promiscSnifferTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("MacStateValue",
                            "The state of LrWpan Mac",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macState),
                            "ns3::TracedValueCallback::LrWpanMacState")
            .AddTraceSource("MacIncSuperframeStatus",
                            "The period status of the incoming superframe",
                            MakeTraceSourceAccessor(&LrWpanMac::m_incSuperframeStatus),
                            "ns3::TracedValueCallback::SuperframeState")
            .AddTraceSource("MacOutSuperframeStatus",
                            "The period status of the outgoing superframe",
                            MakeTraceSourceAccessor(&LrWpanMac::m_outSuperframeStatus),
                            "ns3::TracedValueCallback::SuperframeState")
            .AddTraceSource("MacState",
                            "The state of LrWpan Mac",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macStateLogger),
                            "ns3::LrWpanMac::StateTracedCallback")
            .AddTraceSource("MacSentPkt",
                            "Trace source reporting some information about "
                            "the sent packet",
                            MakeTraceSourceAccessor(&LrWpanMac::m_sentPktTrace),
                            "ns3::LrWpanMac::SentTracedCallback")
            .AddTraceSource("IfsEnd",
                            "Trace source reporting the end of an "
                            "Interframe space (IFS)",
                            MakeTraceSourceAccessor(&LrWpanMac::m_macIfsEndTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

LrWpanMac::LrWpanMac()
{
    // First set the state to a known value, call ChangeMacState to fire trace source.
    m_macState = MAC_IDLE;

    ChangeMacState(MAC_IDLE);

    m_incSuperframeStatus = INACTIVE;
    m_outSuperframeStatus = INACTIVE;

    m_macRxOnWhenIdle = true;
    m_macPanId = 0xffff;
    m_macCoordShortAddress = Mac16Address("ff:ff");
    m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
    m_deviceCapability = DeviceType::FFD;
    m_macExtendedAddress = Mac64Address::Allocate();
    m_macPromiscuousMode = false;
    m_macMaxFrameRetries = 3;
    m_retransmission = 0;
    m_numCsmacaRetry = 0;
    m_txPkt = nullptr;
    m_rxPkt = nullptr;
    m_lastRxFrameLqi = 0;
    m_ifs = 0;

    m_macLIFSPeriod = 40;
    m_macSIFSPeriod = 12;

    m_panCoor = false;
    m_coor = false;
    m_macBeaconOrder = 15;
    m_macSuperframeOrder = 15;
    m_macTransactionPersistenceTime = 500; // 0x01F5
    m_macAssociationPermit = true;
    m_macAutoRequest = true;

    m_incomingBeaconOrder = 15;
    m_incomingSuperframeOrder = 15;
    m_beaconTrackingOn = false;
    m_numLostBeacons = 0;

    m_pendPrimitive = MLME_NONE;
    m_channelScanIndex = 0;
    m_maxEnergyLevel = 0;

    m_macResponseWaitTime = lrwpan::aBaseSuperframeDuration * 32;
    m_assocRespCmdWaitTime = 960;

    m_maxTxQueueSize = m_txQueue.max_size();
    m_maxIndTxQueueSize = m_indTxQueue.max_size();

    Ptr<UniformRandomVariable> uniformVar = CreateObject<UniformRandomVariable>();
    uniformVar->SetAttribute("Min", DoubleValue(0.0));
    uniformVar->SetAttribute("Max", DoubleValue(255.0));
    m_macDsn = SequenceNumber8(uniformVar->GetValue());
    m_macBsn = SequenceNumber8(uniformVar->GetValue());
    m_macBeaconPayload = nullptr;
    m_macBeaconPayloadLength = 0;
    m_shortAddress = Mac16Address("FF:FF"); // FF:FF = The address is not assigned.
}

LrWpanMac::~LrWpanMac()
{
}

void
LrWpanMac::DoInitialize()
{
    if (m_macRxOnWhenIdle)
    {
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
    else
    {
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TRX_OFF);
    }

    Object::DoInitialize();
}

void
LrWpanMac::DoDispose()
{
    if (m_csmaCa)
    {
        m_csmaCa->Dispose();
        m_csmaCa = nullptr;
    }
    m_txPkt = nullptr;

    for (uint32_t i = 0; i < m_txQueue.size(); i++)
    {
        m_txQueue[i]->txQPkt = nullptr;
    }
    m_txQueue.clear();

    for (uint32_t i = 0; i < m_indTxQueue.size(); i++)
    {
        m_indTxQueue[i]->txQPkt = nullptr;
    }
    m_indTxQueue.clear();

    m_phy = nullptr;
    m_mcpsDataConfirmCallback = MakeNullCallback<void, McpsDataConfirmParams>();
    m_mcpsDataIndicationCallback = MakeNullCallback<void, McpsDataIndicationParams, Ptr<Packet>>();
    m_mlmeStartConfirmCallback = MakeNullCallback<void, MlmeStartConfirmParams>();
    m_mlmeBeaconNotifyIndicationCallback =
        MakeNullCallback<void, MlmeBeaconNotifyIndicationParams>();
    m_mlmeSyncLossIndicationCallback = MakeNullCallback<void, MlmeSyncLossIndicationParams>();
    m_mlmePollConfirmCallback = MakeNullCallback<void, MlmePollConfirmParams>();
    m_mlmeScanConfirmCallback = MakeNullCallback<void, MlmeScanConfirmParams>();
    m_mlmeAssociateConfirmCallback = MakeNullCallback<void, MlmeAssociateConfirmParams>();
    m_mlmeAssociateIndicationCallback = MakeNullCallback<void, MlmeAssociateIndicationParams>();
    m_mlmeCommStatusIndicationCallback = MakeNullCallback<void, MlmeCommStatusIndicationParams>();
    m_mlmeOrphanIndicationCallback = MakeNullCallback<void, MlmeOrphanIndicationParams>();

    m_panDescriptorList.clear();
    m_energyDetectList.clear();
    m_unscannedChannels.clear();

    m_scanEvent.Cancel();
    m_scanEnergyEvent.Cancel();
    m_scanOrphanEvent.Cancel();
    m_beaconEvent.Cancel();

    Object::DoDispose();
}

bool
LrWpanMac::GetRxOnWhenIdle() const
{
    return m_macRxOnWhenIdle;
}

void
LrWpanMac::SetRxOnWhenIdle(bool rxOnWhenIdle)
{
    NS_LOG_FUNCTION(this << rxOnWhenIdle);
    m_macRxOnWhenIdle = rxOnWhenIdle;

    if (m_macState == MAC_IDLE)
    {
        if (m_macRxOnWhenIdle)
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
        }
        else
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TRX_OFF);
        }
    }
}

void
LrWpanMac::SetShortAddress(Mac16Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_shortAddress = address;
}

void
LrWpanMac::SetExtendedAddress(Mac64Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_macExtendedAddress = address;
}

Mac16Address
LrWpanMac::GetShortAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_shortAddress;
}

Mac64Address
LrWpanMac::GetExtendedAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_macExtendedAddress;
}

void
LrWpanMac::McpsDataRequest(McpsDataRequestParams params, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    McpsDataConfirmParams confirmParams;
    confirmParams.m_msduHandle = params.m_msduHandle;

    // TODO: We need a drop trace for the case that the packet is too large or the request
    // parameters are maleformed.
    //       The current tx drop trace is not suitable, because packets dropped using this trace
    //       carry the mac header and footer, while packets being dropped here do not have them.

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_DATA, m_macDsn.GetValue());
    m_macDsn++;

    if (p->GetSize() > lrwpan::aMaxPhyPacketSize - lrwpan::aMinMPDUOverhead)
    {
        // Note, this is just testing maximum theoretical frame size per the spec
        // The frame could still be too large once headers are put on
        // in which case the phy will reject it instead
        NS_LOG_ERROR(this << " packet too big: " << p->GetSize());
        confirmParams.m_status = MacStatus::FRAME_TOO_LONG;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    if ((params.m_srcAddrMode == NO_PANID_ADDR) && (params.m_dstAddrMode == NO_PANID_ADDR))
    {
        NS_LOG_ERROR(this << " Can not send packet with no Address field");
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }
    switch (params.m_srcAddrMode)
    {
    case NO_PANID_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetNoPanIdComp();
        break;
    case ADDR_MODE_RESERVED:
        NS_ABORT_MSG("Can not set source address type to ADDR_MODE_RESERVED. Aborting.");
        break;
    case SHORT_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetSrcAddrFields(GetPanId(), GetShortAddress());
        break;
    case EXT_ADDR:
        macHdr.SetSrcAddrMode(params.m_srcAddrMode);
        macHdr.SetSrcAddrFields(GetPanId(), GetExtendedAddress());
        break;
    default:
        NS_LOG_ERROR(this << " Can not send packet with incorrect Source Address mode = "
                          << params.m_srcAddrMode);
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }
    switch (params.m_dstAddrMode)
    {
    case NO_PANID_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetNoPanIdComp();
        break;
    case ADDR_MODE_RESERVED:
        NS_ABORT_MSG("Can not set destination address type to ADDR_MODE_RESERVED. Aborting.");
        break;
    case SHORT_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetDstAddrFields(params.m_dstPanId, params.m_dstAddr);
        break;
    case EXT_ADDR:
        macHdr.SetDstAddrMode(params.m_dstAddrMode);
        macHdr.SetDstAddrFields(params.m_dstPanId, params.m_dstExtAddr);
        break;
    default:
        NS_LOG_ERROR(this << " Can not send packet with incorrect Destination Address mode = "
                          << params.m_dstAddrMode);
        confirmParams.m_status = MacStatus::INVALID_ADDRESS;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    // IEEE 802.15.4-2006 (7.5.6.1)
    // Src & Dst PANs are identical, PAN compression is ON
    // only the dst PAN is serialized making the MAC header 2 bytes smaller
    if ((params.m_dstAddrMode != NO_PANID_ADDR && params.m_srcAddrMode != NO_PANID_ADDR) &&
        (macHdr.GetDstPanId() == macHdr.GetSrcPanId()))
    {
        macHdr.SetPanIdComp();
    }

    macHdr.SetSecDisable();
    // extract the first 3 bits in TxOptions
    int b0 = params.m_txOptions & TX_OPTION_ACK;
    int b1 = params.m_txOptions & TX_OPTION_GTS;
    int b2 = params.m_txOptions & TX_OPTION_INDIRECT;

    if (b0 == TX_OPTION_ACK)
    {
        // Set AckReq bit only if the destination is not the broadcast address.
        if (macHdr.GetDstAddrMode() == SHORT_ADDR)
        {
            // short address and ACK requested.
            Mac16Address shortAddr = macHdr.GetShortDstAddr();
            if (shortAddr.IsBroadcast() || shortAddr.IsMulticast())
            {
                NS_LOG_LOGIC("LrWpanMac::McpsDataRequest: requested an ACK on broadcast or "
                             "multicast destination ("
                             << shortAddr << ") - forcefully removing it.");
                macHdr.SetNoAckReq();
                params.m_txOptions &= ~uint8_t(TX_OPTION_ACK);
            }
            else
            {
                macHdr.SetAckReq();
            }
        }
        else
        {
            // other address (not short) and ACK requested
            macHdr.SetAckReq();
        }
    }
    else
    {
        macHdr.SetNoAckReq();
    }

    if (b1 == TX_OPTION_GTS)
    {
        // TODO:GTS Transmission
    }
    else if (b2 == TX_OPTION_INDIRECT)
    {
        // Indirect Tx
        // A COORDINATOR will save the packet in the pending queue and await for data
        // requests from its associated devices. The devices are aware of pending data,
        // from the pending bit information extracted from the received beacon.
        // A DEVICE must be tracking beacons (MLME-SYNC.request is running) before attempting
        // request data from the coordinator.

        //  Indirect Transmission can only be done by PAN coordinator or coordinators.
        NS_ASSERT(m_coor);
        p->AddHeader(macHdr);

        LrWpanMacTrailer macTrailer;
        // Calculate FCS if the global attribute ChecksumEnabled is set.
        if (Node::ChecksumEnabled())
        {
            macTrailer.EnableFcs(true);
            macTrailer.SetFcs(p);
        }
        p->AddTrailer(macTrailer);

        NS_LOG_ERROR(this << " Indirect transmissions not currently supported");
        // Note: The current Pending transaction list should work for indirect transmissions.
        // However, this is not tested yet. For now, we block the use of indirect transmissions.
        // TODO: Save packet in the Pending Transaction list.
        // EnqueueInd (p);
    }
    else
    {
        // Direct Tx
        // From this point the packet will be pushed to a Tx queue and immediately
        // use a slotted (beacon-enabled) or unslotted (nonbeacon-enabled) version of CSMA/CA
        // before sending the packet, depending on whether it has previously
        // received a valid beacon or not.

        p->AddHeader(macHdr);

        LrWpanMacTrailer macTrailer;
        // Calculate FCS if the global attribute ChecksumEnabled is set.
        if (Node::ChecksumEnabled())
        {
            macTrailer.EnableFcs(true);
            macTrailer.SetFcs(p);
        }
        p->AddTrailer(macTrailer);

        Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
        txQElement->txQMsduHandle = params.m_msduHandle;
        txQElement->txQPkt = p;
        EnqueueTxQElement(txQElement);
        CheckQueue();
    }
}

void
LrWpanMac::MlmeStartRequest(MlmeStartRequestParams params)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_deviceCapability == DeviceType::FFD);

    MlmeStartConfirmParams confirmParams;

    if (GetShortAddress() == Mac16Address("ff:ff"))
    {
        NS_LOG_ERROR(this << " Invalid MAC short address");
        confirmParams.m_status = MacStatus::NO_SHORT_ADDRESS;
        if (!m_mlmeStartConfirmCallback.IsNull())
        {
            m_mlmeStartConfirmCallback(confirmParams);
        }
        return;
    }

    if ((params.m_bcnOrd > 15) || (params.m_sfrmOrd > params.m_bcnOrd))
    {
        confirmParams.m_status = MacStatus::INVALID_PARAMETER;
        if (!m_mlmeStartConfirmCallback.IsNull())
        {
            m_mlmeStartConfirmCallback(confirmParams);
        }
        NS_LOG_ERROR(this << "Incorrect superframe order or beacon order.");
        return;
    }

    // Mark primitive as pending and save the start params while the new page and channel is set.
    m_pendPrimitive = MLME_START_REQ;
    m_startParams = params;

    Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
    pibAttr->phyCurrentPage = m_startParams.m_logChPage;
    m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentPage, pibAttr);
}

void
LrWpanMac::MlmeScanRequest(MlmeScanRequestParams params)
{
    NS_LOG_FUNCTION(this);

    MlmeScanConfirmParams confirmParams;
    confirmParams.m_scanType = params.m_scanType;
    confirmParams.m_chPage = params.m_chPage;

    if ((m_scanEvent.IsPending() || m_scanEnergyEvent.IsPending()) || m_scanOrphanEvent.IsPending())
    {
        if (!m_mlmeScanConfirmCallback.IsNull())
        {
            confirmParams.m_status = MacStatus::SCAN_IN_PROGRESS;
            m_mlmeScanConfirmCallback(confirmParams);
        }
        NS_LOG_ERROR(this << " A channel scan is already in progress");
        return;
    }

    if (params.m_scanDuration > 14 || params.m_scanType > MLMESCAN_ORPHAN)
    {
        if (!m_mlmeScanConfirmCallback.IsNull())
        {
            confirmParams.m_status = MacStatus::INVALID_PARAMETER;
            m_mlmeScanConfirmCallback(confirmParams);
        }
        NS_LOG_ERROR(this << "Invalid scan duration or unsupported scan type");
        return;
    }
    // Temporary store macPanId and set macPanId to 0xFFFF to accept all beacons.
    m_macPanIdScan = m_macPanId;
    m_macPanId = 0xFFFF;

    m_panDescriptorList.clear();
    m_energyDetectList.clear();
    m_unscannedChannels.clear();

    // TODO: stop beacon transmission

    // Cancel any ongoing CSMA/CA operations and set to unslotted mode for scan
    m_csmaCa->Cancel();
    m_capEvent.Cancel();
    m_cfpEvent.Cancel();
    m_incCapEvent.Cancel();
    m_incCfpEvent.Cancel();
    m_trackingEvent.Cancel();
    m_csmaCa->SetUnSlottedCsmaCa();

    m_channelScanIndex = 0;

    // Mark primitive as pending and save the scan params while the new page and/or channel is set.
    m_scanParams = params;
    m_pendPrimitive = MLME_SCAN_REQ;

    Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
    pibAttr->phyCurrentPage = params.m_chPage;
    m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentPage, pibAttr);
}

void
LrWpanMac::MlmeAssociateRequest(MlmeAssociateRequestParams params)
{
    NS_LOG_FUNCTION(this);

    // Association is typically preceded by beacon reception and a  MLME-SCAN.request, therefore,
    // the values of the Associate.request params usually come from the information
    // obtained from those operations.
    m_pendPrimitive = MLME_ASSOC_REQ;
    m_associateParams = params;
    bool invalidRequest = false;

    if (params.m_coordPanId == 0xffff)
    {
        invalidRequest = true;
    }

    if (!invalidRequest && params.m_coordAddrMode == SHORT_ADDR)
    {
        if (params.m_coordShortAddr == Mac16Address("ff:ff") ||
            params.m_coordShortAddr == Mac16Address("ff:fe"))
        {
            invalidRequest = true;
        }
    }
    else if (!invalidRequest && params.m_coordAddrMode == EXT_ADDR)
    {
        if (params.m_coordExtAddr == Mac64Address("ff:ff:ff:ff:ff:ff:ff:ff") ||
            params.m_coordExtAddr == Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed"))
        {
            invalidRequest = true;
        }
    }

    if (invalidRequest)
    {
        m_pendPrimitive = MLME_NONE;
        m_associateParams = MlmeAssociateRequestParams();
        NS_LOG_ERROR(this << " Invalid PAN id in Association request");
        if (!m_mlmeAssociateConfirmCallback.IsNull())
        {
            MlmeAssociateConfirmParams confirmParams;
            confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
            confirmParams.m_status = MacStatus::INVALID_PARAMETER;
            m_mlmeAssociateConfirmCallback(confirmParams);
        }
    }
    else
    {
        Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
        pibAttr->phyCurrentPage = params.m_chPage;
        m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentPage, pibAttr);
    }
}

void
LrWpanMac::EndAssociateRequest()
{
    // the primitive is no longer pending (channel & page are set)
    m_pendPrimitive = MLME_NONE;
    // As described in IEEE 802.15.4-2011 (Section 5.1.3.1)
    m_macPanId = m_associateParams.m_coordPanId;
    if (m_associateParams.m_coordAddrMode == SHORT_ADDR)
    {
        m_macCoordShortAddress = m_associateParams.m_coordShortAddr;
    }
    else
    {
        m_macCoordExtendedAddress = m_associateParams.m_coordExtAddr;
        m_macCoordShortAddress = Mac16Address("ff:fe");
    }

    SendAssocRequestCommand();
}

void
LrWpanMac::MlmeAssociateResponse(MlmeAssociateResponseParams params)
{
    // Associate Short Address (m_assocShortAddr)
    // FF:FF = Association Request failed
    // FF:FE = The association request is accepted, but the device should use its extended address
    // Other = The assigned short address by the coordinator

    NS_LOG_FUNCTION(this);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();

    // Mac header Assoc. Response Comm. See 802.15.4-2011 (Section 5.3.2.1)
    macHdr.SetDstAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetPanIdComp();
    macHdr.SetDstAddrFields(m_macPanId, params.m_extDevAddr);
    macHdr.SetSrcAddrFields(0xffff, GetExtendedAddress());

    CommandPayloadHeader macPayload(CommandPayloadHeader::ASSOCIATION_RESP);
    macPayload.SetShortAddr(params.m_assocShortAddr);
    macPayload.SetAssociationStatus(static_cast<uint8_t>(params.m_status));

    macHdr.SetSecDisable();
    macHdr.SetAckReq();

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    // Save packet in the Pending Transaction list.
    EnqueueInd(commandPacket);
}

void
LrWpanMac::MlmeOrphanResponse(MlmeOrphanResponseParams params)
{
    NS_LOG_FUNCTION(this);
    // Mac header Coordinator realigment Command
    // See 802.15.4-2011 (Section 6.2.7.2)
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();
    macHdr.SetPanIdComp();
    macHdr.SetDstAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetDstAddrFields(0xffff, params.m_orphanAddr);

    macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetSrcAddrFields(m_macPanId, GetExtendedAddress());
    macHdr.SetSrcAddrFields(m_macPanId, Mac16Address("FF:FF"));

    macHdr.SetFrameVer(0x01);
    macHdr.SetSecDisable();
    macHdr.SetAckReq();

    CommandPayloadHeader macPayload(CommandPayloadHeader::COOR_REALIGN);
    macPayload.SetPanId(m_macPanId);
    macPayload.SetCoordShortAddr(GetShortAddress());
    macPayload.SetChannel(m_phy->GetCurrentChannelNum());
    macPayload.SetPage(m_phy->GetCurrentPage());

    if (params.m_assocMember)
    {
        // The orphan device was associated with the coordinator

        // Either FF:FE for extended address mode
        // or the short address assigned by the coord.
        macPayload.SetShortAddr(params.m_shortAddr);
    }
    else
    {
        // The orphan device was NOT associated with the coordinator
        macPayload.SetShortAddr(Mac16Address("FF:FF"));
    }

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = commandPacket;
    EnqueueTxQElement(txQElement);
    CheckQueue();
}

void
LrWpanMac::MlmeSyncRequest(MlmeSyncRequestParams params)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(params.m_logCh <= 26 && m_macPanId != 0xffff);

    auto symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second
    // change phy current logical channel
    Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
    pibAttr->phyCurrentChannel = params.m_logCh;
    m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel, pibAttr);

    // Enable Phy receiver
    m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);

    uint64_t searchSymbols;
    Time searchBeaconTime;

    if (m_trackingEvent.IsPending())
    {
        m_trackingEvent.Cancel();
    }

    if (params.m_trackBcn)
    {
        m_numLostBeacons = 0;
        // search for a beacon for a time = incomingSuperframe symbols + 960 symbols
        searchSymbols =
            ((uint64_t)1 << m_incomingBeaconOrder) + 1 * lrwpan::aBaseSuperframeDuration;
        searchBeaconTime = Seconds((double)searchSymbols / symbolRate);
        m_beaconTrackingOn = true;
        m_trackingEvent =
            Simulator::Schedule(searchBeaconTime, &LrWpanMac::BeaconSearchTimeout, this);
    }
    else
    {
        m_beaconTrackingOn = false;
    }
}

void
LrWpanMac::MlmePollRequest(MlmePollRequestParams params)
{
    NS_LOG_FUNCTION(this);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macBsn.GetValue());
    m_macBsn++;

    CommandPayloadHeader macPayload(CommandPayloadHeader::DATA_REQ);

    Ptr<Packet> beaconPacket = Create<Packet>();
    // TODO: complete poll request (part of indirect transmissions)
    NS_FATAL_ERROR(this << " Poll request currently not supported");
}

void
LrWpanMac::MlmeSetRequest(MacPibAttributeIdentifier id, Ptr<MacPibAttributes> attribute)
{
    MlmeSetConfirmParams confirmParams;
    confirmParams.m_status = MacStatus::SUCCESS;

    switch (id)
    {
    case macBeaconPayload:
        if (attribute->macBeaconPayload->GetSize() > lrwpan::aMaxBeaconPayloadLength)
        {
            confirmParams.m_status = MacStatus::INVALID_PARAMETER;
        }
        else
        {
            m_macBeaconPayload = attribute->macBeaconPayload;
            m_macBeaconPayloadLength = attribute->macBeaconPayload->GetSize();
        }
        break;
    case macBeaconPayloadLength:
        confirmParams.m_status = MacStatus::INVALID_PARAMETER;
        break;
    case macShortAddress:
        m_shortAddress = attribute->macShortAddress;
        break;
    case macExtendedAddress:
        confirmParams.m_status = MacStatus::READ_ONLY;
        break;
    case macPanId:
        m_macPanId = macPanId;
        break;
    default:
        // TODO: Add support for setting other attributes
        confirmParams.m_status = MacStatus::UNSUPPORTED_ATTRIBUTE;
        break;
    }

    if (!m_mlmeSetConfirmCallback.IsNull())
    {
        confirmParams.id = id;
        m_mlmeSetConfirmCallback(confirmParams);
    }
}

void
LrWpanMac::MlmeGetRequest(MacPibAttributeIdentifier id)
{
    MacStatus status = MacStatus::SUCCESS;
    Ptr<MacPibAttributes> attributes = Create<MacPibAttributes>();

    switch (id)
    {
    case macBeaconPayload:
        attributes->macBeaconPayload = m_macBeaconPayload;
        break;
    case macBeaconPayloadLength:
        attributes->macBeaconPayloadLength = m_macBeaconPayloadLength;
        break;
    case macShortAddress:
        attributes->macShortAddress = m_shortAddress;
        break;
    case macExtendedAddress:
        attributes->macExtendedAddress = m_macExtendedAddress;
        break;
    case macPanId:
        attributes->macPanId = m_macPanId;
        break;
    case pCurrentChannel:
        attributes->pCurrentChannel = m_phy->GetCurrentChannelNum();
        break;
    case pCurrentPage:
        attributes->pCurrentPage = m_phy->GetCurrentPage();
        break;
    default:
        status = MacStatus::UNSUPPORTED_ATTRIBUTE;
        break;
    }

    if (!m_mlmeGetConfirmCallback.IsNull())
    {
        m_mlmeGetConfirmCallback(status, id, attributes);
    }
}

void
LrWpanMac::SendOneBeacon()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_macState == MAC_IDLE);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_BEACON, m_macBsn.GetValue());
    m_macBsn++;
    BeaconPayloadHeader macPayload;
    Ptr<Packet> beaconPacket;
    LrWpanMacTrailer macTrailer;

    if (m_macBeaconPayload == nullptr)
    {
        beaconPacket = Create<Packet>();
    }
    else
    {
        beaconPacket = m_macBeaconPayload;
    }

    macHdr.SetDstAddrMode(LrWpanMacHeader::SHORTADDR);
    macHdr.SetDstAddrFields(GetPanId(), Mac16Address("ff:ff"));

    // see IEEE 802.15.4-2011 Section 5.1.2.4
    if (GetShortAddress() == Mac16Address("ff:fe"))
    {
        macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
        macHdr.SetSrcAddrFields(GetPanId(), GetExtendedAddress());
    }
    else
    {
        macHdr.SetSrcAddrMode(LrWpanMacHeader::SHORTADDR);
        macHdr.SetSrcAddrFields(GetPanId(), GetShortAddress());
    }

    macHdr.SetSecDisable();
    macHdr.SetNoAckReq();

    macPayload.SetSuperframeSpecField(GetSuperframeField());
    macPayload.SetGtsFields(GetGtsFields());
    macPayload.SetPndAddrFields(GetPendingAddrFields());

    beaconPacket->AddHeader(macPayload);
    beaconPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(beaconPacket);
    }

    beaconPacket->AddTrailer(macTrailer);

    // Set the Beacon packet to be transmitted
    m_txPkt = beaconPacket;

    if (m_csmaCa->IsSlottedCsmaCa())
    {
        m_outSuperframeStatus = BEACON;
        NS_LOG_DEBUG("Outgoing superframe Active Portion (Beacon + CAP + CFP): "
                     << m_superframeDuration << " symbols");
    }

    ChangeMacState(MAC_SENDING);
    m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
}

void
LrWpanMac::SendBeaconRequestCommand()
{
    NS_LOG_FUNCTION(this);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();

    // Beacon Request Command Mac header values See IEEE 802.15.4-2011 (Section 5.3.7)
    macHdr.SetNoPanIdComp();
    macHdr.SetDstAddrMode(LrWpanMacHeader::SHORTADDR);
    macHdr.SetSrcAddrMode(LrWpanMacHeader::NOADDR);

    // Not associated PAN, broadcast dst address
    macHdr.SetDstAddrFields(0xFFFF, Mac16Address("FF:FF"));

    macHdr.SetSecDisable();
    macHdr.SetNoAckReq();

    CommandPayloadHeader macPayload;
    macPayload.SetCommandFrameType(CommandPayloadHeader::BEACON_REQ);

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = commandPacket;
    EnqueueTxQElement(txQElement);
    CheckQueue();
}

void
LrWpanMac::SendOrphanNotificationCommand()
{
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();

    // See IEEE 802.15.4-2011 (5.3.6)
    macHdr.SetPanIdComp();

    macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetSrcAddrFields(0xFFFF, GetExtendedAddress());

    macHdr.SetDstAddrMode(LrWpanMacHeader::SHORTADDR);
    macHdr.SetDstAddrFields(0xFFFF, Mac16Address("FF:FF"));

    macHdr.SetSecDisable();
    macHdr.SetNoAckReq();

    CommandPayloadHeader macPayload;
    macPayload.SetCommandFrameType(CommandPayloadHeader::ORPHAN_NOTIF);

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = commandPacket;
    EnqueueTxQElement(txQElement);
    CheckQueue();
}

void
LrWpanMac::SendAssocRequestCommand()
{
    NS_LOG_FUNCTION(this);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();

    // Assoc. Req. Comm. Mac header values See IEEE 802.15.4-2011 (Section 5.3.1.1)
    macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetSrcAddrFields(0xffff, GetExtendedAddress());

    if (m_associateParams.m_coordAddrMode == SHORT_ADDR)
    {
        macHdr.SetDstAddrMode(LrWpanMacHeader::SHORTADDR);
        macHdr.SetDstAddrFields(m_associateParams.m_coordPanId, m_associateParams.m_coordShortAddr);
    }
    else
    {
        macHdr.SetDstAddrMode(LrWpanMacHeader::EXTADDR);
        macHdr.SetDstAddrFields(m_associateParams.m_coordPanId, m_associateParams.m_coordExtAddr);
    }

    macHdr.SetSecDisable();
    macHdr.SetAckReq();

    CommandPayloadHeader macPayload(CommandPayloadHeader::ASSOCIATION_REQ);
    macPayload.SetCapabilityField(m_associateParams.m_capabilityInfo);

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = commandPacket;
    EnqueueTxQElement(txQElement);
    CheckQueue();
}

void
LrWpanMac::SendDataRequestCommand()
{
    // See IEEE 802.15.4-2011 (Section 5.3.5)
    // This command can be sent for 3 different situations:
    // a) In response to a beacon indicating that there is data for the device.
    // b) Triggered by MLME-POLL.request.
    // c) To follow an ACK of an Association Request command and continue the associate process.

    // TODO: Implementation of a) and b) will be done when Indirect transmissions are fully
    // supported.
    //       for now, only case c) is considered.

    NS_LOG_FUNCTION(this);

    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_COMMAND, m_macDsn.GetValue());
    m_macDsn++;
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> commandPacket = Create<Packet>();

    // Mac Header values (Section 5.3.5)
    macHdr.SetSrcAddrMode(LrWpanMacHeader::EXTADDR);
    macHdr.SetSrcAddrFields(0xffff, m_macExtendedAddress);

    if (m_macCoordShortAddress == Mac16Address("ff:fe"))
    {
        macHdr.SetDstAddrMode(LrWpanMacHeader::EXTADDR);
        macHdr.SetDstAddrFields(m_macPanId, m_macCoordExtendedAddress);
    }
    else
    {
        macHdr.SetDstAddrMode(LrWpanMacHeader::SHORTADDR);
        macHdr.SetDstAddrFields(m_macPanId, m_macCoordShortAddress);
    }

    macHdr.SetSecDisable();
    macHdr.SetAckReq();

    CommandPayloadHeader macPayload(CommandPayloadHeader::DATA_REQ);

    commandPacket->AddHeader(macPayload);
    commandPacket->AddHeader(macHdr);

    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(commandPacket);
    }

    commandPacket->AddTrailer(macTrailer);

    // Set the Command packet to be transmitted
    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = commandPacket;
    EnqueueTxQElement(txQElement);
    CheckQueue();
}

void
LrWpanMac::SendAssocResponseCommand(Ptr<Packet> rxDataReqPkt)
{
    LrWpanMacHeader receivedMacHdr;
    rxDataReqPkt->RemoveHeader(receivedMacHdr);
    CommandPayloadHeader receivedMacPayload;
    rxDataReqPkt->RemoveHeader(receivedMacPayload);

    NS_ASSERT(receivedMacPayload.GetCommandFrameType() == CommandPayloadHeader::DATA_REQ);

    Ptr<IndTxQueueElement> indTxQElement = Create<IndTxQueueElement>();
    bool elementFound;
    elementFound = DequeueInd(receivedMacHdr.GetExtSrcAddr(), indTxQElement);
    if (elementFound)
    {
        Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
        txQElement->txQPkt = indTxQElement->txQPkt;
        m_txQueue.emplace_back(txQElement);
    }
    else
    {
        NS_LOG_DEBUG("Requested element not found in pending list");
    }
}

void
LrWpanMac::LostAssocRespCommand()
{
    // Association response command was not received, return to default values.
    m_macPanId = 0xffff;
    m_macCoordShortAddress = Mac16Address("FF:FF");
    m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");

    if (!m_mlmeAssociateConfirmCallback.IsNull())
    {
        MlmeAssociateConfirmParams confirmParams;
        confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
        confirmParams.m_status = MacStatus::NO_DATA;
        m_mlmeAssociateConfirmCallback(confirmParams);
    }
}

void
LrWpanMac::EndStartRequest()
{
    NS_LOG_FUNCTION(this);
    // The primitive is no longer pending (Channel & Page have been set)
    m_pendPrimitive = MLME_NONE;

    if (m_startParams.m_coorRealgn) // Coordinator Realignment
    {
        // TODO: Send realignment request command frame in CSMA/CA
        NS_LOG_ERROR(this << " Coordinator realignment request not supported");
        return;
    }
    else
    {
        if (m_startParams.m_panCoor)
        {
            m_panCoor = true;
        }

        m_coor = true;
        m_macPanId = m_startParams.m_PanId;

        NS_ASSERT(m_startParams.m_PanId != 0xffff);

        m_macBeaconOrder = m_startParams.m_bcnOrd;
        if (m_macBeaconOrder == 15)
        {
            // Non-beacon enabled PAN
            // Cancel any ongoing events and CSMA-CA process
            m_macSuperframeOrder = 15;
            m_fnlCapSlot = 15;
            m_beaconInterval = 0;

            m_csmaCa->Cancel();
            m_capEvent.Cancel();
            m_cfpEvent.Cancel();
            m_incCapEvent.Cancel();
            m_incCfpEvent.Cancel();
            m_trackingEvent.Cancel();
            m_scanEvent.Cancel();
            m_scanOrphanEvent.Cancel();
            m_scanEnergyEvent.Cancel();

            m_csmaCa->SetUnSlottedCsmaCa();

            if (!m_mlmeStartConfirmCallback.IsNull())
            {
                MlmeStartConfirmParams confirmParams;
                confirmParams.m_status = MacStatus::SUCCESS;
                m_mlmeStartConfirmCallback(confirmParams);
            }

            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
        }
        else
        {
            m_macSuperframeOrder = m_startParams.m_sfrmOrd;
            m_csmaCa->SetBatteryLifeExtension(m_startParams.m_battLifeExt);

            m_csmaCa->SetSlottedCsmaCa();

            // TODO: Calculate the real Final CAP slot (requires GTS implementation)
            //  FinalCapSlot = Superframe duration slots - CFP slots.
            //  In the current implementation the value of the final cap slot is equal to
            //  the total number of possible slots in the superframe (15).
            m_fnlCapSlot = 15;

            m_beaconInterval =
                (static_cast<uint32_t>(1 << m_macBeaconOrder)) * lrwpan::aBaseSuperframeDuration;
            m_superframeDuration = (static_cast<uint32_t>(1 << m_macSuperframeOrder)) *
                                   lrwpan::aBaseSuperframeDuration;

            // TODO: change the beacon sending according to the startTime parameter (if not PAN
            // coordinator)

            m_beaconEvent = Simulator::ScheduleNow(&LrWpanMac::SendOneBeacon, this);
        }
    }
}

void
LrWpanMac::EndChannelScan()
{
    NS_LOG_FUNCTION(this);

    m_channelScanIndex++;

    bool channelFound = false;

    for (int i = m_channelScanIndex; i <= 26; i++)
    {
        if ((m_scanParams.m_scanChannels & (1 << m_channelScanIndex)) != 0)
        {
            channelFound = true;
            break;
        }
        m_channelScanIndex++;
    }

    if (channelFound)
    {
        // Switch to the next channel in the list and restart scan
        Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
        pibAttr->phyCurrentChannel = m_channelScanIndex;
        m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel, pibAttr);
    }
    else
    {
        // All channels in the list scan completed.
        // Return variables to the values before the scan and return the status to the next layer.
        m_macPanId = m_macPanIdScan;
        m_macPanIdScan = 0;

        // TODO: restart beacon transmissions that were active before the beginning of the scan
        // (i.e when a coordinator perform a scan and it was already transmitting beacons)
        MlmeScanConfirmParams confirmParams;
        confirmParams.m_chPage = m_scanParams.m_chPage;
        confirmParams.m_scanType = m_scanParams.m_scanType;
        confirmParams.m_energyDetList = {};
        confirmParams.m_unscannedCh = m_unscannedChannels;
        confirmParams.m_resultListSize = m_panDescriptorList.size();

        // See IEEE 802.15.4-2011, Table 31 (panDescriptorList value on macAutoRequest)
        // and Section 6.2.10.2
        switch (confirmParams.m_scanType)
        {
        case MLMESCAN_PASSIVE:
            if (m_macAutoRequest)
            {
                confirmParams.m_panDescList = m_panDescriptorList;
            }
            confirmParams.m_status = MacStatus::SUCCESS;
            break;
        case MLMESCAN_ACTIVE:
            if (m_panDescriptorList.empty())
            {
                confirmParams.m_status = MacStatus::NO_BEACON;
            }
            else
            {
                if (m_macAutoRequest)
                {
                    confirmParams.m_panDescList = m_panDescriptorList;
                }
                confirmParams.m_status = MacStatus::SUCCESS;
            }
            break;
        case MLMESCAN_ORPHAN:
            confirmParams.m_panDescList = {};
            confirmParams.m_status = MacStatus::NO_BEACON;
            confirmParams.m_resultListSize = 0;
            // The device lost track of the coordinator and was unable
            // to locate it, disassociate from the network.
            m_macPanId = 0xffff;
            m_shortAddress = Mac16Address("FF:FF");
            m_macCoordShortAddress = Mac16Address("ff:ff");
            m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
            break;
        default:
            NS_LOG_ERROR(this << " Invalid scan type");
        }

        m_pendPrimitive = MLME_NONE;
        m_channelScanIndex = 0;
        m_scanParams = {};

        if (!m_mlmeScanConfirmCallback.IsNull())
        {
            m_mlmeScanConfirmCallback(confirmParams);
        }
    }
}

void
LrWpanMac::EndChannelEnergyScan()
{
    NS_LOG_FUNCTION(this);
    // Add the results of channel energy scan to the detectList
    m_energyDetectList.emplace_back(m_maxEnergyLevel);
    m_maxEnergyLevel = 0;

    m_channelScanIndex++;

    bool channelFound = false;
    for (int i = m_channelScanIndex; i <= 26; i++)
    {
        if ((m_scanParams.m_scanChannels & (1 << m_channelScanIndex)) != 0)
        {
            channelFound = true;
            break;
        }
        m_channelScanIndex++;
    }

    if (channelFound)
    {
        // switch to the next channel in the list and restart scan
        Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
        pibAttr->phyCurrentChannel = m_channelScanIndex;
        m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel, pibAttr);
    }
    else
    {
        // Scan on all channels on the list completed
        // Return to the MAC values previous to start of the first scan.
        m_macPanId = m_macPanIdScan;
        m_macPanIdScan = 0;

        // TODO: restart beacon transmissions that were active before the beginning of the scan
        // (i.e when a coordinator perform a scan and it was already transmitting beacons)

        // All channels scanned, report success
        MlmeScanConfirmParams confirmParams;
        confirmParams.m_status = MacStatus::SUCCESS;
        confirmParams.m_chPage = m_phy->GetCurrentPage();
        confirmParams.m_scanType = m_scanParams.m_scanType;
        confirmParams.m_energyDetList = m_energyDetectList;
        confirmParams.m_resultListSize = m_energyDetectList.size();

        m_pendPrimitive = MLME_NONE;
        m_channelScanIndex = 0;
        m_scanParams = {};

        if (!m_mlmeScanConfirmCallback.IsNull())
        {
            m_mlmeScanConfirmCallback(confirmParams);
        }
    }
}

void
LrWpanMac::StartCAP(SuperframeType superframeType)
{
    uint32_t activeSlot;
    uint64_t capDuration;
    Time endCapTime;
    uint64_t symbolRate;

    symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second

    if (superframeType == OUTGOING)
    {
        m_outSuperframeStatus = CAP;
        activeSlot = m_superframeDuration / 16;
        capDuration = activeSlot * (m_fnlCapSlot + 1);
        endCapTime = Seconds((double)capDuration / symbolRate);
        // Obtain the end of the CAP by adjust the time it took to send the beacon
        endCapTime -= (Simulator::Now() - m_macBeaconTxTime);

        NS_LOG_DEBUG("Outgoing superframe CAP duration " << (endCapTime.GetSeconds() * symbolRate)
                                                         << " symbols (" << endCapTime.As(Time::S)
                                                         << ")");
        NS_LOG_DEBUG("Active Slots duration " << activeSlot << " symbols");

        m_capEvent =
            Simulator::Schedule(endCapTime, &LrWpanMac::StartCFP, this, SuperframeType::OUTGOING);
    }
    else
    {
        m_incSuperframeStatus = CAP;
        activeSlot = m_incomingSuperframeDuration / 16;
        capDuration = activeSlot * (m_incomingFnlCapSlot + 1);
        endCapTime = Seconds((double)capDuration / symbolRate);
        // Obtain the end of the CAP by adjust the time it took to receive the beacon
        endCapTime -= (Simulator::Now() - m_macBeaconRxTime);

        NS_LOG_DEBUG("Incoming superframe CAP duration " << (endCapTime.GetSeconds() * symbolRate)
                                                         << " symbols (" << endCapTime.As(Time::S)
                                                         << ")");
        NS_LOG_DEBUG("Active Slots duration " << activeSlot << " symbols");

        m_capEvent =
            Simulator::Schedule(endCapTime, &LrWpanMac::StartCFP, this, SuperframeType::INCOMING);
    }

    CheckQueue();
}

void
LrWpanMac::StartCFP(SuperframeType superframeType)
{
    uint32_t activeSlot;
    uint64_t cfpDuration;
    Time endCfpTime;
    uint64_t symbolRate;

    symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second

    if (superframeType == INCOMING)
    {
        activeSlot = m_incomingSuperframeDuration / 16;
        cfpDuration = activeSlot * (15 - m_incomingFnlCapSlot);
        endCfpTime = Seconds((double)cfpDuration / symbolRate);
        if (cfpDuration > 0)
        {
            m_incSuperframeStatus = CFP;
        }

        NS_LOG_DEBUG("Incoming superframe CFP duration " << cfpDuration << " symbols ("
                                                         << endCfpTime.As(Time::S) << ")");

        m_incCfpEvent = Simulator::Schedule(endCfpTime,
                                            &LrWpanMac::StartInactivePeriod,
                                            this,
                                            SuperframeType::INCOMING);
    }
    else
    {
        activeSlot = m_superframeDuration / 16;
        cfpDuration = activeSlot * (15 - m_fnlCapSlot);
        endCfpTime = Seconds((double)cfpDuration / symbolRate);

        if (cfpDuration > 0)
        {
            m_outSuperframeStatus = CFP;
        }

        NS_LOG_DEBUG("Outgoing superframe CFP duration " << cfpDuration << " symbols ("
                                                         << endCfpTime.As(Time::S) << ")");

        m_cfpEvent = Simulator::Schedule(endCfpTime,
                                         &LrWpanMac::StartInactivePeriod,
                                         this,
                                         SuperframeType::OUTGOING);
    }
    // TODO: Start transmit or receive  GTS here.
}

void
LrWpanMac::StartInactivePeriod(SuperframeType superframeType)
{
    uint64_t inactiveDuration;
    Time endInactiveTime;
    uint64_t symbolRate;

    symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second

    if (superframeType == INCOMING)
    {
        inactiveDuration = m_incomingBeaconInterval - m_incomingSuperframeDuration;
        endInactiveTime = Seconds((double)inactiveDuration / symbolRate);

        if (inactiveDuration > 0)
        {
            m_incSuperframeStatus = INACTIVE;
        }

        NS_LOG_DEBUG("Incoming superframe Inactive Portion duration "
                     << inactiveDuration << " symbols (" << endInactiveTime.As(Time::S) << ")");
        m_beaconEvent = Simulator::Schedule(endInactiveTime, &LrWpanMac::AwaitBeacon, this);
    }
    else
    {
        inactiveDuration = m_beaconInterval - m_superframeDuration;
        endInactiveTime = Seconds((double)inactiveDuration / symbolRate);

        if (inactiveDuration > 0)
        {
            m_outSuperframeStatus = INACTIVE;
        }

        NS_LOG_DEBUG("Outgoing superframe Inactive Portion duration "
                     << inactiveDuration << " symbols (" << endInactiveTime.As(Time::S) << ")");
        m_beaconEvent = Simulator::Schedule(endInactiveTime, &LrWpanMac::SendOneBeacon, this);
    }
}

void
LrWpanMac::AwaitBeacon()
{
    m_incSuperframeStatus = BEACON;

    // TODO: If the device waits more than the expected time to receive the beacon (wait = 46
    // symbols for default beacon size)
    //       it should continue with the start of the incoming CAP even if it did not receive the
    //       beacon. At the moment, the start of the incoming CAP is only triggered if the beacon is
    //       received. See MLME-SyncLoss for details.
}

void
LrWpanMac::BeaconSearchTimeout()
{
    auto symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second

    if (m_numLostBeacons > lrwpan::aMaxLostBeacons)
    {
        MlmeSyncLossIndicationParams syncLossParams;
        // syncLossParams.m_logCh =
        syncLossParams.m_lossReason = MacStatus::BEACON_LOSS;
        syncLossParams.m_panId = m_macPanId;
        m_mlmeSyncLossIndicationCallback(syncLossParams);

        m_beaconTrackingOn = false;
        m_numLostBeacons = 0;
    }
    else
    {
        m_numLostBeacons++;

        // Search for one more beacon
        uint64_t searchSymbols;
        Time searchBeaconTime;
        searchSymbols =
            ((uint64_t)1 << m_incomingBeaconOrder) + 1 * lrwpan::aBaseSuperframeDuration;
        searchBeaconTime = Seconds((double)searchSymbols / symbolRate);
        m_trackingEvent =
            Simulator::Schedule(searchBeaconTime, &LrWpanMac::BeaconSearchTimeout, this);
    }
}

void
LrWpanMac::CheckQueue()
{
    NS_LOG_FUNCTION(this);
    // Pull a packet from the queue and start sending if we are not already sending.
    if (m_macState == MAC_IDLE && !m_txQueue.empty() && !m_setMacState.IsPending())
    {
        if (m_csmaCa->IsUnSlottedCsmaCa() || (m_outSuperframeStatus == CAP && m_coor) ||
            m_incSuperframeStatus == CAP)
        {
            // check MAC is not in a IFS
            if (!m_ifsEvent.IsPending())
            {
                Ptr<TxQueueElement> txQElement = m_txQueue.front();
                m_txPkt = txQElement->txQPkt;

                m_setMacState =
                    Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
            }
        }
    }
}

uint16_t
LrWpanMac::GetSuperframeField()
{
    SuperframeField sfrmSpec;

    sfrmSpec.SetBeaconOrder(m_macBeaconOrder);
    sfrmSpec.SetSuperframeOrder(m_macSuperframeOrder);
    sfrmSpec.SetFinalCapSlot(m_fnlCapSlot);

    if (m_csmaCa->GetBatteryLifeExtension())
    {
        sfrmSpec.SetBattLifeExt(true);
    }

    if (m_panCoor)
    {
        sfrmSpec.SetPanCoor(true);
    }

    // used to associate devices via Beacons
    if (m_macAssociationPermit)
    {
        sfrmSpec.SetAssocPermit(true);
    }

    return sfrmSpec.GetSuperframe();
}

GtsFields
LrWpanMac::GetGtsFields()
{
    GtsFields gtsFields;

    // TODO: Logic to populate the GTS Fields from local information here

    return gtsFields;
}

PendingAddrFields
LrWpanMac::GetPendingAddrFields()
{
    PendingAddrFields pndAddrFields;

    // TODO: Logic to populate the Pending Address Fields from local information here
    return pndAddrFields;
}

void
LrWpanMac::SetCsmaCa(Ptr<LrWpanCsmaCa> csmaCa)
{
    m_csmaCa = csmaCa;
}

void
LrWpanMac::SetPhy(Ptr<LrWpanPhy> phy)
{
    m_phy = phy;
}

Ptr<LrWpanPhy>
LrWpanMac::GetPhy()
{
    return m_phy;
}

void
LrWpanMac::PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
    NS_ASSERT(m_macState == MAC_IDLE || m_macState == MAC_ACK_PENDING || m_macState == MAC_CSMA);
    NS_LOG_FUNCTION(this << psduLength << p << (uint16_t)lqi);

    bool acceptFrame;

    // from sec 7.5.6.2 Reception and rejection, Std802.15.4-2006
    // level 1 filtering, test FCS field and reject if frame fails
    // level 2 filtering if promiscuous mode pass frame to higher layer otherwise perform level 3
    // filtering level 3 filtering accept frame if Frame type and version is not reserved, and if
    // there is a dstPanId then dstPanId=m_macPanId or broadcastPanId, and if there is a
    // shortDstAddr then shortDstAddr =shortMacAddr or broadcastAddr, and if beacon frame then
    // srcPanId = m_macPanId if only srcAddr field in Data or Command frame,accept frame if
    // srcPanId=m_macPanId

    Ptr<Packet> originalPkt = p->Copy();                           // because we will strip headers
    auto symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second
    m_promiscSnifferTrace(originalPkt);

    m_macPromiscRxTrace(originalPkt);
    // XXX no rejection tracing (to macRxDropTrace) being performed below

    LrWpanMacTrailer receivedMacTrailer;
    p->RemoveTrailer(receivedMacTrailer);
    if (Node::ChecksumEnabled())
    {
        receivedMacTrailer.EnableFcs(true);
    }

    // level 1 filtering
    if (!receivedMacTrailer.CheckFcs(p))
    {
        m_macRxDropTrace(originalPkt);
    }
    else
    {
        LrWpanMacHeader receivedMacHdr;
        p->RemoveHeader(receivedMacHdr);

        McpsDataIndicationParams params;
        params.m_dsn = receivedMacHdr.GetSeqNum();
        params.m_mpduLinkQuality = lqi;
        params.m_srcPanId = receivedMacHdr.GetSrcPanId();
        params.m_srcAddrMode = receivedMacHdr.GetSrcAddrMode();
        switch (params.m_srcAddrMode)
        {
        case SHORT_ADDR:
            params.m_srcAddr = receivedMacHdr.GetShortSrcAddr();
            break;
        case EXT_ADDR:
            params.m_srcExtAddr = receivedMacHdr.GetExtSrcAddr();
            break;
        default:
            break;
        }
        params.m_dstPanId = receivedMacHdr.GetDstPanId();
        params.m_dstAddrMode = receivedMacHdr.GetDstAddrMode();
        switch (params.m_dstAddrMode)
        {
        case SHORT_ADDR:
            params.m_dstAddr = receivedMacHdr.GetShortDstAddr();
            break;
        case EXT_ADDR:
            params.m_dstExtAddr = receivedMacHdr.GetExtDstAddr();
            break;
        default:
            break;
        }

        if (m_macPromiscuousMode)
        {
            // level 2 filtering
            if (receivedMacHdr.GetDstAddrMode() == SHORT_ADDR)
            {
                NS_LOG_DEBUG("Packet from " << params.m_srcAddr);
                NS_LOG_DEBUG("Packet to " << params.m_dstAddr);
            }
            else if (receivedMacHdr.GetDstAddrMode() == EXT_ADDR)
            {
                NS_LOG_DEBUG("Packet from " << params.m_srcExtAddr);
                NS_LOG_DEBUG("Packet to " << params.m_dstExtAddr);
            }

            // TODO: Fix here, this should trigger different Indication Callbacks
            // depending the type of frame received (data,command, beacon)
            if (!m_mcpsDataIndicationCallback.IsNull())
            {
                NS_LOG_DEBUG("promiscuous mode, forwarding up");
                m_mcpsDataIndicationCallback(params, p);
            }
            else
            {
                NS_LOG_ERROR(this << " Data Indication Callback not initialized");
            }
        }
        else
        {
            // level 3 frame filtering
            acceptFrame = (receivedMacHdr.GetType() != LrWpanMacHeader::LRWPAN_MAC_RESERVED);

            if (acceptFrame)
            {
                acceptFrame = (receivedMacHdr.GetFrameVer() <= 1);
            }

            if (acceptFrame && (receivedMacHdr.GetDstAddrMode() > 1))
            {
                // Accept frame if one of the following is true:

                // 1) Have the same macPanId
                // 2) Is Message to all PANs
                // 3) Is a beacon or command frame and the macPanId is not present (bootstrap)
                acceptFrame = ((receivedMacHdr.GetDstPanId() == m_macPanId ||
                                receivedMacHdr.GetDstPanId() == 0xffff) ||
                               (m_macPanId == 0xffff && receivedMacHdr.IsBeacon())) ||
                              (m_macPanId == 0xffff && receivedMacHdr.IsCommand());
            }

            if (acceptFrame && (receivedMacHdr.GetDstAddrMode() == SHORT_ADDR))
            {
                if (receivedMacHdr.GetShortDstAddr() == m_shortAddress)
                {
                    // unicast, for me
                    acceptFrame = true;
                }
                else if (receivedMacHdr.GetShortDstAddr().IsBroadcast() ||
                         receivedMacHdr.GetShortDstAddr().IsMulticast())
                {
                    // Broadcast or multicast.
                    // Discard broadcast/multicast with the ACK bit set.
                    acceptFrame = !receivedMacHdr.IsAckReq();
                }
                else
                {
                    acceptFrame = false;
                }
            }

            if (acceptFrame && (receivedMacHdr.GetDstAddrMode() == EXT_ADDR))
            {
                acceptFrame = (receivedMacHdr.GetExtDstAddr() == m_macExtendedAddress);
            }

            if (acceptFrame && m_scanEvent.IsPending())
            {
                if (!receivedMacHdr.IsBeacon())
                {
                    acceptFrame = false;
                }
            }
            else if (acceptFrame && m_scanOrphanEvent.IsPending())
            {
                if (!receivedMacHdr.IsCommand())
                {
                    acceptFrame = false;
                }
            }
            else if (m_scanEnergyEvent.IsPending())
            {
                // Reject any frames if energy scan is running
                acceptFrame = false;
            }

            // Check device is panCoor with association permit when receiving Association Request
            // Commands.
            // TODO:: Simple coordinators should also be able to receive it (currently only Pan
            // Coordinators are checked)
            if (acceptFrame && (receivedMacHdr.IsCommand() && receivedMacHdr.IsAckReq()))
            {
                CommandPayloadHeader receivedMacPayload;
                p->PeekHeader(receivedMacPayload);

                if (receivedMacPayload.GetCommandFrameType() ==
                        CommandPayloadHeader::ASSOCIATION_REQ &&
                    !(m_macAssociationPermit && m_coor))
                {
                    acceptFrame = false;
                }

                // Although ACKs do not use CSMA to to be transmitted, we need to make sure
                // that the transmitted ACK will not collide with the transmission of a beacon
                // when beacon-enabled mode is running in the coordinator.
                if (acceptFrame && (m_csmaCa->IsSlottedCsmaCa() && m_capEvent.IsPending()))
                {
                    Time timeLeftInCap = Simulator::GetDelayLeft(m_capEvent);
                    uint64_t ackSymbols = lrwpan::aTurnaroundTime + m_phy->GetPhySHRDuration() +
                                          ceil(6 * m_phy->GetPhySymbolsPerOctet());
                    Time ackTime = Seconds((double)ackSymbols / symbolRate);

                    if (ackTime >= timeLeftInCap)
                    {
                        NS_LOG_DEBUG("Command frame received but not enough time to transmit ACK "
                                     "before the end of CAP ");
                        acceptFrame = false;
                    }
                }
            }

            if (acceptFrame)
            {
                m_macRxTrace(originalPkt);
                // \todo: What should we do if we receive a frame while waiting for an ACK?
                //        Especially if this frame has the ACK request bit set, should we reply with
                //        an ACK, possibly missing the pending ACK?

                // If the received frame is a frame with the ACK request bit set, we immediately
                // send back an ACK. If we are currently waiting for a pending ACK, we assume the
                // ACK was lost and trigger a retransmission after sending the ACK.
                if ((receivedMacHdr.IsData() || receivedMacHdr.IsCommand()) &&
                    receivedMacHdr.IsAckReq() &&
                    !(receivedMacHdr.GetDstAddrMode() == SHORT_ADDR &&
                      (receivedMacHdr.GetShortDstAddr().IsBroadcast() ||
                       receivedMacHdr.GetShortDstAddr().IsMulticast())))
                {
                    // If this is a data or mac command frame, which is not a broadcast or
                    // multicast, with ack req set, generate and send an ack frame. If there is a
                    // CSMA medium access in progress we cancel the medium access for sending the
                    // ACK frame. A new transmission attempt will be started after the ACK was send.
                    if (m_macState == MAC_ACK_PENDING)
                    {
                        m_ackWaitTimeout.Cancel();
                        PrepareRetransmission();
                    }
                    else if (m_macState == MAC_CSMA)
                    {
                        // \todo: If we receive a packet while doing CSMA/CA, should  we drop the
                        // packet because of channel busy,
                        //        or should we restart CSMA/CA for the packet after sending the ACK?
                        // Currently we simply restart CSMA/CA after sending the ACK.
                        NS_LOG_DEBUG("Received a packet with ACK required while in CSMA. Cancel "
                                     "current CSMA-CA");
                        m_csmaCa->Cancel();
                    }
                    // Cancel any pending MAC state change, ACKs have higher priority.
                    m_setMacState.Cancel();
                    ChangeMacState(MAC_IDLE);

                    // save received packet and LQI to process the appropriate indication/response
                    // after sending ACK (PD-DATA.confirm)
                    m_rxPkt = originalPkt->Copy();
                    m_lastRxFrameLqi = lqi;

                    // LOG Commands with ACK required.
                    CommandPayloadHeader receivedMacPayload;
                    p->PeekHeader(receivedMacPayload);
                    switch (receivedMacPayload.GetCommandFrameType())
                    {
                    case CommandPayloadHeader::DATA_REQ:
                        NS_LOG_DEBUG("Data Request Command Received; processing ACK");
                        break;
                    case CommandPayloadHeader::ASSOCIATION_REQ:
                        NS_LOG_DEBUG("Association Request Command Received; processing ACK");
                        break;
                    case CommandPayloadHeader::ASSOCIATION_RESP:
                        m_assocResCmdWaitTimeout.Cancel(); // cancel event to a lost assoc resp cmd.
                        NS_LOG_DEBUG("Association Response Command Received; processing ACK");
                        break;
                    default:
                        break;
                    }

                    m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SendAck,
                                                           this,
                                                           receivedMacHdr.GetSeqNum());
                }

                if (receivedMacHdr.GetDstAddrMode() == SHORT_ADDR)
                {
                    NS_LOG_DEBUG("Packet from " << params.m_srcAddr);
                    NS_LOG_DEBUG("Packet to " << params.m_dstAddr);
                }
                else if (receivedMacHdr.GetDstAddrMode() == EXT_ADDR)
                {
                    NS_LOG_DEBUG("Packet from " << params.m_srcExtAddr);
                    NS_LOG_DEBUG("Packet to " << params.m_dstExtAddr);
                }

                if (receivedMacHdr.IsBeacon())
                {
                    // The received beacon size in symbols
                    // Beacon = 5 bytes Sync Header (SHR) +  1 byte PHY header (PHR) + PSDU (default
                    // 17 bytes)
                    m_rxBeaconSymbols = m_phy->GetPhySHRDuration() +
                                        1 * m_phy->GetPhySymbolsPerOctet() +
                                        (originalPkt->GetSize() * m_phy->GetPhySymbolsPerOctet());

                    // The start of Rx beacon time and start of the Incoming superframe Active
                    // Period
                    m_macBeaconRxTime =
                        Simulator::Now() - Seconds(double(m_rxBeaconSymbols) / symbolRate);

                    NS_LOG_DEBUG("Beacon Received; forwarding up (m_macBeaconRxTime: "
                                 << m_macBeaconRxTime.As(Time::S) << ")");

                    BeaconPayloadHeader receivedMacPayload;
                    p->RemoveHeader(receivedMacPayload);

                    // Fill the PAN descriptor
                    PanDescriptor panDescriptor;

                    if (receivedMacHdr.GetSrcAddrMode() == SHORT_ADDR)
                    {
                        panDescriptor.m_coorAddrMode = SHORT_ADDR;
                        panDescriptor.m_coorShortAddr = receivedMacHdr.GetShortSrcAddr();
                    }
                    else
                    {
                        panDescriptor.m_coorAddrMode = EXT_ADDR;
                        panDescriptor.m_coorExtAddr = receivedMacHdr.GetExtSrcAddr();
                    }

                    panDescriptor.m_coorPanId = receivedMacHdr.GetSrcPanId();
                    panDescriptor.m_gtsPermit = receivedMacPayload.GetGtsFields().GetGtsPermit();
                    panDescriptor.m_linkQuality = lqi;
                    panDescriptor.m_logChPage = m_phy->GetCurrentPage();
                    panDescriptor.m_logCh = m_phy->GetCurrentChannelNum();
                    panDescriptor.m_superframeSpec = receivedMacPayload.GetSuperframeSpecField();
                    panDescriptor.m_timeStamp = m_macBeaconRxTime;

                    // Process beacon when device belongs to a PAN (associated device)
                    if (!m_scanEvent.IsPending() && m_macPanId == receivedMacHdr.GetDstPanId())
                    {
                        // We need to make sure to cancel any possible ongoing unslotted CSMA/CA
                        // operations when receiving a beacon (e.g. Those taking place at the
                        // beginning of an Association).
                        m_csmaCa->Cancel();

                        SuperframeField incomingSuperframe(
                            receivedMacPayload.GetSuperframeSpecField());

                        m_incomingBeaconOrder = incomingSuperframe.GetBeaconOrder();
                        m_incomingSuperframeOrder = incomingSuperframe.GetFrameOrder();
                        m_incomingFnlCapSlot = incomingSuperframe.GetFinalCapSlot();

                        if (m_incomingBeaconOrder < 15)
                        {
                            // Start Beacon-enabled mode
                            m_csmaCa->SetSlottedCsmaCa();
                            m_incomingBeaconInterval =
                                (static_cast<uint32_t>(1 << m_incomingBeaconOrder)) *
                                lrwpan::aBaseSuperframeDuration;
                            m_incomingSuperframeDuration =
                                lrwpan::aBaseSuperframeDuration *
                                (static_cast<uint32_t>(1 << m_incomingSuperframeOrder));

                            if (incomingSuperframe.IsBattLifeExt())
                            {
                                m_csmaCa->SetBatteryLifeExtension(true);
                            }
                            else
                            {
                                m_csmaCa->SetBatteryLifeExtension(false);
                            }

                            // TODO: get Incoming frame GTS Fields here

                            // Begin CAP on the current device using info from
                            // the Incoming superframe
                            NS_LOG_DEBUG("Incoming superframe Active Portion "
                                         << "(Beacon + CAP + CFP): " << m_incomingSuperframeDuration
                                         << " symbols");

                            m_incCapEvent = Simulator::ScheduleNow(&LrWpanMac::StartCAP,
                                                                   this,
                                                                   SuperframeType::INCOMING);
                        }
                        else
                        {
                            // Start non-beacon enabled mode
                            m_csmaCa->SetUnSlottedCsmaCa();
                        }

                        m_setMacState =
                            Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
                    }
                    else if (!m_scanEvent.IsPending() && m_macPanId == 0xFFFF)
                    {
                        NS_LOG_DEBUG(this << " Device not associated, cannot process beacon");
                    }

                    if (m_macAutoRequest)
                    {
                        if (p->GetSize() > 0)
                        {
                            if (!m_mlmeBeaconNotifyIndicationCallback.IsNull())
                            {
                                // The beacon contains payload, send the beacon notification.
                                MlmeBeaconNotifyIndicationParams beaconParams;
                                beaconParams.m_bsn = receivedMacHdr.GetSeqNum();
                                beaconParams.m_panDescriptor = panDescriptor;
                                beaconParams.m_sduLength = p->GetSize();
                                beaconParams.m_sdu = p;
                                m_mlmeBeaconNotifyIndicationCallback(beaconParams);
                            }
                        }

                        if (m_scanEvent.IsPending())
                        {
                            // Channel scanning is taking place, save only unique PAN descriptors
                            bool descriptorExists = false;

                            for (const auto& descriptor : m_panDescriptorList)
                            {
                                if (descriptor.m_coorAddrMode == SHORT_ADDR)
                                {
                                    // Found a coordinator in PAN descriptor list with the same
                                    // registered short address
                                    descriptorExists =
                                        (descriptor.m_coorShortAddr ==
                                             panDescriptor.m_coorShortAddr &&
                                         descriptor.m_coorPanId == panDescriptor.m_coorPanId);
                                }
                                else
                                {
                                    // Found a coordinator in PAN descriptor list with the same
                                    // registered extended address
                                    descriptorExists =
                                        (descriptor.m_coorExtAddr == panDescriptor.m_coorExtAddr &&
                                         descriptor.m_coorPanId == panDescriptor.m_coorPanId);
                                }

                                if (descriptorExists)
                                {
                                    break;
                                }
                            }

                            if (!descriptorExists)
                            {
                                m_panDescriptorList.emplace_back(panDescriptor);
                            }
                            return;
                        }
                        else if (m_trackingEvent.IsPending())
                        {
                            // check if MLME-SYNC.request was previously issued and running
                            // Sync. is necessary to handle pending messages (indirect
                            // transmissions)
                            m_trackingEvent.Cancel();
                            m_numLostBeacons = 0;

                            if (m_beaconTrackingOn)
                            {
                                // if tracking option is on keep tracking the next beacon
                                uint64_t searchSymbols;
                                Time searchBeaconTime;

                                searchSymbols =
                                    (static_cast<uint64_t>(1 << m_incomingBeaconOrder)) +
                                    1 * lrwpan::aBaseSuperframeDuration;
                                searchBeaconTime =
                                    Seconds(static_cast<double>(searchSymbols / symbolRate));
                                m_trackingEvent =
                                    Simulator::Schedule(searchBeaconTime,
                                                        &LrWpanMac::BeaconSearchTimeout,
                                                        this);
                            }

                            PendingAddrFields pndAddrFields;
                            pndAddrFields = receivedMacPayload.GetPndAddrFields();

                            // TODO: Ignore pending data, and do not send data command request if
                            // the address is in the GTS list.
                            //       If the address is not in the GTS list, then  check if the
                            //       address is in the short address pending list or in the extended
                            //       address pending list and send a data command request.
                        }
                    }
                    else
                    {
                        // m_macAutoRequest is FALSE
                        // Data command request are not send, only the beacon notification.
                        // see IEEE 802.15.4-2011 Section 6.2.4.1
                        if (!m_mlmeBeaconNotifyIndicationCallback.IsNull())
                        {
                            MlmeBeaconNotifyIndicationParams beaconParams;
                            beaconParams.m_bsn = receivedMacHdr.GetSeqNum();
                            beaconParams.m_panDescriptor = panDescriptor;
                            beaconParams.m_sduLength = p->GetSize();
                            beaconParams.m_sdu = p;
                            m_mlmeBeaconNotifyIndicationCallback(beaconParams);
                        }
                    }
                }
                else if (receivedMacHdr.IsCommand())
                {
                    // Handle the reception of frame commands that do not require ACK
                    // (i.e. Beacon Request, Orphan notification, Coordinator Realigment)
                    CommandPayloadHeader receivedMacPayload;
                    p->PeekHeader(receivedMacPayload);

                    switch (receivedMacPayload.GetCommandFrameType())
                    {
                    case CommandPayloadHeader::BEACON_REQ:
                        if (m_csmaCa->IsUnSlottedCsmaCa() && m_coor)
                        {
                            SendOneBeacon();
                        }
                        else
                        {
                            m_macRxDropTrace(originalPkt);
                        }
                        break;
                    case CommandPayloadHeader::ORPHAN_NOTIF:
                        if (!m_mlmeOrphanIndicationCallback.IsNull())
                        {
                            if (m_coor)
                            {
                                MlmeOrphanIndicationParams orphanParams;
                                orphanParams.m_orphanAddr = receivedMacHdr.GetExtSrcAddr();
                                m_mlmeOrphanIndicationCallback(orphanParams);
                            }
                        }
                        break;
                    case CommandPayloadHeader::COOR_REALIGN:
                        if (m_scanOrphanEvent.IsPending())
                        {
                            // Coordinator located, no need to keep scanning other channels
                            m_scanOrphanEvent.Cancel();

                            m_macPanIdScan = 0;
                            m_pendPrimitive = MLME_NONE;
                            m_channelScanIndex = 0;

                            // Update the device information with the received information
                            // from the Coordinator Realigment command.
                            m_macPanId = receivedMacPayload.GetPanId();
                            m_shortAddress = receivedMacPayload.GetShortAddr();
                            m_macCoordExtendedAddress = receivedMacHdr.GetExtSrcAddr();
                            m_macCoordShortAddress = receivedMacPayload.GetCoordShortAddr();

                            if (!m_mlmeScanConfirmCallback.IsNull())
                            {
                                MlmeScanConfirmParams confirmParams;
                                confirmParams.m_scanType = m_scanParams.m_scanType;
                                confirmParams.m_chPage = m_scanParams.m_chPage;
                                confirmParams.m_status = MacStatus::SUCCESS;
                                m_mlmeScanConfirmCallback(confirmParams);
                            }
                            m_scanParams = {};
                        }
                        // TODO: handle Coordinator realignment when not
                        //       used during an orphan scan.
                        break;
                    default:
                        m_macRxDropTrace(originalPkt);
                        break;
                    }
                }
                else if (receivedMacHdr.IsData() && !m_mcpsDataIndicationCallback.IsNull())
                {
                    // If it is a data frame, push it up the stack.
                    NS_LOG_DEBUG("Data Packet is for me; forwarding up");
                    m_mcpsDataIndicationCallback(params, p);
                }
                else if (receivedMacHdr.IsAcknowledgment() && m_txPkt &&
                         m_macState == MAC_ACK_PENDING)
                {
                    LrWpanMacHeader peekedMacHdr;
                    m_txPkt->PeekHeader(peekedMacHdr);
                    // If it is an ACK with the expected sequence number, finish the transmission
                    if (receivedMacHdr.GetSeqNum() == peekedMacHdr.GetSeqNum())
                    {
                        m_ackWaitTimeout.Cancel();
                        m_macTxOkTrace(m_txPkt);

                        // TODO: check  if the IFS is the correct size after ACK.
                        Time ifsWaitTime = Seconds((double)GetIfsSize() / symbolRate);

                        // We received an ACK to a command
                        if (peekedMacHdr.IsCommand())
                        {
                            // check the original sent command frame which belongs to this received
                            // ACK
                            Ptr<Packet> pkt = m_txPkt->Copy();
                            LrWpanMacHeader macHdr;
                            CommandPayloadHeader cmdPayload;
                            pkt->RemoveHeader(macHdr);
                            pkt->RemoveHeader(cmdPayload);

                            switch (cmdPayload.GetCommandFrameType())
                            {
                            case CommandPayloadHeader::ASSOCIATION_REQ: {
                                double symbolRate = m_phy->GetDataOrSymbolRate(false);
                                Time waitTime = Seconds(static_cast<double>(m_macResponseWaitTime) /
                                                        symbolRate);
                                if (!m_beaconTrackingOn)
                                {
                                    m_respWaitTimeout =
                                        Simulator::Schedule(waitTime,
                                                            &LrWpanMac::SendDataRequestCommand,
                                                            this);
                                }
                                else
                                {
                                    // TODO: The data must be extracted by the coordinator within
                                    // macResponseWaitTime on timeout, MLME-ASSOCIATE.confirm is set
                                    // with status NO_DATA, and this should trigger the cancellation
                                    // of the beacon tracking (MLME-SYNC.request  trackBeacon
                                    // =FALSE)
                                }
                                break;
                            }

                            case CommandPayloadHeader::ASSOCIATION_RESP: {
                                // MLME-comm-status.Indication generated as a result of an
                                // association response command, therefore src and dst address use
                                // extended mode (see 5.3.2.1)
                                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                                {
                                    MlmeCommStatusIndicationParams commStatusParams;
                                    commStatusParams.m_panId = m_macPanId;
                                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                                    commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                                    commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                                    commStatusParams.m_status = MacStatus::SUCCESS;
                                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                                }
                                // Remove element from Pending Transaction List
                                RemovePendTxQElement(m_txPkt->Copy());
                                break;
                            }

                            case CommandPayloadHeader::DATA_REQ: {
                                // Schedule an event in case the Association Response Command never
                                // reached this device during an association process.
                                double symbolRate = m_phy->GetDataOrSymbolRate(false);
                                Time waitTime = Seconds(
                                    static_cast<double>(m_assocRespCmdWaitTime) / symbolRate);
                                m_assocResCmdWaitTimeout =
                                    Simulator::Schedule(waitTime,
                                                        &LrWpanMac::LostAssocRespCommand,
                                                        this);

                                if (!m_mlmePollConfirmCallback.IsNull())
                                {
                                    MlmePollConfirmParams pollConfirmParams;
                                    pollConfirmParams.m_status = MacStatus::SUCCESS;
                                    m_mlmePollConfirmCallback(pollConfirmParams);
                                }
                                break;
                            }

                            case CommandPayloadHeader::COOR_REALIGN: {
                                // ACK of coordinator realigment commands is not specified in the
                                // standard, in here, we assume they are required as in other
                                // commands.
                                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                                {
                                    MlmeCommStatusIndicationParams commStatusParams;
                                    commStatusParams.m_panId = m_macPanId;
                                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                                    commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                                    commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                                    commStatusParams.m_status = MacStatus::SUCCESS;
                                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                                }
                            }

                            default: {
                                // TODO: add response to other request commands (e.g. Orphan)
                                break;
                            }
                            }
                        }
                        else
                        {
                            if (!m_mcpsDataConfirmCallback.IsNull())
                            {
                                Ptr<TxQueueElement> txQElement = m_txQueue.front();
                                McpsDataConfirmParams confirmParams;
                                confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                                confirmParams.m_status = MacStatus::SUCCESS;
                                m_mcpsDataConfirmCallback(confirmParams);
                            }
                        }

                        // Ack was successfully received, wait for the Interframe Space (IFS) and
                        // then proceed
                        RemoveFirstTxQElement();
                        m_setMacState.Cancel();
                        m_setMacState =
                            Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
                        m_ifsEvent = Simulator::Schedule(ifsWaitTime,
                                                         &LrWpanMac::IfsWaitTimeout,
                                                         this,
                                                         ifsWaitTime);
                    }
                    else
                    {
                        // If it is an ACK with an unexpected sequence number, mark the current
                        // transmission as failed and start a retransmit. (cf 7.5.6.4.3)
                        m_ackWaitTimeout.Cancel();
                        if (!PrepareRetransmission())
                        {
                            m_setMacState.Cancel();
                            m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState,
                                                                   this,
                                                                   MAC_IDLE);
                        }
                        else
                        {
                            m_setMacState.Cancel();
                            m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState,
                                                                   this,
                                                                   MAC_CSMA);
                        }
                    }
                }
            }
            else
            {
                m_macRxDropTrace(originalPkt);
            }
        }
    }
}

void
LrWpanMac::SendAck(uint8_t seqno)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(seqno));

    NS_ASSERT(m_macState == MAC_IDLE);

    // Generate a corresponding ACK Frame.
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_ACKNOWLEDGMENT, seqno);
    LrWpanMacTrailer macTrailer;
    Ptr<Packet> ackPacket = Create<Packet>(0);
    ackPacket->AddHeader(macHdr);
    // Calculate FCS if the global attribute ChecksumEnabled is set.
    if (Node::ChecksumEnabled())
    {
        macTrailer.EnableFcs(true);
        macTrailer.SetFcs(ackPacket);
    }
    ackPacket->AddTrailer(macTrailer);

    // Enqueue the ACK packet for further processing
    // when the transmitter is activated.
    m_txPkt = ackPacket;

    // Switch transceiver to TX mode. Proceed sending the Ack on confirm.
    ChangeMacState(MAC_SENDING);
    m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
}

void
LrWpanMac::EnqueueTxQElement(Ptr<TxQueueElement> txQElement)
{
    if (m_txQueue.size() < m_maxTxQueueSize)
    {
        m_txQueue.emplace_back(txQElement);
        m_macTxEnqueueTrace(txQElement->txQPkt);
    }
    else
    {
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            McpsDataConfirmParams confirmParams;
            confirmParams.m_msduHandle = txQElement->txQMsduHandle;
            confirmParams.m_status = MacStatus::TRANSACTION_OVERFLOW;
            m_mcpsDataConfirmCallback(confirmParams);
        }
        NS_LOG_DEBUG("TX Queue with size " << m_txQueue.size() << " is full, dropping packet");
        m_macTxDropTrace(txQElement->txQPkt);
    }
}

void
LrWpanMac::RemoveFirstTxQElement()
{
    Ptr<TxQueueElement> txQElement = m_txQueue.front();
    Ptr<const Packet> p = txQElement->txQPkt;
    m_numCsmacaRetry += m_csmaCa->GetNB() + 1;

    Ptr<Packet> pkt = p->Copy();
    LrWpanMacHeader hdr;
    pkt->RemoveHeader(hdr);
    if (!hdr.GetShortDstAddr().IsBroadcast() && !hdr.GetShortDstAddr().IsMulticast())
    {
        m_sentPktTrace(p, m_retransmission + 1, m_numCsmacaRetry);
    }

    txQElement->txQPkt = nullptr;
    txQElement = nullptr;
    m_txQueue.pop_front();
    m_txPkt = nullptr;
    m_retransmission = 0;
    m_numCsmacaRetry = 0;
    m_macTxDequeueTrace(p);
}

void
LrWpanMac::AckWaitTimeout()
{
    NS_LOG_FUNCTION(this);

    // TODO: If we are a PAN coordinator and this was an indirect transmission,
    //       we will not initiate a retransmission. Instead we wait for the data
    //       being extracted after a new data request command.
    if (!PrepareRetransmission())
    {
        SetLrWpanMacState(MAC_IDLE);
    }
    else
    {
        SetLrWpanMacState(MAC_CSMA);
    }
}

void
LrWpanMac::IfsWaitTimeout(Time ifsTime)
{
    auto symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false);
    Time lifsTime = Seconds((double)m_macLIFSPeriod / symbolRate);
    Time sifsTime = Seconds((double)m_macSIFSPeriod / symbolRate);

    if (ifsTime == lifsTime)
    {
        NS_LOG_DEBUG("LIFS of " << m_macLIFSPeriod << " symbols (" << ifsTime.As(Time::S)
                                << ") completed ");
    }
    else if (ifsTime == sifsTime)
    {
        NS_LOG_DEBUG("SIFS of " << m_macSIFSPeriod << " symbols (" << ifsTime.As(Time::S)
                                << ") completed ");
    }
    else
    {
        NS_LOG_DEBUG("Unknown IFS size (" << ifsTime.As(Time::S) << ") completed ");
    }

    m_macIfsEndTrace(ifsTime);
    CheckQueue();
}

bool
LrWpanMac::PrepareRetransmission()
{
    NS_LOG_FUNCTION(this);

    // Max retransmissions reached without receiving ACK,
    // send the proper indication/confirmation
    // according to the frame type and call drop trace.
    if (m_retransmission >= m_macMaxFrameRetries)
    {
        LrWpanMacHeader peekedMacHdr;
        m_txPkt->PeekHeader(peekedMacHdr);

        if (peekedMacHdr.IsCommand())
        {
            m_macTxDropTrace(m_txPkt);

            Ptr<Packet> pkt = m_txPkt->Copy();
            LrWpanMacHeader macHdr;
            CommandPayloadHeader cmdPayload;
            pkt->RemoveHeader(macHdr);
            pkt->RemoveHeader(cmdPayload);

            switch (cmdPayload.GetCommandFrameType())
            {
            case CommandPayloadHeader::ASSOCIATION_REQ: {
                m_macPanId = 0xffff;
                m_macCoordShortAddress = Mac16Address("FF:FF");
                m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                m_incCapEvent.Cancel();
                m_incCfpEvent.Cancel();
                m_csmaCa->SetUnSlottedCsmaCa();
                m_incomingBeaconOrder = 15;
                m_incomingSuperframeOrder = 15;

                if (!m_mlmeAssociateConfirmCallback.IsNull())
                {
                    MlmeAssociateConfirmParams confirmParams;
                    confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
                    confirmParams.m_status = MacStatus::NO_ACK;
                    m_mlmeAssociateConfirmCallback(confirmParams);
                }
                break;
            }
            case CommandPayloadHeader::ASSOCIATION_RESP: {
                // IEEE 802.15.4-2006 (Section 7.1.3.3.3 and 7.1.8.2.3)
                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                {
                    MlmeCommStatusIndicationParams commStatusParams;
                    commStatusParams.m_panId = m_macPanId;
                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                    commStatusParams.m_status = MacStatus::NO_ACK;
                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                }
                RemovePendTxQElement(m_txPkt->Copy());
                break;
            }
            case CommandPayloadHeader::DATA_REQ: {
                // IEEE 802.15.4-2006 (Section 7.1.16.1.3)
                m_macPanId = 0xffff;
                m_macCoordShortAddress = Mac16Address("FF:FF");
                m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                m_incCapEvent.Cancel();
                m_incCfpEvent.Cancel();
                m_csmaCa->SetUnSlottedCsmaCa();
                m_incomingBeaconOrder = 15;
                m_incomingSuperframeOrder = 15;

                if (!m_mlmePollConfirmCallback.IsNull())
                {
                    MlmePollConfirmParams pollConfirmParams;
                    pollConfirmParams.m_status = MacStatus::NO_ACK;
                    m_mlmePollConfirmCallback(pollConfirmParams);
                }
                break;
            }
            default: {
                // TODO: Specify other indications according to other commands
                break;
            }
            }
        }
        else
        {
            // Maximum number of retransmissions has been reached.
            // remove the copy of the DATA packet that was just sent
            Ptr<TxQueueElement> txQElement = m_txQueue.front();
            m_macTxDropTrace(txQElement->txQPkt);
            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                McpsDataConfirmParams confirmParams;
                confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                confirmParams.m_status = MacStatus::NO_ACK;
                m_mcpsDataConfirmCallback(confirmParams);
            }
        }

        RemoveFirstTxQElement();
        return false;
    }
    else
    {
        m_retransmission++;
        m_numCsmacaRetry += m_csmaCa->GetNB() + 1;
        // Start next CCA process for this packet.
        return true;
    }
}

void
LrWpanMac::EnqueueInd(Ptr<Packet> p)
{
    Ptr<IndTxQueueElement> indTxQElement = Create<IndTxQueueElement>();
    LrWpanMacHeader peekedMacHdr;
    p->PeekHeader(peekedMacHdr);

    PurgeInd();

    NS_ASSERT(peekedMacHdr.GetDstAddrMode() == SHORT_ADDR ||
              peekedMacHdr.GetDstAddrMode() == EXT_ADDR);

    if (peekedMacHdr.GetDstAddrMode() == SHORT_ADDR)
    {
        indTxQElement->dstShortAddress = peekedMacHdr.GetShortDstAddr();
    }
    else
    {
        indTxQElement->dstExtAddress = peekedMacHdr.GetExtDstAddr();
    }

    indTxQElement->seqNum = peekedMacHdr.GetSeqNum();

    // See IEEE 802.15.4-2006, Table 86
    uint32_t unit = 0; // The persistence time in symbols
    if (m_macBeaconOrder == 15)
    {
        // Non-beacon enabled mode
        unit = lrwpan::aBaseSuperframeDuration * m_macTransactionPersistenceTime;
    }
    else
    {
        // Beacon-enabled mode
        unit = ((static_cast<uint32_t>(1) << m_macBeaconOrder) * lrwpan::aBaseSuperframeDuration) *
               m_macTransactionPersistenceTime;
    }

    if (m_indTxQueue.size() < m_maxIndTxQueueSize)
    {
        double symbolRate = m_phy->GetDataOrSymbolRate(false);
        Time expireTime = Seconds(unit / symbolRate);
        expireTime += Simulator::Now();
        indTxQElement->expireTime = expireTime;
        indTxQElement->txQPkt = p;
        m_indTxQueue.emplace_back(indTxQElement);
        m_macIndTxEnqueueTrace(p);
    }
    else
    {
        if (!m_mlmeCommStatusIndicationCallback.IsNull())
        {
            LrWpanMacHeader peekedMacHdr;
            indTxQElement->txQPkt->PeekHeader(peekedMacHdr);
            MlmeCommStatusIndicationParams commStatusParams;
            commStatusParams.m_panId = m_macPanId;
            commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
            commStatusParams.m_srcExtAddr = peekedMacHdr.GetExtSrcAddr();
            commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
            commStatusParams.m_dstExtAddr = peekedMacHdr.GetExtDstAddr();
            commStatusParams.m_status = MacStatus::TRANSACTION_OVERFLOW;
            m_mlmeCommStatusIndicationCallback(commStatusParams);
        }
        m_macIndTxDropTrace(p);
    }
}

bool
LrWpanMac::DequeueInd(Mac64Address dst, Ptr<IndTxQueueElement> entry)
{
    PurgeInd();

    for (auto iter = m_indTxQueue.begin(); iter != m_indTxQueue.end(); iter++)
    {
        if ((*iter)->dstExtAddress == dst)
        {
            *entry = **iter;
            m_macIndTxDequeueTrace((*iter)->txQPkt->Copy());
            m_indTxQueue.erase(iter);
            return true;
        }
    }
    return false;
}

void
LrWpanMac::PurgeInd()
{
    for (uint32_t i = 0; i < m_indTxQueue.size();)
    {
        if (Simulator::Now() > m_indTxQueue[i]->expireTime)
        {
            // Transaction expired, remove and send proper confirmation/indication to a higher layer
            LrWpanMacHeader peekedMacHdr;
            m_indTxQueue[i]->txQPkt->PeekHeader(peekedMacHdr);

            if (peekedMacHdr.IsCommand())
            {
                // IEEE 802.15.4-2006 (Section 7.1.3.3.3)
                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                {
                    MlmeCommStatusIndicationParams commStatusParams;
                    commStatusParams.m_panId = m_macPanId;
                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_srcExtAddr = peekedMacHdr.GetExtSrcAddr();
                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_dstExtAddr = peekedMacHdr.GetExtDstAddr();
                    commStatusParams.m_status = MacStatus::TRANSACTION_EXPIRED;
                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                }
            }
            else if (peekedMacHdr.IsData())
            {
                // IEEE 802.15.4-2006 (Section 7.1.1.1.3)
                if (!m_mcpsDataConfirmCallback.IsNull())
                {
                    McpsDataConfirmParams confParams;
                    confParams.m_status = MacStatus::TRANSACTION_EXPIRED;
                    m_mcpsDataConfirmCallback(confParams);
                }
            }
            m_macIndTxDropTrace(m_indTxQueue[i]->txQPkt->Copy());
            m_indTxQueue.erase(m_indTxQueue.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

void
LrWpanMac::PrintPendingTxQueue(std::ostream& os) const
{
    LrWpanMacHeader peekedMacHdr;

    os << "Pending Transaction List [" << GetShortAddress() << " | " << GetExtendedAddress()
       << "] | CurrentTime: " << Simulator::Now().As(Time::S) << "\n"
       << "    Destination    |"
       << "    Sequence Number |"
       << "    Frame type    |"
       << "    Expire time\n";

    for (auto transaction : m_indTxQueue)
    {
        transaction->txQPkt->PeekHeader(peekedMacHdr);
        os << transaction->dstExtAddress << "           "
           << static_cast<uint32_t>(transaction->seqNum) << "          ";

        if (peekedMacHdr.IsCommand())
        {
            os << " Command Frame   ";
        }
        else if (peekedMacHdr.IsData())
        {
            os << " Data Frame      ";
        }
        else
        {
            os << " Unknown Frame   ";
        }

        os << transaction->expireTime.As(Time::S) << "\n";
    }
}

void
LrWpanMac::PrintTxQueue(std::ostream& os) const
{
    LrWpanMacHeader peekedMacHdr;

    os << "\nTx Queue [" << GetShortAddress() << " | " << GetExtendedAddress()
       << "] | CurrentTime: " << Simulator::Now().As(Time::S) << "\n"
       << "    Destination    |"
       << "    Sequence Number    |"
       << "    Dst PAN id    |"
       << "    Frame type    |\n";

    for (auto transaction : m_txQueue)
    {
        transaction->txQPkt->PeekHeader(peekedMacHdr);

        os << "[" << peekedMacHdr.GetShortDstAddr() << "]"
           << ", [" << peekedMacHdr.GetExtDstAddr() << "]        "
           << static_cast<uint32_t>(peekedMacHdr.GetSeqNum()) << "               "
           << peekedMacHdr.GetDstPanId() << "          ";

        if (peekedMacHdr.IsCommand())
        {
            os << " Command Frame   ";
        }
        else if (peekedMacHdr.IsData())
        {
            os << " Data Frame      ";
        }
        else
        {
            os << " Unknown Frame   ";
        }

        os << "\n";
    }
    os << "\n";
}

void
LrWpanMac::RemovePendTxQElement(Ptr<Packet> p)
{
    LrWpanMacHeader peekedMacHdr;
    p->PeekHeader(peekedMacHdr);

    for (auto it = m_indTxQueue.begin(); it != m_indTxQueue.end(); it++)
    {
        if (peekedMacHdr.GetDstAddrMode() == EXT_ADDR)
        {
            if (((*it)->dstExtAddress == peekedMacHdr.GetExtDstAddr()) &&
                ((*it)->seqNum == peekedMacHdr.GetSeqNum()))
            {
                m_macIndTxDequeueTrace(p);
                m_indTxQueue.erase(it);
                break;
            }
        }
        else if (peekedMacHdr.GetDstAddrMode() == SHORT_ADDR)
        {
            if (((*it)->dstShortAddress == peekedMacHdr.GetShortDstAddr()) &&
                ((*it)->seqNum == peekedMacHdr.GetSeqNum()))
            {
                m_macIndTxDequeueTrace(p);
                m_indTxQueue.erase(it);
                break;
            }
        }
    }

    p = nullptr;
}

void
LrWpanMac::PdDataConfirm(PhyEnumeration status)
{
    NS_ASSERT(m_macState == MAC_SENDING);
    NS_LOG_FUNCTION(this << status << m_txQueue.size());

    LrWpanMacHeader macHdr;
    Time ifsWaitTime;
    double symbolRate;

    symbolRate = m_phy->GetDataOrSymbolRate(false); // symbols per second

    m_txPkt->PeekHeader(macHdr);

    if (status == IEEE_802_15_4_PHY_SUCCESS)
    {
        if (!macHdr.IsAcknowledgment())
        {
            if (macHdr.IsBeacon())
            {
                // Start CAP only if we are in beacon mode (i.e. if slotted csma-ca is running)
                if (m_csmaCa->IsSlottedCsmaCa())
                {
                    // The Tx Beacon in symbols
                    // Beacon = 5 bytes Sync Header (SHR) +  1 byte PHY header (PHR) + PSDU (default
                    // 17 bytes)
                    uint64_t beaconSymbols = m_phy->GetPhySHRDuration() +
                                             1 * m_phy->GetPhySymbolsPerOctet() +
                                             (m_txPkt->GetSize() * m_phy->GetPhySymbolsPerOctet());

                    // The beacon Tx time and start of the Outgoing superframe Active Period
                    m_macBeaconTxTime =
                        Simulator::Now() - Seconds(static_cast<double>(beaconSymbols) / symbolRate);

                    m_capEvent = Simulator::ScheduleNow(&LrWpanMac::StartCAP,
                                                        this,
                                                        SuperframeType::OUTGOING);
                    NS_LOG_DEBUG("Beacon Sent (m_macBeaconTxTime: " << m_macBeaconTxTime.As(Time::S)
                                                                    << ")");

                    if (!m_mlmeStartConfirmCallback.IsNull())
                    {
                        MlmeStartConfirmParams mlmeConfirmParams;
                        mlmeConfirmParams.m_status = MacStatus::SUCCESS;
                        m_mlmeStartConfirmCallback(mlmeConfirmParams);
                    }
                }

                ifsWaitTime = Seconds(static_cast<double>(GetIfsSize()) / symbolRate);
                m_txPkt = nullptr;
            }
            else if (macHdr.IsAckReq()) // We have sent a regular data packet, check if we have to
                                        // wait  for an ACK.
            {
                // we sent a regular data frame or command frame (e.g. AssocReq command) that
                // require ACK wait for the ack or the next retransmission timeout start
                // retransmission timer
                Time waitTime = Seconds(static_cast<double>(GetMacAckWaitDuration()) / symbolRate);
                NS_ASSERT(m_ackWaitTimeout.IsExpired());
                m_ackWaitTimeout = Simulator::Schedule(waitTime, &LrWpanMac::AckWaitTimeout, this);
                m_setMacState.Cancel();
                m_setMacState =
                    Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_ACK_PENDING);
                return;
            }
            else if (macHdr.IsCommand())
            {
                // We handle commands that do not require ACK
                // (e.g. Coordinator realigment command in an orphan response)
                // Other command with ACK required are handle by the previous if statement.

                // Check the transmitted packet command type and issue the appropriate indication.
                Ptr<Packet> txOriginalPkt = m_txPkt->Copy();
                LrWpanMacHeader txMacHdr;
                txOriginalPkt->RemoveHeader(txMacHdr);
                CommandPayloadHeader txMacPayload;
                txOriginalPkt->RemoveHeader(txMacPayload);

                if (txMacPayload.GetCommandFrameType() == CommandPayloadHeader::COOR_REALIGN)
                {
                    if (!m_mlmeCommStatusIndicationCallback.IsNull())
                    {
                        MlmeCommStatusIndicationParams commStatusParams;
                        commStatusParams.m_panId = m_macPanId;

                        commStatusParams.m_srcAddrMode = macHdr.GetSrcAddrMode();
                        commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                        commStatusParams.m_srcShortAddr = macHdr.GetShortSrcAddr();

                        commStatusParams.m_dstAddrMode = macHdr.GetDstAddrMode();
                        commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                        commStatusParams.m_dstShortAddr = macHdr.GetShortDstAddr();

                        commStatusParams.m_status = MacStatus::SUCCESS;
                        m_mlmeCommStatusIndicationCallback(commStatusParams);
                    }
                }

                ifsWaitTime = Seconds(static_cast<double>(GetIfsSize()) / symbolRate);
                RemoveFirstTxQElement();
            }
            else
            {
                m_macTxOkTrace(m_txPkt);
                // remove the copy of the packet that was just sent
                if (!m_mcpsDataConfirmCallback.IsNull())
                {
                    McpsDataConfirmParams confirmParams;
                    NS_ASSERT_MSG(!m_txQueue.empty(), "TxQsize = 0");
                    Ptr<TxQueueElement> txQElement = m_txQueue.front();
                    confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                    confirmParams.m_status = MacStatus::SUCCESS;
                    m_mcpsDataConfirmCallback(confirmParams);
                }
                ifsWaitTime = Seconds(static_cast<double>(GetIfsSize()) / symbolRate);
                RemoveFirstTxQElement();
            }
        }
        else
        {
            // The packet sent was a successful ACK

            // Check the received frame before the transmission of the ACK,
            // and send the appropriate Indication or Confirmation
            Ptr<Packet> recvOriginalPkt = m_rxPkt->Copy();
            LrWpanMacHeader receivedMacHdr;
            recvOriginalPkt->RemoveHeader(receivedMacHdr);

            if (receivedMacHdr.IsCommand())
            {
                CommandPayloadHeader receivedMacPayload;
                recvOriginalPkt->RemoveHeader(receivedMacPayload);

                if (receivedMacPayload.GetCommandFrameType() ==
                    CommandPayloadHeader::ASSOCIATION_REQ)
                {
                    if (!m_mlmeAssociateIndicationCallback.IsNull())
                    {
                        // NOTE: The LQI parameter is not part of the standard but found
                        // in some implementations as is required for higher layers (See Zboss
                        // implementation).
                        MlmeAssociateIndicationParams associateParams;
                        associateParams.capabilityInfo = receivedMacPayload.GetCapabilityField();
                        associateParams.m_extDevAddr = receivedMacHdr.GetExtSrcAddr();
                        associateParams.lqi = m_lastRxFrameLqi;
                        m_mlmeAssociateIndicationCallback(associateParams);
                    }

                    // Clear the packet buffer for the packet request received.
                    m_rxPkt = nullptr;
                }
                else if (receivedMacPayload.GetCommandFrameType() ==
                         CommandPayloadHeader::ASSOCIATION_RESP)
                {
                    MlmeAssociateConfirmParams confirmParams;

                    switch (static_cast<MacStatus>(receivedMacPayload.GetAssociationStatus()))
                    {
                    case MacStatus::SUCCESS:
                        // The assigned short address by the coordinator
                        SetShortAddress(receivedMacPayload.GetShortAddr());
                        m_macPanId = receivedMacHdr.GetSrcPanId();

                        confirmParams.m_status = MacStatus::SUCCESS;
                        confirmParams.m_assocShortAddr = GetShortAddress();
                        break;
                    case MacStatus::FULL_CAPACITY:
                        confirmParams.m_status = MacStatus::FULL_CAPACITY;
                        m_macPanId = 0xffff;
                        m_macCoordShortAddress = Mac16Address("FF:FF");
                        m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                        m_incCapEvent.Cancel();
                        m_incCfpEvent.Cancel();
                        m_csmaCa->SetUnSlottedCsmaCa();
                        m_incomingBeaconOrder = 15;
                        m_incomingSuperframeOrder = 15;
                        break;
                    case MacStatus::ACCESS_DENIED:
                    default:
                        confirmParams.m_status = MacStatus::ACCESS_DENIED;
                        m_macPanId = 0xffff;
                        m_macCoordShortAddress = Mac16Address("FF:FF");
                        m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                        m_incCapEvent.Cancel();
                        m_incCfpEvent.Cancel();
                        m_csmaCa->SetUnSlottedCsmaCa();
                        m_incomingBeaconOrder = 15;
                        m_incomingSuperframeOrder = 15;
                        break;
                    }

                    if (!m_mlmeAssociateConfirmCallback.IsNull())
                    {
                        m_mlmeAssociateConfirmCallback(confirmParams);
                    }
                }
                else if (receivedMacPayload.GetCommandFrameType() == CommandPayloadHeader::DATA_REQ)
                {
                    // We enqueue the the Assoc Response command frame in the Tx queue
                    // and the packet is transmitted as soon as the PHY is free and the IFS have
                    // taken place.
                    SendAssocResponseCommand(m_rxPkt->Copy());
                }
            }

            // Clear the packet buffer for the ACK packet sent.
            m_txPkt = nullptr;
        }
    }
    else if (status == IEEE_802_15_4_PHY_UNSPECIFIED)
    {
        if (!macHdr.IsAcknowledgment())
        {
            NS_ASSERT_MSG(!m_txQueue.empty(), "TxQsize = 0");
            Ptr<TxQueueElement> txQElement = m_txQueue.front();
            m_macTxDropTrace(txQElement->txQPkt);
            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                McpsDataConfirmParams confirmParams;
                confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                confirmParams.m_status = MacStatus::FRAME_TOO_LONG;
                m_mcpsDataConfirmCallback(confirmParams);
            }
            RemoveFirstTxQElement();
        }
        else
        {
            NS_LOG_ERROR("Unable to send ACK");
        }
    }
    else
    {
        // Something went really wrong. The PHY is not in the correct state for
        // data transmission.
        NS_FATAL_ERROR("Transmission attempt failed with PHY status " << status);
    }

    if (!ifsWaitTime.IsZero())
    {
        m_ifsEvent =
            Simulator::Schedule(ifsWaitTime, &LrWpanMac::IfsWaitTimeout, this, ifsWaitTime);
    }

    m_setMacState.Cancel();
    m_setMacState = Simulator::ScheduleNow(&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
}

void
LrWpanMac::PlmeCcaConfirm(PhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);
    // Direct this call through the csmaCa object
    m_csmaCa->PlmeCcaConfirm(status);
}

void
LrWpanMac::PlmeEdConfirm(PhyEnumeration status, uint8_t energyLevel)
{
    NS_LOG_FUNCTION(this << status << energyLevel);

    if (energyLevel > m_maxEnergyLevel)
    {
        m_maxEnergyLevel = energyLevel;
    }

    if (Simulator::GetDelayLeft(m_scanEnergyEvent) >
        Seconds(8.0 / m_phy->GetDataOrSymbolRate(false)))
    {
        m_phy->PlmeEdRequest();
    }
}

void
LrWpanMac::PlmeGetAttributeConfirm(PhyEnumeration status,
                                   PhyPibAttributeIdentifier id,
                                   Ptr<PhyPibAttributes> attribute)
{
    NS_LOG_FUNCTION(this << status << id << attribute);
}

void
LrWpanMac::PlmeSetTRXStateConfirm(PhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);

    if (m_macState == MAC_SENDING &&
        (status == IEEE_802_15_4_PHY_TX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        NS_ASSERT(m_txPkt);

        // Start sending if we are in state SENDING and the PHY transmitter was enabled.
        m_promiscSnifferTrace(m_txPkt);
        m_snifferTrace(m_txPkt);
        m_macTxTrace(m_txPkt);
        m_phy->PdDataRequest(m_txPkt->GetSize(), m_txPkt);
    }
    else if (m_macState == MAC_CSMA &&
             (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        // Start the CSMA algorithm as soon as the receiver is enabled.
        m_csmaCa->Start();
    }
    else if (m_macState == MAC_IDLE)
    {
        NS_ASSERT(status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS ||
                  status == IEEE_802_15_4_PHY_TRX_OFF);

        if (status == IEEE_802_15_4_PHY_RX_ON && m_scanEnergyEvent.IsPending())
        {
            // Kick start Energy Detection Scan
            m_phy->PlmeEdRequest();
        }
        else if (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS)
        {
            // Check if there is not messages to transmit when going idle
            CheckQueue();
        }
    }
    else if (m_macState == MAC_ACK_PENDING)
    {
        NS_ASSERT(status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS);
    }
    else
    {
        // TODO: What to do when we receive an error?
        // If we want to transmit a packet, but switching the transceiver on results
        // in an error, we have to recover somehow (and start sending again).
        NS_FATAL_ERROR("Error changing transceiver state");
    }
}

void
LrWpanMac::PlmeSetAttributeConfirm(PhyEnumeration status, PhyPibAttributeIdentifier id)
{
    NS_LOG_FUNCTION(this << status << id);
    if (id == PhyPibAttributeIdentifier::phyCurrentPage && m_pendPrimitive == MLME_SCAN_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            // get the first channel to scan from scan channel list
            bool channelFound = false;
            for (int i = m_channelScanIndex; i <= 26; i++)
            {
                if ((m_scanParams.m_scanChannels & (1 << m_channelScanIndex)) != 0)
                {
                    channelFound = true;
                    break;
                }
                m_channelScanIndex++;
            }

            if (channelFound)
            {
                Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
                pibAttr->phyCurrentChannel = m_channelScanIndex;
                m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel,
                                               pibAttr);
            }
        }
        else
        {
            if (!m_mlmeScanConfirmCallback.IsNull())
            {
                MlmeScanConfirmParams confirmParams;
                confirmParams.m_scanType = m_scanParams.m_scanType;
                confirmParams.m_chPage = m_scanParams.m_chPage;
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeScanConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR(this << "Channel Scan: Invalid channel page");
        }
    }
    else if (id == PhyPibAttributeIdentifier::phyCurrentChannel && m_pendPrimitive == MLME_SCAN_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            auto symbolRate = static_cast<uint64_t>(m_phy->GetDataOrSymbolRate(false));
            Time nextScanTime;

            if (m_scanParams.m_scanType == MLMESCAN_ORPHAN)
            {
                nextScanTime = Seconds(static_cast<double>(m_macResponseWaitTime) / symbolRate);
            }
            else
            {
                uint64_t scanDurationSym =
                    lrwpan::aBaseSuperframeDuration * (pow(2, m_scanParams.m_scanDuration) + 1);

                nextScanTime = Seconds(static_cast<double>(scanDurationSym) / symbolRate);
            }

            switch (m_scanParams.m_scanType)
            {
            case MLMESCAN_ED:
                m_maxEnergyLevel = 0;
                m_scanEnergyEvent =
                    Simulator::Schedule(nextScanTime, &LrWpanMac::EndChannelEnergyScan, this);
                // set phy to RX_ON and kick start  the first PLME-ED.request
                m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
                break;
            case MLMESCAN_ACTIVE:
                m_scanEvent = Simulator::Schedule(nextScanTime, &LrWpanMac::EndChannelScan, this);
                SendBeaconRequestCommand();
                break;
            case MLMESCAN_PASSIVE:
                m_scanEvent = Simulator::Schedule(nextScanTime, &LrWpanMac::EndChannelScan, this);
                // turn back the phy to RX_ON after setting Page/channel
                m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
                break;
            case MLMESCAN_ORPHAN:
                m_scanOrphanEvent =
                    Simulator::Schedule(nextScanTime, &LrWpanMac::EndChannelScan, this);
                SendOrphanNotificationCommand();
                break;

            default:
                MlmeScanConfirmParams confirmParams;
                confirmParams.m_scanType = m_scanParams.m_scanType;
                confirmParams.m_chPage = m_scanParams.m_chPage;
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                if (!m_mlmeScanConfirmCallback.IsNull())
                {
                    m_mlmeScanConfirmCallback(confirmParams);
                }
                NS_LOG_ERROR("Scan Type currently not supported");
                return;
            }
        }
        else
        {
            if (!m_mlmeScanConfirmCallback.IsNull())
            {
                MlmeScanConfirmParams confirmParams;
                confirmParams.m_scanType = m_scanParams.m_scanType;
                confirmParams.m_chPage = m_scanParams.m_chPage;
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeScanConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR("Channel " << m_channelScanIndex
                                    << " could not be set in the current page");
        }
    }
    else if (id == PhyPibAttributeIdentifier::phyCurrentPage && m_pendPrimitive == MLME_START_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
            pibAttr->phyCurrentChannel = m_startParams.m_logCh;
            m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel, pibAttr);
        }
        else
        {
            if (!m_mlmeStartConfirmCallback.IsNull())
            {
                MlmeStartConfirmParams confirmParams;
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeStartConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR("Invalid page parameter in MLME-start");
        }
    }
    else if (id == PhyPibAttributeIdentifier::phyCurrentChannel &&
             m_pendPrimitive == MLME_START_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            EndStartRequest();
        }
        else
        {
            if (!m_mlmeStartConfirmCallback.IsNull())
            {
                MlmeStartConfirmParams confirmParams;
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeStartConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR("Invalid channel parameter in MLME-start");
        }
    }
    else if (id == PhyPibAttributeIdentifier::phyCurrentPage && m_pendPrimitive == MLME_ASSOC_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            Ptr<PhyPibAttributes> pibAttr = Create<PhyPibAttributes>();
            pibAttr->phyCurrentChannel = m_associateParams.m_chNum;
            m_phy->PlmeSetAttributeRequest(PhyPibAttributeIdentifier::phyCurrentChannel, pibAttr);
        }
        else
        {
            m_macPanId = 0xffff;
            m_macCoordShortAddress = Mac16Address("FF:FF");
            m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
            m_incCapEvent.Cancel();
            m_incCfpEvent.Cancel();
            m_csmaCa->SetUnSlottedCsmaCa();
            m_incomingBeaconOrder = 15;
            m_incomingSuperframeOrder = 15;

            if (!m_mlmeAssociateConfirmCallback.IsNull())
            {
                MlmeAssociateConfirmParams confirmParams;
                confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeAssociateConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR("Invalid page parameter in MLME-associate");
        }
    }
    else if (id == PhyPibAttributeIdentifier::phyCurrentChannel &&
             m_pendPrimitive == MLME_ASSOC_REQ)
    {
        if (status == PhyEnumeration::IEEE_802_15_4_PHY_SUCCESS)
        {
            EndAssociateRequest();
        }
        else
        {
            m_macPanId = 0xffff;
            m_macCoordShortAddress = Mac16Address("FF:FF");
            m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
            m_incCapEvent.Cancel();
            m_incCfpEvent.Cancel();
            m_csmaCa->SetUnSlottedCsmaCa();
            m_incomingBeaconOrder = 15;
            m_incomingSuperframeOrder = 15;

            if (!m_mlmeAssociateConfirmCallback.IsNull())
            {
                MlmeAssociateConfirmParams confirmParams;
                confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
                confirmParams.m_status = MacStatus::INVALID_PARAMETER;
                m_mlmeAssociateConfirmCallback(confirmParams);
            }
            NS_LOG_ERROR("Invalid channel parameter in MLME-associate");
        }
    }
}

void
LrWpanMac::SetLrWpanMacState(MacState macState)
{
    NS_LOG_FUNCTION(this << "mac state = " << macState);

    if (macState == MAC_IDLE)
    {
        ChangeMacState(MAC_IDLE);
        if (m_macRxOnWhenIdle)
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
        }
        else
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TRX_OFF);
        }
    }
    else if (macState == MAC_ACK_PENDING)
    {
        ChangeMacState(MAC_ACK_PENDING);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
    else if (macState == MAC_CSMA)
    {
        NS_ASSERT(m_macState == MAC_IDLE || m_macState == MAC_ACK_PENDING);
        ChangeMacState(MAC_CSMA);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
    else if (m_macState == MAC_CSMA && macState == CHANNEL_IDLE)
    {
        // Channel is idle, set transmitter to TX_ON
        ChangeMacState(MAC_SENDING);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    }
    else if (m_macState == MAC_CSMA && macState == CHANNEL_ACCESS_FAILURE)
    {
        NS_ASSERT(m_txPkt);

        // Cannot find a clear channel, drop the current packet
        // and send the proper confirm/indication according to the packet type
        NS_LOG_DEBUG(this << " cannot find clear channel");

        m_macTxDropTrace(m_txPkt);

        Ptr<Packet> pkt = m_txPkt->Copy();
        LrWpanMacHeader macHdr;
        pkt->RemoveHeader(macHdr);

        if (macHdr.IsCommand())
        {
            CommandPayloadHeader cmdPayload;
            pkt->RemoveHeader(cmdPayload);

            switch (cmdPayload.GetCommandFrameType())
            {
            case CommandPayloadHeader::ASSOCIATION_REQ: {
                m_macPanId = 0xffff;
                m_macCoordShortAddress = Mac16Address("FF:FF");
                m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                m_incCapEvent.Cancel();
                m_incCfpEvent.Cancel();
                m_csmaCa->SetUnSlottedCsmaCa();
                m_incomingBeaconOrder = 15;
                m_incomingSuperframeOrder = 15;

                if (!m_mlmeAssociateConfirmCallback.IsNull())
                {
                    MlmeAssociateConfirmParams confirmParams;
                    confirmParams.m_assocShortAddr = Mac16Address("FF:FF");
                    confirmParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                    m_mlmeAssociateConfirmCallback(confirmParams);
                }
                break;
            }
            case CommandPayloadHeader::ASSOCIATION_RESP: {
                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                {
                    MlmeCommStatusIndicationParams commStatusParams;
                    commStatusParams.m_panId = m_macPanId;
                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                    commStatusParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                }
                RemovePendTxQElement(m_txPkt->Copy());
                break;
            }
            case CommandPayloadHeader::DATA_REQ: {
                m_macPanId = 0xffff;
                m_macCoordShortAddress = Mac16Address("FF:FF");
                m_macCoordExtendedAddress = Mac64Address("ff:ff:ff:ff:ff:ff:ff:ed");
                m_incCapEvent.Cancel();
                m_incCfpEvent.Cancel();
                m_csmaCa->SetUnSlottedCsmaCa();
                m_incomingBeaconOrder = 15;
                m_incomingSuperframeOrder = 15;

                if (!m_mlmePollConfirmCallback.IsNull())
                {
                    MlmePollConfirmParams pollConfirmParams;
                    pollConfirmParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                    m_mlmePollConfirmCallback(pollConfirmParams);
                }
                break;
            }
            case CommandPayloadHeader::COOR_REALIGN: {
                if (!m_mlmeCommStatusIndicationCallback.IsNull())
                {
                    MlmeCommStatusIndicationParams commStatusParams;
                    commStatusParams.m_panId = m_macPanId;
                    commStatusParams.m_srcAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_srcExtAddr = macHdr.GetExtSrcAddr();
                    commStatusParams.m_dstAddrMode = LrWpanMacHeader::EXTADDR;
                    commStatusParams.m_dstExtAddr = macHdr.GetExtDstAddr();
                    commStatusParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                    m_mlmeCommStatusIndicationCallback(commStatusParams);
                }
                break;
            }
            case CommandPayloadHeader::ORPHAN_NOTIF: {
                if (m_scanOrphanEvent.IsPending())
                {
                    m_unscannedChannels.emplace_back(m_phy->GetCurrentChannelNum());
                }
                // TODO: Handle orphan notification command during a
                //       channel access failure when not is not scanning.
                break;
            }
            case CommandPayloadHeader::BEACON_REQ: {
                if (m_scanEvent.IsPending())
                {
                    m_unscannedChannels.emplace_back(m_phy->GetCurrentChannelNum());
                }
                // TODO: Handle beacon request command during a
                //       channel access failure when not scanning.
                break;
            }
            default: {
                // TODO: Other commands(e.g. Disassociation notification, etc)
                break;
            }
            }
            RemoveFirstTxQElement();
        }
        else if (macHdr.IsData())
        {
            if (!m_mcpsDataConfirmCallback.IsNull())
            {
                McpsDataConfirmParams confirmParams;
                confirmParams.m_msduHandle = m_txQueue.front()->txQMsduHandle;
                confirmParams.m_status = MacStatus::CHANNEL_ACCESS_FAILURE;
                m_mcpsDataConfirmCallback(confirmParams);
            }
            // remove the copy of the packet that was just sent
            RemoveFirstTxQElement();
        }
        else
        {
            // TODO:: specify behavior for other packets
            m_txPkt = nullptr;
            m_retransmission = 0;
            m_numCsmacaRetry = 0;
        }

        ChangeMacState(MAC_IDLE);
        if (m_macRxOnWhenIdle)
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
        }
        else
        {
            m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TRX_OFF);
        }
    }
    else if (m_macState == MAC_CSMA && macState == MAC_CSMA_DEFERRED)
    {
        ChangeMacState(MAC_IDLE);
        m_txPkt = nullptr;
        // The MAC is running on beacon mode and the current packet could not be sent in the
        // current CAP. The packet will be send on the next CAP after receiving the beacon.
        // The PHY state does not change from its current form. The PHY change (RX_ON) will be
        // triggered by the scheduled beacon event.

        NS_LOG_DEBUG("****** PACKET DEFERRED to the next superframe *****");
    }
}

void
LrWpanMac::SetTxQMaxSize(uint32_t queueSize)
{
    m_maxTxQueueSize = queueSize;
}

void
LrWpanMac::SetIndTxQMaxSize(uint32_t queueSize)
{
    m_maxIndTxQueueSize = queueSize;
}

uint16_t
LrWpanMac::GetPanId() const
{
    return m_macPanId;
}

Mac16Address
LrWpanMac::GetCoordShortAddress() const
{
    return m_macCoordShortAddress;
}

Mac64Address
LrWpanMac::GetCoordExtAddress() const
{
    return m_macCoordExtendedAddress;
}

void
LrWpanMac::SetPanId(uint16_t panId)
{
    m_macPanId = panId;
}

void
LrWpanMac::ChangeMacState(MacState newState)
{
    NS_LOG_LOGIC(this << " change lrwpan mac state from " << m_macState << " to " << newState);
    m_macStateLogger(m_macState, newState);
    m_macState = newState;
}

uint64_t
LrWpanMac::GetMacAckWaitDuration() const
{
    return lrwpan::aUnitBackoffPeriod + lrwpan::aTurnaroundTime + m_phy->GetPhySHRDuration() +
           ceil(6 * m_phy->GetPhySymbolsPerOctet());
}

uint8_t
LrWpanMac::GetMacMaxFrameRetries() const
{
    return m_macMaxFrameRetries;
}

void
LrWpanMac::PrintTransmitQueueSize()
{
    NS_LOG_DEBUG("Transmit Queue Size: " << m_txQueue.size());
}

void
LrWpanMac::SetMacMaxFrameRetries(uint8_t retries)
{
    m_macMaxFrameRetries = retries;
}

bool
LrWpanMac::IsCoordDest()
{
    NS_ASSERT(m_txPkt);
    LrWpanMacHeader macHdr;
    m_txPkt->PeekHeader(macHdr);

    if (m_coor)
    {
        // The device is its coordinator and the packet is not to itself
        return false;
    }
    else if (m_macCoordShortAddress == macHdr.GetShortDstAddr() ||
             m_macCoordExtendedAddress == macHdr.GetExtDstAddr())
    {
        return true;
    }
    else
    {
        NS_LOG_DEBUG("ERROR: Packet not for the coordinator!");
        return false;
    }
}

uint32_t
LrWpanMac::GetIfsSize()
{
    NS_ASSERT(m_txPkt);

    if (m_txPkt->GetSize() <= lrwpan::aMaxSIFSFrameSize)
    {
        return m_macSIFSPeriod;
    }
    else
    {
        return m_macLIFSPeriod;
    }
}

void
LrWpanMac::SetAssociatedCoor(Mac16Address mac)
{
    m_macCoordShortAddress = mac;
}

void
LrWpanMac::SetAssociatedCoor(Mac64Address mac)
{
    m_macCoordExtendedAddress = mac;
}

uint64_t
LrWpanMac::GetTxPacketSymbols()
{
    NS_ASSERT(m_txPkt);
    // Sync Header (SHR) +  8 bits PHY header (PHR) + PSDU
    return (m_phy->GetPhySHRDuration() + 1 * m_phy->GetPhySymbolsPerOctet() +
            (m_txPkt->GetSize() * m_phy->GetPhySymbolsPerOctet()));
}

bool
LrWpanMac::IsTxAckReq()
{
    NS_ASSERT(m_txPkt);
    LrWpanMacHeader macHdr;
    m_txPkt->PeekHeader(macHdr);

    return macHdr.IsAckReq();
}

} // namespace lrwpan
} // namespace ns3
