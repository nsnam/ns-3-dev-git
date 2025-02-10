/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HtSigHeader)
 */

#ifndef HT_PPDU_H
#define HT_PPDU_H

#include "ns3/ofdm-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::HtPpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * @brief HT  PPDU (11n)
 * @ingroup wifi
 *
 * HtPpdu stores a preamble, PHY headers and a PSDU of a PPDU with HT header
 */
class HtPpdu : public OfdmPpdu
{
  public:
    /**
     * HT PHY header (HT-SIG1/2).
     * See section 19.3.9 in IEEE 802.11-2016.
     */
    class HtSigHeader
    {
      public:
        HtSigHeader();

        /**
         * Fill the MCS field of HT-SIG.
         *
         * @param mcs the MCS field of HT-SIG
         */
        void SetMcs(uint8_t mcs);
        /**
         * Return the MCS field of HT-SIG.
         *
         * @return the MCS field of HT-SIG
         */
        uint8_t GetMcs() const;
        /**
         * Fill the channel width field of HT-SIG.
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
         * Fill the aggregation field of HT-SIG.
         *
         * @param aggregation whether the PSDU contains A-MPDU or not
         */
        void SetAggregation(bool aggregation);
        /**
         * Return the aggregation field of HT-SIG.
         *
         * @return the aggregation field of HT-SIG
         */
        bool GetAggregation() const;
        /**
         * Fill the short guard interval field of HT-SIG.
         *
         * @param sgi whether short guard interval is used or not
         */
        void SetShortGuardInterval(bool sgi);
        /**
         * Return the short guard interval field of HT-SIG.
         *
         * @return the short guard interval field of HT-SIG
         */
        bool GetShortGuardInterval() const;
        /**
         * Fill the HT length field of HT-SIG (in bytes).
         *
         * @param length the HT length field of HT-SIG (in bytes)
         */
        void SetHtLength(uint16_t length);
        /**
         * Return the HT length field of HT-SIG (in bytes).
         *
         * @return the HT length field of HT-SIG (in bytes)
         */
        uint16_t GetHtLength() const;

      private:
        uint8_t m_mcs;         ///< Modulation and Coding Scheme index
        uint8_t m_cbw20_40;    ///< CBW 20/40
        uint16_t m_htLength;   ///< HT length
        uint8_t m_aggregation; ///< Aggregation
        uint8_t m_sgi;         ///< Short Guard Interval

        // end of class HtSigHeader
    };

    /**
     * Create an HT PPDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU
     */
    HtPpdu(Ptr<const WifiPsdu> psdu,
           const WifiTxVector& txVector,
           const WifiPhyOperatingChannel& channel,
           Time ppduDuration,
           uint64_t uid);

    Time GetTxDuration() const override;
    Ptr<WifiPpdu> Copy() const override;

  private:
    WifiTxVector DoGetTxVector() const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param psduSize the size duration of the PHY payload (PSDU)
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration, std::size_t psduSize);

    /**
     * Fill in the L-SIG header.
     *
     * @param lSig the L-SIG header to fill in
     * @param ppduDuration the transmission duration of this PPDU
     */
    virtual void SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const;

    /**
     * Fill in the HT-SIG header.
     *
     * @param htSig the HT-SIG header to fill in
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param psduSize the size duration of the PHY payload (PSDU)
     */
    void SetHtSigHeader(HtSigHeader& htSig,
                        const WifiTxVector& txVector,
                        std::size_t psduSize) const;

    /**
     * Fill in the TXVECTOR from PHY headers.
     *
     * @param txVector the TXVECTOR to fill in
     * @param lSig the L-SIG header
     * @param htSig the HT-SIG header
     */
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                   const LSigHeader& lSig,
                                   const HtSigHeader& htSig) const;

    HtSigHeader m_htSig; //!< the HT-SIG PHY header
};

} // namespace ns3

#endif /* HT_PPDU_H */
