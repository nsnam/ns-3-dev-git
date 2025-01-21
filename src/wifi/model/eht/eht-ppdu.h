/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EHT_PPDU_H
#define EHT_PPDU_H

#include "ns3/he-ppdu.h"

#include <optional>

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::EhtPpdu class.
 */

namespace ns3
{

/**
 * @brief EHT PPDU (11be)
 * @ingroup wifi
 *
 * EhtPpdu is currently identical to HePpdu
 */
class EhtPpdu : public HePpdu
{
  public:
    /**
     * PHY header for EHT TB PPDUs
     */
    struct EhtTbPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId : 3 {0}; ///< PHY Version Identifier field
        uint8_t m_bandwidth : 3 {0};    ///< Bandwidth field
        uint8_t m_bssColor : 6 {0};     ///< BSS color field
        uint8_t m_ppduType : 2 {0};     ///< PPDU Type And Compressed Mode field
    };

    /**
     * PHY header for EHT MU PPDUs
     */
    struct EhtMuPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId : 3 {0};         ///< PHY Version Identifier field
        uint8_t m_bandwidth : 3 {0};            ///< Bandwidth field
        uint8_t m_bssColor : 6 {0};             ///< BSS color field
        uint8_t m_ppduType : 2 {0};             ///< PPDU Type And Compressed Mode field
        uint8_t m_puncturedChannelInfo : 5 {0}; ///< Punctured Channel Information field
        uint8_t m_ehtSigMcs : 2 {0};            ///< EHT-SIG-B MCS

        // EHT-SIG fields
        uint8_t m_giLtfSize{0}; ///< GI+LTF Size field

        std::optional<RuAllocation> m_ruAllocationA; //!< RU Allocation-A that are going to be
                                                     //!< carried in EHT-SIG common subfields
        std::optional<RuAllocation> m_ruAllocationB; //!< RU Allocation-B that are going to be
                                                     //!< carried in EHT-SIG common subfields

        HeSigBContentChannels m_contentChannels; //!< EHT-SIG Content Channels
    };

    /// type of the EHT PHY header
    using EhtPhyHeader = std::variant<std::monostate, EhtTbPhyHeader, EhtMuPhyHeader>;

    /**
     * Create an EHT PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * @param psdus the PHY payloads (PSDUs)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * @param flag the flag indicating the type of Tx PSD to build
     */
    EhtPpdu(const WifiConstPsduMap& psdus,
            const WifiTxVector& txVector,
            const WifiPhyOperatingChannel& channel,
            Time ppduDuration,
            uint64_t uid,
            TxPsdFlag flag);

    WifiPpduType GetType() const override;
    Ptr<WifiPpdu> Copy() const override;

    /**
     * Get the number of RUs per EHT-SIG-B content channel.
     * This function will be used once EHT PHY headers are implemented.
     *
     * @param channelWidth the channel width occupied by the PPDU
     * @param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * @param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * @param compression flag whether compression mode is used by the PPDU
     * @param numMuMimoUsers the number of MU-MIMO users addressed by the PPDU
     * @return a pair containing the number of RUs in each EHT-SIG-B content channel (resp. 1 and 2)
     */
    static std::pair<std::size_t, std::size_t> GetNumRusPerEhtSigBContentChannel(
        MHz_u channelWidth,
        uint8_t ehtPpduType,
        const RuAllocation& ruAllocation,
        bool compression,
        std::size_t numMuMimoUsers);

    /**
     * Get the EHT-SIG content channels for a given PPDU
     * IEEE 802.11be-D3.1 36.3.12.8.2 EHT-SIG content channels
     *
     * @param txVector the TXVECTOR used for the PPDU
     * @param p20Index the index of the primary20 channel
     * @return EHT-SIG content channels
     */
    static HeSigBContentChannels GetEhtSigContentChannels(const WifiTxVector& txVector,
                                                          uint8_t p20Index);

    /**
     * Get variable length EHT-SIG field size
     * @param channelWidth the channel width occupied by the PPDU
     * @param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * @param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * @param compression flag whether compression mode is used by the PPDU
     * @param numMuMimoUsers the number of MU-MIMO users addressed by the PPDU
     * @return field size in bytes
     */
    static uint32_t GetEhtSigFieldSize(MHz_u channelWidth,
                                       const RuAllocation& ruAllocation,
                                       uint8_t ehtPpduType,
                                       bool compression,
                                       std::size_t numMuMimoUsers);

    /**
     * Get the Punctured Channel Information field in the U-SIG
     * @param inactiveSubchannels the bitmap indexed by the 20 MHz subchannels in ascending order,
     * where each bit indicates whether the corresponding 20 MHz subchannel is punctured or not
     * within the transmission bandwidth
     * @param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * @param isLow80MHz flag whether the 80 MHz frequency subblock where U-SIG processing is
     * performed is the lowest in frequency (if OFDMA and if channel width is larger than 80 MHz)
     * @return the value of the Punctured Channel Information field
     */
    static uint8_t GetPuncturedInfo(const std::vector<bool>& inactiveSubchannels,
                                    uint8_t ehtPpduType,
                                    std::optional<bool> isLow80MHz);

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the EHT PHY header.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     */
    void SetEhtPhyHeader(const WifiTxVector& txVector);

    EhtPhyHeader m_ehtPhyHeader; //!< the EHT PHY header
};

} // namespace ns3

#endif /* EHT_PPDU_H */
