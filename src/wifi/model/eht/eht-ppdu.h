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

#ifndef EHT_PPDU_H
#define EHT_PPDU_H

#include "ns3/he-ppdu.h"

#include <optional>

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::EhtPpdu class.
 */

namespace ns3
{

/**
 * \brief EHT PPDU (11be)
 * \ingroup wifi
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
        uint8_t m_phyVersionId{0}; ///< PHY Version Identifier field
        uint8_t m_bandwidth{0};    ///< Bandwidth field
        uint8_t m_bssColor{0};     ///< BSS color field
        uint8_t m_ppduType{0};     ///< PPDU Type And Compressed Mode field

    }; // struct EhtTbPhyHeader

    /**
     * PHY header for EHT MU PPDUs
     */
    struct EhtMuPhyHeader
    {
        // U-SIG fields
        uint8_t m_phyVersionId{0}; ///< PHY Version Identifier field
        uint8_t m_bandwidth{0};    ///< Bandwidth field
        uint8_t m_bssColor{0};     ///< BSS color field
        uint8_t m_ppduType{0};     ///< PPDU Type And Compressed Mode field
        uint8_t m_ehtSigMcs{0};    ///< EHT-SIG-B MCS

        // EHT-SIG fields
        uint8_t m_giLtfSize{0}; ///< GI+LTF Size field

        std::optional<RuAllocation> m_ruAllocationA; //!< RU Allocation-A that are going to be
                                                     //!< carried in EHT-SIG common subfields
        std::optional<RuAllocation> m_ruAllocationB; //!< RU Allocation-B that are going to be
                                                     //!< carried in EHT-SIG common subfields

        HeSigBContentChannels m_contentChannels; //!< EHT-SIG Content Channels
    };                                           // struct EhtMuPhyHeader

    /// type of the EHT PHY header
    using EhtPhyHeader = std::variant<std::monostate, EhtTbPhyHeader, EhtMuPhyHeader>;

    /**
     * Create an EHT PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param channel the operating channel of the PHY used to transmit this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     * \param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * \param flag the flag indicating the type of Tx PSD to build
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
     * \param channelWidth the channel width occupied by the PPDU (in MHz)
     * \param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * \param ruAllocation 8 bit RU_ALLOCATION per 20 MHz

     * \return a pair containing the number of RUs in each EHT-SIG-B content channel (resp. 1 and 2)
     */
    static std::pair<std::size_t, std::size_t> GetNumRusPerEhtSigBContentChannel(
        uint16_t channelWidth,
        uint8_t ehtPpduType,
        const std::vector<uint8_t>& ruAllocation);

    /**
     * Get the EHT-SIG content channels for a given PPDU
     * IEEE 802.11be-D3.1 36.3.12.8.2 EHT-SIG content channels
     *
     * \param txVector the TXVECTOR used for the PPDU
     * \param p20Index the index of the primary20 channel
     * \return EHT-SIG content channels
     */
    static HeSigBContentChannels GetEhtSigContentChannels(const WifiTxVector& txVector,
                                                          uint8_t p20Index);

    /**
     * Get variable length EHT-SIG field size
     * \param channelWidth the channel width occupied by the PPDU (in MHz)
     * \param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * \param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * \return field size in bytes
     */
    static uint32_t GetEhtSigFieldSize(uint16_t channelWidth,
                                       const std::vector<uint8_t>& ruAllocation,
                                       uint8_t ehtPpduType);

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const override;

    /**
     * Fill in the PHY headers.
     *
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the EHT PHY header.
     *
     * \param txVector the TXVECTOR that was used for this PPDU
     */
    void SetEhtPhyHeader(const WifiTxVector& txVector);

    EhtPhyHeader m_ehtPhyHeader; //!< the EHT PHY header
};                               // class EhtPpdu

} // namespace ns3

#endif /* EHT_PPDU_H */
