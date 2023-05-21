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

#ifndef EHT_OPERATION_H
#define EHT_OPERATION_H

#include <ns3/wifi-information-element.h>

#include <optional>
#include <vector>

namespace ns3
{

/// IEEE 802.11be D2.0 Figure 9-1002ai
constexpr uint8_t WIFI_EHT_MAX_MCS_INDEX = 13;
/// IEEE 802.11be D2.0 Figure 9-1002b
constexpr uint16_t WIFI_EHT_OP_PARAMS_SIZE_B = 1;
/// IEEE 802.11be D2.0 Figure 9-1002c
constexpr uint16_t WIFI_EHT_OP_INFO_BASIC_SIZE_B = 3;
/// IEEE 802.11be D2.0 Figure 9-1002c
constexpr uint16_t WIFI_EHT_DISABLED_SUBCH_BM_SIZE_B = 2;
/// IEEE 802.11be D2.0 Figure 9-1002ai
constexpr uint16_t WIFI_EHT_BASIC_MCS_NSS_SET_SIZE_B = 4;
/// Default max Tx/Rx NSS
constexpr uint8_t WIFI_DEFAULT_EHT_MAX_NSS = 1;
/// Max NSS configurable, 802.11be D2.0 Table 9-401m
constexpr uint8_t WIFI_EHT_MAX_NSS_CONFIGURABLE = 8;
/// Default EHT Operation Info Present
constexpr uint8_t WIFI_DEFAULT_EHT_OP_INFO_PRESENT = 0;
/// Default Disabled Subch Bitmap Present
constexpr uint8_t WIFI_DEFAULT_EHT_OP_DIS_SUBCH_BM_PRESENT = 0;
/// Default PE Duration
constexpr uint8_t WIFI_DEFAULT_EHT_OP_PE_DUR = 0;
/// Default Group Addressed BU Indication Limit
constexpr uint8_t WIFI_DEFAULT_GRP_BU_IND_LIMIT = 0;
/// Default Group Addressed BU Exponent
constexpr uint8_t WIFI_DEFAULT_GRP_BU_EXP = 0;

/**
 * \brief EHT Operation Information Element
 * \ingroup wifi
 *
 * This class serializes and deserializes
 * the EHT Operation Information Element
 * IEEE 802.11be D2.0 9.4.2.311
 *
 */
class EhtOperation : public WifiInformationElement
{
  public:
    /**
     * EHT Operation Parameters subfield
     * IEEE 802.11be D2.0 Figure 9-1002b
     */
    struct EhtOpParams
    {
        /// EHT Operation Information Present
        uint8_t opInfoPresent{WIFI_DEFAULT_EHT_OP_INFO_PRESENT};
        /// Disabled Subchannel Bitmap Present
        uint8_t disabledSubchBmPresent{WIFI_DEFAULT_EHT_OP_DIS_SUBCH_BM_PRESENT};
        /// EHT Default PE Duration
        uint8_t defaultPeDur{WIFI_DEFAULT_EHT_OP_PE_DUR};
        /// Group Addressed BU Indication Limit
        uint8_t grpBuIndLimit{WIFI_DEFAULT_GRP_BU_IND_LIMIT};
        /// Group Addressed BU Indication Exponent
        uint8_t grpBuExp{WIFI_DEFAULT_GRP_BU_EXP};

        /**
         * Serialize the EHT Operation Parameters subfield
         *
         * \param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;
        /**
         * Deserialize the EHT Operation Parameters subfield
         *
         * \param start iterator pointing to where the subfield should be read from
         * \return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    /**
     * EHT Operation Information Control subfield
     * IEEE 802.11be D2.0 Figure 9-1002D
     */
    struct EhtOpControl
    {
        uint8_t channelWidth : 3; ///< EHT BSS bandwidth
    };

    /**
     * EHT Operation Information subfield
     * IEEE 802.11be D2.0 Figure 9-1002c
     */
    struct EhtOpInfo
    {
        EhtOpControl control;                    ///< Control subfield
        uint8_t ccfs0;                           ///< Channel center frequency segment 0
        uint8_t ccfs1;                           ///< Channel center frequency segment 1
        std::optional<uint16_t> disabledSubchBm; ///< Disabled subchannel bitmap

        /**
         * Serialize the EHT Operation Information subfield
         *
         * \param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;
        /**
         * Deserialize the EHT Operation Information subfield
         *
         * \param start iterator pointing to where the subfield should be read from
         * \param disabledSubchBmPresent EHT Operation Param Disabled Subchannel Bitmap Present
         * \return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start, bool disabledSubchBmPresent);
    };

    /**
     * Basic EHT-MCS and NSS Set subfield
     * IEEE 802.11be D2.0 Figure 9-1002ai
     */
    struct EhtBasicMcsNssSet
    {
        std::vector<uint8_t> maxRxNss{}; ///< Max Rx NSS per MCS
        std::vector<uint8_t> maxTxNss{}; ///< Max Tx NSS per MCS

        /**
         * Serialize the Basic EHT-MCS and NSS Set subfield
         *
         * \param start iterator pointing to where the subfield should be written to
         */
        void Serialize(Buffer::Iterator& start) const;
        /**
         * Deserialize the Basic EHT-MCS and NSS Set subfield
         *
         * \param start iterator pointing to where the subfield should be read from
         * \return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    EhtOperation();
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the max Rx NSS for input MCS index range
     * \param maxNss the maximum supported Rx NSS for MCS group
     * \param mcsStart MCS index start
     * \param mcsEnd MCS index end
     */
    void SetMaxRxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd);
    /**
     * Set the max Tx NSS for input MCS index range
     * \param maxNss the maximum supported Rx NSS for MCS group
     * \param mcsStart MCS index start
     * \param mcsEnd MCS index end
     */
    void SetMaxTxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd);

    EhtOpParams m_params;              ///< EHT Operation Parameters
    EhtBasicMcsNssSet m_mcsNssSet;     ///< Basic EHT-MCS and NSS set
    std::optional<EhtOpInfo> m_opInfo; ///< EHT Operation Information

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* EHT_OPERATION_H */
