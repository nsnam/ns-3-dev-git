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

#include "ns3/boolean.h"
#include "ns3/eht-configuration.h"
#include "ns3/log.h"
#include "ns3/pointer.h"

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
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<linkId_t>()))
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
            .AddAttribute("OfflineScanningInterval",
                          "If non-zero, indicates the interval between two successive requests for "
                          "offline channel scanning procedures. Note that a channel scan is "
                          "actually started if the STA is associated and no other scanning is "
                          "ongoing.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&WifiAssocManager::RequestOfflineScanning),
                          MakeTimeChecker())
            .AddAttribute("EnableScanning",
                          "If false, STA does not perform channel scanning. This may be useful in "
                          "case of static configuration via the static setup helper.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&WifiAssocManager::m_enableScanning),
                          MakeBooleanChecker())
            .AddAttribute(
                "ScanningChannels",
                "Use this attribute to set which channels to scan and which PHYs to use during "
                "scanning procedures. For each PHY to use, a PHY band-indexed map of the numbers "
                "of the 20MHz channels to scan must be specified. The string must contain the PHY "
                "ID followed by a colon(':') and the map of the channels to scan using the PHY "
                "with the given ID. A pipe ('|') is used to separate the definition of the channel "
                "maps for two PHYs. The map of channels to scan using a PHY can be specified in "
                "one of the following ways: (a) empty map, causing only the currently configured "
                "frequency channel to be scanned; (b) a single entry given by the pair "
                "(WIFI_PHY_BAND_UNSPECIFIED, {0}): all the channels of all the bands are scanned; "
                "(c) one or more entries, for all of which the key is not the unspecified PHY "
                "band: scan all the channels specified for the respective bands, where 0 indicates "
                "all channels in the corresponding band. The (PHY band, channel numbers list) "
                "pairs shall be separated by a semicolon (;); in every pair, the PHY band and the "
                "channel numbers list are separated by a blank space, and the elements of each "
                "list are separated by a comma (,) without spaces. "
                "Example: \"0 : WIFI_PHY_BAND_UNSPECIFIED 0 | 1: | 2: WIFI_PHY_BAND_5GHZ 104,108;"
                " WIFI_PHY_BAND_6GHZ 0\" means that: "
                "PHY 0 is used to scan all channels in all bands, PHY 1 is used to scan only the "
                "currently configured channel, PHY 2 is used to scan channels 104 and 108 in the 5 "
                "GHz band and all the channels in the 6 GHz band. "
                "Note that if the string is empty, all PHYs will be used to scan their currently "
                "configured channels.",
                StringValue(""),
                MakeStringAccessor(&WifiAssocManager::SetScanningChannelsFromStr),
                MakeStringChecker())
            .AddAttribute(
                "OfflineScanProtection",
                "The technique to use to protect an offline channel scanning, i.e., to inform the "
                "AP of the unavailability of the STA for some period.",
                EnumValue(WifiOfflineScanProtection::NONE),
                MakeEnumAccessor<WifiOfflineScanProtection>(
                    &WifiAssocManager::m_offlineScanProtection),
                MakeEnumChecker(WifiOfflineScanProtection::NONE,
                                "NONE",
                                WifiOfflineScanProtection::POWERSAVE,
                                "POWERSAVE"))
            .AddAttribute("EnableChannelSwitch",
                          "Whether to allow the association manager to switch the channel of the "
                          "STA (on the link used to send the Association Request frame) to match "
                          "the channel used by the AP. NOTE that this attribute has no effect if "
                          "the requirements of the GetApOperatingChannel() function are not met.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&WifiAssocManager::m_enableChannelSwitch),
                          MakeBooleanChecker())
            .AddTraceSource("ScanningEnd",
                            "Traces the end of every scanning procedure. Provides the scan "
                            "duration and the list of APs found during the scanning "
                            "procedure, sorted in the order in which they were discovered.",
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
    m_nextOfflineChannelScan.Cancel();
    Object::DoDispose();
}

Ptr<const AttributeChecker>
WifiAssocManager::GetPhyBandChannelListChecker()
{
    static auto phyBandChannelNumbersMapChecker =
        MakeAttributeContainerChecker<PhyBandChannelNumbersPairValue, ';'>(
            MakePairChecker<EnumValue<WifiPhyBand>, AttributeContainerValue<UintegerValue>>(
                MakeEnumChecker(WIFI_PHY_BAND_2_4GHZ,
                                "2.4GHz",
                                WIFI_PHY_BAND_5GHZ,
                                "5GHz",
                                WIFI_PHY_BAND_6GHZ,
                                "6GHz",
                                WIFI_PHY_BAND_60GHZ,
                                "60GHz",
                                WIFI_PHY_BAND_UNSPECIFIED,
                                "INVALID"),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>())));

    return phyBandChannelNumbersMapChecker;
}

void
WifiAssocManager::SetScanningChannelsFromStr(const std::string& s)
{
    std::istringstream iss(s);
    uint16_t phyId{};
    m_scanChannels.clear();

    while (iss >> phyId)
    {
        char sep;
        if (!(iss >> sep) || sep != ':')
        {
            NS_ABORT_MSG("ScanningChannels: Expected ':', got '" << sep << "'");
        }

        // parse the PHY band-indexed map of channels to scan
        std::string elem;
        std::getline(iss >> std::ws, elem, '|');

        AttributeContainerValue<PhyBandChannelNumbersPairValue, ';'> value;
        value.DeserializeFromString(elem, GetPhyBandChannelListChecker());

        WifiScanParams::ChannelList map;
        value.GetAccessor(map);

        NS_ABORT_MSG_IF(map.size() > 1 && map.contains(WIFI_PHY_BAND_UNSPECIFIED),
                        "PHY " << phyId
                               << ": An unspecified PHY band must be the unique entry of the map "
                                  "of channels to scan");

        NS_ABORT_MSG_IF(map.contains(WIFI_PHY_BAND_UNSPECIFIED) &&
                            map.at(WIFI_PHY_BAND_UNSPECIFIED) !=
                                WifiScanParams::ChannelList::mapped_type{0},
                        "PHY " << phyId
                               << ": An unspecified PHY band entry must be associated with a list "
                                  "containing only a zero");

        for (auto& [band, channels] : map)
        {
            if (band == WIFI_PHY_BAND_UNSPECIFIED)
            {
                continue;
            }

            std::set<uint8_t> uniqueChannels;
            for (auto it = channels.begin(); it != channels.end();)
            {
                const auto channelNumber = static_cast<uint8_t>(*it);
                if (!uniqueChannels.insert(channelNumber).second)
                {
                    it = channels.erase(it);
                    continue;
                }

                if (channelNumber == 0)
                {
                    // 0 means all 20 MHz channels of the band; must be the only entry
                    NS_ABORT_MSG_IF(channels.size() != 1,
                                    "PHY " << phyId
                                           << ": Channel 0 (all channels) must be the unique "
                                              "entry of the channel list for band "
                                           << band);
                    ++it;
                    continue;
                }

                const auto channelIt = WifiPhyOperatingChannel::FindFirst(channelNumber,
                                                                          MHz_u{0},
                                                                          MHz_u{20},
                                                                          WIFI_STANDARD_UNSPECIFIED,
                                                                          band);
                NS_ABORT_MSG_IF(channelIt == WifiPhyOperatingChannel::m_frequencyChannels.cend(),
                                "PHY " << phyId << ": Channel " << +channelNumber
                                       << " is not a valid 20 MHz channel for band " << band);
                ++it;
            }
        }

        m_scanChannels[phyId] = map;
    }

    ExpandScanningChannelsMap();
}

std::string
WifiAssocManager::ScanningChannelsToStr(const WifiScanParams::Map& channels)
{
    std::stringstream ss;

    for (auto phyIdMapIt = channels.cbegin(); phyIdMapIt != channels.cend(); ++phyIdMapIt)
    {
        const auto& phyId = phyIdMapIt->first;
        const auto& bandChannelsMap = phyIdMapIt->second;

        ss << +phyId << ":";

        for (auto bandMapIt = bandChannelsMap.cbegin(); bandMapIt != bandChannelsMap.cend();
             ++bandMapIt)
        {
            const auto& [band, list] = *bandMapIt;

            ss << " " << band << " ";
            for (auto listIt = list.cbegin(); listIt != list.cend(); ++listIt)
            {
                ss << *listIt;
                if (std::next(listIt) != list.cend())
                {
                    ss << ",";
                }
            }

            if (std::next(bandMapIt) != bandChannelsMap.cend())
            {
                ss << ";";
            }
        }

        if (std::next(phyIdMapIt) != channels.cend())
        {
            ss << " | ";
        }
    }

    return ss.str();
}

void
WifiAssocManager::ExpandScanningChannelsMap()
{
    NS_LOG_FUNCTION(this);

    if (!m_mac || !m_mac->GetDevice() || m_mac->GetDevice()->GetNPhys() == 0)
    {
        // PHYs have not been configured yet, do nothing
        return;
    }

    for (auto& [phyId, bandChannelsMap] : m_scanChannels)
    {
        const auto standard = m_mac->GetDevice()->GetPhy(phyId)->GetStandard();

        if (bandChannelsMap.contains(WIFI_PHY_BAND_UNSPECIFIED))
        {
            // this PHY must be used to scan all channels of all bands. Replace the unique entry
            // with an entry for each of the bands supported given the configured standard
            bandChannelsMap.clear();

            auto bandsIt = wifiStandards.find(standard);
            NS_ASSERT(bandsIt != wifiStandards.cend());
            for (const auto band : bandsIt->second)
            {
                bandChannelsMap[band] = {0};
            }
        }

        for (auto& [band, channels] : bandChannelsMap)
        {
            if (channels == WifiScanParams::ChannelList::mapped_type{0})
            {
                // insert all 20 MHz channels in this band
                channels.clear();
                auto startIt = WifiPhyOperatingChannel::m_frequencyChannels.cbegin();

                while ((startIt = WifiPhyOperatingChannel::FindFirst(0,
                                                                     MHz_u{0},
                                                                     MHz_u{20},
                                                                     standard,
                                                                     band,
                                                                     startIt)) !=
                       WifiPhyOperatingChannel::m_frequencyChannels.cend())
                {
                    channels.emplace_back(startIt->number);
                    ++startIt;
                }
            }
        }
    }

    if (m_scanChannels.empty())
    {
        // all PHYs must be used to scan their currently configured channels, thus add an empty map
        // for each PHY
        for (uint8_t phyId = 0; phyId < m_mac->GetDevice()->GetNPhys(); ++phyId)
        {
            m_scanChannels.try_emplace(phyId);
        }
    }
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

WifiScanParams::Map
WifiAssocManager::GetCurrentChannelsForScanning()
{
    ExpandScanningChannelsMap();
    if (!m_mac || !m_mac->GetDevice() || m_mac->GetDevice()->GetNPhys() == 0)
    {
        return m_scanChannels;
    }

    auto scanChannels = m_scanChannels;

    for (auto& [phyId, bandChannelsMap] : scanChannels)
    {
        if (bandChannelsMap.empty())
        {
            // only the currently configured channel must be scanned
            auto phy = m_mac->GetDevice()->GetPhy(phyId);
            NS_ASSERT(phy);
            const auto channelNo =
                (static_cast<uint16_t>(phy->GetChannelWidth()) % 20 != 0)
                    ? phy->GetChannelNumber()
                    : phy->GetOperatingChannel().GetPrimaryChannelNumber(MHz_u{20},
                                                                         phy->GetStandard());
            bandChannelsMap.emplace(phy->GetPhyBand(),
                                    WifiScanParams::ChannelList::mapped_type{channelNo});
        }
    }

    return scanChannels;
}

void
WifiAssocManager::StartOfflineScanning()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_mac);

    if (!m_enableScanning)
    {
        NS_LOG_DEBUG("Scanning disabled");
        return;
    }

    NS_ABORT_MSG_IF(IsScanning(), "Cannot start offline scanning while another scan is ongoing");
    NS_ABORT_MSG_IF(!m_mac->IsAssociated(),
                    "Offline scanning can only be requested when associated");

    // save current channels
    ExpandScanningChannelsMap();
    m_activeBeforeScan.clear();

    using enum WifiOfflineScanProtection;
    switch (m_offlineScanProtection)
    {
    case NONE:
        break;
    case POWERSAVE:
        const auto setupLinkIds = m_mac->GetSetupLinkIds();
        for (const auto linkId : m_mac->GetLinkIds())
        {
            if (auto phy = m_mac->GetWifiPhy(linkId);
                phy && m_scanChannels.contains(phy->GetPhyId()) && setupLinkIds.contains(linkId) &&
                (m_mac->GetPmMode(linkId) == WifiPowerManagementMode::WIFI_PM_ACTIVE))
            {
                m_activeBeforeScan.emplace(linkId);
                m_mac->SetPowerSaveMode({true, linkId});
            }
        }
        if (!m_activeBeforeScan.empty())
        {
            return; // some STAs need to switch to powersave mode first
        }
        break;
    }

    StartScanning(true);
}

void
WifiAssocManager::NotifyPmModeChanged(WifiPowerManagementMode pmMode, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << pmMode << linkId);

    if (m_activeBeforeScan.contains(linkId) &&
        std::all_of(m_activeBeforeScan.cbegin(), m_activeBeforeScan.cend(), [this](auto id) {
            return m_mac->GetPmMode(id) == WifiPowerManagementMode::WIFI_PM_POWERSAVE;
        }))
    {
        // all the STAs we were waiting for have switched to powersave mode; start offline scanning
        StartScanning(true);
    }
}

const WifiScanParams&
WifiAssocManager::GetScanParams() const
{
    return m_scanParams;
}

void
WifiAssocManager::RequestOfflineScanning(const Time& interval)
{
    NS_LOG_FUNCTION(this << interval);

    m_nextOfflineChannelScan.Cancel();

    if (interval.IsZero())
    {
        return;
    }

    if (m_mac && m_mac->IsAssociated() && !IsScanning())
    {
        StartOfflineScanning();
    }

    m_nextOfflineChannelScan =
        Simulator::Schedule(interval, &WifiAssocManager::RequestOfflineScanning, this, interval);
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
WifiAssocManager::StartScanning(bool offline)
{
    NS_LOG_FUNCTION(this << offline);

    m_currScanResults.perChanResults.clear();

    if (!m_enableScanning)
    {
        NS_LOG_DEBUG("Scanning disabled");
        return;
    }

    NS_ASSERT(m_mac);
    if (offline && !m_mac->IsAssociated())
    {
        NS_LOG_DEBUG("Offline scanning cancelled, not associated");
        return;
    }

    m_scanParams.ssid = m_mac->GetSsid();

    m_scanParams.channelList = GetCurrentChannelsForScanning();
    BooleanValue activeProbing;
    m_mac->GetAttribute("ActiveProbing", activeProbing);

    if (activeProbing.Get())
    {
        PointerValue probeDelay;
        m_mac->GetAttribute("ProbeDelay", probeDelay);
        TimeValue probeReqTimeout;
        m_mac->GetAttribute("ProbeRequestTimeout", probeReqTimeout);

        m_scanParams.type = WifiScanType::ACTIVE;
        m_scanParams.probeDelay = MicroSeconds(probeDelay.Get<RandomVariableStream>()->GetValue());
        m_scanParams.minChannelTime = m_scanParams.maxChannelTime = probeReqTimeout.Get();
    }
    else
    {
        TimeValue waitBeaconTimeout;
        m_mac->GetAttribute("WaitBeaconTimeout", waitBeaconTimeout);

        m_scanParams.type = WifiScanType::PASSIVE;
        m_scanParams.maxChannelTime = waitBeaconTimeout.Get();
    }

    const auto expectedChannels = std::accumulate(
        m_scanParams.channelList.cbegin(),
        m_scanParams.channelList.cend(),
        0,
        [](auto sum, const auto& phyIdChannelsPair) {
            return sum + std::accumulate(phyIdChannelsPair.second.cbegin(),
                                         phyIdChannelsPair.second.cend(),
                                         0,
                                         [](auto innerSum, const auto& bandChannelsPair) {
                                             return innerSum + bandChannelsPair.second.size();
                                         });
        });
    m_currScanResults.perChanResults.reserve(expectedChannels);

    for (const auto& [phyId, bandChannelsMap] : m_scanParams.channelList)
    {
        if (const auto linkId = m_mac->GetLinkForPhy(phyId))
        {
            m_mac->NotifyScanningStarted(offline, *linkId);
        }
    }

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
    const auto isMultipleOf20MHz = (static_cast<uint16_t>(phy->GetChannelWidth()) % 20 == 0);
    const auto width = (!isMultipleOf20MHz ? phy->GetChannelWidth() : MHz_u{20});
    const auto channelNo =
        (!isMultipleOf20MHz
             ? phy->GetChannelNumber()
             : phy->GetOperatingChannel().GetPrimaryChannelNumber(width, phy->GetStandard()));

    if (channelNo != *channelIt)
    {
        // we need to switch channel
        if (phy->IsStateSleep())
        {
            // switching channel while a PHY is in sleep state fails
            phy->ResumeFromSleep();
        }

        NS_LOG_DEBUG("Switching PHY " << +phyId << " to channel " << *channelIt << ", " << band);
        phy->SetOperatingChannel(WifiPhy::ChannelSegments{{*channelIt, width, band, 0}});

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

std::optional<WifiAssocManager::PerChanScanResultsIterator>
WifiAssocManager::GetLastPerChanScanResult(uint8_t phyId)
{
    const auto it = std::find_if(m_currScanResults.perChanResults.rbegin(),
                                 m_currScanResults.perChanResults.rend(),
                                 [phyId](auto&& result) { return result.phyId == phyId; });
    if (it == m_currScanResults.perChanResults.rend())
    {
        return std::nullopt;
    }
    return std::prev(it.base());
}

void
WifiAssocManager::ScanCurrentChannel(
    uint8_t phyId,
    WifiPhyBand band,
    WifiScanParams::ChannelList::mapped_type::const_iterator channelIt)
{
    NS_LOG_FUNCTION(this << phyId << band << *channelIt);

    auto phy = m_mac->GetDevice()->GetPhy(phyId);
    NS_ASSERT(phy);
    const auto isMultipleOf20MHz = (static_cast<uint16_t>(phy->GetChannelWidth()) % 20 == 0);
    const auto width = (!isMultipleOf20MHz ? phy->GetChannelWidth() : MHz_u{20});
    const WifiChannelConfig scanChannel{
        WifiChannelConfig::SegmentWithoutUnits{static_cast<uint8_t>(*channelIt), width, band, 0}};

    if (auto last = GetLastPerChanScanResult(phyId))
    {
        (*last)->scanEndTime = Simulator::Now() - phy->GetChannelSwitchDelay();
    }
    m_currScanResults.perChanResults.push_back(
        {.phyId = phyId, .scanStartTime = Simulator::Now(), .scanChannel = scanChannel});

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
WifiAssocManager::NotifyChannelSwitched(linkId_t linkId)
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
            // no more channels to set, we can return to StaWifiMac or handle offline scan end
            if (m_mac->IsScanningChannelsOffline())
            {
                HandleOfflineScanningEnded();
            }
            else
            {
                m_mac->ScanningTimeout(m_bestAp);
            }
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

    // no other channel to scan for this PHY; record the end time of the final channel.
    if (auto last = GetLastPerChanScanResult(phyId))
    {
        (*last)->scanEndTime = Simulator::Now();
    }

    // if we are not waiting for other PHYs to finish their scanning, we are done
    if (!IsScanning())
    {
        EndScanning();
        m_scanEndLogger(m_currScanResults);
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

    if (!m_currScanResults.perChanResults.empty())
    {
        // currently scanning, store the AP info in the current scan results
        m_currScanResults.perChanResults.back().apList.emplace_back(apInfo);
    }

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

    do
    {
        if (m_apList.empty())
        {
            m_mac->ScanningTimeout(std::nullopt);
            return;
        }

        m_bestAp = std::move(m_apList.extract(m_apList.begin()).value());
        m_apListIt.erase(m_bestAp->m_bssid);
    } while (!CanBeReturned(*m_bestAp));

    NS_ABORT_MSG_IF(!m_allowAssocAllChannelWidths && !IsChannelWidthCompatible(*m_bestAp),
                    "Channel width of STA is not part of the channel width set that can be "
                    "advertised to the AP");

    // FIXME The ability to switch channel to match the AP channel requires the usage of the
    // GetApOperatingChannel() function, which currently only works under some assumptions
    if (auto checkOpIes =
            [&](auto&& frame) {
                if (m_bestAp->m_channel.band == WIFI_PHY_BAND_5GHZ)
                {
                    return frame.template Get<VhtOperation>().has_value() &&
                           frame.template Get<HtOperation>().has_value();
                }
                if (m_bestAp->m_channel.band == WIFI_PHY_BAND_6GHZ)
                {
                    return frame.template Get<HeOperation>().has_value();
                }
                return false;
            };
        !m_enableChannelSwitch ||
        m_mac->GetWifiPhy(m_bestAp->m_linkId)->GetStandard() < WIFI_STANDARD_80211ac ||
        !std::visit(checkOpIes, m_bestAp->m_frame))
    {
        m_mac->ScanningTimeout(m_bestAp);
        return;
    }

    // check if a channel switch is required
    const auto phy = m_mac->GetWifiPhy(m_bestAp->m_linkId);
    NS_ASSERT_MSG(phy, "No PHY operating on link " << +m_bestAp->m_linkId);

    auto channelIt = WifiPhyOperatingChannel::FindFirst(m_bestAp->m_channel.number,
                                                        MHz_u{0},
                                                        MHz_u{0},
                                                        phy->GetStandard(),
                                                        m_bestAp->m_channel.band);
    NS_ASSERT_MSG(channelIt != WifiPhyOperatingChannel::m_frequencyChannels.cend(),
                  "Channel number " << m_bestAp->m_channel.number << " not found in "
                                    << m_bestAp->m_channel.band << " band");

    WifiPhyOperatingChannel channel(channelIt);
    const auto phyId = phy->GetPhyId();

    if (static_cast<uint16_t>(phy->GetChannelWidth()) % 20 == 0)
    {
        // first, determine the channel the AP is operating on
        const auto apChannel = GetApOperatingChannel(m_bestAp->m_apBw,
                                                     m_bestAp->m_channel.band,
                                                     phy->GetStandard(),
                                                     m_bestAp->m_frame);
        // then, determine the width of the channel to use, assuming the current channel width
        // as the maximum supported width
        auto channelIt = m_channelsToSet.find(phyId);
        const auto staWidth =
            (channelIt != m_channelsToSet.cend() ? channelIt->second.GetTotalWidth()
                                                 : phy->GetChannelWidth());
        const auto width = std::min(staWidth, apChannel.GetTotalWidth());
        channel = apChannel.GetPrimaryChannel(width);
    }

    if (phy->GetOperatingChannel() != channel)
    {
        // a channel switch is needed
        NS_LOG_DEBUG("Switching PHY " << +phyId << " to " << channel);
        m_channelsToSet = {{phyId, channel}};

        phy->SetOperatingChannel(channel);
        // abort if channel switch does not complete before the channel switch timeout
        m_switchToChannelToSetTimeout[phyId].Cancel();
        m_switchToChannelToSetTimeout[phyId] = Simulator::Schedule(m_channelSwitchTimeout, [=] {
            NS_ABORT_MSG("Unable to set " << channel << " on PHY " << +phyId);
        });
        return;
    }

    m_mac->ScanningTimeout(m_bestAp);
}

void
WifiAssocManager::HandleOfflineScanningEnded()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_mac);

    // restore PM mode
    for (const auto linkId : m_activeBeforeScan)
    {
        m_mac->SetPowerSaveMode({false, linkId});
    }
    m_activeBeforeScan.clear();

    // notify the StaWifiMac that the offline scanning has ended
    m_mac->ScanningTimeout(std::nullopt);
}

void
WifiAssocManager::OfflineScanningTimeout()
{
    NS_LOG_FUNCTION(this);

    m_bestAp.reset();

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
        HandleOfflineScanningEnded();
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
