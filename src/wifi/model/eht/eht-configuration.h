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

#ifndef EHT_CONFIGURATION_H
#define EHT_CONFIGURATION_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-utils.h"

#include <list>
#include <map>

namespace ns3
{

/**
 * \brief TID-to-Link Mapping Negotiation Support
 */
enum WifiTidToLinkMappingNegSupport : uint8_t
{
    WIFI_TID_TO_LINK_MAPPING_NOT_SUPPORTED = 0,
    WIFI_TID_TO_LINK_MAPPING_SAME_LINK_SET = 1,
    WIFI_TID_TO_LINK_MAPPING_ANY_LINK_SET = 3
};

/**
 * \brief EHT configuration
 * \ingroup wifi
 *
 * This object stores EHT configuration information, for use in modifying
 * AP or STA behavior and for constructing EHT-related information elements.
 *
 */
class EhtConfiguration : public Object
{
  public:
    EhtConfiguration();
    ~EhtConfiguration() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \param dir the direction for the requested TID-to-Link Mapping
     * \return a TID-indexed map of the list of links where each TID is mapped to
     */
    WifiTidLinkMapping GetTidLinkMapping(WifiDirection dir) const;

    /**
     * Set the TID-to-Link mapping for the given direction.
     *
     * \param dir the direction for the TID-to-Link Mapping
     * \param mapping a list-of-TIDs-indexed map of the list of links where the TIDs are mapped to
     */
    void SetTidLinkMapping(WifiDirection dir,
                           const std::map<std::list<uint8_t>, std::list<uint8_t>>& mapping);

  private:
    bool m_emlsrActivated;    //!< whether EMLSR option is activated
    Time m_transitionTimeout; //!< Transition timeout
    WifiTidToLinkMappingNegSupport
        m_tidLinkMappingSupport; //!< TID-to-Link Mapping Negotiation Support
    std::map<std::list<uint64_t>, std::list<uint64_t>>
        m_linkMappingDl; //!< TIDs-indexed Link Mapping for downlink
    std::map<std::list<uint64_t>, std::list<uint64_t>>
        m_linkMappingUl; //!< TIDs-indexed Link Mapping for uplink
};

} // namespace ns3

#endif /* EHT_CONFIGURATION_H */
