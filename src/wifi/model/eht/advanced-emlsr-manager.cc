/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "advanced-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/wifi-phy.h"

#include <algorithm>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdvancedEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(AdvancedEmlsrManager);

/**
 * PHY listener connected to the main PHY while operating on the link of an aux PHY that is
 * not TX capable.
 *
 * PHY notifications are forwarded to this EMLSR manager one timestep later because this EMLSR
 * manager may then decide to switch the main PHY back to the preferred link. Given that notifying
 * a PHY listener is only one of the actions that are performed when handling events such as RX end
 * or CCA busy start, it is not a good idea to request a main PHY switch while performing other
 * actions. Forwarding notifications a timestep later allows to first complete the handling of the
 * given event and then (possibly) starting a main PHY switch.
 */
class EmlsrPhyListener : public WifiPhyListener
{
  public:
    /**
     * Constructor
     *
     * @param emlsrManager the EMLSR manager
     */
    EmlsrPhyListener(Ptr<AdvancedEmlsrManager> emlsrManager)
        : m_emlsrManager(emlsrManager)
    {
    }

    void NotifyRxStart(Time /* duration */) override
    {
        Simulator::Schedule(TimeStep(1),
                            &AdvancedEmlsrManager::InterruptSwitchMainPhyBackTimerIfNeeded,
                            m_emlsrManager);
    }

    void NotifyRxEndOk() override
    {
        Simulator::Schedule(TimeStep(1),
                            &AdvancedEmlsrManager::InterruptSwitchMainPhyBackTimerIfNeeded,
                            m_emlsrManager);
    }

    void NotifyRxEndError(const WifiTxVector& /* txVector */) override
    {
    }

    void NotifyTxStart(Time /* duration */, dBm_u /* txPower */) override
    {
    }

    void NotifyCcaBusyStart(Time /* duration */,
                            WifiChannelListType /* channelType */,
                            const std::vector<Time>& /* per20MhzDurations */) override
    {
        Simulator::Schedule(TimeStep(1),
                            &AdvancedEmlsrManager::InterruptSwitchMainPhyBackTimerIfNeeded,
                            m_emlsrManager);
    }

    void NotifySwitchingStart(Time /* duration */) override
    {
    }

    void NotifySleep() override
    {
    }

    void NotifyOff() override
    {
    }

    void NotifyWakeup() override
    {
    }

    void NotifyOn() override
    {
    }

  private:
    Ptr<AdvancedEmlsrManager> m_emlsrManager; //!< the EMLSR manager
};

TypeId
AdvancedEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AdvancedEmlsrManager")
            .SetParent<DefaultEmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AdvancedEmlsrManager>()
            .AddAttribute("AllowUlTxopInRx",
                          "Whether a (main or aux) PHY is allowed to start an UL TXOP if "
                          "another PHY is receiving a PPDU (possibly starting a DL TXOP). "
                          "If this attribute is true, the PPDU may be dropped.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_allowUlTxopInRx),
                          MakeBooleanChecker())
            .AddAttribute("InterruptSwitch",
                          "Whether the main PHY can be interrupted while switching to start "
                          "switching to another link.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_interruptSwitching),
                          MakeBooleanChecker())
            .AddAttribute("UseAuxPhyCca",
                          "Whether the CCA performed in the last PIFS interval by a non-TX "
                          "capable aux PHY should be used when the main PHY ends switching to "
                          "the aux PHY's link to determine whether TX can start or not (and what "
                          "bandwidth can be used for transmission) independently of whether the "
                          "aux PHY bandwidth is smaller than the main PHY bandwidth or not.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_useAuxPhyCca),
                          MakeBooleanChecker())
            .AddAttribute("SwitchMainPhyBackDelay",
                          "Duration of the timer started in case of non-TX capable aux PHY (that "
                          "does not switch link) when medium is sensed busy during the PIFS "
                          "interval preceding/following the main PHY switch end. When the timer "
                          "expires, the main PHY is switched back to the preferred link.",
                          TimeValue(MilliSeconds(5)),
                          MakeTimeAccessor(&AdvancedEmlsrManager::m_switchMainPhyBackDelay),
                          MakeTimeChecker())
            .AddAttribute("KeepMainPhyAfterDlTxop",
                          "In case aux PHYs are not TX capable and do not switch link, after the "
                          "end of a DL TXOP carried out on an aux PHY link, the main PHY stays on "
                          "that link for a switch main PHY back delay, if this attribute is true, "
                          "or it returns to the preferred link, otherwise.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_keepMainPhyAfterDlTxop),
                          MakeBooleanChecker())
            .AddAttribute("CheckAccessOnMainPhyLink",
                          "In case aux PHYs are not TX capable and an Access Category, say it AC "
                          "X, is about to gain channel access on an aux PHY link, determine "
                          "whether the time the ACs with priority higher than or equal to AC X and "
                          "with frames to send on the main PHY link are expected to gain access on "
                          "the main PHY link should be taken into account when taking the decision "
                          "to switch the main PHY to the aux PHY link.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_checkAccessOnMainPhyLink),
                          MakeBooleanChecker())
            .AddAttribute(
                "MinAcToSkipCheckAccess",
                "If the CheckAccessOnMainPhyLink attribute is set to false, indicate the "
                "minimum priority AC for which it is allowed to skip the check related "
                "to the expected channel access time on the main PHY link.",
                EnumValue(AcIndex::AC_BK),
                MakeEnumAccessor<AcIndex>(&AdvancedEmlsrManager::m_minAcToSkipCheckAccess),
                MakeEnumChecker(AcIndex::AC_BE,
                                "AC_BE",
                                AcIndex::AC_VI,
                                "AC_VI",
                                AcIndex::AC_VO,
                                "AC_VO",
                                AcIndex::AC_BK,
                                "AC_BK"));
    return tid;
}

AdvancedEmlsrManager::AdvancedEmlsrManager()
{
    NS_LOG_FUNCTION(this);
    m_phyListener = std::make_shared<EmlsrPhyListener>(this);
}

AdvancedEmlsrManager::~AdvancedEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
AdvancedEmlsrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto phy : GetStaMac()->GetDevice()->GetPhys())
    {
        phy->TraceDisconnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedEmlsrManager::ReceivedMacHdr, this).Bind(phy));
    }
    UnregisterListener();
    m_phyListener.reset();
    DefaultEmlsrManager::DoDispose();
}

void
AdvancedEmlsrManager::NotifyEmlsrModeChanged()
{
    NS_LOG_FUNCTION(this);

    // disconnect callbacks on all links
    for (const auto& linkId : GetStaMac()->GetLinkIds())
    {
        GetStaMac()->GetChannelAccessManager(linkId)->TraceDisconnectWithoutContext(
            "NSlotsLeftAlert",
            MakeCallback(&AdvancedEmlsrManager::SwitchMainPhyIfTxopToBeGainedByAuxPhy, this));
    }

    // connect callbacks on EMLSR links
    for (const auto& emlsrLinkId : GetEmlsrLinks())
    {
        GetStaMac()
            ->GetChannelAccessManager(emlsrLinkId)
            ->TraceConnectWithoutContext(
                "NSlotsLeftAlert",
                MakeCallback(&AdvancedEmlsrManager::SwitchMainPhyIfTxopToBeGainedByAuxPhy, this));
    }

    DefaultEmlsrManager::NotifyEmlsrModeChanged();
}

void
AdvancedEmlsrManager::DoSetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);

    for (auto phy : GetStaMac()->GetDevice()->GetPhys())
    {
        phy->TraceConnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedEmlsrManager::ReceivedMacHdr, this).Bind(phy));
    }
}

void
AdvancedEmlsrManager::RegisterListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy->GetPhyId());

    NS_ASSERT_MSG(!m_auxPhyWithListener,
                  "PHY listener is still connected to PHY " << +m_auxPhyWithListener->GetPhyId());
    phy->RegisterListener(m_phyListener);
    m_auxPhyWithListener = phy;
}

void
AdvancedEmlsrManager::UnregisterListener()
{
    if (!m_auxPhyWithListener)
    {
        return; // do nothing
    }

    NS_LOG_FUNCTION(this << m_auxPhyWithListener->GetPhyId());
    m_auxPhyWithListener->UnregisterListener(m_phyListener);
    m_auxPhyWithListener = nullptr;
}

std::pair<bool, Time>
AdvancedEmlsrManager::DoGetDelayUntilAccessRequest(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // prevent or allow an UL TXOP depending on whether another PHY is receiving a PPDU
    for (const auto id : GetStaMac()->GetLinkIds())
    {
        if (id == linkId)
        {
            continue;
        }

        const auto [maybeIcf, delay] = CheckPossiblyReceivingIcf(id);

        if (!maybeIcf)
        {
            // not receiving anything or receiving something that is certainly not an ICF
            continue;
        }

        // a PPDU that may be an ICF is being received
        if (!m_allowUlTxopInRx)
        {
            return {false, delay};
        }
    }

    auto phy = GetStaMac()->GetWifiPhy(linkId);
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    if (phy == mainPhy)
    {
        if (!m_auxPhyTxCapable && m_ccaLastPifs.IsPending())
        {
            // main PHY has got access on the link it switched to (because the aux PHY is not TX
            // capable) before a PIFS interval was elapsed: do not start the TXOP now
            return {false, Time{0}};
        }

        // UL TXOP is going to start
        m_rtsStartingUlTxop[linkId] = {Simulator::Now(), false};
    }

    return {true, Time{0}};
}

void
AdvancedEmlsrManager::ReceivedMacHdr(Ptr<WifiPhy> phy,
                                     const WifiMacHeader& macHdr,
                                     const WifiTxVector& txVector,
                                     Time psduDuration)
{
    auto linkId = GetStaMac()->GetLinkForPhy(phy);
    if (!linkId.has_value() || !m_useNotifiedMacHdr)
    {
        return;
    }
    NS_LOG_FUNCTION(this << *linkId << macHdr << txVector << psduDuration.As(Time::MS));

    auto& ongoingTxopEnd = GetEhtFem(*linkId)->GetOngoingTxopEndEvent();
    const auto isMainPhy = (phy->GetPhyId() == GetMainPhyId());

    if (ongoingTxopEnd.IsPending() && macHdr.GetAddr1() != GetEhtFem(*linkId)->GetAddress() &&
        !macHdr.IsTrigger() && !macHdr.IsBlockAck() &&
        !(macHdr.IsCts() && macHdr.GetAddr1() == GetEhtFem(*linkId)->GetBssid() /* CTS-to-self */))
    {
        // the EMLSR client is no longer involved in the TXOP and switching to listening mode
        ongoingTxopEnd.Cancel();
        // this method is a callback connected to the PhyRxMacHeaderEnd trace source of WifiPhy
        // and is called within a for loop that executes all the callbacks. The call to NotifyTxop
        // below leads the main PHY to be connected back to the preferred link, thus
        // the ResetPhy() method of the FEM on the auxiliary link is called, which disconnects
        // another callback (FEM::ReceivedMacHdr) from the PhyRxMacHeaderEnd trace source of
        // the main PHY, thus invalidating the list of callbacks on which the for loop iterates.
        // Hence, schedule the call to NotifyTxopEnd to execute it outside such for loop.
        Simulator::ScheduleNow(&AdvancedEmlsrManager::NotifyTxopEnd, this, *linkId, nullptr);
    }

    if (!ongoingTxopEnd.IsPending() && GetStaMac()->IsEmlsrLink(*linkId) && isMainPhy &&
        !GetEhtFem(*linkId)->UsingOtherEmlsrLink() &&
        (macHdr.IsRts() || macHdr.IsBlockAckReq() || macHdr.IsData()) &&
        (macHdr.GetAddr1() == GetEhtFem(*linkId)->GetAddress()))
    {
        // a frame that is starting a DL TXOP is being received by the main PHY; start blocking
        // transmission on other links (which is normally done later on by PostProcessFrame()) to
        // avoid starting an UL TXOP before the end of the MPDU
        for (auto id : GetStaMac()->GetLinkIds())
        {
            if (id != *linkId && GetStaMac()->IsEmlsrLink(id))
            {
                GetStaMac()->BlockTxOnLink(id, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
            }
        }
        return;
    }

    // if the MAC header has been received on the link on which the main PHY is operating (or on
    // the link the main PHY is switching to), the switch main PHY back timer is running and channel
    // access is not expected to be gained by the main PHY before the switch main PHY back timer
    // expires (plus a channel switch delay), try to switch the main PHY back to the preferred link
    const auto mainPhyInvolved =
        isMainPhy || (m_mainPhySwitchInfo.disconnected && m_mainPhySwitchInfo.to == *linkId);
    const auto delay =
        Simulator::GetDelayLeft(m_switchMainPhyBackEvent) + phy->GetChannelSwitchDelay();

    Simulator::ScheduleNow([=, this]() {
        if (WifiExpectedAccessReason reason;
            m_switchMainPhyBackEvent.IsPending() && mainPhyInvolved &&
            (reason = GetStaMac()->GetChannelAccessManager(*linkId)->GetExpectedAccessWithin(
                 delay)) != WifiExpectedAccessReason::ACCESS_EXPECTED)
        {
            SwitchMainPhyBackDelayExpired(*linkId, reason);
        }
    });
}

void
AdvancedEmlsrManager::DoNotifyTxopEnd(uint8_t linkId, Ptr<QosTxop> edca)
{
    NS_LOG_FUNCTION(this << linkId << edca);

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    if (m_switchAuxPhy && (!mainPhy->IsStateSwitching() || !m_interruptSwitching))
    {
        NS_LOG_DEBUG("SwitchAuxPhy true, nothing to do");
        return;
    }

    if (!m_switchAuxPhy && !m_auxPhyToReconnect)
    {
        NS_LOG_DEBUG("SwitchAuxPhy false, nothing to do");
        return;
    }

    // we get here if:
    // - SwitchAuxPhy is true, the main PHY is switching and switching can be interrupted
    // or
    // - SwitchAuxPhy is false and there is an aux PHY to reconnect

    if (!m_auxPhyTxCapable && !m_switchAuxPhy && !edca && m_keepMainPhyAfterDlTxop)
    {
        // DL TXOP ended, check if the main PHY must be kept on this link to try to gain an UL TXOP
        NS_ASSERT_MSG(!m_switchMainPhyBackEvent.IsPending(),
                      "Switch main PHY back timer should not be running at the end of a DL TXOP");
        NS_ASSERT_MSG(!mainPhy->IsStateSwitching(),
                      "Main PHY should not be switching at the end of a DL TXOP");

        if (GetStaMac()->GetChannelAccessManager(linkId)->GetExpectedAccessWithin(
                m_switchMainPhyBackDelay) == WifiExpectedAccessReason::ACCESS_EXPECTED)
        {
            NS_LOG_DEBUG("Keep main PHY on link " << +linkId << " to try to gain an UL TXOP");
            m_switchMainPhyBackEvent =
                Simulator::Schedule(m_switchMainPhyBackDelay,
                                    &AdvancedEmlsrManager::SwitchMainPhyBackDelayExpired,
                                    this,
                                    linkId,
                                    std::nullopt);
            // start checking PHY activity on the link the main PHY is operating
            RegisterListener(GetStaMac()->GetWifiPhy(linkId));
            return;
        }
    }

    std::shared_ptr<EmlsrMainPhySwitchTrace> traceInfo;

    if (const auto it = m_rtsStartingUlTxop.find(linkId);
        it != m_rtsStartingUlTxop.cend() && it->second.second)
    {
        // TXOP ended due to a CTS timeout following the RTS that started a TXOP
        traceInfo = std::make_shared<EmlsrCtsAfterRtsTimeoutTrace>(Time{0});
        m_rtsStartingUlTxop.erase(it);
    }
    else
    {
        traceInfo = std::make_shared<EmlsrTxopEndedTrace>();
    }

    // Note that the main PHY may be switching at the end of a TXOP when, e.g., the main PHY
    // starts switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
    // is not received and the UL TXOP ends before the main PHY channel switch is completed.
    // In such cases, wait until the main PHY channel switch is completed (unless the channel
    // switching can be interrupted) before requesting a new channel switch.
    // Backoff shall not be reset on the link left by the main PHY because a TXOP ended and
    // a new backoff value must be generated.
    if (m_switchAuxPhy || !mainPhy->IsStateSwitching() || m_interruptSwitching)
    {
        NS_ASSERT_MSG(
            !m_switchAuxPhy || m_mainPhySwitchInfo.disconnected,
            "Aux PHY next link ID should have a value when interrupting a main PHY switch");
        uint8_t nextLinkId = m_switchAuxPhy ? m_mainPhySwitchInfo.from : GetMainPhyId();
        SwitchMainPhy(nextLinkId, false, REQUEST_ACCESS, std::move(*traceInfo));
    }
    else
    {
        // delay link switch until current channel switching is completed
        const auto delay = mainPhy->GetDelayUntilIdle();

        if (auto info = std::dynamic_pointer_cast<EmlsrCtsAfterRtsTimeoutTrace>(traceInfo))
        {
            info->sinceCtsTimeout = delay;
        }

        Simulator::Schedule(delay, [=, this]() {
            // request the main PHY to switch back to the preferred link only if in the meantime
            // no TXOP started on another link (which will require the main PHY to switch link)
            if (!GetEhtFem(linkId)->UsingOtherEmlsrLink())
            {
                SwitchMainPhy(GetMainPhyId(), false, REQUEST_ACCESS, std::move(*traceInfo));
            }
        });
    }
}

std::optional<WifiIcfDrop>
AdvancedEmlsrManager::CheckMainPhyTakesOverDlTxop(uint8_t linkId) const
{
    auto reason = DefaultEmlsrManager::CheckMainPhyTakesOverDlTxop(linkId);

    // if the switching can be interrupted, do not drop an ICF due to not enough time for switching
    if (reason == WifiIcfDrop::NOT_ENOUGH_TIME_SWITCH && m_interruptSwitching)
    {
        return std::nullopt;
    }
    return reason;
}

std::pair<bool, Time>
AdvancedEmlsrManager::GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (!m_interruptSwitching)
    {
        return DefaultEmlsrManager::GetDelayUnlessMainPhyTakesOverUlTxop(linkId);
    }

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    auto state = mainPhy->GetState()->GetState();

    NS_ABORT_MSG_UNLESS(state == WifiPhyState::SWITCHING || state == WifiPhyState::RX ||
                            state == WifiPhyState::IDLE || state == WifiPhyState::CCA_BUSY,
                        "Main PHY cannot be in state " << state);

    auto timeToCtsEnd = GetTimeToCtsEnd(linkId);
    auto switchingTime = mainPhy->GetChannelSwitchDelay();

    if (switchingTime > timeToCtsEnd)
    {
        // switching takes longer than RTS/CTS exchange, release channel
        NS_LOG_DEBUG("Not enough time for main PHY to switch link (main PHY state: "
                     << mainPhy->GetState()->GetState() << ")");
        // retry channel access when the CTS was expected to be received
        return {false, timeToCtsEnd};
    }

    // TXOP can be started, main PHY will be scheduled to switch by NotifyRtsSent as soon as the
    // transmission of the RTS is notified
    m_rtsStartingUlTxop[linkId] = {Simulator::Now(), false};

    return {true, Time{0}};
}

void
AdvancedEmlsrManager::CheckNavAndCcaLastPifs(Ptr<WifiPhy> phy, uint8_t linkId, Ptr<QosTxop> edca)
{
    NS_LOG_FUNCTION(this << phy->GetPhyId() << linkId << edca->GetAccessCategory());

    const auto caManager = GetStaMac()->GetChannelAccessManager(linkId);
    const auto pifs = phy->GetSifs() + phy->GetSlot();

    const auto isBusy = caManager->IsBusy(); // check NAV and CCA on primary20
    // check CCA on the entire channel
    auto width = caManager->GetLargestIdlePrimaryChannel(pifs, Simulator::Now());

    // lambda to perform the actions needed when a TXOP is not started
    auto txopNotStarted = [=, this]() {
        // check when access may be granted to determine whether to switch the main PHY back
        // to the preferred link (if aux PHYs do not switch link)
        const auto mainPhy = GetStaMac()->GetDevice()->GetPhy(GetMainPhyId());
        const auto delay =
            Simulator::GetDelayLeft(m_switchMainPhyBackEvent) + mainPhy->GetChannelSwitchDelay();

        if (WifiExpectedAccessReason reason;
            !m_switchAuxPhy && m_switchMainPhyBackEvent.IsPending() &&
            (reason = GetStaMac()->GetChannelAccessManager(linkId)->GetExpectedAccessWithin(
                 delay)) != WifiExpectedAccessReason::ACCESS_EXPECTED)
        {
            NS_LOG_DEBUG("No AC is expected to get backoff soon, switch main PHY back");
            SwitchMainPhyBackDelayExpired(linkId, reason);
        }

        // restart channel access
        edca->NotifyChannelReleased(linkId); // to set access to NOT_REQUESTED
        edca->StartAccessAfterEvent(linkId,
                                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);
    };

    if (!m_switchAuxPhy && !m_switchMainPhyBackEvent.IsPending())
    {
        NS_LOG_DEBUG("Main PHY switched back (or scheduled to switch back) before PIFS check");
        txopNotStarted();
    }
    else if (!isBusy && width > MHz_u{0})
    {
        // medium idle, start TXOP
        width = std::min(width, GetChannelForMainPhy(linkId).GetTotalWidth());

        // if this function is called at the end of the main PHY switch, it is executed before the
        // main PHY is connected to this link in order to use the CCA information of the aux PHY.
        // Schedule now the TXOP start so that we first connect the main PHY to this link.
        m_ccaLastPifs = Simulator::ScheduleNow([=, this]() {
            if (GetEhtFem(linkId)->StartTransmission(edca, width))
            {
                NotifyUlTxopStart(linkId);
            }
            else
            {
                txopNotStarted();
            }
        });
    }
    else
    {
        NS_LOG_DEBUG("Medium busy in the last PIFS interval");
        txopNotStarted();
    }
}

void
AdvancedEmlsrManager::SwitchMainPhyBackDelayExpired(
    uint8_t linkId,
    std::optional<WifiExpectedAccessReason> stopReason)
{
    if (g_log.IsEnabled(ns3::LOG_FUNCTION))
    {
        std::stringstream ss;
        if (stopReason.has_value())
        {
            ss << stopReason.value();
        }
        NS_LOG_FUNCTION(this << linkId << ss.str());
    }

    m_switchMainPhyBackEvent.Cancel();

    NS_ASSERT_MSG(!m_switchAuxPhy, "Don't expect this to be called when aux PHYs switch link");
    Time extension{0};

    // check if the timer must be restarted because a frame is being received on any link
    for (const auto id : GetStaMac()->GetLinkIds())
    {
        auto phy = GetStaMac()->GetWifiPhy(id);

        if (!phy || !GetStaMac()->IsEmlsrLink(id))
        {
            continue;
        }

        if (!GetEhtFem(id)->VirtualCsMediumIdle() &&
            GetEhtFem(id)->GetTxopHolder() != GetEhtFem(id)->GetBssid())
        {
            NS_LOG_DEBUG("NAV is set and TXOP holder is not the associated AP MLD on link " << +id);
            continue;
        }

        const auto [maybeIcf, delay] = CheckPossiblyReceivingIcf(id);

        if (maybeIcf)
        {
            extension = Max(extension, delay);
        }
        else if (id == linkId && phy->IsStateIdle())
        {
            // this is the link on which the main PHY is operating. If an AC with traffic is
            // expected to get channel access soon (within a channel switch delay), restart
            // the timer to have the main PHY stay a bit longer on this link
            if (GetStaMac()->GetChannelAccessManager(linkId)->GetExpectedAccessWithin(
                    phy->GetChannelSwitchDelay()) == WifiExpectedAccessReason::ACCESS_EXPECTED)
            {
                extension = Max(extension, phy->GetChannelSwitchDelay());
            }
        }
    }

    if (extension.IsStrictlyPositive())
    {
        NS_LOG_DEBUG("Restarting the timer, check again in " << extension.As(Time::US));
        m_switchMainPhyBackEvent =
            Simulator::Schedule(extension,
                                &AdvancedEmlsrManager::SwitchMainPhyBackDelayExpired,
                                this,
                                linkId,
                                stopReason);
        return;
    }

    // no need to wait further, switch the main PHY back to the preferred link and unregister
    // the PHY listener from the aux PHY
    const auto elapsed = Simulator::Now() - m_mainPhySwitchInfo.start;
    const auto isSwitching = GetStaMac()->GetDevice()->GetPhy(GetMainPhyId())->IsStateSwitching();
    SwitchMainPhyBackToPreferredLink(linkId,
                                     EmlsrSwitchMainPhyBackTrace(elapsed, stopReason, isSwitching));
    // if scheduled, invoke CheckNavAndCcaLastPifs(), which will just restart channel access
    if (m_ccaLastPifs.IsPending())
    {
        m_ccaLastPifs.PeekEventImpl()->Invoke();
        m_ccaLastPifs.Cancel();
    }
    UnregisterListener();
}

void
AdvancedEmlsrManager::SwitchMainPhyBackToPreferredLink(uint8_t linkId,
                                                       EmlsrMainPhySwitchTrace&& traceInfo)
{
    if (!m_interruptSwitching)
    {
        DefaultEmlsrManager::SwitchMainPhyBackToPreferredLink(
            linkId,
            std::forward<EmlsrMainPhySwitchTrace>(traceInfo));
        return;
    }

    NS_LOG_FUNCTION(this << linkId << traceInfo.GetName());

    NS_ABORT_MSG_IF(m_switchAuxPhy, "This method can only be called when SwitchAuxPhy is false");

    if (!m_auxPhyToReconnect)
    {
        return;
    }

    SwitchMainPhy(GetMainPhyId(),
                  false,
                  REQUEST_ACCESS,
                  std::forward<EmlsrMainPhySwitchTrace>(traceInfo));
}

void
AdvancedEmlsrManager::InterruptSwitchMainPhyBackTimerIfNeeded()
{
    NS_LOG_FUNCTION(this);

    if (!m_switchMainPhyBackEvent.IsPending())
    {
        return; // nothing to do
    }

    // a busy event occurred, check if the main PHY has to switch back to the preferred link
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(GetMainPhyId());
    auto linkId = GetStaMac()->GetLinkForPhy(GetMainPhyId());

    if (!linkId.has_value())
    {
        NS_ASSERT(m_mainPhySwitchInfo.disconnected);
        linkId = m_mainPhySwitchInfo.to;
        NS_LOG_DEBUG("Main PHY is switching to link " << +linkId.value());
    }

    const auto delay =
        Simulator::GetDelayLeft(m_switchMainPhyBackEvent) + mainPhy->GetChannelSwitchDelay();
    if (auto reason = GetStaMac()->GetChannelAccessManager(*linkId)->GetExpectedAccessWithin(delay);
        reason != WifiExpectedAccessReason::ACCESS_EXPECTED)
    {
        SwitchMainPhyBackDelayExpired(*linkId, reason);
    }
}

void
AdvancedEmlsrManager::DoNotifyDlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    m_switchMainPhyBackEvent.Cancel();
    m_ccaLastPifs.Cancel();
    UnregisterListener();
}

void
AdvancedEmlsrManager::DoNotifyUlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    m_switchMainPhyBackEvent.Cancel();
    m_ccaLastPifs.Cancel();
    UnregisterListener();
}

bool
AdvancedEmlsrManager::RequestMainPhyToSwitch(uint8_t linkId, AcIndex aci, const Time& delay)
{
    NS_LOG_FUNCTION(this << linkId << aci << delay.As(Time::US));

    // the aux PHY is not TX capable; check if main PHY has to switch to the aux PHY's link
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    // if main PHY is not operating on a link and is trying to start a (DL or UL) TXOP, then do
    // not request another switch
    if (m_mainPhySwitchInfo.disconnected &&
        (!m_interruptSwitching || m_mainPhySwitchInfo.reason == "DlTxopIcfReceivedByAuxPhy" ||
         m_mainPhySwitchInfo.reason == "UlTxopAuxPhyNotTxCapable"))
    {
        NS_LOG_DEBUG("Main PHY is not operating on any link and cannot switch to another link");
        return false;
    }

    // if the main PHY is already trying to get access on a link, do not request another switch
    if (m_ccaLastPifs.IsPending() || m_switchMainPhyBackEvent.IsPending())
    {
        NS_LOG_DEBUG("Main PHY is trying to get access on another link");
        return false;
    }

    // delay until the earliest time the main PHY can access medium on the aux PHY link
    auto minDelay = mainPhy->GetChannelSwitchDelay();
    if (!m_useAuxPhyCca && (GetChannelForAuxPhy(linkId).GetTotalWidth() <
                            GetChannelForMainPhy(linkId).GetTotalWidth()))
    {
        // cannot use aux PHY CCA
        const auto pifs = GetStaMac()->GetWifiPhy(linkId)->GetPifs();
        if (m_switchMainPhyBackDelay < pifs)
        {
            NS_LOG_DEBUG(
                "Main PHY has to perform CCA but switch main PHY back delay is less than PIFS");
            return false;
        }
        minDelay += pifs;
    }
    minDelay = std::max(delay, minDelay);

    if (const auto elapsed = GetElapsedMediumSyncDelayTimer(linkId);
        elapsed && MediumSyncDelayNTxopsExceeded(linkId) &&
        (GetMediumSyncDuration() - *elapsed > minDelay))
    {
        NS_LOG_DEBUG("No more TXOP attempts allowed on aux PHY link and MSD timer still running");
        return false;
    }

    // DoGetDelayUntilAccessRequest has already checked if the main PHY is receiving an ICF and
    // above it is checked whether we can request another switch while already switching
    if (const auto state = mainPhy->GetState()->GetState();
        state != WifiPhyState::IDLE && state != WifiPhyState::CCA_BUSY &&
        state != WifiPhyState::RX && state != WifiPhyState::SWITCHING)
    {
        NS_LOG_DEBUG("Cannot request main PHY to switch when in state " << state);
        return false;
    }

    // if the AC that is about to get channel access on the aux PHY link has no frames to send on
    // that link, do not request the main PHY to switch
    if (!GetStaMac()->GetQosTxop(aci)->HasFramesToTransmit(linkId))
    {
        NS_LOG_DEBUG("No frames of " << aci << " to send on link " << +linkId);
        return false;
    }

    // if user has configured to skip the check related to the expected channel access time on
    // the main PHY link and the AC that is about to gain access on the aux PHY link has a priority
    // greater than or equal to the minimum priority that has been configured, switch the main PHY
    if (!m_checkAccessOnMainPhyLink && (aci >= m_minAcToSkipCheckAccess))
    {
        NS_LOG_DEBUG("Skipping check related to the expected channel access time on main PHY link");
        return true;
    }

    const auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy);
    if (!mainPhyLinkId.has_value())
    {
        NS_ASSERT(m_mainPhySwitchInfo.disconnected);
        NS_LOG_DEBUG("The main PHY is not connected to any link");
        // we don't know when the main PHY will be connected to the link it is switching to, nor
        // which backoff value it will possibly generate; therefore, request it to switch to the
        // aux PHY link
        return true;
    }

    // let AC X be the AC that is about to gain channel access on the aux PHY link, request to
    // switch the main PHY if we do not expect any AC, with priority higher than or equal to that
    // of AC X and with frames to send on the main PHY link, to gain channel access on the main PHY
    // link before AC X is able to start transmitting on the aux PHY link.

    const auto now = Simulator::Now();

    for (const auto& [acIndex, ac] : wifiAcList)
    {
        // ignore ACs with lower priority than the AC that is about to get access on aux PHY link
        if (acIndex < aci)
        {
            continue;
        }

        const auto edca = GetStaMac()->GetQosTxop(acIndex);
        const auto backoffEnd =
            GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetBackoffEndFor(edca);
        NS_LOG_DEBUG("Backoff end for " << acIndex
                                        << " on main PHY link: " << backoffEnd.As(Time::US));

        if ((backoffEnd <= now + minDelay) && edca->HasFramesToTransmit(*mainPhyLinkId))
        {
            NS_LOG_DEBUG(acIndex << " is expected to gain access on link " << +mainPhyLinkId.value()
                                 << " sooner than " << aci << " on link " << +linkId);
            return false;
        }
    }

    return true;
}

void
AdvancedEmlsrManager::SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci)
{
    NS_LOG_FUNCTION(this << linkId << aci);

    NS_ASSERT_MSG(!m_auxPhyTxCapable,
                  "This function should only be called if aux PHY is not TX capable");
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    if (mainPhy->IsStateSwitching() && m_mainPhySwitchInfo.to == linkId)
    {
        // the main PHY is switching to the link on which the aux PHY gained a TXOP. This can
        // happen, e.g., if the main PHY was requested to switch to that link before the backoff
        // counter reached zero. Or, this can happen in case of internal collision: the first AC
        // requests the main PHY to switch and the second one finds the main PHY to be switching.
        // In both cases, we do nothing because we have already scheduled the necessary actions
        NS_LOG_DEBUG("Main PHY is already switching to link " << +linkId);
        return;
    }

    if (RequestMainPhyToSwitch(linkId, aci, Time{0}))
    {
        const auto auxPhy = GetStaMac()->GetWifiPhy(linkId);
        const auto pifs = auxPhy->GetSifs() + auxPhy->GetSlot();

        // schedule actions to take based on CCA sensing for a PIFS
        if (m_useAuxPhyCca || GetChannelForAuxPhy(linkId).GetTotalWidth() >=
                                  GetChannelForMainPhy(linkId).GetTotalWidth())
        {
            // use aux PHY CCA in the last PIFS interval before main PHY switch end
            NS_LOG_DEBUG("Schedule CCA check at the end of main PHY switch");
            m_ccaLastPifs = Simulator::Schedule(mainPhy->GetChannelSwitchDelay(),
                                                &AdvancedEmlsrManager::CheckNavAndCcaLastPifs,
                                                this,
                                                auxPhy,
                                                linkId,
                                                GetStaMac()->GetQosTxop(aci));
        }
        else
        {
            // use main PHY CCA in the last PIFS interval after main PHY switch end
            NS_LOG_DEBUG("Schedule CCA check a PIFS after the end of main PHY switch");
            m_ccaLastPifs = Simulator::Schedule(mainPhy->GetChannelSwitchDelay() + pifs,
                                                &AdvancedEmlsrManager::CheckNavAndCcaLastPifs,
                                                this,
                                                mainPhy,
                                                linkId,
                                                GetStaMac()->GetQosTxop(aci));
        }

        // switch main PHY
        Time remNav{0};
        if (const auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy))
        {
            auto mainPhyNavEnd = GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetNavEnd();
            remNav = Max(remNav, mainPhyNavEnd - Simulator::Now());
        }

        SwitchMainPhy(linkId,
                      false,
                      DONT_REQUEST_ACCESS,
                      EmlsrUlTxopAuxPhyNotTxCapableTrace(aci, Time{0}, remNav));

        // if SwitchAuxPhy is false, the main PHY must stay for some time on this link to check if
        // it gets channel access. The timer is stopped if a DL or UL TXOP is started. When the
        // timer expires, the main PHY switches back to the preferred link
        if (!m_switchAuxPhy)
        {
            m_switchMainPhyBackEvent.Cancel();
            m_switchMainPhyBackEvent =
                Simulator::Schedule(mainPhy->GetChannelSwitchDelay() + m_switchMainPhyBackDelay,
                                    &AdvancedEmlsrManager::SwitchMainPhyBackDelayExpired,
                                    this,
                                    linkId,
                                    std::nullopt);
            // start checking PHY activity on the link the main PHY is switching to
            RegisterListener(auxPhy);
        }
        return;
    }

    // Determine if and when we need to request channel access again for the aux PHY based on
    // the main PHY state.
    // Note that, if we have requested the main PHY to switch (above), the function has returned
    // and the EHT FEM will start a TXOP if medium is idle for a PIFS interval preceding/following
    // the end of the main PHY channel switch.
    // If the main PHY has been requested to switch by another aux PHY, this aux PHY will request
    // channel access again when we have completed the CCA assessment on the other link.
    // If the state is switching, CCA_BUSY or RX, then we request channel access again for the
    // aux PHY when the main PHY state is back to IDLE.
    // If the state is TX, it means that the main PHY is involved in a TXOP. Do nothing because
    // the channel access will be requested when unblocking links at the end of the TXOP.
    // If the state is IDLE, then either no AC has traffic to send or the backoff on the link
    // of the main PHY is shorter than the channel switch delay. In the former case, do
    // nothing because channel access will be triggered when new packets arrive; in the latter
    // case, do nothing because the main PHY will start a TXOP and at the end of such TXOP
    // links will be unblocked and the channel access requested on all links

    std::optional<Time> delay;

    if (m_ccaLastPifs.IsPending() || m_switchMainPhyBackEvent.IsPending())
    {
        delay = std::max(Simulator::GetDelayLeft(m_ccaLastPifs),
                         Simulator::GetDelayLeft(m_switchMainPhyBackEvent));
    }
    else if (mainPhy->GetState()->GetLastTime(
                 {WifiPhyState::SWITCHING, WifiPhyState::CCA_BUSY, WifiPhyState::RX}) ==
             Simulator::Now())
    {
        delay = mainPhy->GetDelayUntilIdle();
    }

    NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState());
    auto edca = GetStaMac()->GetQosTxop(aci);
    edca->NotifyChannelReleased(linkId); // to set access to NOT_REQUESTED

    if (!delay.has_value())
    {
        NS_LOG_DEBUG("Do nothing");
        return;
    }

    NS_LOG_DEBUG("Schedule channel access request on link "
                 << +linkId << " at time " << (Simulator::Now() + *delay).As(Time::NS));
    Simulator::Schedule(*delay, [=]() {
        edca->StartAccessAfterEvent(linkId,
                                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);
    });
}

void
AdvancedEmlsrManager::SwitchMainPhyIfTxopToBeGainedByAuxPhy(uint8_t linkId,
                                                            AcIndex aci,
                                                            const Time& delay)
{
    NS_LOG_FUNCTION(this << linkId << aci << delay.As(Time::US));

    if (m_auxPhyTxCapable)
    {
        NS_LOG_DEBUG("Nothing to do if aux PHY is TX capable");
        return;
    }

    if (!delay.IsStrictlyPositive())
    {
        NS_LOG_DEBUG("Do nothing if delay is not strictly positive");
        return;
    }

    if (GetEhtFem(linkId)->UsingOtherEmlsrLink())
    {
        NS_LOG_DEBUG("Do nothing because another EMLSR link is being used");
        return;
    }

    if (!DoGetDelayUntilAccessRequest(linkId).first)
    {
        NS_LOG_DEBUG("Do nothing because a frame is being received on another EMLSR link");
        return;
    }

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    auto phy = GetStaMac()->GetWifiPhy(linkId);

    if (!phy || phy == mainPhy)
    {
        NS_LOG_DEBUG("No aux PHY is operating on link " << +linkId);
        return;
    }

    if (!RequestMainPhyToSwitch(linkId, aci, delay))
    {
        NS_LOG_DEBUG("Chosen not to request the main PHY to switch");
        if (const auto untilIdle = mainPhy->GetDelayUntilIdle();
            untilIdle.IsStrictlyPositive() && untilIdle < delay)
        {
            NS_LOG_DEBUG("Retrying in " << untilIdle.As(Time::US));
            Simulator::Schedule(untilIdle,
                                &AdvancedEmlsrManager::SwitchMainPhyIfTxopToBeGainedByAuxPhy,
                                this,
                                linkId,
                                aci,
                                delay - untilIdle);
        }
        return;
    }

    // switch main PHY

    // use aux PHY CCA (if allowed) if the backoff has already counted down to zero on the aux PHY
    // link when the main PHY completes the switch
    const auto edca = GetStaMac()->GetQosTxop(aci);
    const auto auxPhy = GetStaMac()->GetWifiPhy(linkId);
    const auto switchDelay = mainPhy->GetChannelSwitchDelay();
    const auto auxPhyCcaCanBeUsed =
        m_useAuxPhyCca || (GetChannelForAuxPhy(linkId).GetTotalWidth() >=
                           GetChannelForMainPhy(linkId).GetTotalWidth());

    // check expected channel access delay when switch is completed
    Simulator::Schedule(switchDelay, [=, this]() {
        // this is scheduled before starting the main PHY switch, hence it is executed before the
        // main PHY is connected to the aux PHY link

        if (!m_switchAuxPhy && !m_switchMainPhyBackEvent.IsPending())
        {
            // if SwitchAuxPhy is false and the switch main PHY back timer is not running, it means
            // that the channel switch was interrupted, hence there is nothing to check
            return;
        }

        const auto backoffEnd =
            GetStaMac()->GetChannelAccessManager(linkId)->GetBackoffEndFor(edca);
        const auto pifs = GetStaMac()->GetWifiPhy(linkId)->GetPifs();
        const auto now = Simulator::Now();

        // In case aux PHY CCA can be used and the backoff has not yet reached zero, no NAV and CCA
        // check is needed. The channel width that will be used is the width of the aux PHY if less
        // than a PIFS remains until the backoff reaches zero, and the width of the main PHY,
        // otherwise. If aux PHY CCA can be used and the backoff has already reached zero, a NAV and
        // CCA check is needed.

        if (auxPhyCcaCanBeUsed && backoffEnd < now)
        {
            /**
             * use aux PHY CCA in the last PIFS interval before main PHY switch end
             *
             *        Backoff    Switch
             *          end     end (now)
             * ──────────┴─────────┴──────────
             *      |---- PIFS ----|
             */
            CheckNavAndCcaLastPifs(auxPhy, linkId, edca);
        }
        else if (!auxPhyCcaCanBeUsed && (backoffEnd - now <= pifs))
        {
            /**
             * the remaining backoff time (if any) when the main PHY completes the switch is shorter
             * than or equal to a PIFS, thus the main PHY performs CCA in the last PIFS interval
             * after switch end
             *
             *        Switch    Backoff                 Backoff    Switch
             *       end (now)    end                     end     end (now)
             * ──────────┴─────────┴──────────   ──────────┴─────────┴──────────
             *           |---- PIFS ----|                            |---- PIFS ----|
             */
            NS_LOG_DEBUG("Schedule CCA check a PIFS after the end of main PHY switch");
            m_ccaLastPifs = Simulator::Schedule(pifs,
                                                &AdvancedEmlsrManager::CheckNavAndCcaLastPifs,
                                                this,
                                                mainPhy,
                                                linkId,
                                                edca);
        }
        else if (WifiExpectedAccessReason reason;
                 !m_switchAuxPhy &&
                 (reason = GetStaMac()->GetChannelAccessManager(linkId)->GetExpectedAccessWithin(
                      Simulator::GetDelayLeft(m_switchMainPhyBackEvent) +
                      mainPhy->GetChannelSwitchDelay())) !=
                     WifiExpectedAccessReason::ACCESS_EXPECTED)
        {
            NS_LOG_DEBUG("No AC is expected to get backoff soon, switch main PHY back");
            SwitchMainPhyBackDelayExpired(linkId, reason);
        }
    });

    Time remNav{0};
    if (const auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy))
    {
        auto mainPhyNavEnd = GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetNavEnd();
        remNav = Max(remNav, mainPhyNavEnd - Simulator::Now());
    }

    SwitchMainPhy(linkId,
                  false,
                  DONT_REQUEST_ACCESS,
                  EmlsrUlTxopAuxPhyNotTxCapableTrace(aci, delay, remNav));

    // if SwitchAuxPhy is false, the main PHY must stay for some time on this link to check if it
    // gets channel access. The timer is stopped if a DL or UL TXOP is started. When the timer
    // expires, the main PHY switches back to the preferred link
    if (!m_switchAuxPhy)
    {
        m_switchMainPhyBackEvent.Cancel();
        m_switchMainPhyBackEvent =
            Simulator::Schedule(switchDelay + m_switchMainPhyBackDelay,
                                &AdvancedEmlsrManager::SwitchMainPhyBackDelayExpired,
                                this,
                                linkId,
                                std::nullopt);
        // start checking PHY activity on the link the main PHY is switching to
        RegisterListener(auxPhy);
    }
}

} // namespace ns3
