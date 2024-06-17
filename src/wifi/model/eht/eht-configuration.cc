/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "eht-configuration.h"

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/pair.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtConfiguration");

NS_OBJECT_ENSURE_REGISTERED(EhtConfiguration);

std::ostream&
operator<<(std::ostream& os, WifiTidToLinkMappingNegSupport negsupport)
{
    switch (negsupport)
    {
    case WifiTidToLinkMappingNegSupport::NOT_SUPPORTED:
        return os << "NOT_SUPPORTED";
    case WifiTidToLinkMappingNegSupport::SAME_LINK_SET:
        return os << "SAME_LINK_SET";
    case WifiTidToLinkMappingNegSupport::ANY_LINK_SET:
        return os << "ANY_LINK_SET";
    };
    return os << "UNKNOWN(" << static_cast<uint32_t>(negsupport) << ")";
}

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
                "MediumSyncDuration",
                "The duration of the MediumSyncDelay timer (must be a multiple of 32 us). "
                "The value of this attribute is only used by AP MLDs with EMLSR activated.",
                TimeValue(MicroSeconds(DEFAULT_MSD_DURATION_USEC)),
                MakeTimeAccessor(&EhtConfiguration::m_mediumSyncDuration),
                MakeTimeChecker(MicroSeconds(0), MicroSeconds(255 * 32)))
            .AddAttribute(
                "MsdOfdmEdThreshold",
                "Threshold (dBm) to be used instead of the normal CCA sensitivity for the primary "
                "20 MHz channel if the MediumSyncDelay timer has a nonzero value. "
                "The value of this attribute is only used by AP MLDs with EMLSR activated.",
                IntegerValue(DEFAULT_MSD_OFDM_ED_THRESH),
                MakeIntegerAccessor(&EhtConfiguration::m_msdOfdmEdThreshold),
                MakeIntegerChecker<int8_t>(-72, -62))
            .AddAttribute(
                "MsdMaxNTxops",
                "Maximum number of TXOPs that an EMLSR client is allowed to attempt to initiate "
                "while the MediumSyncDelay timer is running (zero indicates no limit). "
                "The value of this attribute is only used by AP MLDs with EMLSR activated.",
                UintegerValue(DEFAULT_MSD_MAX_N_TXOPS),
                MakeUintegerAccessor(&EhtConfiguration::m_msdMaxNTxops),
                MakeUintegerChecker<uint8_t>(0, 15))
            .AddAttribute("TidToLinkMappingNegSupport",
                          "TID-to-Link Mapping Negotiation Support.",
                          EnumValue(WifiTidToLinkMappingNegSupport::ANY_LINK_SET),
                          MakeEnumAccessor<WifiTidToLinkMappingNegSupport>(
                              &EhtConfiguration::m_tidLinkMappingSupport),
                          MakeEnumChecker(WifiTidToLinkMappingNegSupport::NOT_SUPPORTED,
                                          "NOT_SUPPORTED",
                                          WifiTidToLinkMappingNegSupport::SAME_LINK_SET,
                                          "SAME_LINK_SET",
                                          WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
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
