/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "qos-frame-exchange-manager.h"

#include "ap-wifi-mac.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"

#include "ns3/abort.h"
#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QosFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(QosFrameExchangeManager);

TypeId
QosFrameExchangeManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::QosFrameExchangeManager")
            .SetParent<FrameExchangeManager>()
            .AddConstructor<QosFrameExchangeManager>()
            .SetGroupName("Wifi")
            .AddAttribute("PifsRecovery",
                          "Perform a PIFS recovery as a response to transmission failure "
                          "within a TXOP",
                          BooleanValue(true),
                          MakeBooleanAccessor(&QosFrameExchangeManager::m_pifsRecovery),
                          MakeBooleanChecker())
            .AddAttribute("SetQueueSize",
                          "Whether to set the Queue Size subfield of the QoS Control field "
                          "of QoS data frames sent by non-AP stations",
                          BooleanValue(false),
                          MakeBooleanAccessor(&QosFrameExchangeManager::m_setQosQueueSize),
                          MakeBooleanChecker());
    return tid;
}

QosFrameExchangeManager::QosFrameExchangeManager()
    : m_initialFrame(false)
{
    NS_LOG_FUNCTION(this);
}

QosFrameExchangeManager::~QosFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
QosFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_edca = nullptr;
    m_edcaBackingOff = nullptr;
    m_pifsRecoveryEvent.Cancel();
    FrameExchangeManager::DoDispose();
}

bool
QosFrameExchangeManager::SendCfEndIfNeeded()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_edca);

    WifiMacHeader cfEnd;
    cfEnd.SetType(WIFI_MAC_CTL_END);
    cfEnd.SetDsNotFrom();
    cfEnd.SetDsNotTo();
    cfEnd.SetNoRetry();
    cfEnd.SetNoMoreFragments();
    cfEnd.SetDuration(Seconds(0));
    cfEnd.SetAddr1(Mac48Address::GetBroadcast());
    cfEnd.SetAddr2(m_self);

    WifiTxVector cfEndTxVector =
        GetWifiRemoteStationManager()->GetRtsTxVector(cfEnd.GetAddr1(), m_allowedWidth);

    auto mpdu = Create<WifiMpdu>(Create<Packet>(), cfEnd);
    auto txDuration =
        WifiPhy::CalculateTxDuration(mpdu->GetSize(), cfEndTxVector, m_phy->GetPhyBand());

    // Send the CF-End frame if the remaining TXNAV is long enough to transmit this frame
    if (m_txNav > Simulator::Now() + txDuration)
    {
        NS_LOG_DEBUG("Send CF-End frame");
        ForwardMpduDown(mpdu, cfEndTxVector);
        Simulator::Schedule(txDuration,
                            &QosFrameExchangeManager::NotifyChannelReleased,
                            this,
                            m_edca);
        return true;
    }

    NotifyChannelReleased(m_edca);
    m_edca = nullptr;
    return false;
}

void
QosFrameExchangeManager::PifsRecovery(bool forceCurrentCw)
{
    NS_LOG_FUNCTION(this << forceCurrentCw);
    NS_ASSERT(m_edca);
    NS_ASSERT(m_edca->GetTxopStartTime(m_linkId).has_value());

    // Release the channel if it has not been idle for the last PIFS interval
    m_allowedWidth = std::min(
        m_allowedWidth,
        m_channelAccessManager->GetLargestIdlePrimaryChannel(m_phy->GetPifs(), Simulator::Now()));

    if (m_allowedWidth == MHz_u{0})
    {
        // PIFS recovery failed, TXOP is terminated
        NotifyChannelReleased(m_edca);
        if (!forceCurrentCw)
        {
            m_edca->UpdateFailedCw(m_linkId);
        }
        m_edca = nullptr;
    }
    else
    {
        // the txopDuration parameter is unused because we are not starting a new TXOP
        StartTransmission(m_edca, Seconds(0));
    }
}

void
QosFrameExchangeManager::CancelPifsRecovery()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_pifsRecoveryEvent.IsPending());
    NS_ASSERT(m_edca);

    NS_LOG_DEBUG("Cancel PIFS recovery being attempted by EDCAF " << m_edca);
    m_pifsRecoveryEvent.Cancel();
    NotifyChannelReleased(m_edca);
}

bool
QosFrameExchangeManager::StartTransmission(Ptr<Txop> edca, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << edca << allowedWidth);

    if (m_pifsRecoveryEvent.IsPending())
    {
        // Another AC (having AIFS=1 or lower, if the user changed the default settings)
        // gained channel access while performing PIFS recovery. Abort PIFS recovery
        CancelPifsRecovery();
    }

    // TODO This will become an assert once no Txop is installed on a QoS station
    if (!edca->IsQosTxop())
    {
        m_edca = nullptr;
        return FrameExchangeManager::StartTransmission(edca, allowedWidth);
    }

    m_allowedWidth = allowedWidth;
    auto qosTxop = StaticCast<QosTxop>(edca);
    return StartTransmission(qosTxop, qosTxop->GetTxopLimit(m_linkId));
}

bool
QosFrameExchangeManager::StartTransmission(Ptr<QosTxop> edca, Time txopDuration)
{
    NS_LOG_FUNCTION(this << edca << txopDuration);

    if (m_pifsRecoveryEvent.IsPending())
    {
        // Another AC (having AIFS=1 or lower, if the user changed the default settings)
        // gained channel access while performing PIFS recovery. Abort PIFS recovery
        CancelPifsRecovery();
    }

    if (m_txTimer.IsRunning())
    {
        m_txTimer.Cancel();
    }
    m_dcf = edca;
    m_edca = edca;

    // We check if this EDCAF invoked the backoff procedure (without terminating
    // the TXOP) because the transmission of a non-initial frame of a TXOP failed
    bool backingOff = (m_edcaBackingOff == m_edca);

    if (backingOff)
    {
        NS_ASSERT(m_edca->GetTxopLimit(m_linkId).IsStrictlyPositive());
        NS_ASSERT(m_edca->GetTxopStartTime(m_linkId));
        NS_ASSERT(!m_pifsRecovery);
        NS_ASSERT(!m_initialFrame);

        // clear the member variable
        m_edcaBackingOff = nullptr;
    }

    if (m_edca->GetTxopLimit(m_linkId).IsStrictlyPositive())
    {
        // TXOP limit is not null. We have to check if this EDCAF is starting a
        // new TXOP. This includes the case when the transmission of a non-initial
        // frame of a TXOP failed and backoff was invoked without terminating the
        // TXOP. In such a case, we assume that a new TXOP is being started if it
        // elapsed more than TXOPlimit since the start of the paused TXOP. Note
        // that GetRemainingTxop returns 0 iff Now - TXOPstart >= TXOPlimit
        if (!m_edca->GetTxopStartTime(m_linkId) ||
            (backingOff && m_edca->GetRemainingTxop(m_linkId).IsZero()))
        {
            // starting a new TXOP
            m_edca->NotifyChannelAccessed(m_linkId, txopDuration);

            if (StartFrameExchange(m_edca, txopDuration, true))
            {
                m_initialFrame = true;
                return true;
            }

            // TXOP not even started, return false
            NS_LOG_DEBUG("No frame transmitted");
            NotifyChannelReleased(m_edca);
            m_edca = nullptr;
            return false;
        }

        // We are continuing a TXOP, check if we can transmit another frame
        NS_ASSERT(!m_initialFrame);

        if (!StartFrameExchange(m_edca, m_edca->GetRemainingTxop(m_linkId), false))
        {
            NS_LOG_DEBUG("Not enough remaining TXOP time");
            return SendCfEndIfNeeded();
        }

        return true;
    }

    // we get here if TXOP limit is null
    m_initialFrame = true;

    if (StartFrameExchange(m_edca, Time::Min(), true))
    {
        m_edca->NotifyChannelAccessed(m_linkId, Seconds(0));
        return true;
    }

    NS_LOG_DEBUG("No frame transmitted");
    NotifyChannelReleased(m_edca);
    m_edca = nullptr;
    return false;
}

bool
QosFrameExchangeManager::StartFrameExchange(Ptr<QosTxop> edca,
                                            Time availableTime,
                                            bool initialFrame)
{
    NS_LOG_FUNCTION(this << edca << availableTime << initialFrame);

    Ptr<WifiMpdu> mpdu = edca->PeekNextMpdu(m_linkId);

    // Even though channel access is requested when the queue is not empty, at
    // the time channel access is granted the lifetime of the packet might be
    // expired and the queue might be empty.
    if (!mpdu)
    {
        NS_LOG_DEBUG("Queue empty");
        return false;
    }

    mpdu = CreateAliasIfNeeded(mpdu);
    WifiTxParameters txParams;
    txParams.m_txVector =
        GetWifiRemoteStationManager()->GetDataTxVector(mpdu->GetHeader(), m_allowedWidth);

    Ptr<WifiMpdu> item = edca->GetNextMpdu(m_linkId, mpdu, txParams, availableTime, initialFrame);

    if (!item)
    {
        NS_LOG_DEBUG("Not enough time to transmit a frame");
        return false;
    }

    NS_ASSERT_MSG(!item->GetHeader().IsQosData() || !item->GetHeader().IsQosAmsdu(),
                  "We should not get an A-MSDU here");

    // check if the MSDU needs to be fragmented
    item = GetFirstFragmentIfNeeded(item);

    // update the protection method if the frame was fragmented
    if (item->IsFragment() && item->GetSize() != mpdu->GetSize())
    {
        WifiTxParameters fragmentTxParams;
        fragmentTxParams.m_txVector = txParams.m_txVector;
        fragmentTxParams.AddMpdu(item);
        UpdateTxDuration(item->GetHeader().GetAddr1(), fragmentTxParams);
        txParams.m_protection = GetProtectionManager()->TryAddMpdu(item, fragmentTxParams);
        NS_ASSERT(txParams.m_protection);
    }

    SendMpduWithProtection(item, txParams);

    return true;
}

Ptr<WifiMpdu>
QosFrameExchangeManager::CreateAliasIfNeeded(Ptr<WifiMpdu> mpdu) const
{
    return mpdu;
}

bool
QosFrameExchangeManager::TryAddMpdu(Ptr<const WifiMpdu> mpdu,
                                    WifiTxParameters& txParams,
                                    Time availableTime) const
{
    NS_ASSERT(mpdu);
    NS_LOG_FUNCTION(this << *mpdu << &txParams << availableTime);

    // tentatively add the given MPDU
    auto prevTxDuration = txParams.m_txDuration;
    txParams.AddMpdu(mpdu);
    UpdateTxDuration(mpdu->GetHeader().GetAddr1(), txParams);

    // check if adding the given MPDU requires a different protection method
    std::optional<Time> protectionTime; // uninitialized
    if (txParams.m_protection)
    {
        protectionTime = txParams.m_protection->protectionTime;
    }

    std::unique_ptr<WifiProtection> protection;
    protection = GetProtectionManager()->TryAddMpdu(mpdu, txParams);
    bool protectionSwapped = false;

    if (protection)
    {
        // the protection method has changed, calculate the new protection time
        CalculateProtectionTime(protection.get());
        protectionTime = protection->protectionTime;
        // swap unique pointers, so that the txParams that is passed to the next
        // call to IsWithinLimitsIfAddMpdu is the most updated one
        txParams.m_protection.swap(protection);
        protectionSwapped = true;
    }
    NS_ASSERT(protectionTime.has_value());
    NS_LOG_DEBUG("protection time=" << *protectionTime);

    // check if adding the given MPDU requires a different acknowledgment method
    std::optional<Time> acknowledgmentTime; // uninitialized
    if (txParams.m_acknowledgment)
    {
        acknowledgmentTime = txParams.m_acknowledgment->acknowledgmentTime;
    }

    std::unique_ptr<WifiAcknowledgment> acknowledgment;
    acknowledgment = GetAckManager()->TryAddMpdu(mpdu, txParams);
    bool acknowledgmentSwapped = false;

    if (acknowledgment)
    {
        // the acknowledgment method has changed, calculate the new acknowledgment time
        CalculateAcknowledgmentTime(acknowledgment.get());
        acknowledgmentTime = acknowledgment->acknowledgmentTime;
        // swap unique pointers, so that the txParams that is passed to the next
        // call to IsWithinLimitsIfAddMpdu is the most updated one
        txParams.m_acknowledgment.swap(acknowledgment);
        acknowledgmentSwapped = true;
    }
    NS_ASSERT(acknowledgmentTime.has_value());
    NS_LOG_DEBUG("acknowledgment time=" << *acknowledgmentTime);

    Time ppduDurationLimit = Time::Min();
    if (availableTime != Time::Min())
    {
        ppduDurationLimit = availableTime - *protectionTime - *acknowledgmentTime;
    }

    if (!IsWithinLimitsIfAddMpdu(mpdu, txParams, ppduDurationLimit))
    {
        // adding MPDU failed, undo the addition of the MPDU and restore protection and
        // acknowledgment methods if they were swapped
        txParams.UndoAddMpdu();
        txParams.m_txDuration = prevTxDuration;
        if (protectionSwapped)
        {
            txParams.m_protection.swap(protection);
        }
        if (acknowledgmentSwapped)
        {
            txParams.m_acknowledgment.swap(acknowledgment);
        }
        return false;
    }

    return true;
}

bool
QosFrameExchangeManager::IsWithinLimitsIfAddMpdu(Ptr<const WifiMpdu> mpdu,
                                                 const WifiTxParameters& txParams,
                                                 Time ppduDurationLimit) const
{
    NS_ASSERT(mpdu);
    NS_LOG_FUNCTION(this << *mpdu << &txParams << ppduDurationLimit);

    // A QoS station only has to check that the MPDU transmission time does not
    // exceed the given limit
    return IsWithinSizeAndTimeLimits(mpdu->GetSize(),
                                     mpdu->GetHeader().GetAddr1(),
                                     txParams,
                                     ppduDurationLimit);
}

bool
QosFrameExchangeManager::IsWithinSizeAndTimeLimits(uint32_t ppduPayloadSize,
                                                   Mac48Address receiver,
                                                   const WifiTxParameters& txParams,
                                                   Time ppduDurationLimit) const
{
    NS_LOG_FUNCTION(this << ppduPayloadSize << receiver << &txParams << ppduDurationLimit);

    if (ppduDurationLimit != Time::Min() && ppduDurationLimit.IsNegative())
    {
        NS_LOG_DEBUG("ppduDurationLimit is null or negative, time limit is trivially exceeded");
        return false;
    }

    if (ppduPayloadSize > WifiPhy::GetMaxPsduSize(txParams.m_txVector.GetModulationClass()))
    {
        NS_LOG_DEBUG("the frame exceeds the max PSDU size");
        return false;
    }

    // Get the maximum PPDU Duration based on the preamble type
    Time maxPpduDuration = GetPpduMaxTime(txParams.m_txVector.GetPreambleType());

    NS_ASSERT_MSG(txParams.m_txDuration, "TX duration not yet computed");
    auto txTime = txParams.m_txDuration.value();
    NS_LOG_DEBUG("PPDU duration: " << txTime.As(Time::MS));

    if ((ppduDurationLimit.IsStrictlyPositive() && txTime > ppduDurationLimit) ||
        (maxPpduDuration.IsStrictlyPositive() && txTime > maxPpduDuration))
    {
        NS_LOG_DEBUG(
            "the frame does not meet the constraint on max PPDU duration or PPDU duration limit");
        return false;
    }

    return true;
}

Time
QosFrameExchangeManager::GetFrameDurationId(const WifiMacHeader& header,
                                            uint32_t size,
                                            const WifiTxParameters& txParams,
                                            Ptr<Packet> fragmentedPacket) const
{
    NS_LOG_FUNCTION(this << header << size << &txParams << fragmentedPacket);

    // TODO This will be removed once no Txop is installed on a QoS station
    if (!m_edca)
    {
        return FrameExchangeManager::GetFrameDurationId(header, size, txParams, fragmentedPacket);
    }

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        return FrameExchangeManager::GetFrameDurationId(header, size, txParams, fragmentedPacket);
    }

    NS_ASSERT(txParams.m_acknowledgment &&
              txParams.m_acknowledgment->acknowledgmentTime.has_value());

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    return std::max(
        m_edca->GetRemainingTxop(m_linkId) -
            WifiPhy::CalculateTxDuration(size, txParams.m_txVector, m_phy->GetPhyBand()),
        *txParams.m_acknowledgment->acknowledgmentTime);
}

Time
QosFrameExchangeManager::GetRtsDurationId(const WifiTxVector& rtsTxVector,
                                          Time txDuration,
                                          Time response) const
{
    NS_LOG_FUNCTION(this << rtsTxVector << txDuration << response);

    // TODO This will be removed once no Txop is installed on a QoS station
    if (!m_edca)
    {
        return FrameExchangeManager::GetRtsDurationId(rtsTxVector, txDuration, response);
    }

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        return FrameExchangeManager::GetRtsDurationId(rtsTxVector, txDuration, response);
    }

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    return std::max(
        m_edca->GetRemainingTxop(m_linkId) -
            WifiPhy::CalculateTxDuration(GetRtsSize(), rtsTxVector, m_phy->GetPhyBand()),
        Seconds(0));
}

Time
QosFrameExchangeManager::GetCtsToSelfDurationId(const WifiTxVector& ctsTxVector,
                                                Time txDuration,
                                                Time response) const
{
    NS_LOG_FUNCTION(this << ctsTxVector << txDuration << response);

    // TODO This will be removed once no Txop is installed on a QoS station
    if (!m_edca)
    {
        return FrameExchangeManager::GetCtsToSelfDurationId(ctsTxVector, txDuration, response);
    }

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        return FrameExchangeManager::GetCtsToSelfDurationId(ctsTxVector, txDuration, response);
    }

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    return std::max(
        m_edca->GetRemainingTxop(m_linkId) -
            WifiPhy::CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand()),
        Seconds(0));
}

void
QosFrameExchangeManager::ForwardMpduDown(Ptr<WifiMpdu> mpdu, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *mpdu << txVector);

    WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsQosData() && m_mac->GetTypeOfStation() == STA &&
        (m_setQosQueueSize || hdr.IsQosEosp()))
    {
        uint8_t tid = hdr.GetQosTid();
        hdr.SetQosEosp();
        hdr.SetQosQueueSize(
            m_mac->GetQosTxop(tid)->GetQosQueueSize(tid,
                                                    mpdu->GetOriginal()->GetHeader().GetAddr1()));
    }
    FrameExchangeManager::ForwardMpduDown(mpdu, txVector);
}

void
QosFrameExchangeManager::TransmissionSucceeded()
{
    NS_LOG_DEBUG(this);

    // TODO This will be removed once no Txop is installed on a QoS station
    if (!m_edca)
    {
        FrameExchangeManager::TransmissionSucceeded();
        return;
    }

    if (m_edca->GetTxopLimit(m_linkId).IsStrictlyPositive() &&
        m_edca->GetRemainingTxop(m_linkId) > m_phy->GetSifs())
    {
        NS_LOG_DEBUG("Schedule another transmission in a SIFS");
        bool (QosFrameExchangeManager::*fp)(Ptr<QosTxop>, Time) =
            &QosFrameExchangeManager::StartTransmission;

        // we are continuing a TXOP, hence the txopDuration parameter is unused
        Simulator::Schedule(m_phy->GetSifs(), fp, this, m_edca, Seconds(0));

        if (m_protectedIfResponded)
        {
            m_protectedStas.merge(m_sentFrameTo);
        }
    }
    else
    {
        NotifyChannelReleased(m_edca);
        m_edca = nullptr;
    }
    m_initialFrame = false;
    m_sentFrameTo.clear();
}

void
QosFrameExchangeManager::TransmissionFailed(bool forceCurrentCw)
{
    NS_LOG_FUNCTION(this << forceCurrentCw);

    // TODO This will be removed once no Txop is installed on a QoS station
    if (!m_edca)
    {
        FrameExchangeManager::TransmissionFailed(forceCurrentCw);
        return;
    }

    if (m_initialFrame)
    {
        // The backoff procedure shall be invoked by an EDCAF when the transmission
        // of an MPDU in the initial PPDU of a TXOP fails (Sec. 10.22.2.2 of 802.11-2016)
        NS_LOG_DEBUG("TX of the initial frame of a TXOP failed: terminate TXOP");
        if (!forceCurrentCw)
        {
            m_edca->UpdateFailedCw(m_linkId);
        }
        NotifyChannelReleased(m_edca);
        m_edca = nullptr;
    }
    else
    {
        // some STA(s) did not respond, they are no longer protected
        for (const auto& address : m_txTimer.GetStasExpectedToRespond())
        {
            NS_LOG_DEBUG(address << " did not respond, hence it is no longer protected");
            m_protectedStas.erase(address);
        }

        NS_ASSERT_MSG(m_edca->GetTxopLimit(m_linkId).IsStrictlyPositive(),
                      "Cannot transmit more than one frame if TXOP Limit is zero");

        // A STA can perform a PIFS recovery or perform a backoff as a response to
        // transmission failure within a TXOP. How it chooses between these two is
        // implementation dependent. (Sec. 10.22.2.2 of 802.11-2016)
        if (m_pifsRecovery)
        {
            // we can continue the TXOP if the carrier sense mechanism indicates that
            // the medium is idle in a PIFS
            NS_LOG_DEBUG("TX of a non-initial frame of a TXOP failed: perform PIFS recovery");
            NS_ASSERT(!m_pifsRecoveryEvent.IsPending());
            m_pifsRecoveryEvent = Simulator::Schedule(m_phy->GetPifs(),
                                                      &QosFrameExchangeManager::PifsRecovery,
                                                      this,
                                                      forceCurrentCw);
        }
        else
        {
            // In order not to terminate (yet) the TXOP, we call the NotifyChannelReleased
            // method of the Txop class, which only generates a new backoff value and
            // requests channel access if needed,
            NS_LOG_DEBUG("TX of a non-initial frame of a TXOP failed: invoke backoff");
            m_edca->Txop::NotifyChannelReleased(m_linkId);
            // CW and QSRC shall be updated in this case (see Section 10.23.2.2 of 802.11-2020)
            if (!forceCurrentCw)
            {
                m_edca->UpdateFailedCw(m_linkId);
            }
            m_edcaBackingOff = m_edca;
            m_edca = nullptr;
        }
    }
    m_initialFrame = false;
    m_sentFrameTo.clear();
    // reset TXNAV because transmission failed
    m_txNav = Simulator::Now();
}

void
QosFrameExchangeManager::PreProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    // APs store buffer size report of associated stations
    if (m_mac->GetTypeOfStation() == AP && psdu->GetAddr1() == m_self)
    {
        for (const auto& mpdu : *PeekPointer(psdu))
        {
            const WifiMacHeader& hdr = mpdu->GetHeader();

            if (hdr.IsQosData() && hdr.IsQosEosp())
            {
                NS_LOG_DEBUG("Station " << hdr.GetAddr2() << " reported a buffer status of "
                                        << +hdr.GetQosQueueSize()
                                        << " for tid=" << +hdr.GetQosTid());
                m_apMac->SetBufferStatus(hdr.GetQosTid(),
                                         mpdu->GetOriginal()->GetHeader().GetAddr2(),
                                         hdr.GetQosQueueSize());
            }
        }
    }

    // before updating the NAV, check if the NAV counted down to zero. In such a
    // case, clear the saved TXOP holder address.
    ClearTxopHolderIfNeeded();

    FrameExchangeManager::PreProcessFrame(psdu, txVector);
}

void
QosFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    SetTxopHolder(psdu, txVector);
    FrameExchangeManager::PostProcessFrame(psdu, txVector);
}

void
QosFrameExchangeManager::SetTxopHolder(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);
    if (auto txopHolder = FindTxopHolder(psdu->GetHeader(0), txVector))
    {
        m_txopHolder = *txopHolder;
    }
}

std::optional<Mac48Address>
QosFrameExchangeManager::GetTxopHolder() const
{
    return m_txopHolder;
}

std::optional<Mac48Address>
QosFrameExchangeManager::FindTxopHolder(const WifiMacHeader& hdr, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << hdr << txVector);

    // A STA shall save the TXOP holder address for the BSS in which it is associated.
    // The TXOP holder address is the MAC address from the Address 2 field of the frame
    // that initiated a frame exchange sequence, except if this is a CTS frame, in which
    // case the TXOP holder address is the Address 1 field. (Sec. 10.23.2.4 of 802.11-2020)
    if ((hdr.IsQosData() || hdr.IsMgt() || hdr.IsRts() || hdr.IsBlockAckReq()) &&
        (hdr.GetAddr1() == m_bssid || hdr.GetAddr2() == m_bssid))
    {
        return hdr.GetAddr2();
    }
    if (hdr.IsCts() && hdr.GetAddr1() == m_bssid)
    {
        return hdr.GetAddr1();
    }
    return std::nullopt;
}

void
QosFrameExchangeManager::ClearTxopHolderIfNeeded()
{
    NS_LOG_FUNCTION(this);
    if (m_navEnd <= Simulator::Now())
    {
        m_txopHolder.reset();
    }
}

void
QosFrameExchangeManager::UpdateNav(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);
    if (psdu->GetHeader(0).IsCfEnd())
    {
        NS_LOG_DEBUG("Received CF-End, resetting NAV");
        NavResetTimeout();
        return;
    }

    FrameExchangeManager::UpdateNav(psdu, txVector);
}

void
QosFrameExchangeManager::NavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    FrameExchangeManager::NavResetTimeout();
    ClearTxopHolderIfNeeded();
}

void
QosFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                     RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector,
                                     bool inAmpdu)
{
    NS_LOG_FUNCTION(this << *mpdu << rxSignalInfo << txVector << inAmpdu);

    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    double rxSnr = rxSignalInfo.snr;
    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsRts())
    {
        NS_ABORT_MSG_IF(inAmpdu, "Received RTS as part of an A-MPDU");

        // If a non-VHT STA receives an RTS frame with the RA address matching the
        // MAC address of the STA and the MAC address in the TA field in the RTS
        // frame matches the saved TXOP holder address, then the STA shall send the
        // CTS frame after SIFS, without regard for, and without resetting, its NAV.
        // (sec. 10.22.2.4 of 802.11-2016)
        if (hdr.GetAddr2() == m_txopHolder || VirtualCsMediumIdle())
        {
            NS_LOG_DEBUG("Received RTS from=" << hdr.GetAddr2() << ", schedule CTS");
            m_sendCtsEvent = Simulator::Schedule(m_phy->GetSifs(),
                                                 &QosFrameExchangeManager::SendCtsAfterRts,
                                                 this,
                                                 hdr,
                                                 txVector.GetMode(),
                                                 rxSnr);
        }
        else
        {
            NS_LOG_DEBUG("Received RTS from=" << hdr.GetAddr2() << ", cannot schedule CTS");
        }
        return;
    }

    if (hdr.IsQosData())
    {
        if (hdr.GetAddr1() == m_self && hdr.GetQosAckPolicy() == WifiMacHeader::NORMAL_ACK)
        {
            NS_LOG_DEBUG("Received " << hdr.GetTypeString() << " from=" << hdr.GetAddr2()
                                     << ", schedule ACK");
            Simulator::Schedule(m_phy->GetSifs(),
                                &QosFrameExchangeManager::SendNormalAck,
                                this,
                                hdr,
                                txVector,
                                rxSnr);
        }

        // Forward up the frame
        m_rxMiddle->Receive(mpdu, m_linkId);

        // the received data frame has been processed
        return;
    }

    return FrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

} // namespace ns3
