/*
 * Copyright (c) 2020 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HeSigHeader)
 */

#ifndef HE_PPDU_H
#define HE_PPDU_H

#include "ns3/ofdm-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HePpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * \brief HE PPDU (11ax)
 * \ingroup wifi
 *
 * HePpdu stores a preamble, PHY headers and a map of PSDUs of a PPDU with HE header
 */
class HePpdu : public OfdmPpdu
{
  public:
    /**
     * HE-SIG PHY header (HE-SIG-A1/A2/B)
     */
    class HeSigHeader : public Header
    {
      public:
        HeSigHeader();

        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId();

        TypeId GetInstanceTypeId() const override;
        void Print(std::ostream& os) const override;
        uint32_t GetSerializedSize() const override;
        void Serialize(Buffer::Iterator start) const override;
        uint32_t Deserialize(Buffer::Iterator start) override;

        /**
         * Set the Multi-User (MU) flag.
         *
         * \param mu the MU flag
         */
        void SetMuFlag(bool mu);

        /**
         * Fill the MCS field of HE-SIG-A1.
         *
         * \param mcs the MCS field of HE-SIG-A1
         */
        void SetMcs(uint8_t mcs);
        /**
         * Return the MCS field of HE-SIG-A1.
         *
         * \return the MCS field of HE-SIG-A1
         */
        uint8_t GetMcs() const;
        /**
         * Fill the BSS Color field of HE-SIG-A1.
         *
         * \param bssColor the BSS Color value
         */
        void SetBssColor(uint8_t bssColor);
        /**
         * Return the BSS Color field in the HE-SIG-A1.
         *
         * \return the BSS Color field in the HE-SIG-A1
         */
        uint8_t GetBssColor() const;
        /**
         * Fill the channel width field of HE-SIG-A1 (in MHz).
         *
         * \param channelWidth the channel width (in MHz)
         */
        void SetChannelWidth(uint16_t channelWidth);
        /**
         * Return the channel width (in MHz).
         *
         * \return the channel width (in MHz)
         */
        uint16_t GetChannelWidth() const;
        /**
         * Fill the GI + LTF size field of HE-SIG-A1.
         *
         * \param gi the guard interval (in nanoseconds)
         * \param ltf the sequence of HE-LTF
         */
        void SetGuardIntervalAndLtfSize(uint16_t gi, uint8_t ltf);
        /**
         * Return the guard interval (in nanoseconds).
         *
         * \return the guard interval (in nanoseconds)
         */
        uint16_t GetGuardInterval() const;
        /**
         * Fill the number of streams field of HE-SIG-A1.
         *
         * \param nStreams the number of streams
         */
        void SetNStreams(uint8_t nStreams);
        /**
         * Return the number of streams.
         *
         * \return the number of streams
         */
        uint8_t GetNStreams() const;

      private:
        // HE-SIG-A1 fields
        uint8_t m_format;       ///< Format bit
        uint8_t m_bssColor;     ///< BSS color field
        uint8_t m_ul_dl;        ///< UL/DL bit
        uint8_t m_mcs;          ///< MCS field
        uint8_t m_spatialReuse; ///< Spatial Reuse field
        uint8_t m_bandwidth;    ///< Bandwidth field
        uint8_t m_gi_ltf_size;  ///< GI+LTF Size field
        uint8_t m_nsts;         ///< NSTS

        /// This is used to decide whether MU SIG-B should be added or not
        bool m_mu;
    }; // class HeSigHeader

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
     * \param psdu the PHY payload (PSDU)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param txCenterFreq the center frequency (MHz) that was used for this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     * \param band the WifiPhyBand used for the transmission of this PPDU
     * \param uid the unique ID of this PPDU
     */
    HePpdu(Ptr<const WifiPsdu> psdu,
           const WifiTxVector& txVector,
           uint16_t txCenterFreq,
           Time ppduDuration,
           WifiPhyBand band,
           uint64_t uid);
    /**
     * Create an MU HE PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param txCenterFreq the center frequency (MHz) that was used for this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     * \param band the WifiPhyBand used for the transmission of this PPDU
     * \param uid the unique ID of this PPDU or of the triggering PPDU if this is an HE TB PPDU
     * \param flag the flag indicating the type of Tx PSD to build
     */
    HePpdu(const WifiConstPsduMap& psdus,
           const WifiTxVector& txVector,
           uint16_t txCenterFreq,
           Time ppduDuration,
           WifiPhyBand band,
           uint64_t uid,
           TxPsdFlag flag);

    Time GetTxDuration() const override;
    Ptr<WifiPpdu> Copy() const override;
    WifiPpduType GetType() const override;
    uint16_t GetStaId() const override;
    uint16_t GetTransmissionChannelWidth() const override;

    /**
     * Get the payload of the PPDU.
     *
     * \param bssColor the BSS color of the PHY calling this function.
     * \param staId the STA-ID of the PHY calling this function.
     * \return the PSDU
     */
    Ptr<const WifiPsdu> GetPsdu(uint8_t bssColor, uint16_t staId = SU_STA_ID) const;

    /**
     * \return the transmit PSD flag set for this PPDU
     *
     * \see TxPsdFlag
     */
    TxPsdFlag GetTxPsdFlag() const;

    /**
     * \param flag the transmit PSD flag set for this PPDU
     *
     * \see TxPsdFlag
     */
    void SetTxPsdFlag(TxPsdFlag flag) const;

    /**
     * Update the TXVECTOR for HE TB PPDUs, since the information to decode HE TB PPDUs
     * is not available from the PHY headers but it requires information from the TRIGVECTOR
     * of the AP expecting these HE TB PPDUs.
     *
     * \param trigVector the TRIGVECTOR or std::nullopt if no TRIGVECTOR is available at the caller
     */
    void UpdateTxVectorForUlMu(const std::optional<WifiTxVector>& trigVector) const;

    /**
     * Check if STA ID is in HE SIG-B Content Channel ID
     * \param staId STA ID
     * \param channelId Content Channel ID
     * \return true if STA ID in content channel ID, false otherwise
     */
    bool IsStaInContentChannel(uint16_t staId, size_t channelId) const;

    /**
     * Check if STA ID is allocated
     * \param staId STA ID
     * \return true if allocated, false otherwise
     */
    bool IsAllocated(uint16_t staId) const;

  protected:
    /**
     * Fill in the TXVECTOR from PHY headers.
     *
     * \param txVector the TXVECTOR to fill in
     * \param lSig the L-SIG header
     * \param heSig the HE-SIG header
     */
    virtual void SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                           const LSigHeader& lSig,
                                           const HeSigHeader& heSig) const;

#ifndef NS3_BUILD_PROFILE_DEBUG
    HeSigHeader m_heSig; //!< the HE-SIG PHY header
#endif
    mutable TxPsdFlag m_txPsdFlag; //!< the transmit power spectral density flag

    WifiTxVector::HeMuUserInfoMap m_muUserInfos; //!< HE MU specific per-user information (to be
                                                 //!< removed once HE-SIG-B headers are implemented)
    ContentChannelAllocation
        m_contentChannelAlloc; //!< HE SIG-B Content Channel allocation (to be removed once HE-SIG-B
                               //!< headers are implemented)
    RuAllocation m_ruAllocation; //!< RU_ALLOCATION in SIG-B common field (to be removed once
                                 //!< HE-SIG-B headers are implemented)

  private:
    std::string PrintPayload() const override;
    WifiTxVector DoGetTxVector() const override;

    /**
     * Return true if the PPDU is a MU PPDU
     * \return true if the PPDU is a MU PPDU
     */
    virtual bool IsMu() const;

    /**
     * Return true if the PPDU is a DL MU PPDU
     * \return true if the PPDU is a DL MU PPDU
     */
    virtual bool IsDlMu() const;

    /**
     * Return true if the PPDU is an UL MU PPDU
     * \return true if the PPDU is an UL MU PPDU
     */
    virtual bool IsUlMu() const;

    /**
     * Fill in the PHY headers.
     *
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     */
    virtual void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the L-SIG header.
     *
     * \param lSig the L-SIG header to fill in
     * \param ppduDuration the transmission duration of this PPDU
     */
    virtual void SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const;

    /**
     * Fill in the HE-SIG header.
     *
     * \param heSig the HE-SIG header to fill in
     * \param txVector the TXVECTOR that was used for this PPDU
     */
    void SetHeSigHeader(HeSigHeader& heSig, const WifiTxVector& txVector) const;
}; // class HePpdu

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param flag the transmit power spectral density flag
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const HePpdu::TxPsdFlag& flag);

} // namespace ns3

#endif /* HE_PPDU_H */
