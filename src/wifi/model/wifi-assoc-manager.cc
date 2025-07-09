/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II

 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-assoc-manager.h"

#include "sta-wifi-mac.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/eht-configuration.h"
#include "ns3/enum.h"
#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiAssocManager");

NS_OBJECT_ENSURE_REGISTERED(WifiAssocManager);

WifiAssocManager::ApInfoCompare::ApInfoCompare(const WifiAssocManager& manager)
    : m_manager(manager)
{
}

bool
WifiAssocManager::ApInfoCompare::operator()(const StaWifiMac::ApInfo& lhs,
                                            const StaWifiMac::ApInfo& rhs) const
{
    NS_ASSERT_MSG(lhs.m_bssid != rhs.m_bssid,
                  "Comparing two ApInfo objects with the same BSSID: " << lhs.m_bssid);

    bool lhsBefore = m_manager.Compare(lhs, rhs);
    if (lhsBefore)
    {
        return true;
    }

    bool rhsBefore = m_manager.Compare(rhs, lhs);
    if (rhsBefore)
    {
        return false;
    }

    // the Compare method implemented by subclass may be such that the two ApInfo objects
    // compare equal; in such a case, use the BSSID as tie breaker
    return lhs.m_bssid < rhs.m_bssid;
}

TypeId
WifiAssocManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiAssocManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute(
                "AllowedLinks",
                "Only Beacon and Probe Response frames received on a link belonging to the given "
                "set are processed. An empty set is equivalent to the set of all links.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&WifiAssocManager::m_allowedLinks),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()))
            .AddAttribute("ChannelSwitchTimeout",
                          "After requesting a channel switch on a link (e.g., to scan a new "
                          "channel or to setup that link), wait at most this amount of time. If a "
                          "channel switch is not notified within this amount of time, we cancel "
                          "the pending operation (e.g., we move on to the next channel to scan or "
                          "we give up setting up that link).",
                          TimeValue(MilliSeconds(5)),
                          MakeTimeAccessor(&WifiAssocManager::m_channelSwitchTimeout),
                          MakeTimeChecker(Seconds(0)))
            .AddAttribute("AllowAssocAllChannelWidths",
                          "If set to true, it bypasses the check on channel width compatibility "
                          "with the candidate AP. A channel width is compatible if the STA can "
                          "advertise it to the AP, or AP operates on a channel width that is equal "
                          "or lower than that channel width.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WifiAssocManager::m_allowAssocAllChannelWidths),
                          MakeBooleanChecker())
            .AddTraceSource("ScanningEnd",
                            "Traces the end of every scanning procedure. Provides the list of APs "
                            "found during the scanning procedure, sorted in the order in which "
                            "they were discovered.",
                            MakeTraceSourceAccessor(&WifiAssocManager::m_scanEndLogger),
                            "ns3::WifiAssocManager::ScanEndCallback");
    return tid;
}

WifiAssocManager::WifiAssocManager()
    : m_scanParams(), // zero-initialization
      m_apList(ApInfoCompare(*this))
{
}

WifiAssocManager::~WifiAssocManager()
{
    NS_LOG_FUNCTION(this);
}

void
WifiAssocManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_mac = nullptr;
    for (auto& [phyId, event] : m_currentChannelScanEnd)
    {
        event.Cancel();
    }
    for (auto& [phyId, event] : m_switchToChannelToScanTimeout)
    {
        event.Cancel();
    }
    Object::DoDispose();
}

void
WifiAssocManager::SetStaWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
}

const WifiAssocManager::SortedList&
WifiAssocManager::GetSortedList() const
{
    return m_apList;
}

const WifiScanParams&
WifiAssocManager::GetScanParams() const
{
    return m_scanParams;
}

bool
WifiAssocManager::MatchScanParams(const StaWifiMac::ApInfo& apInfo) const
{
    NS_LOG_FUNCTION(this << apInfo);

    if (!m_scanParams.ssid.IsBroadcast())
    {
        // we need to check if AP's advertised SSID matches the requested SSID
        Ssid apSsid;
        if (auto beacon = std::get_if<MgtBeaconHeader>(&apInfo.m_frame); beacon)
        {
            apSsid = beacon->Get<Ssid>().value();
        }
        else
        {
            auto probeResp = std::get_if<MgtProbeResponseHeader>(&apInfo.m_frame);
            NS_ASSERT(probeResp);
            apSsid = probeResp->Get<Ssid>().value();
        }
        if (!apSsid.IsEqual(m_scanParams.ssid))
        {
            NS_LOG_DEBUG("AP " << apInfo.m_bssid << " does not advertise our SSID " << apSsid
                               << "  " << m_scanParams.ssid);
            return false;
        }
    }

    // we need to check if the AP is operating on a requested channel
    auto channelMatch = [&apInfo](auto&& channels) {
        if (channels.first != WIFI_PHY_BAND_UNSPECIFIED && channels.first != apInfo.m_channel.band)
        {
            return false;
        }
        return std::find_if(channels.second.cbegin(),
                            channels.second.cend(),
                            [&apInfo](auto channel) {
                                return channel == 0 || channel == apInfo.m_channel.number;
                            }) != channels.second.cend();
    };

    if (std::all_of(m_scanParams.channelList.cbegin(),
                    m_scanParams.channelList.cend(),
                    [&](auto&& phyIdChannelsPair) {
                        return std::find_if(phyIdChannelsPair.second.cbegin(),
                                            phyIdChannelsPair.second.cend(),
                                            channelMatch) == phyIdChannelsPair.second.cend();
                    }))
    {
        NS_LOG_DEBUG("AP " << apInfo.m_bssid << " is not operating on a requested channel");
        return false;
    }

    return true;
}

void
WifiAssocManager::StartScanning(WifiScanParams&& scanParams)
{
    NS_LOG_FUNCTION(this);
    m_scanParams = std::move(scanParams);

    // remove stored AP information not matching the scanning parameters or related to APs
    // that are not reachable on an allowed link
    for (auto ap = m_apList.begin(); ap != m_apList.end();)
    {
        if (!MatchScanParams(*ap) ||
            (!m_allowedLinks.empty() && !m_allowedLinks.contains(ap->m_linkId)))
        {
            // remove AP info from list
            m_apListIt.erase(ap->m_bssid);
            ap = m_apList.erase(ap);
        }
        else
        {
            ++ap;
        }
    }

    DoStartScanning();
}

void
WifiAssocManager::ClearStoredApInfo()
{
    NS_LOG_FUNCTION(this);
    m_apList.clear();
    m_apListIt.clear();
}

void
WifiAssocManager::ScanChannels()
{
    NS_LOG_FUNCTION(this);

    // find the first channel to scan for each PHY
    NS_ABORT_MSG_IF(m_scanParams.channelList.empty(), "No channels to scan");
    m_channelsToSet.clear();

    m_currScanAps.clear();

    for (const auto& [phyId, phyBandChannelsMap] : m_scanParams.channelList)
    {
        NS_ABORT_MSG_IF(phyBandChannelsMap.empty(), "No PHY band for PHY ID " << +phyId);

        const auto& [band, channels] = *phyBandChannelsMap.cbegin();

        NS_ABORT_MSG_IF(channels.empty(),
                        "No channel to scan in " << band << " band for PHY ID " << +phyId);

        // store the current channel before starting scanning
        m_channelsToSet[phyId] = m_mac->GetDevice()->GetPhy(phyId)->GetOperatingChannel();
        SwitchToChannelToScan(phyId, band, channels.cbegin());
    }
}

void
WifiAssocManager::SwitchToChannelToScan(
    uint8_t phyId,
    WifiPhyBand band,
    WifiScanParams::ChannelList::mapped_type::const_iterator channelIt)
{
    NS_LOG_FUNCTION(this << phyId << band << *channelIt);

    auto phy = m_mac->GetDevice()->GetPhy(phyId);
    NS_ASSERT(phy);
    // check if we need to switch channel to scan the given channel
    const auto isMultipleOf20MHz = phy->GetChannelWidth().IsMultipleOf(20_MHz);
    const auto channelNo =
        (!isMultipleOf20MHz
             ? phy->GetChannelNumber()
             : phy->GetOperatingChannel().GetPrimaryChannelNumber(MHz_t{20}, phy->GetStandard()));

    if (channelNo != *channelIt)
    {
        // we need to switch channel
        if (phy->IsStateSleep())
        {
            // switching channel while a PHY is in sleep state fails
            phy->ResumeFromSleep();
        }

        const auto width = (!isMultipleOf20MHz ? phy->GetChannelWidth() : MHz_t{20});
        NS_LOG_DEBUG("Switching PHY " << +phyId << " to channel " << *channelIt << ", " << band);
        phy->SetOperatingChannel(WifiPhy::ChannelSegments{{*channelIt, width.in_MHz(), band, 0}});

        // actual channel switching may be delayed; if it takes more than the configured
        // maximum amount of time, move to the next channel to scan
        m_switchToChannelToScanTimeout[phyId].Cancel();
        m_switchToChannelToScanTimeout[phyId] =
            Simulator::Schedule(m_channelSwitchTimeout,
                                &WifiAssocManager::SwitchToNextChannelToScan,
                                this,
                                phyId,
                                band,
                                *channelIt);
    }
    else
    {
        // start scanning
        ScanCurrentChannel(phyId, band, channelIt);
    }
}

void
WifiAssocManager::ScanCurrentChannel(
    uint8_t phyId,
    WifiPhyBand band,
    WifiScanParams::ChannelList::mapped_type::const_iterator channelIt)
{
    NS_LOG_FUNCTION(this << phyId << band << *channelIt);

    Time scanDuration;

    if (m_scanParams.type == WifiScanType::ACTIVE)
    {
        scanDuration = m_scanParams.probeDelay + m_scanParams.maxChannelTime;
        auto linkId = m_mac->GetLinkForPhy(phyId);
        NS_ASSERT_MSG(linkId.has_value(), "PHY " << +phyId << " is not operating on any link");

        // enqueue probe request after the probe delay
        Simulator::Schedule(m_scanParams.probeDelay,
                            &StaWifiMac::EnqueueProbeRequest,
                            m_mac,
                            m_mac->GetProbeRequest(*linkId),
                            *linkId,
                            Mac48Address::GetBroadcast(),
                            Mac48Address::GetBroadcast());
    }
    else
    {
        scanDuration = m_scanParams.maxChannelTime;
    }

    m_currentChannelScanEnd[phyId].Cancel();
    m_currentChannelScanEnd[phyId] =
        Simulator::Schedule(scanDuration,
                            &WifiAssocManager::SwitchToNextChannelToScan,
                            this,
                            phyId,
                            band,
                            *channelIt);
}

bool
WifiAssocManager::NotifyChannelSwitched(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto phy = m_mac->GetWifiPhy(linkId);
    NS_ASSERT(phy);
    const auto phyId = phy->GetPhyId();

    if (const auto it = m_switchToChannelToScanTimeout.find(phyId);
        it != m_switchToChannelToScanTimeout.cend() && it->second.IsPending())
    {
        // we were waiting for this PHY to switch, start scanning
        it->second.Cancel();
        auto list = m_scanParams.channelList.at(phyId).at(phy->GetPhyBand());
        auto channelIt = std::find(list.cbegin(), list.cend(), phy->GetChannelNumber());
        NS_ASSERT_MSG(channelIt != list.cend(),
                      "Current channel for PHY " << +phyId << "(" << phy->GetOperatingChannel()
                                                 << ") is not present in the scanning list");
        ScanCurrentChannel(phyId, phy->GetPhyBand(), channelIt);
        return true;
    }
    else if (const auto it = m_switchToChannelToSetTimeout.find(phyId);
             it != m_switchToChannelToSetTimeout.cend() && it->second.IsPending())
    {
        it->second.Cancel();
        auto channelIt = m_channelsToSet.find(phyId);
        NS_ASSERT_MSG(channelIt != m_channelsToSet.cend(),
                      "Started timer for PHY " << +phyId << " but no channel expected to be set");
        NS_ASSERT_MSG(channelIt->second == phy->GetOperatingChannel(),
                      "Expected PHY " << +phyId << " to switch to " << channelIt->second
                                      << " but it switched to " << phy->GetOperatingChannel());
        NS_ASSERT_MSG(!IsScanning(), "Expected to set channels when scanning is completed");
        m_channelsToSet.erase(channelIt);
        if (m_channelsToSet.empty())
        {
            // no more channels to set, we can return to StaWifiMac
            m_mac->ScanningTimeout(std::nullopt);
        }
        return true;
    }

    return DoNotifyChannelSwitched(linkId);
}

void
WifiAssocManager::SwitchToNextChannelToScan(uint8_t phyId, WifiPhyBand band, uint8_t channelNo)
{
    NS_LOG_FUNCTION(this << phyId << band << channelNo);

    const auto phyIdChannelsMapIt = m_scanParams.channelList.find(phyId);
    NS_ASSERT_MSG(phyIdChannelsMapIt != m_scanParams.channelList.cend(),
                  "Cannot find PHY " << +phyId << " in the channel scanning map");

    auto phyBandChannelsMapIt = phyIdChannelsMapIt->second.find(band);
    NS_ASSERT_MSG(phyBandChannelsMapIt != phyIdChannelsMapIt->second.cend(),
                  "Cannot find " << band << " band in the channel scanning map");

    auto channelIt = std::find(phyBandChannelsMapIt->second.cbegin(),
                               phyBandChannelsMapIt->second.cend(),
                               channelNo);
    NS_ASSERT(channelIt != phyBandChannelsMapIt->second.cend());

    if (++channelIt != phyBandChannelsMapIt->second.cend())
    {
        NS_LOG_DEBUG("Found another channel to scan (" << *channelIt << ") in the same band ("
                                                       << band << ")");
        SwitchToChannelToScan(phyId, band, channelIt);
        return;
    }

    if (++phyBandChannelsMapIt != phyIdChannelsMapIt->second.cend())
    {
        band = phyBandChannelsMapIt->first;
        channelIt = phyBandChannelsMapIt->second.cbegin();
        NS_LOG_DEBUG("Found a channel to scan (" << *channelIt << ") in another band (" << band
                                                 << ")");
        SwitchToChannelToScan(phyId, band, channelIt);
        return;
    }

    // no other channel to scan for this PHY; if we are not waiting for other PHYs to finish their
    // scanning, we are done
    if (!IsScanning())
    {
        EndScanning();
        m_scanEndLogger(m_currScanAps);
        m_currScanAps.clear();
    }
}

bool
WifiAssocManager::IsScanning() const
{
    return std::any_of(m_currentChannelScanEnd.cbegin(),
                       m_currentChannelScanEnd.cend(),
                       [](auto&& phyIdEventPair) { return phyIdEventPair.second.IsPending(); }) ||
           std::any_of(m_switchToChannelToScanTimeout.cbegin(),
                       m_switchToChannelToScanTimeout.cend(),
                       [](auto&& phyIdEventPair) { return phyIdEventPair.second.IsPending(); });
}

void
WifiAssocManager::NotifyApInfo(const StaWifiMac::ApInfo&& apInfo)
{
    NS_LOG_FUNCTION(this << apInfo);

    if (!CanBeInserted(apInfo) || !MatchScanParams(apInfo) ||
        (!m_allowedLinks.empty() && !m_allowedLinks.contains(apInfo.m_linkId)))
    {
        NS_LOG_DEBUG("AP info cannot be inserted");
        return;
    }

    m_currScanAps.emplace_back(apInfo);

    // check if an ApInfo object with the same BSSID is already present in the
    // sorted list of ApInfo objects. This is done by trying to insert the BSSID
    // in the hash table (insertion fails if the BSSID is already present)
    auto [hashIt, hashInserted] = m_apListIt.insert({apInfo.m_bssid, {}});
    if (!hashInserted)
    {
        // an element with the searched BSSID is already present in the hash table.
        // Remove the corresponding ApInfo object from the sorted list.
        m_apList.erase(hashIt->second);
    }
    // insert the ApInfo object
    auto [listIt, listInserted] = m_apList.insert(std::move(apInfo));
    // update the hash table entry
    NS_ASSERT_MSG(listInserted,
                  "An entry (" << listIt->m_apAddr << ", " << listIt->m_bssid << ", "
                               << +listIt->m_linkId
                               << ") prevented insertion of given ApInfo object");
    hashIt->second = listIt;
}

void
WifiAssocManager::NotifyLinkSwapped(const std::map<uint8_t, uint8_t>& swapInfo)
{
    NS_LOG_FUNCTION(this);

    // update the information gathered from APs
    for (auto& apInfoConst : m_apList)
    {
        auto& apInfo = const_cast<StaWifiMac::ApInfo&>(apInfoConst);
        apInfo.m_setupLinks.clear(); // will be cleared anyway before selecting this AP
        if (const auto it = swapInfo.find(apInfo.m_linkId); it != swapInfo.end())
        {
            apInfo.m_linkId = it->second;
        }
    }
}

void
WifiAssocManager::ScanningTimeout()
{
    NS_LOG_FUNCTION(this);

    StaWifiMac::ApInfo bestAp;

    do
    {
        if (m_apList.empty())
        {
            m_mac->ScanningTimeout(std::nullopt);
            return;
        }

        bestAp = std::move(m_apList.extract(m_apList.begin()).value());
        m_apListIt.erase(bestAp.m_bssid);
    } while (!CanBeReturned(bestAp));

    NS_ABORT_MSG_IF(!m_allowAssocAllChannelWidths && !IsChannelWidthCompatible(bestAp),
                    "Channel width of STA is not part of the channel width set that can be "
                    "advertised to the AP");

    m_mac->ScanningTimeout(std::move(bestAp));
}

void
WifiAssocManager::OfflineScanningTimeout()
{
    NS_LOG_FUNCTION(this);

    // restore previous channels
    for (auto it = m_channelsToSet.begin(); it != m_channelsToSet.end();)
    {
        const auto phyId = it->first;
        const auto& channel = it->second;
        const auto phy = m_mac->GetDevice()->GetPhy(phyId);

        if (phy->GetOperatingChannel() != channel)
        {
            phy->SetOperatingChannel(channel);
            // abort if channel switch does not complete before the channel switch timeout
            m_switchToChannelToSetTimeout[phyId].Cancel();
            m_switchToChannelToSetTimeout[phyId] = Simulator::Schedule(m_channelSwitchTimeout, [=] {
                NS_ABORT_MSG("Unable to set " << channel << " on PHY " << +phyId);
            });
            ++it;
        }
        else
        {
            it = m_channelsToSet.erase(it);
        }
    }

    // if no channels to set, we can return to StaWifiMac immediately; otherwise, we wait for
    // channel switches to complete
    if (m_channelsToSet.empty())
    {
        m_mac->ScanningTimeout(std::nullopt);
    }
}

std::list<StaWifiMac::ApInfo::SetupLinksInfo>&
WifiAssocManager::GetSetupLinks(const StaWifiMac::ApInfo& apInfo)
{
    return const_cast<std::list<StaWifiMac::ApInfo::SetupLinksInfo>&>(apInfo.m_setupLinks);
}

bool
WifiAssocManager::CanSetupMultiLink(OptMleConstRef& mle, OptRnrConstRef& rnr)
{
    NS_LOG_FUNCTION(this);

    if (m_mac->GetAssocType() == WifiAssocType::LEGACY || GetSortedList().empty())
    {
        return false;
    }

    // Get the Multi-Link Element and the RNR element, if present,
    // from Beacon or Probe Response
    if (auto beacon = std::get_if<MgtBeaconHeader>(&m_apList.begin()->m_frame); beacon)
    {
        mle = beacon->Get<MultiLinkElement>();
        rnr = beacon->Get<ReducedNeighborReport>();
    }
    else
    {
        auto probeResp = std::get_if<MgtProbeResponseHeader>(&m_apList.begin()->m_frame);
        NS_ASSERT(probeResp);
        mle = probeResp->Get<MultiLinkElement>();
        rnr = probeResp->Get<ReducedNeighborReport>();
    }

    if (!mle.has_value())
    {
        NS_LOG_DEBUG("No Multi-Link Element in Beacon/Probe Response");
        return false;
    }

    if (!rnr.has_value() || rnr->get().GetNNbrApInfoFields() == 0)
    {
        NS_LOG_DEBUG("No Reduced Neighbor Report Element in Beacon/Probe Response");
        return false;
    }

    // The Multi-Link Element must contain the MLD MAC Address subfield and the
    // Link ID Info subfield
    if (!mle->get().HasLinkIdInfo())
    {
        NS_LOG_DEBUG("No Link ID Info subfield in the Multi-Link Element");
        return false;
    }

    if (const auto& mldCapabilities = mle->get().GetCommonInfoBasic().m_mldCapabilities)
    {
        auto ehtConfig = m_mac->GetEhtConfiguration();
        NS_ASSERT(ehtConfig);

        // A non-AP MLD that performs multi-link (re)setup on at least two links with an AP MLD
        // that sets the TID-To-Link Mapping Negotiation Support subfield of the MLD Capabilities
        // field of the Basic Multi-Link element to a nonzero value shall support TID-to-link
        // mapping negotiation with the TID-To-Link Mapping Negotiation Support subfield of the
        // MLD Capabilities field of the Basic Multi-Link element it transmits to at least 1.
        // (Sec. 35.3.7.1.1 of 802.11be D3.1)
        if (mldCapabilities->tidToLinkMappingSupport > 0 &&
            ehtConfig->m_tidLinkMappingSupport == WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
        {
            NS_LOG_DEBUG("AP MLD supports TID-to-Link Mapping negotiation, while we don't");
            return false;
        }
    }

    return true;
}

std::optional<WifiAssocManager::RnrLinkInfo>
WifiAssocManager::GetNextAffiliatedAp(const ReducedNeighborReport& rnr, std::size_t nbrApInfoId)
{
    NS_LOG_FUNCTION(nbrApInfoId);

    while (nbrApInfoId < rnr.GetNNbrApInfoFields())
    {
        if (!rnr.HasMldParameters(nbrApInfoId))
        {
            // this Neighbor AP Info field is not suitable to setup a link
            nbrApInfoId++;
            continue;
        }

        std::size_t tbttInfoFieldIndex = 0;
        while (tbttInfoFieldIndex < rnr.GetNTbttInformationFields(nbrApInfoId) &&
               rnr.GetMldParameters(nbrApInfoId, tbttInfoFieldIndex).apMldId != 0)
        {
            tbttInfoFieldIndex++;
        }

        if (tbttInfoFieldIndex < rnr.GetNTbttInformationFields(nbrApInfoId))
        {
            // this Neighbor AP Info field contains an AP affiliated to the
            // same AP MLD as the reporting AP
            return RnrLinkInfo{nbrApInfoId, tbttInfoFieldIndex};
        }
        nbrApInfoId++;
    }

    return std::nullopt;
}

std::list<WifiAssocManager::RnrLinkInfo>
WifiAssocManager::GetAllAffiliatedAps(const ReducedNeighborReport& rnr)
{
    std::list<WifiAssocManager::RnrLinkInfo> apList;
    std::size_t nbrApInfoId = 0;
    std::optional<WifiAssocManager::RnrLinkInfo> next;

    while ((next = GetNextAffiliatedAp(rnr, nbrApInfoId)).has_value())
    {
        apList.push_back({*next});
        nbrApInfoId = next->m_nbrApInfoId + 1;
    }

    return apList;
}

bool
WifiAssocManager::IsChannelWidthCompatible(const StaWifiMac::ApInfo& apInfo) const
{
    auto phy = m_mac->GetWifiPhy(apInfo.m_linkId);
    return GetSupportedChannelWidthSet(phy->GetStandard(), apInfo.m_channel.band)
               .contains(phy->GetChannelWidth()) ||
           (phy->GetChannelWidth() >= m_mac->GetWifiRemoteStationManager(apInfo.m_linkId)
                                          ->GetChannelWidthSupported(apInfo.m_bssid));
}

} // namespace ns3
