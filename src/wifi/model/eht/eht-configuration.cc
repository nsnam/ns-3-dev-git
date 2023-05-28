/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "eht-configuration.h"

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pair.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtConfiguration");

NS_OBJECT_ENSURE_REGISTERED(EhtConfiguration);

EhtConfiguration::EhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

EhtConfiguration::~EhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
EhtConfiguration::GetTypeId()
{
    using TidLinkMapValue =
        PairValue<AttributeContainerValue<UintegerValue>, AttributeContainerValue<UintegerValue>>;

    static ns3::TypeId tid =
        ns3::TypeId("ns3::EhtConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<EhtConfiguration>()
            .AddAttribute("EmlsrActivated",
                          "Whether EMLSR option is activated. If activated, EMLSR mode can be "
                          "enabled on the EMLSR links by an installed EMLSR Manager.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          BooleanValue(false),
                          MakeBooleanAccessor(&EhtConfiguration::m_emlsrActivated),
                          MakeBooleanChecker())
            .AddAttribute("TransitionTimeout",
                          "The Transition Timeout (not used by non-AP MLDs). "
                          "Possible values are 0us or 2^n us, with n=7..16.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&EhtConfiguration::m_transitionTimeout),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(65536)))
            .AddAttribute(
                "TidToLinkMappingNegSupport",
                "TID-to-Link Mapping Negotiation Support.",
                EnumValue(WifiTidToLinkMappingNegSupport::WIFI_TID_TO_LINK_MAPPING_ANY_LINK_SET),
                MakeEnumAccessor(&EhtConfiguration::m_tidLinkMappingSupport),
                MakeEnumChecker(
                    WifiTidToLinkMappingNegSupport::WIFI_TID_TO_LINK_MAPPING_NOT_SUPPORTED,
                    "NOT_SUPPORTED",
                    WifiTidToLinkMappingNegSupport::WIFI_TID_TO_LINK_MAPPING_SAME_LINK_SET,
                    "SAME_LINK_SET",
                    WifiTidToLinkMappingNegSupport::WIFI_TID_TO_LINK_MAPPING_ANY_LINK_SET,
                    "ANY_LINK_SET"))
            .AddAttribute(
                "TidToLinkMappingDl",
                "A list-of-TIDs-indexed map of the list of links where the TIDs are mapped to "
                "for the downlink direction. "
                "In case a string is used to set this attribute, the string shall contain the "
                "(TID list, link list) pairs separated by a semicolon (;); in every pair, the "
                "TID list and the link list are separated by a blank space, and the elements of "
                "each list are separated by a comma (,) without spaces. "
                "E.g., \"0,4 1,2,3; 1 0;2 0,1\" means that TIDs 0 and 4 are mapped on links "
                "1, 2 and 3; TID 1 is mapped on link 0 and TID 2 is mapped on links 0 and 1. "
                "An empty map indicates the default mapping, i.e., all TIDs are mapped to all "
                "setup links. If the map contains the mapping for some TID(s), the mapping "
                "corresponding to the missing TID(s) remains unchanged. "
                "A non-AP MLD includes this mapping in the Association Request frame sent to "
                "an AP MLD, unless the AP MLD advertises a negotiation support of 1 and this "
                "mapping is such that TIDs are mapped to distinct link sets, in which case "
                "the default mapping is included.",
                StringValue(""),
                MakeAttributeContainerAccessor<TidLinkMapValue, ';'>(
                    &EhtConfiguration::m_linkMappingDl),
                MakeAttributeContainerChecker<TidLinkMapValue, ';'>(
                    MakePairChecker<AttributeContainerValue<UintegerValue>,
                                    AttributeContainerValue<UintegerValue>>(
                        MakeAttributeContainerChecker<UintegerValue>(
                            MakeUintegerChecker<uint8_t>()),
                        MakeAttributeContainerChecker<UintegerValue>(
                            MakeUintegerChecker<uint8_t>()))))
            .AddAttribute(
                "TidToLinkMappingUl",
                "A list-of-TIDs-indexed map of the list of links where the TIDs are mapped to "
                "for the uplink direction. "
                "In case a string is used to set this attribute, the string shall contain the "
                "(TID list, link list) pairs separated by a semicolon (;); in every pair, the "
                "TID list and the link list are separated by a blank space, and the elements of "
                "each list are separated by a comma (,) without spaces. "
                "E.g., \"0,4 1,2,3; 1 0;2 0,1\" means that TIDs 0 and 4 are mapped on links "
                "1, 2 and 3; TID 1 is mapped on link 0 and TID 2 is mapped on links 0 and 1. "
                "An empty map indicates the default mapping, i.e., all TIDs are mapped to all "
                "setup links. If the map contains the mapping for some TID(s), the mapping "
                "corresponding to the missing TID(s) remains unchanged. "
                "A non-AP MLD includes this mapping in the Association Request frame sent to "
                "an AP MLD, unless the AP MLD advertises a negotiation support of 1 and this "
                "mapping is such that TIDs are mapped to distinct link sets, in which case "
                "the default mapping is included.",
                StringValue(""),
                MakeAttributeContainerAccessor<TidLinkMapValue, ';'>(
                    &EhtConfiguration::m_linkMappingUl),
                MakeAttributeContainerChecker<TidLinkMapValue, ';'>(
                    MakePairChecker<AttributeContainerValue<UintegerValue>,
                                    AttributeContainerValue<UintegerValue>>(
                        MakeAttributeContainerChecker<UintegerValue>(
                            MakeUintegerChecker<uint8_t>()),
                        MakeAttributeContainerChecker<UintegerValue>(
                            MakeUintegerChecker<uint8_t>()))));
    return tid;
}

WifiTidLinkMapping
EhtConfiguration::GetTidLinkMapping(WifiDirection dir) const
{
    NS_ASSERT(dir != WifiDirection::BOTH_DIRECTIONS);
    WifiTidLinkMapping ret;
    const auto& linkMapping = (dir == WifiDirection::DOWNLINK ? m_linkMappingDl : m_linkMappingUl);

    for (const auto& [tids, links] : linkMapping)
    {
        for (auto tid : tids)
        {
            ret[tid] = std::set<uint8_t>(links.cbegin(), links.cend());
        }
    }
    return ret;
}

void
EhtConfiguration::SetTidLinkMapping(WifiDirection dir,
                                    const std::map<std::list<uint8_t>, std::list<uint8_t>>& mapping)
{
    NS_ASSERT(dir != WifiDirection::BOTH_DIRECTIONS);
    auto& linkMapping = (dir == WifiDirection::DOWNLINK ? m_linkMappingDl : m_linkMappingUl);
    linkMapping.clear();
    for (const auto& [tids, links] : mapping)
    {
        linkMapping.emplace(std::list<uint64_t>(tids.cbegin(), tids.cend()),
                            std::list<uint64_t>(links.cbegin(), links.cend()));
    }
}

} // namespace ns3
