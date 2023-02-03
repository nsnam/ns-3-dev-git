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
                "EmlsrLinkSet",
                "IDs of the links on which EMLSR mode will be enabled. An empty set "
                "indicates to disable EMLSR.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&EmlsrManager::SetEmlsrLinks),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()));
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
        SendEmlOperatingModeNotification();
    }
}

void
EmlsrManager::NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    const auto& hdr = mpdu->GetHeader();

    DoNotifyMgtFrameReceived(mpdu, linkId);

    if (hdr.IsAssocResp() && GetStaMac()->IsAssociated() && GetTransitionTimeout() &&
        m_nextEmlsrLinks && !m_nextEmlsrLinks->empty())
    {
        // we just completed ML setup with an AP MLD that supports EMLSR and a non-empty
        // set of EMLSR links have been configured, hence enable EMLSR mode on those links
        SendEmlOperatingModeNotification();
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
EmlsrManager::SendEmlOperatingModeNotification()
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_IF(!m_emlsrTransitionTimeout,
                    "AP did not advertise a Transition Timeout, cannot send EML notification");
    NS_ASSERT_MSG(m_nextEmlsrLinks, "Need to set EMLSR links before calling this method");

    MgtEmlOperatingModeNotification frame;

    // Add the EMLSR Parameter Update field if needed
    if (m_lastAdvPaddingDelay != m_emlsrPaddingDelay ||
        m_lastAdvTransitionDelay != m_emlsrTransitionDelay)
    {
        m_lastAdvPaddingDelay = m_emlsrPaddingDelay;
        m_lastAdvTransitionDelay = m_emlsrTransitionDelay;
        frame.m_emlControl.emlsrParamUpdateCtrl = 1;
        frame.m_emlsrParamUpdate = MgtEmlOperatingModeNotification::EmlsrParamUpdate{};
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
            auto apLinkId = m_staMac->GetApLinkId(*emlsrLinkIt);
            NS_ASSERT(apLinkId);
            frame.SetLinkIdInBitmap(*apLinkId);
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

    // TODO if this is a single radio non-AP MLD and not all setup links are in the EMLSR link
    // set, we have to put setup links that are not included in the given EMLSR link set (i.e.,
    // those remaining in setupLinkIds, if m_nextEmlsrLinks is not empty) in the sleep mode:
    // For the EMLSR mode enabled in a single radio non-AP MLD, the STA(s) affiliated with
    // the non-AP MLD that operates on the enabled link(s) that corresponds to the bit
    // position(s) of the EMLSR Link Bitmap subfield set to 0 shall be in doze state if a
    // non-AP STA affiliated with the non-AP MLD that operates on one of the EMLSR links is
    // in awake state. (Sec. 35.3.17 of 802.11be D3.0)

    auto linkId = GetLinkToSendEmlNotification();
    GetEhtFem(linkId)->SendEmlOperatingModeNotification(m_staMac->GetBssid(linkId), frame);
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
                MgtEmlOperatingModeNotification frame;
                pkt->RemoveHeader(frame);
                GetEhtFem(*linkId)->SendEmlOperatingModeNotification(m_staMac->GetBssid(*linkId),
                                                                     frame);
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

    // TODO Make other non-AP STAs operating on the corresponding EMLSR links transition to
    // active mode or passive mode (depending on whether EMLSR mode has been enabled or disabled)

    NotifyEmlsrModeChanged();
}

} // namespace ns3
