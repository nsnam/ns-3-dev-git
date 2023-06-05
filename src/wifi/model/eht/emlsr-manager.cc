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
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state-helper.h"

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
                "WifiNetDevice). This attribute cannot be set after initialization.",
                UintegerValue(0),
                MakeUintegerAccessor(&EmlsrManager::SetMainPhyId, &EmlsrManager::GetMainPhyId),
                MakeUintegerChecker<uint8_t>())
            .AddAttribute("AuxPhyChannelWidth",
                          "The maximum channel width (MHz) supported by Aux PHYs",
                          UintegerValue(20),
                          MakeUintegerAccessor(&EmlsrManager::m_auxPhyMaxWidth),
                          MakeUintegerChecker<uint16_t>(20, 160))
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
EmlsrManager::SetEmlsrLinks(const std::set<uint8_t>& linkIds)
{
    NS_LOG_FUNCTION(this);
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
            if (m_transitionTimeoutEvent.IsRunning())
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

    SwitchMainPhy(linkId, true); // channel switch should occur instantaneously

    // aux PHY received the ICF but main PHY will send the response
    auto uid = auxPhy->GetPreviouslyRxPpduUid();
    mainPhy->SetPreviouslyRxPpduUid(uid);
}

void
EmlsrManager::NotifyUlTxopStart(uint8_t linkId)
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

    // if this TXOP is being started by an aux PHY, wait until the end of RTS transmission and
    // then have the main PHY (instantaneously) take over the TXOP on this link. We may start the
    // channel switch now and use the channel switch delay configured for the main PHY, but then
    // we would have no guarantees that the channel switch is completed in RTS TX time plus SIFS.
    if (m_staMac->GetLinkForPhy(m_mainPhyId) != linkId)
    {
        auto stateHelper = m_staMac->GetWifiPhy(linkId)->GetState();
        NS_ASSERT(stateHelper);
        NS_ASSERT_MSG(stateHelper->GetState() == TX,
                      "Expecting the aux PHY to be transmitting (an RTS frame)");
        Simulator::Schedule(stateHelper->GetDelayUntilIdle(),
                            &EmlsrManager::SwitchMainPhy,
                            this,
                            linkId,
                            true); // channel switch should occur instantaneously
    }
}

void
EmlsrManager::NotifyTxopEnd(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (!m_staMac->IsEmlsrLink(linkId))
    {
        NS_LOG_DEBUG("EMLSR is not enabled on link " << +linkId);
        return;
    }

    // unblock transmissions and resume medium access on other EMLSR links
    for (auto id : m_staMac->GetLinkIds())
    {
        if (id != linkId && m_staMac->IsEmlsrLink(id))
        {
            m_staMac->UnblockTxOnLink(id, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
            m_staMac->GetChannelAccessManager(id)->NotifyStopUsingOtherEmlsrLink();
        }
    }
}

void
EmlsrManager::SwitchMainPhy(uint8_t linkId, bool noSwitchDelay)
{
    NS_LOG_FUNCTION(this << linkId << noSwitchDelay);

    auto mainPhy = m_staMac->GetDevice()->GetPhy(m_mainPhyId);

    NS_ASSERT_MSG(mainPhy != m_staMac->GetWifiPhy(linkId),
                  "Main PHY is already operating on link " << +linkId);

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
    Simulator::ScheduleNow([=]() {
        auto delay = mainPhy->GetChannelSwitchDelay();
        NS_ASSERT_MSG(noSwitchDelay || delay <= m_lastAdvTransitionDelay,
                      "Transition delay (" << m_lastAdvTransitionDelay.As(Time::US)
                                           << ") should exceed the channel switch delay ("
                                           << delay.As(Time::US) << ")");
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
    });

    NotifyMainPhySwitch(*currMainPhyLinkId, linkId);
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
    ApplyMaxChannelWidthOnAuxPhys();

    NotifyEmlsrModeChanged();
}

void
EmlsrManager::ApplyMaxChannelWidthOnAuxPhys()
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

        NS_LOG_DEBUG("Aux PHY (" << auxPhy << ") is about to switch to " << channel
                                 << " to operate on link " << +linkId);
        // We cannot simply set the new channel, because otherwise the MAC will disable
        // the setup link. We need to inform the MAC (via the Channel Access Manager) that
        // this channel switch must not have such a consequence. We already have a method
        // for doing so, i.e., inform the MAC that the PHY is switching channel to operate
        // on the "same" link.
        m_staMac->GetChannelAccessManager(linkId)->NotifySwitchingEmlsrLink(auxPhy,
                                                                            channel,
                                                                            linkId);

        void (WifiPhy::*fp)(const WifiPhyOperatingChannel&) = &WifiPhy::SetOperatingChannel;
        Simulator::ScheduleNow(fp, auxPhy, channel);
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
        if (m_auxPhyMaxWidth >= mainPhyChWidth)
        {
            // same channel can be used by aux PHYs
            m_auxPhyChannels.emplace(linkId, channel);
            continue;
        }
        // aux PHYs will operate on a primary subchannel
        auto freq = channel.GetPrimaryChannelCenterFrequency(m_auxPhyMaxWidth);
        auto chIt = WifiPhyOperatingChannel::FindFirst(0,
                                                       freq,
                                                       m_auxPhyMaxWidth,
                                                       WIFI_STANDARD_UNSPECIFIED,
                                                       channel.GetPhyBand());
        NS_ASSERT_MSG(chIt != WifiPhyOperatingChannel::m_frequencyChannels.end(),
                      "Primary" << m_auxPhyMaxWidth << " channel not found");
        m_auxPhyChannels.emplace(linkId, chIt);
        // find the P20 index for the channel used by the aux PHYs
        auto p20Index = channel.GetPrimaryChannelIndex(20);
        while (mainPhyChWidth > m_auxPhyMaxWidth)
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
