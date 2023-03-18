/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "frame-exchange-manager.h"

#include "snr-tag.h"
#include "sta-wifi-mac.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-utils.h"

#include "ns3/abort.h"
#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

// Time (in nanoseconds) to be added to the PSDU duration to yield the duration
// of the timer that is started when the PHY indicates the start of the reception
// of a frame and we are waiting for a response.
#define PSDU_DURATION_SAFEGUARD 400

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(FrameExchangeManager);

TypeId
FrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FrameExchangeManager")
                            .SetParent<Object>()
                            .AddConstructor<FrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

FrameExchangeManager::FrameExchangeManager()
    : m_navEnd(Seconds(0)),
      m_linkId(0),
      m_allowedWidth(0),
      m_promisc(false),
      m_moreFragments(false)
{
    NS_LOG_FUNCTION(this);
}

FrameExchangeManager::~FrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
FrameExchangeManager::Reset()
{
    NS_LOG_FUNCTION(this);
    m_txTimer.Cancel();
    if (m_navResetEvent.IsRunning())
    {
        m_navResetEvent.Cancel();
    }
    m_navEnd = Simulator::Now();
    m_mpdu = nullptr;
    m_txParams.Clear();
    m_dcf = nullptr;
}

void
FrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Reset();
    m_fragmentedPacket = nullptr;
    m_mac = nullptr;
    m_txMiddle = nullptr;
    m_rxMiddle = nullptr;
    m_channelAccessManager = nullptr;
    m_protectionManager = nullptr;
    m_ackManager = nullptr;
    ResetPhy();
    Object::DoDispose();
}

void
FrameExchangeManager::SetProtectionManager(Ptr<WifiProtectionManager> protectionManager)
{
    NS_LOG_FUNCTION(this << protectionManager);
    m_protectionManager = protectionManager;
}

Ptr<WifiProtectionManager>
FrameExchangeManager::GetProtectionManager() const
{
    return m_protectionManager;
}

void
FrameExchangeManager::SetAckManager(Ptr<WifiAckManager> ackManager)
{
    NS_LOG_FUNCTION(this << ackManager);
    m_ackManager = ackManager;
}

Ptr<WifiAckManager>
FrameExchangeManager::GetAckManager() const
{
    return m_ackManager;
}

void
FrameExchangeManager::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
}

void
FrameExchangeManager::SetWifiMac(Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
}

void
FrameExchangeManager::SetMacTxMiddle(const Ptr<MacTxMiddle> txMiddle)
{
    NS_LOG_FUNCTION(this << txMiddle);
    m_txMiddle = txMiddle;
}

void
FrameExchangeManager::SetMacRxMiddle(const Ptr<MacRxMiddle> rxMiddle)
{
    NS_LOG_FUNCTION(this << rxMiddle);
    m_rxMiddle = rxMiddle;
}

void
FrameExchangeManager::SetChannelAccessManager(const Ptr<ChannelAccessManager> channelAccessManager)
{
    NS_LOG_FUNCTION(this << channelAccessManager);
    m_channelAccessManager = channelAccessManager;
}

Ptr<WifiRemoteStationManager>
FrameExchangeManager::GetWifiRemoteStationManager() const
{
    return m_mac->GetWifiRemoteStationManager(m_linkId);
}

void
FrameExchangeManager::SetWifiPhy(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    m_phy = phy;
    m_phy->TraceConnectWithoutContext("PhyRxPayloadBegin",
                                      MakeCallback(&FrameExchangeManager::RxStartIndication, this));
    m_phy->SetReceiveOkCallback(MakeCallback(&FrameExchangeManager::Receive, this));
}

void
FrameExchangeManager::ResetPhy()
{
    NS_LOG_FUNCTION(this);
    if (m_phy)
    {
        m_phy->TraceDisconnectWithoutContext(
            "PhyRxPayloadBegin",
            MakeCallback(&FrameExchangeManager::RxStartIndication, this));
        if (m_phy->GetState())
        {
            m_phy->SetReceiveOkCallback(MakeNullCallback<void,
                                                         Ptr<const WifiPsdu>,
                                                         RxSignalInfo,
                                                         WifiTxVector,
                                                         std::vector<bool>>());
        }
        m_phy = nullptr;
    }
}

void
FrameExchangeManager::SetAddress(Mac48Address address)
{
    NS_LOG_FUNCTION(this << address);
    // For APs, the BSSID is the MAC address. For STAs, the BSSID will be overwritten
    // when receiving Beacon frames or Probe Response frames
    SetBssid(address);
    m_self = address;
}

Mac48Address
FrameExchangeManager::GetAddress() const
{
    return m_self;
}

void
FrameExchangeManager::SetBssid(Mac48Address bssid)
{
    NS_LOG_FUNCTION(this << bssid);
    m_bssid = bssid;
}

Mac48Address
FrameExchangeManager::GetBssid() const
{
    return m_bssid;
}

void
FrameExchangeManager::SetDroppedMpduCallback(DroppedMpdu callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_droppedMpduCallback = callback;
}

void
FrameExchangeManager::SetAckedMpduCallback(AckedMpdu callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_ackedMpduCallback = callback;
}

void
FrameExchangeManager::SetPromisc()
{
    m_promisc = true;
}

bool
FrameExchangeManager::IsPromisc() const
{
    return m_promisc;
}

const WifiTxTimer&
FrameExchangeManager::GetWifiTxTimer() const
{
    return m_txTimer;
}

void
FrameExchangeManager::NotifyPacketDiscarded(Ptr<const WifiMpdu> mpdu)
{
    if (!m_droppedMpduCallback.IsNull())
    {
        m_droppedMpduCallback(WIFI_MAC_DROP_REACHED_RETRY_LIMIT, mpdu);
    }
}

void
FrameExchangeManager::RxStartIndication(WifiTxVector txVector, Time psduDuration)
{
    NS_LOG_FUNCTION(this << "PSDU reception started for " << psduDuration.As(Time::US)
                         << " (txVector: " << txVector << ")");

    NS_ASSERT_MSG(!m_txTimer.IsRunning() || !m_navResetEvent.IsRunning(),
                  "The TX timer and the NAV reset event cannot be both running");

    // No need to reschedule timeouts if PSDU duration is null. In this case,
    // PHY-RXEND immediately follows PHY-RXSTART (e.g. when PPDU has been filtered)
    // and CCA will take over
    if (m_txTimer.IsRunning() && psduDuration.IsStrictlyPositive())
    {
        // we are waiting for a response and something arrived
        NS_LOG_DEBUG("Rescheduling timeout event");
        m_txTimer.Reschedule(psduDuration + NanoSeconds(PSDU_DURATION_SAFEGUARD));
        // PHY has switched to RX, so we can reset the ack timeout
        m_channelAccessManager->NotifyAckTimeoutResetNow();
    }

    if (m_navResetEvent.IsRunning())
    {
        m_navResetEvent.Cancel();
    }
}

bool
FrameExchangeManager::StartTransmission(Ptr<Txop> dcf, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << dcf << allowedWidth);

    NS_ASSERT(!m_mpdu);
    if (m_txTimer.IsRunning())
    {
        m_txTimer.Cancel();
    }
    m_dcf = dcf;
    m_allowedWidth = allowedWidth;

    Ptr<WifiMacQueue> queue = dcf->GetWifiMacQueue();

    // Even though channel access is requested when the queue is not empty, at
    // the time channel access is granted the lifetime of the packet might be
    // expired and the queue might be empty.
    queue->WipeAllExpiredMpdus();

    Ptr<WifiMpdu> mpdu = queue->Peek(m_linkId);

    if (!mpdu)
    {
        NS_LOG_DEBUG("Queue empty");
        NotifyChannelReleased(m_dcf);
        m_dcf = nullptr;
        return false;
    }

    m_dcf->NotifyChannelAccessed(m_linkId);

    NS_ASSERT(mpdu->GetHeader().IsData() || mpdu->GetHeader().IsMgt());

    // assign a sequence number if this is not a fragment nor a retransmission
    if (!mpdu->IsFragment() && !mpdu->GetHeader().IsRetry())
    {
        uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor(&mpdu->GetHeader());
        mpdu->AssignSeqNo(sequence);
    }

    NS_LOG_DEBUG("MPDU payload size=" << mpdu->GetPacketSize()
                                      << ", to=" << mpdu->GetHeader().GetAddr1()
                                      << ", seq=" << mpdu->GetHeader().GetSequenceControl());

    // check if the MSDU needs to be fragmented
    mpdu = GetFirstFragmentIfNeeded(mpdu);

    NS_ASSERT(m_protectionManager);
    NS_ASSERT(m_ackManager);
    WifiTxParameters txParams;
    txParams.m_txVector =
        GetWifiRemoteStationManager()->GetDataTxVector(mpdu->GetHeader(), m_allowedWidth);
    txParams.m_protection = m_protectionManager->TryAddMpdu(mpdu, txParams);
    txParams.m_acknowledgment = m_ackManager->TryAddMpdu(mpdu, txParams);
    txParams.AddMpdu(mpdu);
    UpdateTxDuration(mpdu->GetHeader().GetAddr1(), txParams);

    SendMpduWithProtection(mpdu, txParams);

    return true;
}

Ptr<WifiMpdu>
FrameExchangeManager::GetFirstFragmentIfNeeded(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (mpdu->IsFragment())
    {
        // a fragment cannot be further fragmented
        NS_ASSERT(m_fragmentedPacket);
    }
    else if (GetWifiRemoteStationManager()->NeedFragmentation(mpdu))
    {
        NS_LOG_DEBUG("Fragmenting the MSDU");
        m_fragmentedPacket = mpdu->GetPacket()->Copy();
        // create the first fragment
        Ptr<Packet> fragment = m_fragmentedPacket->CreateFragment(
            0,
            GetWifiRemoteStationManager()->GetFragmentSize(mpdu, 0));
        // enqueue the first fragment
        Ptr<WifiMpdu> item = Create<WifiMpdu>(fragment, mpdu->GetHeader());
        item->GetHeader().SetMoreFragments();
        m_mac->GetTxopQueue(mpdu->GetQueueAc())->Replace(mpdu, item);
        return item;
    }
    return mpdu;
}

void
FrameExchangeManager::SendMpduWithProtection(Ptr<WifiMpdu> mpdu, WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << *mpdu << &txParams);

    m_mpdu = mpdu;
    m_txParams = std::move(txParams);

    // If protection is required, the MPDU must be stored in some queue because
    // it is not put back in a queue if the RTS/CTS exchange fails
    NS_ASSERT(m_txParams.m_protection->method == WifiProtection::NONE ||
              m_mpdu->GetHeader().IsCtl() || m_mpdu->IsQueued());

    // Make sure that the acknowledgment time has been computed, so that SendRts()
    // and SendCtsToSelf() can reuse this value.
    NS_ASSERT(m_txParams.m_acknowledgment);

    if (m_txParams.m_acknowledgment->acknowledgmentTime == Time::Min())
    {
        CalculateAcknowledgmentTime(m_txParams.m_acknowledgment.get());
    }

    // Set QoS Ack policy if this is a QoS data frame
    WifiAckManager::SetQosAckPolicy(m_mpdu, m_txParams.m_acknowledgment.get());

    if (m_mpdu->IsQueued())
    {
        m_mpdu->SetInFlight(m_linkId);
    }

    StartProtection(m_txParams);

    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        // we are done with frames that do not require acknowledgment
        m_mpdu = nullptr;
    }
}

void
FrameExchangeManager::StartProtection(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    switch (txParams.m_protection->method)
    {
    case WifiProtection::RTS_CTS:
        SendRts(txParams);
        break;
    case WifiProtection::CTS_TO_SELF:
        SendCtsToSelf(txParams);
        break;
    case WifiProtection::NONE:
        ProtectionCompleted();
        break;
    default:
        NS_ABORT_MSG("Unknown protection type: " << txParams.m_protection.get());
    }
}

void
FrameExchangeManager::ProtectionCompleted()
{
    NS_LOG_FUNCTION(this);
    m_protectedStas.merge(m_sentRtsTo);
    m_sentRtsTo.clear();
    NS_ASSERT(m_mpdu);
    SendMpdu();
}

const std::set<Mac48Address>&
FrameExchangeManager::GetProtectedStas() const
{
    return m_protectedStas;
}

void
FrameExchangeManager::SendMpdu()
{
    NS_LOG_FUNCTION(this);

    Time txDuration = m_phy->CalculateTxDuration(GetPsduSize(m_mpdu, m_txParams.m_txVector),
                                                 m_txParams.m_txVector,
                                                 m_phy->GetPhyBand());

    NS_ASSERT(m_txParams.m_acknowledgment);

    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        Simulator::Schedule(txDuration, &FrameExchangeManager::TransmissionSucceeded, this);

        if (!m_mpdu->GetHeader().IsQosData() ||
            m_mpdu->GetHeader().GetQosAckPolicy() == WifiMacHeader::NO_ACK)
        {
            // No acknowledgment, hence dequeue the MPDU if it is stored in a queue
            DequeueMpdu(m_mpdu);
        }
    }
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NORMAL_ACK)
    {
        m_mpdu->GetHeader().SetDuration(
            GetFrameDurationId(m_mpdu->GetHeader(),
                               GetPsduSize(m_mpdu, m_txParams.m_txVector),
                               m_txParams,
                               m_fragmentedPacket));

        // the timeout duration is "aSIFSTime + aSlotTime + aRxPHYStartDelay, starting
        // at the PHY-TXEND.confirm primitive" (section 10.3.2.9 or 10.22.2.2 of 802.11-2016).
        // aRxPHYStartDelay equals the time to transmit the PHY header.
        WifiNormalAck* normalAcknowledgment =
            static_cast<WifiNormalAck*>(m_txParams.m_acknowledgment.get());

        Time timeout =
            txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
            m_phy->CalculatePhyPreambleAndHeaderDuration(normalAcknowledgment->ackTxVector);
        NS_ASSERT(!m_txTimer.IsRunning());
        m_txTimer.Set(WifiTxTimer::WAIT_NORMAL_ACK,
                      timeout,
                      &FrameExchangeManager::NormalAckTimeout,
                      this,
                      m_mpdu,
                      m_txParams.m_txVector);
        m_channelAccessManager->NotifyAckTimeoutStartNow(timeout);
    }
    else
    {
        NS_ABORT_MSG("Unable to handle the selected acknowledgment method ("
                     << m_txParams.m_acknowledgment.get() << ")");
    }

    // transmit the MPDU
    ForwardMpduDown(m_mpdu, m_txParams.m_txVector);
}

void
FrameExchangeManager::ForwardMpduDown(Ptr<WifiMpdu> mpdu, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *mpdu << txVector);

    auto psdu = Create<WifiPsdu>(mpdu, false);
    FinalizeMacHeader(psdu);
    m_phy->Send(psdu, txVector);
}

void
FrameExchangeManager::FinalizeMacHeader(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << psdu);

    if (m_mac->GetTypeOfStation() != STA)
    {
        return;
    }

    auto pmMode = StaticCast<StaWifiMac>(m_mac)->GetPmMode(m_linkId);

    for (const auto& mpdu : *PeekPointer(psdu))
    {
        switch (pmMode)
        {
        case WIFI_PM_ACTIVE:
        case WIFI_PM_SWITCHING_TO_ACTIVE:
            mpdu->GetHeader().SetNoPowerManagement();
            break;
        case WIFI_PM_POWERSAVE:
        case WIFI_PM_SWITCHING_TO_PS:
            mpdu->GetHeader().SetPowerManagement();
            break;
        default:
            NS_ABORT_MSG("Unknown PM mode: " << +pmMode);
        }
    }
}

void
FrameExchangeManager::DequeueMpdu(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_DEBUG(this << *mpdu);

    if (mpdu->IsQueued())
    {
        m_mac->GetTxopQueue(mpdu->GetQueueAc())->DequeueIfQueued({mpdu});
    }
}

uint32_t
FrameExchangeManager::GetPsduSize(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector) const
{
    return mpdu->GetSize();
}

void
FrameExchangeManager::CalculateProtectionTime(WifiProtection* protection) const
{
    NS_LOG_FUNCTION(this << protection);
    NS_ASSERT(protection);

    if (protection->method == WifiProtection::NONE)
    {
        protection->protectionTime = Seconds(0);
    }
    else if (protection->method == WifiProtection::RTS_CTS)
    {
        WifiRtsCtsProtection* rtsCtsProtection = static_cast<WifiRtsCtsProtection*>(protection);
        rtsCtsProtection->protectionTime = m_phy->CalculateTxDuration(GetRtsSize(),
                                                                      rtsCtsProtection->rtsTxVector,
                                                                      m_phy->GetPhyBand()) +
                                           m_phy->CalculateTxDuration(GetCtsSize(),
                                                                      rtsCtsProtection->ctsTxVector,
                                                                      m_phy->GetPhyBand()) +
                                           2 * m_phy->GetSifs();
    }
    else if (protection->method == WifiProtection::CTS_TO_SELF)
    {
        WifiCtsToSelfProtection* ctsToSelfProtection =
            static_cast<WifiCtsToSelfProtection*>(protection);
        ctsToSelfProtection->protectionTime =
            m_phy->CalculateTxDuration(GetCtsSize(),
                                       ctsToSelfProtection->ctsTxVector,
                                       m_phy->GetPhyBand()) +
            m_phy->GetSifs();
    }
}

void
FrameExchangeManager::CalculateAcknowledgmentTime(WifiAcknowledgment* acknowledgment) const
{
    NS_LOG_FUNCTION(this << acknowledgment);
    NS_ASSERT(acknowledgment);

    if (acknowledgment->method == WifiAcknowledgment::NONE)
    {
        acknowledgment->acknowledgmentTime = Seconds(0);
    }
    else if (acknowledgment->method == WifiAcknowledgment::NORMAL_ACK)
    {
        WifiNormalAck* normalAcknowledgment = static_cast<WifiNormalAck*>(acknowledgment);
        normalAcknowledgment->acknowledgmentTime =
            m_phy->GetSifs() + m_phy->CalculateTxDuration(GetAckSize(),
                                                          normalAcknowledgment->ackTxVector,
                                                          m_phy->GetPhyBand());
    }
}

Time
FrameExchangeManager::GetTxDuration(uint32_t ppduPayloadSize,
                                    Mac48Address receiver,
                                    const WifiTxParameters& txParams) const
{
    return m_phy->CalculateTxDuration(ppduPayloadSize, txParams.m_txVector, m_phy->GetPhyBand());
}

void
FrameExchangeManager::UpdateTxDuration(Mac48Address receiver, WifiTxParameters& txParams) const
{
    txParams.m_txDuration = GetTxDuration(txParams.GetSize(receiver), receiver, txParams);
}

Time
FrameExchangeManager::GetFrameDurationId(const WifiMacHeader& header,
                                         uint32_t size,
                                         const WifiTxParameters& txParams,
                                         Ptr<Packet> fragmentedPacket) const
{
    NS_LOG_FUNCTION(this << header << size << &txParams << fragmentedPacket);

    NS_ASSERT(txParams.m_acknowledgment &&
              txParams.m_acknowledgment->acknowledgmentTime != Time::Min());
    Time durationId = txParams.m_acknowledgment->acknowledgmentTime;

    // if the current frame is a fragment followed by another fragment, we have to
    // update the Duration/ID to cover the next fragment and the corresponding Ack
    if (header.IsMoreFragments())
    {
        uint32_t payloadSize = size - header.GetSize() - WIFI_MAC_FCS_LENGTH;
        uint32_t nextFragmentOffset = (header.GetFragmentNumber() + 1) * payloadSize;
        uint32_t nextFragmentSize =
            std::min(fragmentedPacket->GetSize() - nextFragmentOffset, payloadSize);
        WifiTxVector ackTxVector =
            GetWifiRemoteStationManager()->GetAckTxVector(header.GetAddr1(), txParams.m_txVector);

        durationId +=
            2 * m_phy->GetSifs() +
            m_phy->CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand()) +
            m_phy->CalculateTxDuration(nextFragmentSize, txParams.m_txVector, m_phy->GetPhyBand());
    }
    return durationId;
}

Time
FrameExchangeManager::GetRtsDurationId(const WifiTxVector& rtsTxVector,
                                       Time txDuration,
                                       Time response) const
{
    NS_LOG_FUNCTION(this << rtsTxVector << txDuration << response);

    WifiTxVector ctsTxVector;
    ctsTxVector = GetWifiRemoteStationManager()->GetCtsTxVector(m_self, rtsTxVector.GetMode());

    return m_phy->GetSifs() +
           m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand()) /* CTS */
           + m_phy->GetSifs() + txDuration + response;
}

void
FrameExchangeManager::SendRts(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    NS_ASSERT(txParams.GetPsduInfoMap().size() == 1);
    Mac48Address receiver = txParams.GetPsduInfoMap().begin()->first;

    WifiMacHeader rts;
    rts.SetType(WIFI_MAC_CTL_RTS);
    rts.SetDsNotFrom();
    rts.SetDsNotTo();
    rts.SetNoRetry();
    rts.SetNoMoreFragments();
    rts.SetAddr1(receiver);
    rts.SetAddr2(m_self);

    NS_ASSERT(txParams.m_protection && txParams.m_protection->method == WifiProtection::RTS_CTS);
    WifiRtsCtsProtection* rtsCtsProtection =
        static_cast<WifiRtsCtsProtection*>(txParams.m_protection.get());

    NS_ASSERT(txParams.m_txDuration != Time::Min());
    rts.SetDuration(GetRtsDurationId(rtsCtsProtection->rtsTxVector,
                                     txParams.m_txDuration,
                                     txParams.m_acknowledgment->acknowledgmentTime));
    Ptr<WifiMpdu> mpdu = Create<WifiMpdu>(Create<Packet>(), rts);

    // After transmitting an RTS frame, the STA shall wait for a CTSTimeout interval with
    // a value of aSIFSTime + aSlotTime + aRxPHYStartDelay (IEEE 802.11-2016 sec. 10.3.2.7).
    // aRxPHYStartDelay equals the time to transmit the PHY header.
    Time timeout = m_phy->CalculateTxDuration(GetRtsSize(),
                                              rtsCtsProtection->rtsTxVector,
                                              m_phy->GetPhyBand()) +
                   m_phy->GetSifs() + m_phy->GetSlot() +
                   m_phy->CalculatePhyPreambleAndHeaderDuration(rtsCtsProtection->ctsTxVector);
    NS_ASSERT(!m_txTimer.IsRunning());
    m_txTimer.Set(WifiTxTimer::WAIT_CTS,
                  timeout,
                  &FrameExchangeManager::CtsTimeout,
                  this,
                  mpdu,
                  rtsCtsProtection->rtsTxVector);
    m_channelAccessManager->NotifyCtsTimeoutStartNow(timeout);
    NS_ASSERT(m_sentRtsTo.empty());
    m_sentRtsTo = {receiver};

    ForwardMpduDown(mpdu, rtsCtsProtection->rtsTxVector);
}

void
FrameExchangeManager::DoSendCtsAfterRts(const WifiMacHeader& rtsHdr,
                                        WifiTxVector& ctsTxVector,
                                        double rtsSnr)
{
    NS_LOG_FUNCTION(this << rtsHdr << ctsTxVector << rtsSnr);

    WifiMacHeader cts;
    cts.SetType(WIFI_MAC_CTL_CTS);
    cts.SetDsNotFrom();
    cts.SetDsNotTo();
    cts.SetNoMoreFragments();
    cts.SetNoRetry();
    cts.SetAddr1(rtsHdr.GetAddr2());
    Time duration = rtsHdr.GetDuration() - m_phy->GetSifs() -
                    m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand());
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8 of 802.11-2016)
    if (duration.IsStrictlyNegative())
    {
        duration = Seconds(0);
    }
    cts.SetDuration(duration);

    Ptr<Packet> packet = Create<Packet>();

    SnrTag tag;
    tag.Set(rtsSnr);
    packet->AddPacketTag(tag);

    // CTS should always use non-HT PPDU (HT PPDU cases not supported yet)
    ForwardMpduDown(Create<WifiMpdu>(packet, cts), ctsTxVector);
}

void
FrameExchangeManager::SendCtsAfterRts(const WifiMacHeader& rtsHdr,
                                      WifiMode rtsTxMode,
                                      double rtsSnr)
{
    NS_LOG_FUNCTION(this << rtsHdr << rtsTxMode << rtsSnr);

    WifiTxVector ctsTxVector =
        GetWifiRemoteStationManager()->GetCtsTxVector(rtsHdr.GetAddr2(), rtsTxMode);
    DoSendCtsAfterRts(rtsHdr, ctsTxVector, rtsSnr);
}

Time
FrameExchangeManager::GetCtsToSelfDurationId(const WifiTxVector& ctsTxVector,
                                             Time txDuration,
                                             Time response) const
{
    NS_LOG_FUNCTION(this << ctsTxVector << txDuration << response);

    return m_phy->GetSifs() + txDuration + response;
}

void
FrameExchangeManager::SendCtsToSelf(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    WifiMacHeader cts;
    cts.SetType(WIFI_MAC_CTL_CTS);
    cts.SetDsNotFrom();
    cts.SetDsNotTo();
    cts.SetNoMoreFragments();
    cts.SetNoRetry();
    cts.SetAddr1(m_self);

    NS_ASSERT(txParams.m_protection &&
              txParams.m_protection->method == WifiProtection::CTS_TO_SELF);
    WifiCtsToSelfProtection* ctsToSelfProtection =
        static_cast<WifiCtsToSelfProtection*>(txParams.m_protection.get());

    NS_ASSERT(txParams.m_txDuration != Time::Min());
    cts.SetDuration(GetCtsToSelfDurationId(ctsToSelfProtection->ctsTxVector,
                                           txParams.m_txDuration,
                                           txParams.m_acknowledgment->acknowledgmentTime));

    ForwardMpduDown(Create<WifiMpdu>(Create<Packet>(), cts), ctsToSelfProtection->ctsTxVector);

    Time ctsDuration = m_phy->CalculateTxDuration(GetCtsSize(),
                                                  ctsToSelfProtection->ctsTxVector,
                                                  m_phy->GetPhyBand());
    Simulator::Schedule(ctsDuration + m_phy->GetSifs(),
                        &FrameExchangeManager::ProtectionCompleted,
                        this);
}

void
FrameExchangeManager::SendNormalAck(const WifiMacHeader& hdr,
                                    const WifiTxVector& dataTxVector,
                                    double dataSnr)
{
    NS_LOG_FUNCTION(this << hdr << dataTxVector << dataSnr);

    WifiTxVector ackTxVector =
        GetWifiRemoteStationManager()->GetAckTxVector(hdr.GetAddr2(), dataTxVector);
    WifiMacHeader ack;
    ack.SetType(WIFI_MAC_CTL_ACK);
    ack.SetDsNotFrom();
    ack.SetDsNotTo();
    ack.SetNoRetry();
    ack.SetNoMoreFragments();
    ack.SetAddr1(hdr.GetAddr2());
    // 802.11-2016, Section 9.2.5.7: Duration/ID is received duration value
    // minus the time to transmit the Ack frame and its SIFS interval
    Time duration = hdr.GetDuration() - m_phy->GetSifs() -
                    m_phy->CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand());
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8 of 802.11-2016)
    if (duration.IsStrictlyNegative())
    {
        duration = Seconds(0);
    }
    ack.SetDuration(duration);

    Ptr<Packet> packet = Create<Packet>();

    SnrTag tag;
    tag.Set(dataSnr);
    packet->AddPacketTag(tag);

    ForwardMpduDown(Create<WifiMpdu>(packet, ack), ackTxVector);
}

Ptr<WifiMpdu>
FrameExchangeManager::GetNextFragment()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_mpdu->GetHeader().IsMoreFragments());

    WifiMacHeader& hdr = m_mpdu->GetHeader();
    hdr.SetFragmentNumber(hdr.GetFragmentNumber() + 1);

    uint32_t startOffset = hdr.GetFragmentNumber() * m_mpdu->GetPacketSize();
    uint32_t size = m_fragmentedPacket->GetSize() - startOffset;

    if (size > m_mpdu->GetPacketSize())
    {
        // this is not the last fragment
        size = m_mpdu->GetPacketSize();
        hdr.SetMoreFragments();
    }
    else
    {
        hdr.SetNoMoreFragments();
    }

    return Create<WifiMpdu>(m_fragmentedPacket->CreateFragment(startOffset, size), hdr);
}

void
FrameExchangeManager::TransmissionSucceeded()
{
    NS_LOG_FUNCTION(this);

    // Upon a transmission success, a non-QoS station transmits the next fragment,
    // if any, or releases the channel, otherwise
    if (m_moreFragments)
    {
        NS_LOG_DEBUG("Schedule transmission of next fragment in a SIFS");
        Simulator::Schedule(m_phy->GetSifs(),
                            &FrameExchangeManager::StartTransmission,
                            this,
                            m_dcf,
                            m_allowedWidth);
        m_moreFragments = false;
    }
    else
    {
        NotifyChannelReleased(m_dcf);
        m_dcf = nullptr;
    }
}

void
FrameExchangeManager::TransmissionFailed()
{
    NS_LOG_FUNCTION(this);
    // A non-QoS station always releases the channel upon a transmission failure
    NotifyChannelReleased(m_dcf);
    m_dcf = nullptr;
}

void
FrameExchangeManager::NotifyChannelReleased(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    txop->NotifyChannelReleased(m_linkId);
    m_protectedStas.clear();
}

void
FrameExchangeManager::NormalAckTimeout(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *mpdu << txVector);

    GetWifiRemoteStationManager()->ReportDataFailed(mpdu);

    if (!GetWifiRemoteStationManager()->NeedRetransmission(mpdu))
    {
        NS_LOG_DEBUG("Missed Ack, discard MPDU");
        NotifyPacketDiscarded(mpdu);
        // Dequeue the MPDU if it is stored in a queue
        DequeueMpdu(mpdu);
        GetWifiRemoteStationManager()->ReportFinalDataFailed(mpdu);
        m_dcf->ResetCw(m_linkId);
    }
    else
    {
        NS_LOG_DEBUG("Missed Ack, retransmit MPDU");
        if (mpdu->IsQueued()) // the MPDU may have been removed due to lifetime expiration
        {
            mpdu = m_mac->GetTxopQueue(mpdu->GetQueueAc())->GetOriginal(mpdu);
            mpdu->ResetInFlight(m_linkId);
        }
        mpdu->GetHeader().SetRetry();
        RetransmitMpduAfterMissedAck(mpdu);
        m_dcf->UpdateFailedCw(m_linkId);
    }

    m_mpdu = nullptr;
    TransmissionFailed();
}

void
FrameExchangeManager::RetransmitMpduAfterMissedAck(Ptr<WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);
}

void
FrameExchangeManager::CtsTimeout(Ptr<WifiMpdu> rts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *rts << txVector);

    DoCtsTimeout(Create<WifiPsdu>(m_mpdu, true));
    m_mpdu = nullptr;
}

void
FrameExchangeManager::DoCtsTimeout(Ptr<WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);

    m_sentRtsTo.clear();
    for (const auto& mpdu : *PeekPointer(psdu))
    {
        if (mpdu->IsQueued())
        {
            mpdu->ResetInFlight(m_linkId);
        }
    }

    GetWifiRemoteStationManager()->ReportRtsFailed(psdu->GetHeader(0));

    if (!GetWifiRemoteStationManager()->NeedRetransmission(*psdu->begin()))
    {
        NS_LOG_DEBUG("Missed CTS, discard MPDU(s)");
        GetWifiRemoteStationManager()->ReportFinalRtsFailed(psdu->GetHeader(0));
        for (const auto& mpdu : *PeekPointer(psdu))
        {
            // Dequeue the MPDU if it is stored in a queue
            DequeueMpdu(mpdu);
            NotifyPacketDiscarded(mpdu);
        }
        m_dcf->ResetCw(m_linkId);
    }
    else
    {
        NS_LOG_DEBUG("Missed CTS, retransmit MPDU(s)");
        m_dcf->UpdateFailedCw(m_linkId);
    }
    // Make the sequence numbers of the MPDUs available again if the MPDUs have never
    // been transmitted, both in case the MPDUs have been discarded and in case the
    // MPDUs have to be transmitted (because a new sequence number is assigned to
    // MPDUs that have never been transmitted and are selected for transmission)
    ReleaseSequenceNumbers(psdu);

    TransmissionFailed();
}

void
FrameExchangeManager::ReleaseSequenceNumbers(Ptr<const WifiPsdu> psdu) const
{
    NS_LOG_FUNCTION(this << *psdu);

    NS_ASSERT_MSG(psdu->GetNMpdus() == 1, "A-MPDUs should be handled by the HT FEM override");
    auto mpdu = *psdu->begin();

    // the MPDU should be still in the DCF queue, unless it expired.
    // If the MPDU has never been transmitted and is not in-flight, it will be assigned
    // a sequence number again the next time we try to transmit it. Therefore, we need to
    // make its sequence number available again
    if (!mpdu->GetHeader().IsRetry() && !mpdu->IsInFlight())
    {
        mpdu->UnassignSeqNo();
        m_txMiddle->SetSequenceNumberFor(&mpdu->GetOriginal()->GetHeader());
    }
}

void
FrameExchangeManager::NotifyInternalCollision(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this);

    // For internal collisions occurring with the EDCA access method, the appropriate
    // retry counters (short retry counter for MSDU, A-MSDU, or MMPDU and QSRC[AC] or
    // long retry counter for MSDU, A-MSDU, or MMPDU and QLRC[AC]) are incremented
    // (Sec. 10.22.2.11.1 of 802.11-2016).
    // We do not prepare the PSDU that the AC losing the internal collision would have
    // sent. As an approximation, we consider the frame peeked from the queues of the AC.
    Ptr<QosTxop> qosTxop = (txop->IsQosTxop() ? StaticCast<QosTxop>(txop) : nullptr);

    auto mpdu =
        (qosTxop ? qosTxop->PeekNextMpdu(m_linkId) : txop->GetWifiMacQueue()->Peek(m_linkId));

    if (mpdu)
    {
        if (mpdu->GetHeader().HasData() && !mpdu->GetHeader().GetAddr1().IsGroup())
        {
            GetWifiRemoteStationManager()->ReportDataFailed(mpdu);
        }

        if (!mpdu->GetHeader().GetAddr1().IsGroup() &&
            !GetWifiRemoteStationManager()->NeedRetransmission(mpdu))
        {
            NS_LOG_DEBUG("reset DCF");
            GetWifiRemoteStationManager()->ReportFinalDataFailed(mpdu);
            DequeueMpdu(mpdu);
            NotifyPacketDiscarded(mpdu);
            txop->ResetCw(m_linkId);
        }
        else
        {
            NS_LOG_DEBUG("Update CW");
            txop->UpdateFailedCw(m_linkId);
        }
    }

    txop->Txop::NotifyChannelReleased(m_linkId);
}

void
FrameExchangeManager::NotifySwitchingStartNow(Time duration)
{
    NS_LOG_DEBUG("Switching channel. Cancelling MAC pending events");
    Simulator::Schedule(duration, &WifiMac::NotifyChannelSwitching, m_mac, m_linkId);
    if (m_txTimer.IsRunning())
    {
        // we were transmitting something before channel switching. Since we will
        // not be able to receive the response, have the timer expire now, so that
        // we perform the actions required in case of missing response
        m_txTimer.Reschedule(Seconds(0));
    }
    Simulator::ScheduleNow(&FrameExchangeManager::Reset, this);
}

void
FrameExchangeManager::NotifySleepNow()
{
    NS_LOG_DEBUG("Device in sleep mode. Cancelling MAC pending events");
    Reset();
}

void
FrameExchangeManager::NotifyOffNow()
{
    NS_LOG_DEBUG("Device is switched off. Cancelling MAC pending events");
    Reset();
}

void
FrameExchangeManager::Receive(Ptr<const WifiPsdu> psdu,
                              RxSignalInfo rxSignalInfo,
                              WifiTxVector txVector,
                              std::vector<bool> perMpduStatus)
{
    NS_LOG_FUNCTION(
        this << psdu << rxSignalInfo << txVector << perMpduStatus.size()
             << std::all_of(perMpduStatus.begin(), perMpduStatus.end(), [](bool v) { return v; }));

    if (!perMpduStatus.empty())
    {
        // for A-MPDUs, we get here only once
        PreProcessFrame(psdu, txVector);
    }

    Mac48Address addr1 = psdu->GetAddr1();

    if (addr1.IsGroup() || addr1 == m_self)
    {
        // receive broadcast frames or frames addressed to us only
        if (psdu->GetNMpdus() == 1)
        {
            // if perMpduStatus is not empty (i.e., this MPDU is not included in an A-MPDU)
            // then it must contain a single value which must be true (i.e., the MPDU
            // has been correctly received)
            NS_ASSERT(perMpduStatus.empty() || (perMpduStatus.size() == 1 && perMpduStatus[0]));
            // Ack and CTS do not carry Addr2
            if (!psdu->GetHeader(0).IsAck() && !psdu->GetHeader(0).IsCts())
            {
                GetWifiRemoteStationManager()->ReportRxOk(psdu->GetHeader(0).GetAddr2(),
                                                          rxSignalInfo,
                                                          txVector);
            }
            ReceiveMpdu(*(psdu->begin()), rxSignalInfo, txVector, perMpduStatus.empty());
        }
        else
        {
            EndReceiveAmpdu(psdu, rxSignalInfo, txVector, perMpduStatus);
        }
    }
    else if (m_promisc)
    {
        for (const auto& mpdu : *PeekPointer(psdu))
        {
            if (!mpdu->GetHeader().IsCtl())
            {
                m_rxMiddle->Receive(mpdu, m_linkId);
            }
        }
    }

    if (!perMpduStatus.empty())
    {
        // for A-MPDUs, we get here only once
        PostProcessFrame(psdu, txVector);
    }
}

void
FrameExchangeManager::PreProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);
}

void
FrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    UpdateNav(psdu, txVector);
}

void
FrameExchangeManager::UpdateNav(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    if (!psdu->HasNav())
    {
        return;
    }

    Time duration = psdu->GetDuration();
    NS_LOG_DEBUG("Duration/ID=" << duration);

    if (psdu->GetAddr1() == m_self)
    {
        // When the received frame’s RA is equal to the STA’s own MAC address, the STA
        // shall not update its NAV (IEEE 802.11-2016, sec. 10.3.2.4)
        return;
    }

    // For all other received frames the STA shall update its NAV when the received
    // Duration is greater than the STA’s current NAV value (IEEE 802.11-2016 sec. 10.3.2.4)
    Time navEnd = Simulator::Now() + duration;
    if (navEnd > m_navEnd)
    {
        m_navEnd = navEnd;
        NS_LOG_DEBUG("Updated NAV=" << m_navEnd);

        // A STA that used information from an RTS frame as the most recent basis to update
        // its NAV setting is permitted to reset its NAV if no PHY-RXSTART.indication
        // primitive is received from the PHY during a NAVTimeout period starting when the
        // MAC receives a PHY-RXEND.indication primitive corresponding to the detection of
        // the RTS frame. NAVTimeout period is equal to:
        // (2 x aSIFSTime) + (CTS_Time) + aRxPHYStartDelay + (2 x aSlotTime)
        // The “CTS_Time” shall be calculated using the length of the CTS frame and the data
        // rate at which the RTS frame used for the most recent NAV update was received
        // (IEEE 802.11-2016 sec. 10.3.2.4)
        if (psdu->GetHeader(0).IsRts())
        {
            WifiTxVector ctsTxVector =
                GetWifiRemoteStationManager()->GetCtsTxVector(psdu->GetAddr2(), txVector.GetMode());
            Time navResetDelay =
                2 * m_phy->GetSifs() +
                WifiPhy::CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand()) +
                m_phy->CalculatePhyPreambleAndHeaderDuration(ctsTxVector) + 2 * m_phy->GetSlot();
            m_navResetEvent =
                Simulator::Schedule(navResetDelay, &FrameExchangeManager::NavResetTimeout, this);
        }
    }
    NS_LOG_DEBUG("Current NAV=" << m_navEnd);

    m_channelAccessManager->NotifyNavStartNow(duration);
}

void
FrameExchangeManager::NavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    m_navEnd = Simulator::Now();
    m_channelAccessManager->NotifyNavResetNow(Seconds(0));
}

bool
FrameExchangeManager::VirtualCsMediumIdle() const
{
    return m_navEnd <= Simulator::Now();
}

void
FrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                  RxSignalInfo rxSignalInfo,
                                  const WifiTxVector& txVector,
                                  bool inAmpdu)
{
    NS_LOG_FUNCTION(this << *mpdu << rxSignalInfo << txVector << inAmpdu);
    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    double rxSnr = rxSignalInfo.snr;
    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsCtl())
    {
        if (hdr.IsRts())
        {
            NS_ABORT_MSG_IF(inAmpdu, "Received RTS as part of an A-MPDU");

            // A non-VHT STA that is addressed by an RTS frame behaves as follows:
            // - If the NAV indicates idle, the STA shall respond with a CTS frame after a SIFS
            // - Otherwise, the STA shall not respond with a CTS frame
            // (IEEE 802.11-2016 sec. 10.3.2.7)
            if (VirtualCsMediumIdle())
            {
                NS_LOG_DEBUG("Received RTS from=" << hdr.GetAddr2() << ", schedule CTS");
                Simulator::Schedule(m_phy->GetSifs(),
                                    &FrameExchangeManager::SendCtsAfterRts,
                                    this,
                                    hdr,
                                    txVector.GetMode(),
                                    rxSnr);
            }
            else
            {
                NS_LOG_DEBUG("Received RTS from=" << hdr.GetAddr2() << ", cannot schedule CTS");
            }
        }
        else if (hdr.IsCts() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_CTS && m_mpdu)
        {
            NS_ABORT_MSG_IF(inAmpdu, "Received CTS as part of an A-MPDU");
            NS_ASSERT(hdr.GetAddr1() == m_self);

            Mac48Address sender = m_mpdu->GetHeader().GetAddr1();
            NS_LOG_DEBUG("Received CTS from=" << sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);
            GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);
            GetWifiRemoteStationManager()->ReportRtsOk(m_mpdu->GetHeader(),
                                                       rxSnr,
                                                       txVector.GetMode(),
                                                       tag.Get());

            m_txTimer.Cancel();
            m_channelAccessManager->NotifyCtsTimeoutResetNow();
            Simulator::Schedule(m_phy->GetSifs(), &FrameExchangeManager::ProtectionCompleted, this);
        }
        else if (hdr.IsAck() && m_mpdu && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_NORMAL_ACK)
        {
            NS_ASSERT(hdr.GetAddr1() == m_self);
            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);
            ReceivedNormalAck(m_mpdu, m_txParams.m_txVector, txVector, rxSignalInfo, tag.Get());
            m_mpdu = nullptr;
        }
    }
    else if (hdr.IsMgt())
    {
        NS_ABORT_MSG_IF(inAmpdu, "Received management frame as part of an A-MPDU");

        if (hdr.IsBeacon() || hdr.IsProbeResp())
        {
            // Apply SNR tag for beacon quality measurements
            SnrTag tag;
            tag.Set(rxSnr);
            Ptr<Packet> packet = mpdu->GetPacket()->Copy();
            packet->AddPacketTag(tag);
            mpdu = Create<WifiMpdu>(packet, hdr);
        }

        if (hdr.GetAddr1() == m_self)
        {
            NS_LOG_DEBUG("Received " << hdr.GetTypeString() << " from=" << hdr.GetAddr2()
                                     << ", schedule ACK");
            Simulator::Schedule(m_phy->GetSifs(),
                                &FrameExchangeManager::SendNormalAck,
                                this,
                                hdr,
                                txVector,
                                rxSnr);
        }

        m_rxMiddle->Receive(mpdu, m_linkId);
    }
    else if (hdr.IsData() && !hdr.IsQosData())
    {
        if (hdr.GetAddr1() == m_self)
        {
            NS_LOG_DEBUG("Received " << hdr.GetTypeString() << " from=" << hdr.GetAddr2()
                                     << ", schedule ACK");
            Simulator::Schedule(m_phy->GetSifs(),
                                &FrameExchangeManager::SendNormalAck,
                                this,
                                hdr,
                                txVector,
                                rxSnr);
        }

        m_rxMiddle->Receive(mpdu, m_linkId);
    }
}

void
FrameExchangeManager::ReceivedNormalAck(Ptr<WifiMpdu> mpdu,
                                        const WifiTxVector& txVector,
                                        const WifiTxVector& ackTxVector,
                                        const RxSignalInfo& rxInfo,
                                        double snr)
{
    Mac48Address sender = mpdu->GetHeader().GetAddr1();
    NS_LOG_DEBUG("Received ACK from=" << sender);

    NotifyReceivedNormalAck(mpdu);

    // When fragmentation is used, only update manager when the last fragment is acknowledged
    if (!mpdu->GetHeader().IsMoreFragments())
    {
        GetWifiRemoteStationManager()->ReportRxOk(sender, rxInfo, ackTxVector);
        GetWifiRemoteStationManager()->ReportDataOk(mpdu,
                                                    rxInfo.snr,
                                                    ackTxVector.GetMode(),
                                                    snr,
                                                    txVector);
    }
    // cancel the timer
    m_txTimer.Cancel();
    m_channelAccessManager->NotifyAckTimeoutResetNow();

    // The CW shall be reset to aCWmin after every successful attempt to transmit
    // a frame containing all or part of an MSDU or MMPDU (sec. 10.3.3 of 802.11-2016)
    m_dcf->ResetCw(m_linkId);

    if (mpdu->GetHeader().IsMoreFragments())
    {
        // replace the current fragment with the next one
        m_dcf->GetWifiMacQueue()->Replace(mpdu, GetNextFragment());
        m_moreFragments = true;
    }
    else
    {
        // the MPDU has been acknowledged, we can now dequeue it if it is stored in a queue
        DequeueMpdu(mpdu);
    }

    TransmissionSucceeded();
}

void
FrameExchangeManager::NotifyReceivedNormalAck(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    // inform the MAC that the transmission was successful
    if (!m_ackedMpduCallback.IsNull())
    {
        m_ackedMpduCallback(mpdu);
    }
}

void
FrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                      const RxSignalInfo& rxSignalInfo,
                                      const WifiTxVector& txVector,
                                      const std::vector<bool>& perMpduStatus)
{
    NS_ASSERT_MSG(false, "A non-QoS station should not receive an A-MPDU");
}

} // namespace ns3
