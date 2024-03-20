/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II

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

#include "wifi-assoc-manager.h"

#include "sta-wifi-mac.h"

#include "ns3/attribute-container.h"
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
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()));
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
    auto channelMatch = [&apInfo](auto&& channel) {
        if (channel.number != 0 && channel.number != apInfo.m_channel.number)
        {
            return false;
        }
        if (channel.band != WIFI_PHY_BAND_UNSPECIFIED && channel.band != apInfo.m_channel.band)
        {
            return false;
        }
        return true;
    };

    NS_ASSERT(apInfo.m_linkId < m_scanParams.channelList.size());
    if (std::find_if(m_scanParams.channelList[apInfo.m_linkId].cbegin(),
                     m_scanParams.channelList[apInfo.m_linkId].cend(),
                     channelMatch) == m_scanParams.channelList[apInfo.m_linkId].cend())
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
WifiAssocManager::NotifyApInfo(const StaWifiMac::ApInfo&& apInfo)
{
    NS_LOG_FUNCTION(this << apInfo);

    if (!CanBeInserted(apInfo) || !MatchScanParams(apInfo) ||
        (!m_allowedLinks.empty() && !m_allowedLinks.contains(apInfo.m_linkId)))
    {
        return;
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

    m_mac->ScanningTimeout(std::move(bestAp));
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

    if (m_mac->GetNLinks() == 1 || GetSortedList().empty())
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
        EnumValue<WifiTidToLinkMappingNegSupport> negSupport;
        ehtConfig->GetAttributeFailSafe("TidToLinkMappingNegSupport", negSupport);

        // A non-AP MLD that performs multi-link (re)setup on at least two links with an AP MLD
        // that sets the TID-To-Link Mapping Negotiation Support subfield of the MLD Capabilities
        // field of the Basic Multi-Link element to a nonzero value shall support TID-to-link
        // mapping negotiation with the TID-To-Link Mapping Negotiation Support subfield of the
        // MLD Capabilities field of the Basic Multi-Link element it transmits to at least 1.
        // (Sec. 35.3.7.1.1 of 802.11be D3.1)
        if (mldCapabilities->tidToLinkMappingSupport > 0 &&
            negSupport.Get() == WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
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
               rnr.GetMldId(nbrApInfoId, tbttInfoFieldIndex) != 0)
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

} // namespace ns3
