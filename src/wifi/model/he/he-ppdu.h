/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HeSigHeader)
 */

#ifndef HE_PPDU_H
#define HE_PPDU_H

#include "ns3/ofdm-ppdu.h"

#include <variant>

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::HePpdu class.
 */

namespace ns3
{

/// HE SIG-B Content Channels
constexpr size_t WIFI_MAX_NUM_HE_SIGB_CONTENT_CHANNELS = 2;

class WifiPsdu;

/**
 * @brief HE PPDU (11ax)
 * @ingroup wifi
 *
 * HePpdu stores a preamble, PHY headers and a map of PSDUs of a PPDU with HE header
 */
class HePpdu : public OfdmPpdu
{
  public:
    /// User Specific Fields in HE-SIG-Bs.
    struct HeSigBUserSpecificField
    {
        uint16_t staId : 11; ///< STA-ID
        uint8_t nss : 4;     ///< number of spatial streams
        uint8_t mcs : 4;     ///< MCS index
    };

    /// HE SIG-B Content Channels
    using HeSigBContentChannels = std::vector<std::vector<HeSigBUserSpecificField>>;

    /**
     * HE-SIG PHY header for HE SU PPDUs (HE-SIG-A1/A2)
     */
    struct HeSuSigHeader
    {
        uint8_t m_format{1};    ///< Format bit
        uint8_t m_bssColor{0};  ///< BSS color field
        uint8_t m_mcs{0};       ///< MCS field
        uint8_t m_bandwidth{0}; ///< Bandwidth field
        uint8_t m_giLtfSize{0}; ///< GI+LTF Size field
        uint8_t m_nsts{0};      ///< NSTS
    };

    /**
     * HE-SIG PHY header for HE TB PPDUs (HE-SIG-A1/A2)
     */
    struct HeTbSigHeader
    {
        uint8_t m_format{0};    ///< Format bit
        uint8_t m_bssColor{0};  ///< BSS color field
        uint8_t m_bandwidth{0}; ///< Bandwidth field
    };

    /**
     * HE-SIG PHY header for HE MU PPDUs (HE-SIG-A1/A2/B)
     */
    struct HeMuSigHeader
    {
        // HE-SIG-A fields
        uint8_t m_bssColor{0};        ///< BSS color field
        uint8_t m_bandwidth{0};       ///< Bandwidth field
        uint8_t m_sigBMcs{0};         ///< HE-SIG-B MCS
        uint8_t m_muMimoUsers;        ///< MU-MIMO users
        uint8_t m_sigBCompression{0}; ///< SIG-B compression
        uint8_t m_giLtfSize{0};       ///< GI+LTF Size field

        // HE-SIG-B fields
        RuAllocation m_ruAllocation; //!< RU allocations that are going to be carried in SIG-B
                                     //!< common subfields
        HeSigBContentChannels m_contentChannels; //!< HE SIG-B Content Channels
        std::optional<Center26ToneRuIndication>
            m_center26ToneRuIndication; //!< center 26 tone RU indication in SIG-B common subfields
    };

    /// type of the HE-SIG PHY header
    using HeSigHeader = std::variant<std::monostate, HeSuSigHeader, HeTbSigHeader, HeMuSigHeader>;

    /**
     * The transmit power spectral density flag, namely used
     * to correctly build PSDs for pre-HE and HE portions.
     */
    enum TxPsdFlag
    {
        PSD_NON_HE_PORTION, //!< Non-HE portion of an HE PPDU
        PSD_HE_PORTION      //!< HE portion of an HE PPDU
    };

    /**
     * Create an SU HE PPDU, storing a PSDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU
     */
    HePpdu(Ptr<const WifiPsdu> psdu,
           const WifiTxVector& txVector,
           const WifiPhyOperatingChannel& channel,
           Time ppduDuration,
           uint64_t uid);
    /**
     * Create an MU HE PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * @param psdus the PHY payloads (PSDUs)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU or of the triggering PPDU if this is an HE TB PPDU
     * @param flag the flag indicating the type of Tx PSD to build
     */
    HePpdu(const WifiConstPsduMap& psdus,
           const WifiTxVector& txVector,
           const WifiPhyOperatingChannel& channel,
           Time ppduDuration,
           uint64_t uid,
           TxPsdFlag flag);

    Time GetTxDuration() const override;
    Ptr<WifiPpdu> Copy() const override;
    WifiPpduType GetType() const override;
    uint16_t GetStaId() const override;
    MHz_u GetTxChannelWidth() const override;

    /**
     * Get the payload of the PPDU.
     *
     * @param bssColor the BSS color of the PHY calling this function.
     * @param staId the STA-ID of the PHY calling this function.
     * @return the PSDU
     */
    Ptr<const WifiPsdu> GetPsdu(uint8_t bssColor, uint16_t staId = SU_STA_ID) const;

    /**
     * @return the transmit PSD flag set for this PPDU
     *
     * @see TxPsdFlag
     */
    TxPsdFlag GetTxPsdFlag() const;

    /**
     * @param flag the transmit PSD flag set for this PPDU
     *
     * @see TxPsdFlag
     */
    void SetTxPsdFlag(TxPsdFlag flag) const;

    /**
     * Update the TXVECTOR for HE TB PPDUs, since the information to decode HE TB PPDUs
     * is not available from the PHY headers but it requires information from the TRIGVECTOR
     * of the AP expecting these HE TB PPDUs.
     *
     * @param trigVector the TRIGVECTOR or std::nullopt if no TRIGVECTOR is available at the caller
     */
    void UpdateTxVectorForUlMu(const std::optional<WifiTxVector>& trigVector) const;

    /**
     * Get the number of STAs per HE-SIG-B content channel.
     * This is applicable only for MU.
     * See section 27.3.10.8.3 of IEEE 802.11ax draft 4.0.
     *
     * @param channelWidth the channel width occupied by the PPDU
     * @param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * @param sigBCompression flag whether SIG-B compression is used by the PPDU
     * @param numMuMimoUsers the number of MU-MIMO users addressed by the PPDU
     * @return a pair containing the number of RUs in each HE-SIG-B content channel (resp. 1 and 2)
     */
    static std::pair<std::size_t, std::size_t> GetNumRusPerHeSigBContentChannel(
        MHz_u channelWidth,
        const RuAllocation& ruAllocation,
        bool sigBCompression,
        uint8_t numMuMimoUsers);

    /**
     * Get the HE SIG-B content channels for a given PPDU
     * IEEE 802.11ax-2021 27.3.11.8.2 HE-SIG-B content channels
     *
     * @param txVector the TXVECTOR used for the PPDU
     * @param p20Index the index of the primary20 channel
     * @return HE-SIG-B content channels
     */
    static HeSigBContentChannels GetHeSigBContentChannels(const WifiTxVector& txVector,
                                                          uint8_t p20Index);

    /**
     * Get variable length HE SIG-B field size
     * @param channelWidth the channel width occupied by the PPDU
     * @param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * @param sigBCompression flag whether SIG-B compression is used by the PPDU
     * @param numMuMimoUsers the number of MU-MIMO users addressed by the PPDU
     * @return field size in bytes
     */
    static uint32_t GetSigBFieldSize(MHz_u channelWidth,
                                     const RuAllocation& ruAllocation,
                                     bool sigBCompression,
                                     std::size_t numMuMimoUsers);

  protected:
    /**
     * Fill in the TXVECTOR from PHY headers.
     *
     * @param txVector the TXVECTOR to fill in
     */
    virtual void SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const;

    /**
     * Reconstruct HeMuUserInfoMap from HE-SIG-B header.
     *
     * @param txVector the TXVECTOR to set its HeMuUserInfoMap
     * @param ruAllocation the RU_ALLOCATION per 20 MHz
     * @param contentChannels the HE-SIG-B content channels
     * @param sigBCompression flag whether SIG-B compression is used by the PPDU
     * @param numMuMimoUsers the number of MU-MIMO users addressed by the PPDU
     */
    void SetHeMuUserInfos(WifiTxVector& txVector,
                          const RuAllocation& ruAllocation,
                          const HeSigBContentChannels& contentChannels,
                          bool sigBCompression,
                          uint8_t numMuMimoUsers) const;

    /**
     * Convert channel width expressed in MHz to bandwidth field encoding in HE-SIG-A.
     *
     * @param channelWidth the channel width in MHz
     * @return the value used to encode the bandwidth field in HE-SIG-A
     */
    static uint8_t GetChannelWidthEncodingFromMhz(MHz_u channelWidth);

    /**
     * Convert number of spatial streams to NSTS field encoding in HE-SIG-A.
     *
     * @param nss the number of spatial streams
     * @return the value used to encode the NSTS field in HE-SIG-A
     */
    static uint8_t GetNstsEncodingFromNss(uint8_t nss);

    /**
     * Convert guard interval and NLTF to its encoding in HE-SIG-A.
     *
     * @param guardInterval the guard interval
     * @param nltf the the number of long training symbols
     * @return the value used to encode the NSTS field in HE-SIG-A
     */
    static uint8_t GetGuardIntervalAndNltfEncoding(Time guardInterval, uint8_t nltf);

    /**
     * Convert number of spatial streams from NSTS field encoding in HE-SIG-A.
     *
     * @param nsts the value of the NSTS field in HE-SIG-A
     * @return the number of spatial streams
     */
    static uint8_t GetNssFromNstsEncoding(uint8_t nsts);

    /**
     * Convert channel width expressed in MHz from bandwidth field encoding in HE-SIG-A.
     *
     * @param bandwidth the value of the bandwidth field in HE-SIG-A
     * @return the channel width in MHz
     */
    static MHz_u GetChannelWidthMhzFromEncoding(uint8_t bandwidth);

    /**
     * Convert guard interval from its encoding in HE-SIG-A.
     *
     * @param giAndNltfSize the value used to encode the guard interval and NLTF field in HE-SIG-A
     * @return the guard interval
     */
    static Time GetGuardIntervalFromEncoding(uint8_t giAndNltfSize);

    /**
     * Convert number of MU-MIMO users to its encoding in HE-SIG-A.
     *
     * @param nUsers the number of MU-MIMO users
     * @return the number of MU-MIMO users to its encoding in HE-SIG-A
     */
    static uint8_t GetMuMimoUsersEncoding(uint8_t nUsers);

    /**
     * Convert number of MU-MIMO users from its encoding in HE-SIG-A.
     *
     * @param encoding the number of MU-MIMO users encoded in HE-SIG-A
     * @return the number of MU-MIMO users from its encoding in HE-SIG-A
     */
    static uint8_t GetMuMimoUsersFromEncoding(uint8_t encoding);

    mutable TxPsdFlag m_txPsdFlag; //!< the transmit power spectral density flag

  private:
    std::string PrintPayload() const override;
    WifiTxVector DoGetTxVector() const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the L-SIG header.
     *
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetLSigHeader(Time ppduDuration);

    /**
     * Fill in the HE-SIG header.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     */
    void SetHeSigHeader(const WifiTxVector& txVector);

    /**
     * Return true if the PPDU is a MU PPDU
     * @return true if the PPDU is a MU PPDU
     */
    virtual bool IsMu() const;

    /**
     * Return true if the PPDU is a DL MU PPDU
     * @return true if the PPDU is a DL MU PPDU
     */
    virtual bool IsDlMu() const;

    /**
     * Return true if the PPDU is an UL MU PPDU
     * @return true if the PPDU is an UL MU PPDU
     */
    virtual bool IsUlMu() const;

    HeSigHeader m_heSig; //!< the HE-SIG PHY header
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param flag the transmit power spectral density flag
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const HePpdu::TxPsdFlag& flag);

} // namespace ns3

#endif /* HE_PPDU_H */
