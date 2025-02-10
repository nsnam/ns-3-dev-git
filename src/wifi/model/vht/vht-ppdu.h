/*
 * Copyright (c) 2019 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (VhtSigHeader)
 */

#ifndef VHT_PPDU_H
#define VHT_PPDU_H

#include "ns3/ofdm-ppdu.h"
#include "ns3/wifi-phy-operating-channel.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::VhtPpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * @brief VHT PPDU (11ac)
 * @ingroup wifi
 *
 * VhtPpdu stores a preamble, PHY headers and a PSDU of a PPDU with VHT header
 */
class VhtPpdu : public OfdmPpdu
{
  public:
    /**
     * VHT PHY header (VHT-SIG-A1/A2/B).
     * See section 21.3.8 in IEEE 802.11-2016.
     */
    class VhtSigHeader
    {
      public:
        VhtSigHeader();

        /**
         * Set the Multi-User (MU) flag.
         *
         * @param mu the MU flag
         */
        void SetMuFlag(bool mu);

        /**
         * Fill the channel width field of VHT-SIG-A1.
         *
         * @param channelWidth the channel width
         */
        void SetChannelWidth(MHz_u channelWidth);
        /**
         * Return the channel width.
         *
         * @return the channel width
         */
        MHz_u GetChannelWidth() const;
        /**
         * Fill the number of streams field of VHT-SIG-A1.
         *
         * @param nStreams the number of streams
         */
        void SetNStreams(uint8_t nStreams);
        /**
         * Return the number of streams.
         *
         * @return the number of streams
         */
        uint8_t GetNStreams() const;

        /**
         * Fill the short guard interval field of VHT-SIG-A2.
         *
         * @param sgi whether short guard interval is used or not
         */
        void SetShortGuardInterval(bool sgi);
        /**
         * Return the short GI field of VHT-SIG-A2.
         *
         * @return the short GI field of VHT-SIG-A2
         */
        bool GetShortGuardInterval() const;
        /**
         * Fill the short GI NSYM disambiguation field of VHT-SIG-A2.
         *
         * @param disambiguation whether short GI NSYM disambiguation is set or not
         */
        void SetShortGuardIntervalDisambiguation(bool disambiguation);
        /**
         * Return the short GI NSYM disambiguation field of VHT-SIG-A2.
         *
         * @return the short GI NSYM disambiguation field of VHT-SIG-A2
         */
        bool GetShortGuardIntervalDisambiguation() const;
        /**
         * Fill the SU VHT MCS field of VHT-SIG-A2.
         *
         * @param mcs the SU VHT MCS field of VHT-SIG-A2
         */
        void SetSuMcs(uint8_t mcs);
        /**
         * Return the SU VHT MCS field of VHT-SIG-A2.
         *
         * @return the SU VHT MCS field of VHT-SIG-A2
         */
        uint8_t GetSuMcs() const;

      private:
        // VHT-SIG-A1 fields
        uint8_t m_bw;   ///< BW
        uint8_t m_nsts; ///< NSTS

        // VHT-SIG-A2 fields
        uint8_t m_sgi;                ///< Short GI
        uint8_t m_sgi_disambiguation; ///< Short GI NSYM Disambiguation
        uint8_t m_suMcs;              ///< SU VHT MCS

        /// This is used to decide whether MU SIG-B should be added or not
        bool m_mu;

        // end of class VhtSigHeader
    };

    /**
     * Create a VHT PPDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU
     */
    VhtPpdu(Ptr<const WifiPsdu> psdu,
            const WifiTxVector& txVector,
            const WifiPhyOperatingChannel& channel,
            Time ppduDuration,
            uint64_t uid);

    Time GetTxDuration() const override;
    Ptr<WifiPpdu> Copy() const override;
    WifiPpduType GetType() const override;

  private:
    WifiTxVector DoGetTxVector() const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    virtual void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the L-SIG header.
     *
     * @param lSig the L-SIG header to fill in
     * @param ppduDuration the transmission duration of this PPDU
     */
    virtual void SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const;

    /**
     * Fill in the VHT-SIG header.
     *
     * @param vhtSig the VHT-SIG header to fill in
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetVhtSigHeader(VhtSigHeader& vhtSig,
                         const WifiTxVector& txVector,
                         Time ppduDuration) const;

    /**
     * Fill in the TXVECTOR from PHY headers.
     *
     * @param txVector the TXVECTOR to fill in
     * @param lSig the L-SIG header
     * @param vhtSig the VHT-SIG header
     */
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                   const LSigHeader& lSig,
                                   const VhtSigHeader& vhtSig) const;

    VhtSigHeader m_vhtSig; //!< the VHT-SIG PHY header
};

} // namespace ns3

#endif /* VHT_PPDU_H */
