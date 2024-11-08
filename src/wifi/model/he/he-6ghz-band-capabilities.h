/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rami Abdallah <abdallah.rami@gmail.com>
 */

#ifndef HE_6GHZ_BAND_CAPABILITIES_H
#define HE_6GHZ_BAND_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The HE 6 GHz Band Capabilities (IEEE 802.11ax-2021 9.4.2.263)
 */
class He6GhzBandCapabilities : public WifiInformationElement
{
  public:
    He6GhzBandCapabilities();

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /// Capabilities Information field
    struct CapabilitiesInfo
    {
        uint8_t m_minMpduStartSpacing : 3;         ///< Minimum MPDU Start Spacing
        uint8_t m_maxAmpduLengthExponent : 3;      ///< Maximum A-MPDU Length Exponent (can
                                                   ///< also be set via convenient methods)
        uint8_t m_maxMpduLength : 2;               ///< Maximum MPDU Length (can also
                                                   ///< be set via convenient methods)
        uint8_t : 1;                               ///< Reserved Bits
        uint8_t m_smPowerSave : 2;                 ///< SM Power Save
        uint8_t m_rdResponder : 1;                 ///< RD Responder
        uint8_t m_rxAntennaPatternConsistency : 1; ///< Receive Antenna Pattern Consistency
        uint8_t m_txAntennaPatternConsistency : 1; ///< Transmit Antenna Pattern Consistency
        uint8_t : 2;                               ///< Reserved Bits
    };

    CapabilitiesInfo m_capabilitiesInfo; ///< capabilities field

    /**
     * Set the maximum AMPDU length.
     *
     * @param maxAmpduLength 2^(13 + x) - 1, x in the range 0 to 7
     */
    void SetMaxAmpduLength(uint32_t maxAmpduLength);
    /**
     * Return the maximum A-MPDU length.
     *
     * @return the maximum A-MPDU length in bytes
     */
    uint32_t GetMaxAmpduLength() const;

    /**
     * Set the maximum MPDU length.
     *
     * @param length the maximum MPDU length (3895, 7991 or 11454)
     */
    void SetMaxMpduLength(uint16_t length);
    /**
     * Get the maximum MPDU length.
     *
     * @return the maximum MPDU length in bytes
     */
    uint16_t GetMaxMpduLength() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* HE_6GHZ_BAND_CAPABILITY_H */
