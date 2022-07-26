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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EHT_CAPABILITIES_H
#define EHT_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * EHT MAC Capabilities Info subfield.
 * See IEEE 802.11be D1.5 9.4.2.313.2 EHT MAC Capabilities Information subfield
 */
struct EhtMacCapabilities
{
    uint8_t epcsPriorityAccessSupported : 1;      //!< EPCS Priority Access Supported
    uint8_t ehtOmControlSupport : 1;              //!< EHT OM Control Support
    uint8_t triggeredTxopSharingMode1Support : 1; //!< Triggered TXOP Sharing Mode 1 Support
    uint8_t triggeredTxopSharingMode2Support : 1; //!< Triggered TXOP Sharing Mode 2 Support
    uint8_t restrictedTwtSupport : 1;             //!< Restricted TWT Support
    uint8_t scsTrafficDescriptionSupport : 1;     //!< SCS Traffic Description Support
    uint8_t maxMpduLength : 2;                    //!< Maximum MPDU Length
    uint8_t maxAmpduLengthExponentExtension : 1;  //!< Maximum A-MPDU length exponent extension

    /**
     * Get the size of the serialized EHT MAC capabilities subfield
     *
     * \return the size of the serialized EHT MAC capabilities subfield
     */
    uint16_t GetSize() const;
    /**
     * Serialize the EHT MAC capabilities subfield
     *
     * \param start iterator pointing to where the EHT MAC capabilities subfield should be written
     * to
     */
    void Serialize(Buffer::Iterator& start) const;
    /**
     * Deserialize the EHT MAC capabilities subfield
     *
     * \param start iterator pointing to where the EHT MAC capabilities subfield should be read from
     * \return the number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator start);
};

/**
 * EHT PHY Capabilities Info subfield.
 * See IEEE 802.11be D1.5 9.4.2.313.3 EHT PHY Capabilities Information subfield
 */
struct EhtPhyCapabilities
{
    uint8_t support320MhzIn6Ghz : 1;                 //!< Support For 320 MHz In 6 GHz
    uint8_t support242ToneRuInBwLargerThan20Mhz : 1; //!< Support For 242-tone RU In BW Wider Than
                                                     //!< 20 MHz
    uint8_t ndpWith4TimesEhtLtfAnd32usGi : 1;        //!< NDP With 4x EHT-LTF And 3.2 μs GI
    uint8_t partialBandwidthUlMuMimo : 1;            //!< Partial Bandwidth UL MU-MIMO
    uint8_t suBeamformer : 1;                        //!< SU Beamformer
    uint8_t suBeamformee : 1;                        //!< SU Beamformee
    uint8_t beamformeeSsBwNotLargerThan80Mhz : 3;    //!< Beamformee SS (≤ 80 MHz)
    uint8_t beamformeeSs160Mhz : 3;                  //!< Beamformee SS (= 160 MHz)
    uint8_t beamformeeSs320Mhz : 3;                  //!< Beamformee SS (= 320 MHz)
    uint8_t nSoundingDimensionsBwNotLargerThan80Mhz : 3; //!< Beamformee SS (≤ 80 MHz)
    uint8_t nSoundingDimensions160Mhz : 3;               //!< Beamformee SS (= 160 MHz)
    uint8_t nSoundingDimensions320Mhz : 3;               //!< Beamformee SS (= 320 MHz)
    uint8_t ng16SuFeedback : 1;         //!< Support for subcarrier grouping of 16 for SU feedback
    uint8_t ng16MuFeedback : 1;         //!< Support for subcarrier grouping of 16 for MU feedback
    uint8_t codebooksizeSuFeedback : 1; //!< Support for a codebook size for SU feedback.
    uint8_t codebooksizeMuFeedback : 1; //!< Support for a codebook size for MU feedback.
    uint8_t triggeredSuBeamformingFeedback : 1;          //!< Triggered SU Beamforming Feedback
    uint8_t triggeredMuBeamformingPartialBwFeedback : 1; //!< Triggered MU Beamforming Partial BW
                                                         //!< Feedback
    uint8_t triggeredCqiFeedback : 1;                    //!< Triggered CQI Feedback
    uint8_t partialBandwidthDlMuMimo : 1;                //!< Partial Bandwidth DL MU-MIMO
    uint8_t psrBasedSpatialReuseSupport : 1;             //!< EHT PSR-Based SR Support
    uint8_t powerBoostFactorSupport : 1;                 //!< Power Boost Factor Support
    uint8_t muPpdu4xEhtLtfAnd800nsGi : 1; //!< EHT MU PPDU With 4x EHT-LTF And 0.8 μs GI
    uint8_t maxNc : 4;                    //!< Max Nc
    uint8_t nonTriggeredCqiFeedback : 1;  //!< Non-Triggered CQI Feedback
    uint8_t supportTx1024And4096QamForRuSmallerThan242Tones : 1; //!< Tx 1024-QAM And 4096-QAM <
                                                                 //!< 242-tone RU Support
    uint8_t supportRx1024And4096QamForRuSmallerThan242Tones : 1; //!< Rx 1024-QAM And 4096-QAM <
                                                                 //!< 242-tone RU Support
    uint8_t ppeThresholdsPresent : 1;                            //!< PPE Thresholds Present
    uint8_t commonNominalPacketPadding : 2;                      //!< Common Nominal Packet Padding
    uint8_t maxNumSupportedEhtLtfs : 5; //!< Maximum Number Of Supported EHT-LTFs
    uint8_t supportMcs15 : 4;           //!< Support Of MCS 15
    uint8_t supportEhtDupIn6GHz : 1;    //!< Support Of EHT DUP (MCS 14) In 6 GHz
    uint8_t
        support20MhzOperatingStaReceivingNdpWithWiderBw : 1; //!< Support For 20 MHz Operating STA
                                                             //!< Receiving NDP With Wider Bandwidth
    uint8_t nonOfdmaUlMuMimoBwNotLargerThan80Mhz : 1; //!< Non-OFDMA UL MU-MIMO (BW ≤ 80 MHz)
    uint8_t nonOfdmaUlMuMimo160Mhz : 1;               //!< Non-OFDMA UL MU-MIMO (BW = 160 MHz)
    uint8_t nonOfdmaUlMuMimo320Mhz : 1;               //!< Non-OFDMA UL MU-MIMO (BW = 320 MHz)
    uint8_t muBeamformerBwNotLargerThan80Mhz : 1;     //!< MU Beamformer (BW ≤ 80 MHz)
    uint8_t muBeamformer160Mhz : 1;                   //!< MU Beamformer (BW = 160 MHz)
    uint8_t muBeamformer320Mhz : 1;                   //!< MU Beamformer (BW = 320 MHz)
    uint8_t tbSoundingFeedbackRateLimit : 1;          //!< TB Sounding Feedback Rate Limit
    uint8_t
        rx1024QamInWiderBwDlOfdmaSupport : 1; //!< Rx 1024-QAM In Wider Bandwidth DL OFDMA Support
    uint8_t
        rx4096QamInWiderBwDlOfdmaSupport : 1; //!< Rx 4096-QAM In Wider Bandwidth DL OFDMA Support

    /**
     * Get the size of the serialized EHT PHY capabilities subfield
     *
     * \return the size of the serialized EHT PHY capabilities subfield
     */
    uint16_t GetSize() const;
    /**
     * Serialize the EHT PHY capabilities subfield
     *
     * \param start iterator pointing to where the EHT PHY capabilities subfield should be written
     * to
     */
    void Serialize(Buffer::Iterator& start) const;
    /**
     * Deserialize the EHT PHY capabilities subfield
     *
     * \param start iterator pointing to where the EHT PHY capabilities subfield should be read from
     * \return the number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator start);
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11be EHT Capabilities
 */
class EhtCapabilities : public WifiInformationElement
{
  public:
    EhtCapabilities();
    // Implementations of pure virtual methods, or overridden from base class.
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;

    /**
     * Set the maximum MPDU length.
     *
     * \param length the maximum MPDU length (3895, 7991 or 11454)
     */
    void SetMaxMpduLength(uint16_t length);

    /**
     * Set the maximum A-MPDU length.
     *
     * \param maxAmpduLength 2^(23 + x) - 1, x in the range 0 to 3
     */
    void SetMaxAmpduLength(uint32_t maxAmpduLength);

    /**
     * Return the maximum A-MPDU length.
     *
     * \return the maximum A-MPDU length
     */
    uint32_t GetMaxAmpduLength() const;

    /**
     * Get the maximum MPDU length.
     *
     * \return the maximum MPDU length in bytes
     */
    uint16_t GetMaxMpduLength() const;

    EhtMacCapabilities m_macCapabilities; //!< EHT MAC Capabilities Info subfield
    EhtPhyCapabilities m_phyCapabilities; //!< EHT PHY Capabilities Info subfield

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    friend std::ostream& operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities);
};

/**
 * output stream output operator
 * \param os the output stream
 * \param ehtCapabilities the EHT capabilities
 * \returns the output stream
 */
std::ostream& operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities);

} // namespace ns3

#endif /* EHT_CAPABILITIES_H */
