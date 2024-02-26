/*
 * Copyright (c) 2017 Sébastien Deronne
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

#ifndef HE_OPERATION_H
#define HE_OPERATION_H

#include "ns3/wifi-information-element.h"
#include "ns3/wifi-opt-field.h"

namespace ns3
{

/**
 * \brief The HE Operation Information Element
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise
 * the HE Operation Information Element
 */
class HeOperation : public WifiInformationElement
{
  public:
    HeOperation();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /// HE Operation Parameters field
    struct HeOperationParams
    {
        uint8_t m_defaultPeDuration : 3 {0};  ///< Default PE Duration
        uint8_t m_twtRequired : 1 {0};        ///< TWT Required
        uint16_t m_txopDurRtsThresh : 10 {0}; ///< TXOP Duration RTS Threshold
        uint8_t m_vhOpPresent : 1 {0}; ///< VHT Operation Information Present (value 1 unsupported)
        uint8_t m_coHostedBss : 1 {0}; ///< Co-Hosted BSS (value 1 unsupported)
        uint8_t m_erSuDisable : 1 {0}; ///< ER SU Disable
        bool m_6GHzOpPresent{false};   ///< 6 GHz Operation Information Present (do not set, it is
                                       ///< set by the OptFieldWithPresenceInd)

        /**
         * Print the content of the HE Operation Parameters field.
         *
         * \param os output stream
         */
        void Print(std::ostream& os) const;

        /**
         * \return the serialized size of the HE Operation Parameters field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Serialize the HE Operation Parameters field
         *
         * \param start an iterator which points to where the information should be written
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the HE Operation Parameters field.
         *
         * \param start an iterator which points to where the information should be read from
         * \return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator& start);
    };

    /// BSS Color Information field
    struct BssColorInfo
    {
        uint8_t m_bssColor : 6 {0};         ///< BSS Color
        uint8_t m_partialBssColor : 1 {0};  ///< Partial BSS Color
        uint8_t m_bssColorDisabled : 1 {0}; ///< BSS Color Disabled

        /**
         * Print the content of the BSS Color Information field.
         *
         * \param os output stream
         */
        void Print(std::ostream& os) const;

        /**
         * \return the serialized size of the BSS Color Information field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Serialize the BSS Color Information field
         *
         * \param start an iterator which points to where the information should be written
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the BSS Color Information field.
         *
         * \param start an iterator which points to where the information should be read from
         * \return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator& start);
    };

    /// 6 GHz Operation Information field
    struct OpInfo6GHz
    {
        uint8_t m_primCh{0};         ///< Primary Channel
        uint8_t m_chWid : 2 {0};     ///< Channel Width
        uint8_t m_dupBeacon : 1 {0}; ///< Duplicate Beacon
        uint8_t m_regInfo : 3 {0};   ///< Regulatory Info
        uint8_t : 2;                 ///< Reserved bits
        uint8_t m_chCntrFreqSeg0{0}; ///< Channel center frequency segment 0
        uint8_t m_chCntrFreqSeg1{0}; ///< Channel center frequency segment 1
        uint8_t m_minRate{0};        ///< Minimum Rate

        /**
         * Print the content of the 6 GHz Operation Information field.
         *
         * \param os output stream
         */
        void Print(std::ostream& os) const;

        /**
         * \return the serialized size of the 6 GHz Operation Information field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Serialize the 6 GHz Operation Information field
         *
         * \param start an iterator which points to where the information should be written
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the 6 GHz Operation Information field.
         *
         * \param start an iterator which points to where the information should be read from
         * \return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator& start);
    };

    /**
     * Set the Basic HE-MCS and NSS field in the HE Operation information element
     * by specifying the pair (<i>nss</i>, <i>maxMcs</i>).
     *
     * \param nss the NSS
     * \param maxHeMcs the maximum supported HE-MCS value corresponding to that NSS
     */
    void SetMaxHeMcsPerNss(uint8_t nss, uint8_t maxHeMcs);

    // Fields
    HeOperationParams m_heOpParams; //!< HE Operation Parameters field
    BssColorInfo m_bssColorInfo;    //!< BSS Color Information field
    uint16_t m_basicHeMcsAndNssSet; ///< Basic HE-MCS And NSS set (use setter to set value)
    OptFieldWithPresenceInd<OpInfo6GHz> m_6GHzOpInfo; ///< 6 GHz Operation Information field

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* HE_OPERATION_H */
