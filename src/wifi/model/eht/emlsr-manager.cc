/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#include "emlsr-manager.h"

#include "eht-configuration.h"
#include "eht-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/attribute-container.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state-helper.h"

#include <iterator>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(EmlsrManager);

TypeId
EmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EmlsrManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute("EmlsrPaddingDelay",
                          "The EMLSR Paddind Delay (not used by AP MLDs). "
                          "Possible values are 0 us, 32 us, 64 us, 128 us or 256 us.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&EmlsrManager::m_emlsrPaddingDelay),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(256)))
            .AddAttribute("EmlsrTransitionDelay",
                          "The EMLSR Transition Delay (not used by AP MLDs). "
                          "Possible values are 0 us, 16 us, 32 us, 64 us, 128 us or 256 us.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&EmlsrManager::m_emlsrTransitionDelay),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(256)))
            .AddAttribute(
                "MainPhyId",
                "The ID of the main PHY (position in the vector of PHYs held by "
                "WifiNetDevice). This attribute cannot be set after construction.",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                UintegerValue(0),
                MakeUintegerAccessor(&EmlsrManager::SetMainPhyId, &EmlsrManager::GetMainPhyId),
                MakeUintegerChecker<uint8_t>())
            .AddAttribute("AuxPhyChannelWidth",
                          "The maximum channel width (MHz) supported by Aux PHYs. Note that the "
                          "maximum channel width is capped to the maximum channel width supported "
                          "by the configured maximum modulation class supported.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          UintegerValue(20),
                          MakeUintegerAccessor(&EmlsrManager::m_auxPhyMaxWidth),
                          MakeUintegerChecker<uint16_t>(20, 160))
            .AddAttribute("AuxPhyMaxModClass",
                          "The maximum modulation class supported by Aux PHYs. Use "
                          "WIFI_MOD_CLASS_OFDM for non-HT.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          EnumValue(WIFI_MOD_CLASS_OFDM),
                          MakeEnumAccessor<WifiModulationClass>(&EmlsrManager::m_auxPhyMaxModClass),
                          MakeEnumChecker(WIFI_MOD_CLASS_HR_DSSS,
                                          "HR-DSSS",
                                          WIFI_MOD_CLASS_ERP_OFDM,
                                          "ERP-OFDM",
                                          WIFI_MOD_CLASS_OFDM,
                                          "OFDM",
                                          WIFI_MOD_CLASS_HT,
                                          "HT",
                                          WIFI_MOD_CLASS_VHT,
                                          "VHT",
                                          WIFI_MOD_CLASS_HE,
                                          "HE",
                                          WIFI_MOD_CLASS_EHT,
                                          "EHT"))
            .AddAttribute("AuxPhyTxCapable",
                          "Whether Aux PHYs are capable of transmitting PPDUs.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&EmlsrManager::SetAuxPhyTxCapable,
                                              &EmlsrManager::GetAuxPhyTxCapable),
                          MakeBooleanChecker())
            .AddAttribute(
                "EmlsrLinkSet",
                "IDs of the links on which EMLSR mode will be enabled. An empty set "
                "indicates to disable EMLSR.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&EmlsrManager::SetEmlsrLinks),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()))
            .AddAttribute("ResetCamState",
                          "Whether to reset the state of the ChannelAccessManager associated with "
                          "the link on which the main PHY has just switched to.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&EmlsrManager::SetCamStateReset,
                                              &EmlsrManager::GetCamStateReset),
                          MakeBooleanChecker());
    return tid;
}

EmlsrManager::EmlsrManager()
    // The STA initializes dot11MSDTimerDuration to aPPDUMaxTime defined in Table 36-70
    // (Sec. 35.3.16.8.1 of 802.11be D3.1)
    : m_mediumSyncDuration(MicroSeconds(DEFAULT_MSD_DURATION_USEC)),
      // The default value of dot11MSDOFDMEDthreshold is –72 dBm and the default value of
      // dot11MSDTXOPMax is 1, respectively (Sec. 35.3.16.8.1 of 802.11be D3.1)
      m_msdOfdmEdThreshold(DEFAULT_MSD_OFDM_ED_THRESH),
      m_msdMaxNTxops(DEFAULT_MSD_MAX_N_TXOPS)
{
    NS_LOG_FUNCTION(this);
}

EmlsrManager::~EmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
EmlsrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_staMac->TraceDisconnectWithoutContext("AckedMpdu", MakeCallback(&EmlsrManager::TxOk, this));
    m_staMac->TraceDisconnectWithoutContext("DroppedMpdu",
                                            MakeCallback(&EmlsrManager::TxDropped, this));
    m_staMac = nullptr;
    m_transitionTimeoutEvent.Cancel();
    for (auto& [id, status] : m_mediumSyncDelayStatus)
    {
        status.timer.Cancel();
    }
    Object::DoDispose();
}

void
EmlsrManager::SetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ASSERT(mac);
    m_staMac = mac;

    NS_ABORT_MSG_IF(!m_staMac->GetEhtConfiguration(), "EmlsrManager requires EHT support");
    NS_ABORT_MSG_IF(m_staMac->GetNLinks() <= 1, "EmlsrManager can only be installed on MLDs");
    NS_ABORT_MSG_IF(m_staMac->GetTypeOfStation() != STA,
                    "EmlsrManager can only be installed on non-AP MLDs");

    m_staMac->TraceConnectWithoutContext("AckedMpdu", MakeCallback(&EmlsrManager::TxOk, this));
    m_staMac->TraceConnectWithoutContext("DroppedMpdu",
                                         MakeCallback(&EmlsrManager::TxDropped, this));
}

void
EmlsrManager::SetMainPhyId(uint8_t mainPhyId)
{
    NS_LOG_FUNCTION(this << mainPhyId);
    NS_ABORT_MSG_IF(IsInitialized(), "Cannot be called once this object has been initialized");
    m_mainPhyId = mainPhyId;
}

uint8_t
EmlsrManager::GetMainPhyId() const
{
    return m_mainPhyId;
}

void
EmlsrManager::SetCamStateReset(bool enable)
{
    m_resetCamState = enable;
}

bool
EmlsrManager::GetCamStateReset() const
{
    return m_resetCamState;
}

void
EmlsrManager::SetAuxPhyTxCapable(bool capable)
{
    m_auxPhyTxCapable = capable;
}

bool
EmlsrManager::GetAuxPhyTxCapable() const
{
    return m_auxPhyTxCapable;
}

const std::set<uint8_t>&
EmlsrManager::GetEmlsrLinks() const
{
    return m_emlsrLinks;
}

Ptr<StaWifiMac>
EmlsrManager::GetStaMac() const
{
    return m_staMac;
}

Ptr<EhtFrameExchangeManager>
EmlsrManager::GetEhtFem(uint8_t linkId) const
{
    return StaticCast<EhtFrameExchangeManager>(m_staMac->GetFrameExchangeManager(linkId));
}

std::optional<Time>
EmlsrManager::GetElapsedMediumSyncDelayTimer(uint8_t linkId) const
{
    if (const auto statusIt = m_mediumSyncDelayStatus.find(linkId);
        statusIt != m_mediumSyncDelayStatus.cend() && statusIt->second.timer.IsPending())
    {
        return m_mediumSyncDuration - Simulator::GetDelayLeft(statusIt->second.timer);
    }
    return std::nullopt;
}

void
EmlsrManager::SetTransitionTimeout(Time timeout)
{
    NS_LOG_FUNCTION(this << timeout.As(Time::US));
    m_emlsrTransitionTimeout = timeout;
}

std::optional<Time>
EmlsrManager::GetTransitionTimeout() const
{
    return m_emlsrTransitionTimeout;
}

void
EmlsrManager::SetMediumSyncDuration(Time duration)
{
    NS_LOG_FUNCTION(this << duration.As(Time::US));
    m_mediumSyncDuration = duration;
}

Time
EmlsrManager::GetMediumSyncDuration() const
{
    return m_mediumSyncDuration;
}

void
EmlsrManager::SetMediumSyncOfdmEdThreshold(int8_t threshold)
{
    NS_LOG_FUNCTION(this << threshold);
    m_msdOfdmEdThreshold = threshold;
}

int8_t
EmlsrManager::GetMediumSyncOfdmEdThreshold() const
{
    return m_msdOfdmEdThreshold;
}

void
EmlsrManager::SetMediumSyncMaxNTxops(std::optional<uint8_t> nTxops)
{
    NS_LOG_FUNCTION(this << nTxops.has_value());
    m_msdMaxNTxops = nTxops;
}

std::optional<uint8_t>
EmlsrManager::GetMediumSyncMaxNTxops() const
{
    return m_msdMaxNTxops;
}

void
EmlsrManager::SetEmlsrLinks(const std::set<uint8_t>& linkIds)
{
    std::stringstream ss;
    if (g_log.IsEnabled(ns3::LOG_FUNCTION))
    {
        std::copy(linkIds.cbegin(), linkIds.cend(), std::ostream_iterator<uint16_t>(ss, " "));
    }
    NS_LOG_FUNCTION(this << ss.str());
    NS_ABORT_MSG_IF(linkIds.size() == 1, "Cannot enable EMLSR mode on a single link");

    if (linkIds != m_emlsrLinks)
    {
        m_nextEmlsrLinks = linkIds;
    }

    if (GetStaMac() && GetStaMac()->IsAssociated() && GetTransitionTimeout() && m_nextEmlsrLinks)
    {
        // Request to enable EMLSR mode on the given links, provided that they have been setup
        SendEmlOmn();
    }
}

void
EmlsrManager::NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    const auto& hdr = mpdu->GetHeader();

    DoNotifyMgtFrameReceived(mpdu, linkId);

    if (hdr.IsAssocResp() && GetStaMac()->IsAssociated() && GetTransitionTimeout())
    {
        // we just completed ML setup with an AP MLD that supports EMLSR
        ComputeOperatingChannels();

        if (m_nextEmlsrLinks && !m_nextEmlsrLinks->empty())
        {
            // a non-empty set of EMLSR links have been configured, hence enable EMLSR mode
            // on those links
            SendEmlOmn();
        }
    }

    if (hdr.IsAction() && hdr.GetAddr2() == m_staMac->GetBssid(linkId))
    {
        // this is an action frame sent by an AP of the AP MLD we are associated with
        auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
        if (category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            if (m_transitionTimeoutEvent.IsPending())
            {
                // no need to wait until the expiration of the transition timeout
                m_transitionTimeoutEvent.PeekEventImpl()->Invoke();
                m_transitionTimeoutEvent.Cancel();
            }
        }
    }
}

void
EmlsrManager::NotifyIcfReceived(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    NS_ASSERT(m_staMac->IsEmlsrLink(linkId));

    // block transmissions and suspend medium access on all other EMLSR links
    for (auto id : m_staMac->GetLinkIds())
    {
        if (id != linkId && m_staMac->IsEmlsrLink(id))
        {
            m_staMac->BlockTxOnLink(id, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
            m_staMac->GetChannelAccessManager(id)->NotifyStartUsingOtherEmlsrLink();
        }
    }

    auto mainPhy = m_staMac->GetDevice()->GetPhy(m_mainPhyId);
    auto auxPhy = m_staMac->GetWifiPhy(linkId);

    if (m_staMac->GetWifiPhy(linkId) == mainPhy)
    {
        // nothing to do, we received an ICF from the main PHY
        return;
    }

    Simulator::ScheduleNow([=, this]() {
        SwitchMainPhy(linkId,
                      true, // channel switch should occur instantaneously
                      RESET_BACKOFF,
                      DONT_REQUEST_ACCESS);

        // aux PHY received the ICF but main PHY will send the response
        auto uid = auxPhy->GetPreviouslyRxPpduUid();
        mainPhy->SetPreviouslyRxPpduUid(uid);

        DoNotifyIcfReceived(linkId);
    });
}

void
EmlsrManager::NotifyUlTxopStart(uint8_t linkId, std::optional<Time> timeToCtsEnd)
{
    NS_LOG_FUNCTION(this << linkId);

    if (!m_staMac->IsEmlsrLink(linkId))
    {
        NS_LOG_DEBUG("EMLSR is not enabled on link " << +linkId);
        return;
    }

    // block transmissions and suspend medium access on all other EMLSR links
    for (auto id : m_staMac->GetLinkIds())
    {
        if (id != linkId && m_staMac->IsEmlsrLink(id))
        {
            m_staMac->BlockTxOnLink(id, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
            m_staMac->GetChannelAccessManager(id)->NotifyStartUsingOtherEmlsrLink();
        }
    }

    // if this TXOP is being started by an aux PHY, schedule a channel switch for the main PHY
    // such that the channel switch is completed by the time the CTS response is received. The
    // delay has been passed by the FEM.
    if (m_staMac->GetLinkForPhy(m_mainPhyId) != linkId)
    {
        auto stateHelper = m_staMac->GetWifiPhy(linkId)->GetState();
        NS_ASSERT(stateHelper);
        NS_ASSERT_MSG(stateHelper->GetState() == WifiPhyState::TX,
                      "Expecting the aux PHY to be transmitting (an RTS frame)");
        NS_ASSERT_MSG(timeToCtsEnd.has_value(),
                      "Aux PHY is sending RTS, expected to get the time to CTS end");

        auto mainPhy = m_staMac->GetDevice()->GetPhy(m_mainPhyId);

        // the main PHY shall terminate the channel switch at the end of CTS reception;
        // the time remaining to the end of CTS reception includes two propagation delays
        const auto delay = *timeToCtsEnd - mainPhy->GetChannelSwitchDelay();

        NS_ASSERT(delay.IsPositive());
        NS_LOG_DEBUG("Schedule main Phy switch in " << delay.As(Time::US));
        m_ulMainPhySwitch[linkId] = Simulator::Schedule(delay,
                                                        &EmlsrManager::SwitchMainPhy,
                                                        this,
                                                        linkId,
                                                        false,
                                                        RESET_BACKOFF,
                                                        DONT_REQUEST_ACCESS);
    }

    DoNotifyUlTxopStart(linkId);
}

void
EmlsrManager::NotifyTxopEnd(uint8_t linkId, bool ulTxopNotStarted, bool ongoingDlTxop)
{
    NS_LOG_FUNCTION(this << linkId << ulTxopNotStarted << ongoingDlTxop);

    if (!m_staMac->IsEmlsrLink(linkId))
    {
        NS_LOG_DEBUG("EMLSR is not enabled on link " << +linkId);
        return;
    }

    // If the main PHY has been scheduled to switch to this link, cancel the channel switch.
    // This happens, e.g., when an aux PHY sent an RTS to start an UL TXOP but it did not
    // receive a CTS response.
    if (auto it = m_ulMainPhySwitch.find(linkId); it != m_ulMainPhySwitch.end())
    {
        if (it->second.IsPending())
        {
            NS_LOG_DEBUG("Cancelling main PHY channel switch event on link " << +linkId);
            it->second.Cancel();
        }
        m_ulMainPhySwitch.erase(it);
    }

    // Unblock the other EMLSR links and start the MediumSyncDelay timer, provided that the TXOP
    // included the transmission of at least a frame and there is no ongoing DL TXOP on this link.
    // Indeed, the UL TXOP may have ended because the transmission of a frame failed and the
    // corresponding TX timeout (leading to this call) may have occurred after the reception on
    // this link of an ICF starting a DL TXOP. If the EMLSR Manager unblocked the other EMLSR
    // links, another TXOP could be started on another EMLSR link (possibly leading to a crash)
    // while the DL TXOP on this link is ongoing.
    if (ongoingDlTxop)
    {
        NS_LOG_DEBUG("DL TXOP ongoing");
        return;
    }
    if (ulTxopNotStarted)
    {
        NS_LOG_DEBUG("TXOP did not even start");
        return;
    }

    DoNotifyTxopEnd(linkId);

    Simulator::ScheduleNow([=, this]() {
        // unblock transmissions and resume medium access on other EMLSR links
        std::set<uint8_t> linkIds;
        for (auto id : m_staMac->GetLinkIds())
        {
            if ((id != linkId) && m_staMac->IsEmlsrLink(id))
            {
                m_staMac->GetChannelAccessManager(id)->NotifyStopUsingOtherEmlsrLink();
                linkIds.insert(id);
            }
        }
        m_staMac->UnblockTxOnLink(linkIds, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);

        StartMediumSyncDelayTimer(linkId);
    });
}

void
EmlsrManager::SetCcaEdThresholdOnLinkSwitch(Ptr<WifiPhy> phy, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << phy << linkId);

    // if a MediumSyncDelay timer is running for the link on which the main PHY is going to
    // operate, set the CCA ED threshold to the MediumSyncDelay OFDM ED threshold
    if (auto statusIt = m_mediumSyncDelayStatus.find(linkId);
        statusIt != m_mediumSyncDelayStatus.cend() && statusIt->second.timer.IsPending())
    {
        NS_LOG_DEBUG("Setting CCA ED threshold of PHY " << phy << " to " << +m_msdOfdmEdThreshold
                                                        << " on link " << +linkId);

        // store the current CCA ED threshold in the m_prevCcaEdThreshold map, if not present
        m_prevCcaEdThreshold.try_emplace(phy, phy->GetCcaEdThreshold());

        phy->SetCcaEdThreshold(m_msdOfdmEdThreshold);
    }
    // otherwise, restore the previous value for the CCA ED threshold (if any)
    else if (auto threshIt = m_prevCcaEdThreshold.find(phy);
             threshIt != m_prevCcaEdThreshold.cend())
    {
        NS_LOG_DEBUG("Resetting CCA ED threshold of PHY " << phy << " to " << threshIt->second
                                                          << " on link " << +linkId);
        phy->SetCcaEdThreshold(threshIt->second);
        m_prevCcaEdThreshold.erase(threshIt);
    }
}

void
EmlsrManager::SwitchMainPhy(uint8_t linkId,
                            bool noSwitchDelay,
                            bool resetBackoff,
                            bool requestAccess)
{
    NS_LOG_FUNCTION(this << linkId << noSwitchDelay << resetBackoff << requestAccess);

    auto mainPhy = m_staMac->GetDevice()->GetPhy(m_mainPhyId);

    NS_ASSERT_MSG(mainPhy != m_staMac->GetWifiPhy(linkId),
                  "Main PHY is already operating on link " << +linkId);

    if (mainPhy->IsStateSwitching())
    {
        NS_LOG_DEBUG("Main PHY is already switching, ignore new switching request");
        return;
    }

    // find the link on which the main PHY is operating
    auto currMainPhyLinkId = m_staMac->GetLinkForPhy(mainPhy);
    NS_ASSERT_MSG(currMainPhyLinkId, "Current link ID for main PHY not found");

    auto newMainPhyChannel = GetChannelForMainPhy(linkId);

    NS_LOG_DEBUG("Main PHY (" << mainPhy << ") is about to switch to " << newMainPhyChannel
                              << " to operate on link " << +linkId);

    // notify the channel access manager of the upcoming channel switch(es)
    m_staMac->GetChannelAccessManager(*currMainPhyLinkId)
        ->NotifySwitchingEmlsrLink(mainPhy, newMainPhyChannel, linkId);

    // this assert also ensures that the actual channel switch is not delayed
    NS_ASSERT_MSG(!mainPhy->GetState()->IsStateTx(),
                  "We should not ask the main PHY to switch channel while transmitting");

    // request the main PHY to switch channel
    const auto delay = mainPhy->GetChannelSwitchDelay();
    const auto pifs = mainPhy->GetSifs() + mainPhy->GetSlot();
    NS_ASSERT_MSG(noSwitchDelay || delay <= std::max(m_lastAdvTransitionDelay, pifs),
                  "Channel switch delay ("
                      << delay.As(Time::US)
                      << ") should be shorter than the maximum between the Transition delay ("
                      << m_lastAdvTransitionDelay.As(Time::US) << ") and a PIFS ("
                      << pifs.As(Time::US) << ")");
    if (noSwitchDelay)
    {
        mainPhy->SetAttribute("ChannelSwitchDelay", TimeValue(Seconds(0)));
    }
    mainPhy->SetOperatingChannel(newMainPhyChannel);
    // restore previous channel switch delay
    if (noSwitchDelay)
    {
        mainPhy->SetAttribute("ChannelSwitchDelay", TimeValue(delay));
    }
    // re-enable short time slot, if needed
    if (m_staMac->GetWifiRemoteStationManager(linkId)->GetShortSlotTimeEnabled())
    {
        mainPhy->SetSlot(MicroSeconds(9));
    }

    if (resetBackoff)
    {
        // reset the backoffs on the link left by the main PHY
        m_staMac->GetChannelAccessManager(*currMainPhyLinkId)->ResetAllBackoffs();
    }

    const auto timeToSwitchEnd = noSwitchDelay ? Seconds(0) : mainPhy->GetChannelSwitchDelay();

    if (requestAccess)
    {
        // schedule channel access request on the new link when switch is completed
        Simulator::Schedule(timeToSwitchEnd, [=, this]() {
            for (const auto& [acIndex, ac] : wifiAcList)
            {
                m_staMac->GetQosTxop(acIndex)->StartAccessAfterEvent(
                    linkId,
                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                    Txop::CHECK_MEDIUM_BUSY);
            }
        });
    }

    SetCcaEdThresholdOnLinkSwitch(mainPhy, linkId);
    NotifyMainPhySwitch(*currMainPhyLinkId, linkId);
}

void
EmlsrManager::SwitchAuxPhy(uint8_t currLinkId, uint8_t nextLinkId)
{
    NS_LOG_FUNCTION(this << currLinkId << nextLinkId);

    auto auxPhy = GetStaMac()->GetWifiPhy(currLinkId);

    auto newAuxPhyChannel = GetChannelForAuxPhy(nextLinkId);

    NS_LOG_DEBUG("Aux PHY (" << auxPhy << ") is about to switch to " << newAuxPhyChannel
                             << " to operate on link " << +nextLinkId);

    GetStaMac()
        ->GetChannelAccessManager(currLinkId)
        ->NotifySwitchingEmlsrLink(auxPhy, newAuxPhyChannel, nextLinkId);

    auxPhy->SetOperatingChannel(newAuxPhyChannel);
    // re-enable short time slot, if needed
    if (m_staMac->GetWifiRemoteStationManager(nextLinkId)->GetShortSlotTimeEnabled())
    {
        auxPhy->SetSlot(MicroSeconds(9));
    }

    // schedule channel access request on the new link when switch is completed
    Simulator::Schedule(auxPhy->GetChannelSwitchDelay(), [=, this]() {
        for (const auto& [acIndex, ac] : wifiAcList)
        {
            m_staMac->GetQosTxop(acIndex)->StartAccessAfterEvent(
                nextLinkId,
                Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                Txop::CHECK_MEDIUM_BUSY);
        }
    });

    SetCcaEdThresholdOnLinkSwitch(auxPhy, nextLinkId);
}

void
EmlsrManager::StartMediumSyncDelayTimer(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // iterate over all the other EMLSR links
    for (auto id : m_staMac->GetLinkIds())
    {
        if (id != linkId && m_staMac->IsEmlsrLink(id))
        {
            const auto [it, inserted] = m_mediumSyncDelayStatus.try_emplace(id);

            // reset the max number of TXOP attempts
            it->second.msdNTxopsLeft = m_msdMaxNTxops;

            // there are cases in which no PHY is operating on a link; e.g., the main PHY starts
            // switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
            // is not received and the UL TXOP ends before the main PHY channel switch is
            // completed. The MSD timer is started on the link left "uncovered" by the main PHY
            if (auto phy = m_staMac->GetWifiPhy(id); phy && !it->second.timer.IsPending())
            {
                NS_LOG_DEBUG("Setting CCA ED threshold on link "
                             << +id << " to " << +m_msdOfdmEdThreshold << " PHY " << phy);
                m_prevCcaEdThreshold[phy] = phy->GetCcaEdThreshold();
                phy->SetCcaEdThreshold(m_msdOfdmEdThreshold);
            }

            // (re)start the timer
            it->second.timer.Cancel();
            it->second.timer = Simulator::Schedule(m_mediumSyncDuration,
                                                   &EmlsrManager::MediumSyncDelayTimerExpired,
                                                   this,
                                                   id);
        }
    }
}

void
EmlsrManager::CancelMediumSyncDelayTimer(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto timerIt = m_mediumSyncDelayStatus.find(linkId);

    NS_ASSERT(timerIt != m_mediumSyncDelayStatus.cend() && timerIt->second.timer.IsPending());

    timerIt->second.timer.Cancel();
    MediumSyncDelayTimerExpired(linkId);
}

void
EmlsrManager::MediumSyncDelayTimerExpired(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto timerIt = m_mediumSyncDelayStatus.find(linkId);

    NS_ASSERT(timerIt != m_mediumSyncDelayStatus.cend() && !timerIt->second.timer.IsPending());

    // reset the MSD OFDM ED threshold
    auto phy = m_staMac->GetWifiPhy(linkId);

    if (!phy)
    {
        // no PHY is operating on this link. This may happen when a MediumSyncDelay timer expires
        // on the link left "uncovered" by the main PHY that is operating on another link (and the
        // aux PHY of that link did not switch). In this case, do nothing, since the CCA ED
        // threshold on the main PHY will be restored once the main PHY switches back to its link
        return;
    }

    auto threshIt = m_prevCcaEdThreshold.find(phy);
    NS_ASSERT_MSG(threshIt != m_prevCcaEdThreshold.cend(),
                  "No value to restore for CCA ED threshold on PHY " << phy);
    NS_LOG_DEBUG("Resetting CCA ED threshold of PHY " << phy << " to " << threshIt->second
                                                      << " on link " << +linkId);
    phy->SetCcaEdThreshold(threshIt->second);
    m_prevCcaEdThreshold.erase(threshIt);
}

void
EmlsrManager::DecrementMediumSyncDelayNTxops(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    const auto timerIt = m_mediumSyncDelayStatus.find(linkId);

    NS_ASSERT(timerIt != m_mediumSyncDelayStatus.cend() && timerIt->second.timer.IsPending());
    NS_ASSERT(timerIt->second.msdNTxopsLeft != 0);

    if (timerIt->second.msdNTxopsLeft)
    {
        --timerIt->second.msdNTxopsLeft.value();
    }
}

void
EmlsrManager::ResetMediumSyncDelayNTxops(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto timerIt = m_mediumSyncDelayStatus.find(linkId);

    NS_ASSERT(timerIt != m_mediumSyncDelayStatus.cend() && timerIt->second.timer.IsPending());
    timerIt->second.msdNTxopsLeft.reset();
}

bool
EmlsrManager::MediumSyncDelayNTxopsExceeded(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto timerIt = m_mediumSyncDelayStatus.find(linkId);

    NS_ASSERT(timerIt != m_mediumSyncDelayStatus.cend() && timerIt->second.timer.IsPending());
    return timerIt->second.msdNTxopsLeft == 0;
}

MgtEmlOmn
EmlsrManager::GetEmlOmn()
{
    MgtEmlOmn frame;

    // Add the EMLSR Parameter Update field if needed
    if (m_lastAdvPaddingDelay != m_emlsrPaddingDelay ||
        m_lastAdvTransitionDelay != m_emlsrTransitionDelay)
    {
        m_lastAdvPaddingDelay = m_emlsrPaddingDelay;
        m_lastAdvTransitionDelay = m_emlsrTransitionDelay;
        frame.m_emlControl.emlsrParamUpdateCtrl = 1;
        frame.m_emlsrParamUpdate = MgtEmlOmn::EmlsrParamUpdate{};
        frame.m_emlsrParamUpdate->paddingDelay =
            CommonInfoBasicMle::EncodeEmlsrPaddingDelay(m_lastAdvPaddingDelay);
        frame.m_emlsrParamUpdate->transitionDelay =
            CommonInfoBasicMle::EncodeEmlsrTransitionDelay(m_lastAdvTransitionDelay);
    }

    // We must verify that the links included in the given EMLSR link set (if any) have been setup.
    auto setupLinkIds = m_staMac->GetSetupLinkIds();

    for (auto emlsrLinkIt = m_nextEmlsrLinks->begin(); emlsrLinkIt != m_nextEmlsrLinks->end();)
    {
        if (auto setupLinkIt = setupLinkIds.find(*emlsrLinkIt); setupLinkIt != setupLinkIds.cend())
        {
            setupLinkIds.erase(setupLinkIt);
            frame.SetLinkIdInBitmap(*emlsrLinkIt);
            emlsrLinkIt++;
        }
        else
        {
            NS_LOG_DEBUG("Link ID " << +(*emlsrLinkIt) << " has not been setup");
            emlsrLinkIt = m_nextEmlsrLinks->erase(emlsrLinkIt);
        }
    }

    // EMLSR Mode is enabled if and only if the set of EMLSR links is not empty
    frame.m_emlControl.emlsrMode = m_nextEmlsrLinks->empty() ? 0 : 1;

    return frame;
}

void
EmlsrManager::SendEmlOmn()
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_IF(!m_emlsrTransitionTimeout,
                    "AP did not advertise a Transition Timeout, cannot send EML notification");
    NS_ASSERT_MSG(m_nextEmlsrLinks, "Need to set EMLSR links before calling this method");

    // TODO if this is a single radio non-AP MLD and not all setup links are in the EMLSR link
    // set, we have to put setup links that are not included in the given EMLSR link set (i.e.,
    // those remaining in setupLinkIds, if m_nextEmlsrLinks is not empty) in the sleep mode:
    // For the EMLSR mode enabled in a single radio non-AP MLD, the STA(s) affiliated with
    // the non-AP MLD that operates on the enabled link(s) that corresponds to the bit
    // position(s) of the EMLSR Link Bitmap subfield set to 0 shall be in doze state if a
    // non-AP STA affiliated with the non-AP MLD that operates on one of the EMLSR links is
    // in awake state. (Sec. 35.3.17 of 802.11be D3.0)

    auto frame = GetEmlOmn();
    auto linkId = GetLinkToSendEmlOmn();
    GetEhtFem(linkId)->SendEmlOmn(m_staMac->GetBssid(linkId), frame);
}

void
EmlsrManager::TxOk(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsAssocReq())
    {
        // store padding delay and transition delay advertised in AssocReq
        MgtAssocRequestHeader assocReq;
        mpdu->GetPacket()->PeekHeader(assocReq);
        auto& mle = assocReq.Get<MultiLinkElement>();
        NS_ASSERT_MSG(mle, "AssocReq should contain a Multi-Link Element");
        m_lastAdvPaddingDelay = mle->GetEmlsrPaddingDelay();
        m_lastAdvTransitionDelay = mle->GetEmlsrTransitionDelay();
    }

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame that we sent has been acknowledged.
            // Start the transition timeout to wait until the request can be made effective
            NS_ASSERT_MSG(m_emlsrTransitionTimeout, "No transition timeout received from AP");
            m_transitionTimeoutEvent = Simulator::Schedule(*m_emlsrTransitionTimeout,
                                                           &EmlsrManager::ChangeEmlsrMode,
                                                           this);
        }
    }
}

void
EmlsrManager::TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << reason << *mpdu);

    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        auto pkt = mpdu->GetPacket()->Copy();
        if (auto [category, action] = WifiActionHeader::Remove(pkt);
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame has been dropped. Ask the subclass
            // whether the frame needs to be resent
            auto linkId = ResendNotification(mpdu);
            if (linkId)
            {
                MgtEmlOmn frame;
                pkt->RemoveHeader(frame);
                GetEhtFem(*linkId)->SendEmlOmn(m_staMac->GetBssid(*linkId), frame);
            }
            else
            {
                m_nextEmlsrLinks.reset();
            }
        }
    }
}

void
EmlsrManager::ChangeEmlsrMode()
{
    NS_LOG_FUNCTION(this);

    // After the successful transmission of the EML Operating Mode Notification frame by the
    // non-AP STA affiliated with the non-AP MLD, the non-AP MLD shall operate in the EMLSR mode
    // and the other non-AP STAs operating on the corresponding EMLSR links shall transition to
    // active mode after the transition delay indicated in the Transition Timeout subfield in the
    // EML Capabilities subfield of the Basic Multi-Link element or immediately after receiving an
    // EML Operating Mode Notification frame from one of the APs operating on the EMLSR links and
    // affiliated with the AP MLD. (Sec. 35.3.17 of 802.11be D3.0)
    NS_ASSERT_MSG(m_nextEmlsrLinks, "No set of EMLSR links stored");
    m_emlsrLinks.swap(*m_nextEmlsrLinks);
    m_nextEmlsrLinks.reset();

    // Make other non-AP STAs operating on the corresponding EMLSR links transition to
    // active mode or passive mode (depending on whether EMLSR mode has been enabled or disabled)
    m_staMac->NotifyEmlsrModeChanged(m_emlsrLinks);
    // Enforce the limit on the max channel width supported by aux PHYs
    ApplyMaxChannelWidthAndModClassOnAuxPhys();

    NotifyEmlsrModeChanged();
}

void
EmlsrManager::ApplyMaxChannelWidthAndModClassOnAuxPhys()
{
    NS_LOG_FUNCTION(this);
    auto currMainPhyLinkId = m_staMac->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT(currMainPhyLinkId);

    for (const auto linkId : m_staMac->GetLinkIds())
    {
        auto auxPhy = m_staMac->GetWifiPhy(linkId);
        auto channel = GetChannelForAuxPhy(linkId);

        if (linkId == currMainPhyLinkId || !m_staMac->IsEmlsrLink(linkId) ||
            auxPhy->GetOperatingChannel() == channel)
        {
            continue;
        }

        auxPhy->SetMaxModulationClassSupported(m_auxPhyMaxModClass);

        NS_LOG_DEBUG("Aux PHY (" << auxPhy << ") is about to switch to " << channel
                                 << " to operate on link " << +linkId);
        // We cannot simply set the new channel, because otherwise the MAC will disable
        // the setup link. We need to inform the MAC (via the Channel Access Manager) that
        // this channel switch must not have such a consequence. We already have a method
        // for doing so, i.e., inform the MAC that the PHY is switching channel to operate
        // on the "same" link.
        auto cam = m_staMac->GetChannelAccessManager(linkId);
        cam->NotifySwitchingEmlsrLink(auxPhy, channel, linkId);

        auxPhy->SetOperatingChannel(channel);

        // the way the ChannelAccessManager handles EMLSR link switch implies that a PHY listener
        // is removed when the channel switch starts and another one is attached when the channel
        // switch ends. In the meantime, no PHY is connected to the ChannelAccessManager. Thus,
        // reset all backoffs (so that access timeout is also cancelled) when the channel switch
        // starts and request channel access (if needed) when the channel switch ends.
        cam->ResetAllBackoffs();
        Simulator::Schedule(auxPhy->GetChannelSwitchDelay(), [=, this]() {
            for (const auto& [acIndex, ac] : wifiAcList)
            {
                m_staMac->GetQosTxop(acIndex)->StartAccessAfterEvent(
                    linkId,
                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                    Txop::CHECK_MEDIUM_BUSY);
            }
        });
    }
}

void
EmlsrManager::ComputeOperatingChannels()
{
    NS_LOG_FUNCTION(this);

    m_mainPhyChannels.clear();
    m_auxPhyChannels.clear();

    auto linkIds = m_staMac->GetSetupLinkIds();

    for (auto linkId : linkIds)
    {
        const auto& channel = m_staMac->GetWifiPhy(linkId)->GetOperatingChannel();
        m_mainPhyChannels.emplace(linkId, channel);

        auto mainPhyChWidth = channel.GetWidth();
        auto auxPhyMaxWidth =
            std::min(m_auxPhyMaxWidth, GetMaximumChannelWidth(m_auxPhyMaxModClass));
        if (auxPhyMaxWidth >= mainPhyChWidth)
        {
            // same channel can be used by aux PHYs
            m_auxPhyChannels.emplace(linkId, channel);
            continue;
        }
        // aux PHYs will operate on a primary subchannel
        auto freq = channel.GetPrimaryChannelCenterFrequency(auxPhyMaxWidth);
        auto chIt = WifiPhyOperatingChannel::FindFirst(0,
                                                       freq,
                                                       auxPhyMaxWidth,
                                                       WIFI_STANDARD_UNSPECIFIED,
                                                       channel.GetPhyBand());
        NS_ASSERT_MSG(chIt != WifiPhyOperatingChannel::m_frequencyChannels.end(),
                      "Primary" << auxPhyMaxWidth << " channel not found");
        m_auxPhyChannels.emplace(linkId, chIt);
        // find the P20 index for the channel used by the aux PHYs
        auto p20Index = channel.GetPrimaryChannelIndex(20);
        while (mainPhyChWidth > auxPhyMaxWidth)
        {
            mainPhyChWidth /= 2;
            p20Index /= 2;
        }
        m_auxPhyChannels[linkId].SetPrimary20Index(p20Index);
    }
}

const WifiPhyOperatingChannel&
EmlsrManager::GetChannelForMainPhy(uint8_t linkId) const
{
    auto it = m_mainPhyChannels.find(linkId);
    NS_ASSERT_MSG(it != m_mainPhyChannels.end(),
                  "Channel for main PHY on link ID " << +linkId << " not found");
    return it->second;
}

const WifiPhyOperatingChannel&
EmlsrManager::GetChannelForAuxPhy(uint8_t linkId) const
{
    auto it = m_auxPhyChannels.find(linkId);
    NS_ASSERT_MSG(it != m_auxPhyChannels.end(),
                  "Channel for aux PHY on link ID " << +linkId << " not found");
    return it->second;
}

} // namespace ns3
