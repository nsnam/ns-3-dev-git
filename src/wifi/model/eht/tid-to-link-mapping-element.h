/*
 * Copyright (c) 2022
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
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef TID_TO_LINK_MAPPING_H
#define TID_TO_LINK_MAPPING_H

#include <ns3/wifi-information-element.h>

#include <list>
#include <map>
#include <optional>
#include <vector>

namespace ns3
{

/**
 * TID-to-Link Mapping Control Direction
 * IEEE 802.11be D2.0 Figure 9-1002an
 */
enum class TidLinkMapDir : uint8_t
{
    DOWNLINK = 0,
    UPLINK = 1,
    BOTH_DIRECTIONS = 2,
};

/// whether to enforce the default link mapping
constexpr auto DEFAULT_WIFI_TID_LINK_MAPPING{true};
/// default value for the Direction subfield of the TID-To-Link Control field
constexpr auto DEFAULT_WIFI_TID_LINK_MAP_DIR{TidLinkMapDir::BOTH_DIRECTIONS};
/// size in bytes of the Link Mapping Of TID n field (IEEE 802.11be D2.0 9.4.2.314)
constexpr uint16_t WIFI_LINK_MAPPING_PER_TID_SIZE_B = 2;
/// size in bytes of the TID-To-Link Control field with default link mapping
constexpr uint16_t WIFI_TID_TO_LINK_MAPPING_CONTROL_BASIC_SIZE_B =
    1; // IEEE 802.11be D2.0 9.4.2.314
/// size in bytes of the Link Mapping Presence Indicator field (IEEE 802.11be D2.0 9.4.2.314)
constexpr uint16_t WIFI_LINK_MAPPING_PRESENCE_IND_SIZE_B = 1;

/**
 * \brief TID-to-Link Mapping Information Element
 * \ingroup wifi
 *
 * This class serializes and deserializes
 * the TID-to-Link Mapping element
 * IEEE 802.11be D2.0 9.4.2.314
 *
 */
class TidToLinkMapping : public WifiInformationElement
{
  public:
    /**
     * TID-to-Link Mapping Control subfield
     * IEEE 802.11be D2.0 Figure 9-1002an
     */
    struct Control
    {
        TidLinkMapDir direction{DEFAULT_WIFI_TID_LINK_MAP_DIR}; ///< Direction
        bool defaultMapping{DEFAULT_WIFI_TID_LINK_MAPPING};     ///< Default link mapping
        std::optional<uint8_t> presenceBitmap;                  ///< Link Mapping Presence Indicator

        /// \return Serialized size of TID-to-Link Mapping Control in octets
        uint16_t GetSubfieldSize() const;

        /**
         * Serialize the TID-to-Link Mapping Control subfield
         *
         * \param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;
        /**
         * Deserialize the TID-to-Link Mapping Control subfield
         *
         * \param start iterator pointing to where the subfield should be read from
         * \return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;

    /**
     * Set the Link Mapping field of the given TID such that the given TID is mapped
     * to the links associated with the given link IDs.
     *
     * \param tid the given TID
     * \param linkIds the IDs of the links which the given TID is mapped to
     */
    void SetLinkMappingOfTid(uint8_t tid, std::list<uint8_t> linkIds);
    /**
     * Get the Link Mapping field of the given TID.
     *
     * \param tid the given TID
     * \return the IDs of the links which the given TID is mapped to
     */
    std::list<uint8_t> GetLinkMappingOfTid(uint8_t tid) const;

    TidToLinkMapping::Control m_control;       ///< TID-to-link Mapping Control
    std::map<uint8_t, uint16_t> m_linkMapping; ///< TID-indexed Link Mapping

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* TID_TO_LINK_MAPPING_H */
