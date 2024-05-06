/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef COMMON_INFO_BASIC_MLE_H
#define COMMON_INFO_BASIC_MLE_H

#include "ns3/buffer.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"

#include <cstdint>
#include <optional>

namespace ns3
{

/**
 * Common Info field of the Basic Multi-Link element.
 * See Sec. 9.4.2.312.2.2 of 802.11be D1.5
 */
struct CommonInfoBasicMle
{
    /**
     * Medium Synchronization Delay Information subfield
     */
    struct MediumSyncDelayInfo
    {
        uint8_t mediumSyncDuration;            //!< Medium Synchronization Duration
        uint8_t mediumSyncOfdmEdThreshold : 4; //!< Medium Synchronization OFDM ED Threshold
        uint8_t mediumSyncMaxNTxops : 4;       //!< Medium Synchronization MAximum Number of TXOPs
    };

    /**
     * EML Capabilities subfield
     */
    struct EmlCapabilities
    {
        uint8_t emlsrSupport : 1;         //!< EMLSR Support
        uint8_t emlsrPaddingDelay : 3;    //!< EMLSR Padding Delay
        uint8_t emlsrTransitionDelay : 3; //!< EMLSR Transition Delay
        uint8_t emlmrSupport : 1;         //!< EMLMR Support
        uint8_t emlmrDelay : 3;           //!< EMLMR Delay
        uint8_t transitionTimeout : 4;    //!< Transition Timeout
    };

    /**
     * MLD Capabilities subfield
     */
    struct MldCapabilities
    {
        uint8_t maxNSimultaneousLinks : 4;   //!< Max number of simultaneous links
        uint8_t srsSupport : 1;              //!< SRS Support
        uint8_t tidToLinkMappingSupport : 2; //!< TID-To-Link Mapping Negotiation Supported
        uint8_t freqSepForStrApMld : 5; //!< Frequency Separation For STR/AP MLD Type Indication
        uint8_t aarSupport : 1;         //!< AAR Support
    };

    /**
     * Extended MLD Capabilities and Operations subfield
     */
    struct ExtMldCapabilities
    {
        uint8_t opParamUpdateSupp : 1;    ///< Operation Parameter Update Support
        uint8_t recommMaxSimulLinks : 4;  ///< Recommended Max Simultaneous Links
        uint8_t nstrStatusUpdateSupp : 1; ///< NSTR Status Update Support
    };

    /**
     * Subfields
     */
    Mac48Address m_mldMacAddress;                  //!< MLD MAC Address
    std::optional<uint8_t> m_linkIdInfo;           //!< Link ID Info
    std::optional<uint8_t> m_bssParamsChangeCount; //!< BSS Parameters Change Count
    std::optional<MediumSyncDelayInfo>
        m_mediumSyncDelayInfo;                        //!< Medium Synchronization Delay Information
    std::optional<EmlCapabilities> m_emlCapabilities; //!< EML Capabilities
    std::optional<MldCapabilities> m_mldCapabilities; //!< MLD Capabilities
    std::optional<uint8_t> m_apMldId;                 ///< AP MLD ID
    std::optional<ExtMldCapabilities> m_extMldCapabilities; ///< Extended MLD Capabilities

    /**
     * Get the Presence Bitmap subfield of the Common Info field
     *
     * @return the Presence Bitmap subfield of the Common Info field
     */
    uint16_t GetPresenceBitmap() const;

    /**
     * Get the size of the serialized Common Info field
     *
     * @return the size of the serialized Common Info field
     */
    uint8_t GetSize() const;

    /**
     * Serialize the Common Info field
     *
     * @param start iterator pointing to where the Common Info field should be written to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize the Common Info field
     *
     * @param start iterator pointing to where the Common Info field should be read from
     * @param presence the value of the Presence Bitmap field indicating which subfields
     *                 are present in the Common Info field
     * @return the number of bytes read
     */
    uint8_t Deserialize(Buffer::Iterator start, uint16_t presence);

    /**
     * @param delay the EMLSR Padding delay
     * @return the encoded value for the EMLSR Padding Delay subfield
     */
    static uint8_t EncodeEmlsrPaddingDelay(Time delay);

    /**
     * @param value the value for the EMLSR Padding Delay subfield
     * @return the corresponding EMLSR Padding delay
     */
    static Time DecodeEmlsrPaddingDelay(uint8_t value);

    /**
     * @param delay the EMLSR Transition delay
     * @return the encoded value for the EMLSR Transition Delay subfield
     */
    static uint8_t EncodeEmlsrTransitionDelay(Time delay);

    /**
     * @param value the value for the EMLSR Transition Delay subfield
     * @return the corresponding EMLSR Transition delay
     */
    static Time DecodeEmlsrTransitionDelay(uint8_t value);

    /**
     * Set the Medium Synchronization Duration subfield of the Medium Synchronization
     * Delay Information in the Common Info field.
     *
     * @param delay the timer duration (must be a multiple of 32 microseconds)
     */
    void SetMediumSyncDelayTimer(Time delay);

    /**
     * Set the Medium Synchronization OFDM ED Threshold subfield of the Medium Synchronization
     * Delay Information in the Common Info field.
     *
     * @param threshold the threshold in dBm (ranges from -72 to -62 dBm)
     */
    void SetMediumSyncOfdmEdThreshold(int8_t threshold);

    /**
     * Set the Medium Synchronization Maximum Number of TXOPs subfield of the Medium Synchronization
     * Delay Information in the Common Info field. A value of zero indicates no limit on the
     * maximum number of TXOPs.
     *
     * @param nTxops the maximum number of TXOPs a non-AP STA is allowed to attempt to
     *               initiate while the MediumSyncDelay timer is running at a non-AP STA
     */
    void SetMediumSyncMaxNTxops(uint8_t nTxops);

    /**
     * Get the Medium Synchronization Duration subfield of the Medium Synchronization Delay
     * Information in the Common Info field. Make sure that the Medium Synchronization Delay
     * Information subfield is present.
     *
     * @return the timer duration
     */
    Time GetMediumSyncDelayTimer() const;

    /**
     * Get the Medium Synchronization OFDM ED Threshold in dBm. Make sure that the Medium
     * Synchronization Delay Information subfield is present.
     *
     * @return the threshold in dBm
     */
    int8_t GetMediumSyncOfdmEdThreshold() const;

    /**
     * Get the maximum number of TXOPs a non-AP STA is allowed to attempt to initiate
     * while the MediumSyncDelay timer is running at a non-AP STA. If no value is returned,
     * no limit is imposed on the number of TXOPs. Make sure that the Medium Synchronization
     * Delay Information subfield is present.
     *
     * @return the number of TXOPs
     */
    std::optional<uint8_t> GetMediumSyncMaxNTxops() const;
};

} // namespace ns3

#endif /* COMMON_INFO_BASIC_MLE_H */
