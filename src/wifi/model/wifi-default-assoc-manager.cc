/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II

 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-default-assoc-manager.h"

#include "sta-wifi-mac.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/vht-configuration.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiDefaultAssocManager");

NS_OBJECT_ENSURE_REGISTERED(WifiDefaultAssocManager);

TypeId
WifiDefaultAssocManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiDefaultAssocManager")
            .SetParent<WifiAssocManager>()
            .AddConstructor<WifiDefaultAssocManager>()
            .SetGroupName("Wifi")
            .AddAttribute("ChannelSwitchTimeout",
                          "After requesting a channel switch on a link to setup that link, "
                          "wait at most this amount of time. If a channel switch is not "
                          "notified within this amount of time, we give up setting up that link.",
                          TimeValue(MilliSeconds(5)),
                          MakeTimeAccessor(&WifiDefaultAssocManager::m_channelSwitchTimeout),
                          MakeTimeChecker(Seconds(0)));
    return tid;
}

WifiDefaultAssocManager::WifiDefaultAssocManager()
{
    NS_LOG_FUNCTION(this);
}

WifiDefaultAssocManager::~WifiDefaultAssocManager()
{
    NS_LOG_FUNCTION(this);
}

void
WifiDefaultAssocManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_probeRequestEvent.Cancel();
    m_waitBeaconEvent.Cancel();
    WifiAssocManager::DoDispose();
}

bool
WifiDefaultAssocManager::Compare(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const
{
    return lhs.m_snr > rhs.m_snr;
}

void
WifiDefaultAssocManager::DoStartScanning()
{
    NS_LOG_FUNCTION(this);

    // if there are entries in the sorted list of AP information, reuse them and
    // do not perform scanning
    if (!GetSortedList().empty())
    {
        Simulator::ScheduleNow(&WifiDefaultAssocManager::EndScanning, this);
        return;
    }

    m_probeRequestEvent.Cancel();
    m_waitBeaconEvent.Cancel();

    if (GetScanParams().type == WifiScanType::ACTIVE)
    {
        for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
        {
            Simulator::Schedule(GetScanParams().probeDelay,
                                &StaWifiMac::EnqueueProbeRequest,
                                m_mac,
                                m_mac->GetProbeRequest(linkId),
                                linkId,
                                Mac48Address::GetBroadcast(),
                                Mac48Address::GetBroadcast());
        }
        m_probeRequestEvent =
            Simulator::Schedule(GetScanParams().probeDelay + GetScanParams().maxChannelTime,
                                &WifiDefaultAssocManager::EndScanning,
                                this);
    }
    else
    {
        m_waitBeaconEvent = Simulator::Schedule(GetScanParams().maxChannelTime,
                                                &WifiDefaultAssocManager::EndScanning,
                                                this);
    }
}

void
WifiDefaultAssocManager::EndScanning()
{
    NS_LOG_FUNCTION(this);

    OptMleConstRef mle;
    OptRnrConstRef rnr;
    std::list<WifiAssocManager::RnrLinkInfo> apList;

    // If multi-link setup is not possible, just call ScanningTimeout() and return
    if (!CanSetupMultiLink(mle, rnr) || (apList = GetAllAffiliatedAps(*rnr)).empty())
    {
        ScanningTimeout();
        return;
    }

    auto& bestAp = *GetSortedList().begin();
    auto& setupLinks = GetSetupLinks(bestAp);

    setupLinks.clear();
    setupLinks.emplace_back(StaWifiMac::ApInfo::SetupLinksInfo{bestAp.m_linkId,
                                                               mle->get().GetLinkIdInfo(),
                                                               bestAp.m_bssid});

    // sort local PHY objects so that radios with constrained PHY band comes first,
    // then radios with no constraint
    std::list<uint8_t> localLinkIds;

    for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
    {
        if (linkId == bestAp.m_linkId)
        {
            // this link has been already added (it is the link on which the Beacon/Probe
            // Response was received)
            continue;
        }

        if (m_mac->GetWifiPhy(linkId)->HasFixedPhyBand())
        {
            localLinkIds.push_front(linkId);
        }
        else
        {
            localLinkIds.push_back(linkId);
        }
    }

    // iterate over all the local links and find if we can setup a link for each of them
    for (const auto& linkId : localLinkIds)
    {
        auto phy = m_mac->GetWifiPhy(linkId);
        auto apIt = apList.begin();

        while (apIt != apList.end())
        {
            auto apChannel = rnr->get().GetOperatingChannel(apIt->m_nbrApInfoId);

            // we cannot setup a link with this affiliated AP if this PHY object is
            // constrained to operate in the current PHY band and this affiliated AP
            // is operating in a different PHY band than this PHY object
            if (phy->HasFixedPhyBand() && phy->GetPhyBand() != apChannel.GetPhyBand())
            {
                apIt++;
                continue;
            }

            bool needChannelSwitch = false;
            if (phy->GetOperatingChannel() != apChannel)
            {
                needChannelSwitch = true;
            }

            if (needChannelSwitch && phy->IsStateSwitching())
            {
                // skip this affiliated AP, which is operating on a different channel
                // than ours, because we are already switching channel and cannot
                // schedule another channel switch to match the affiliated AP channel
                apIt++;
                continue;
            }

            // if we get here, it means we can setup a link with this affiliated AP
            // set the BSSID for this link
            Mac48Address bssid = rnr->get().GetBssid(apIt->m_nbrApInfoId, apIt->m_tbttInfoFieldId);
            setupLinks.emplace_back(StaWifiMac::ApInfo::SetupLinksInfo{
                linkId,
                rnr->get().GetMldParameters(apIt->m_nbrApInfoId, apIt->m_tbttInfoFieldId).linkId,
                bssid});

            if (needChannelSwitch)
            {
                if (phy->IsStateSleep())
                {
                    // switching channel while a PHY is in sleep state fails
                    phy->ResumeFromSleep();
                }
                // switch this link to using the channel used by a reported AP (or its primary80
                // in case the reported AP is using a 160 MHz and the non-AP MLD does not support
                // 160 MHz operations)
                if (apChannel.GetTotalWidth() > MHz_u{80} &&
                    !phy->GetDevice()->GetVhtConfiguration()->m_160MHzSupported)
                {
                    apChannel = apChannel.GetPrimaryChannel(MHz_u{80});
                }

                NS_LOG_DEBUG("Switch link " << +linkId << " to using " << apChannel);
                phy->SetOperatingChannel(apChannel);
                // actual channel switching may be delayed, thus setup a channel switch timer
                m_channelSwitchInfo.resize(m_mac->GetNLinks());
                m_channelSwitchInfo[linkId].timer.Cancel();
                m_channelSwitchInfo[linkId].timer =
                    Simulator::Schedule(m_channelSwitchTimeout,
                                        &WifiDefaultAssocManager::ChannelSwitchTimeout,
                                        this,
                                        linkId);
                m_channelSwitchInfo[linkId].apLinkAddress = bssid;
                m_channelSwitchInfo[linkId].apMldAddress = mle->get().GetMldMacAddress();
            }

            // remove the affiliated AP with which we are going to setup a link and move
            // to the next local linkId
            apList.erase(apIt);
            break;
        }
    }

    if (std::none_of(m_channelSwitchInfo.begin(), m_channelSwitchInfo.end(), [](auto&& info) {
            return info.timer.IsPending();
        }))
    {
        // we are done
        ScanningTimeout();
    }
}

void
WifiDefaultAssocManager::NotifyChannelSwitched(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    if (m_channelSwitchInfo.size() > linkId && m_channelSwitchInfo[linkId].timer.IsPending())
    {
        // we were waiting for this notification
        m_channelSwitchInfo[linkId].timer.Cancel();

        if (std::none_of(m_channelSwitchInfo.begin(), m_channelSwitchInfo.end(), [](auto&& info) {
                return info.timer.IsPending();
            }))
        {
            // we are done
            ScanningTimeout();
        }
    }
}

void
WifiDefaultAssocManager::ChannelSwitchTimeout(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);

    // we give up setting up this link
    auto& bestAp = *GetSortedList().begin();
    auto& setupLinks = GetSetupLinks(bestAp);
    auto it = std::find_if(setupLinks.begin(), setupLinks.end(), [&linkId](auto&& linkIds) {
        return linkIds.localLinkId == linkId;
    });
    NS_ASSERT(it != setupLinks.end());
    setupLinks.erase(it);

    if (std::none_of(m_channelSwitchInfo.begin(), m_channelSwitchInfo.end(), [](auto&& info) {
            return info.timer.IsPending();
        }))
    {
        // we are done
        ScanningTimeout();
    }
}

bool
WifiDefaultAssocManager::CanBeInserted(const StaWifiMac::ApInfo& apInfo) const
{
    return (m_waitBeaconEvent.IsPending() || m_probeRequestEvent.IsPending());
}

bool
WifiDefaultAssocManager::CanBeReturned(const StaWifiMac::ApInfo& apInfo) const
{
    return true;
}

} // namespace ns3
