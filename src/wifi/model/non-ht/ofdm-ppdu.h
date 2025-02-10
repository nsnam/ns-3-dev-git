/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (LSigHeader)
 */

#ifndef OFDM_PPDU_H
#define OFDM_PPDU_H

#include "ns3/wifi-phy-band.h"
#include "ns3/wifi-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::OfdmPpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * @brief OFDM PPDU (11a)
 * @ingroup wifi
 *
 * OfdmPpdu stores a preamble, PHY headers and a PSDU of a PPDU with non-HT header,
 * i.e., PPDU that uses OFDM modulation.
 */
class OfdmPpdu : public WifiPpdu
{
  public:
    /**
     * OFDM and ERP OFDM L-SIG PHY header.
     * See section 17.3.4 in IEEE 802.11-2016.
     */
    class LSigHeader
    {
      public:
        LSigHeader();

        /**
         * Fill the RATE field of L-SIG (in bit/s).
         *
         * @param rate the RATE field of L-SIG expressed in bit/s
         * @param channelWidth the channel width
         */
        void SetRate(uint64_t rate, MHz_u channelWidth = MHz_u{20});
        /**
         * Return the RATE field of L-SIG (in bit/s).
         *
         * @param channelWidth the channel width
         * @return the RATE field of L-SIG expressed in bit/s
         */
        uint64_t GetRate(MHz_u channelWidth = MHz_u{20}) const;
        /**
         * Fill the LENGTH field of L-SIG (in bytes).
         *
         * @param length the LENGTH field of L-SIG expressed in bytes
         */
        void SetLength(uint16_t length);
        /**
         * Return the LENGTH field of L-SIG (in bytes).
         *
         * @return the LENGTH field of L-SIG expressed in bytes
         */
        uint16_t GetLength() const;

      private:
        uint8_t m_rate;    ///< RATE field
        uint16_t m_length; ///< LENGTH field

        // end of class LSigHeader
    };

    /**
     * Create an OFDM PPDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param uid the unique ID of this PPDU
     * @param instantiateLSig flag used to instantiate LSigHeader (set LSigHeader's
     *                        rate and length), should be disabled by child classes
     */
    OfdmPpdu(Ptr<const WifiPsdu> psdu,
             const WifiTxVector& txVector,
             const WifiPhyOperatingChannel& channel,
             uint64_t uid,
             bool instantiateLSig = true);

    Time GetTxDuration() const override;
    Ptr<WifiPpdu> Copy() const override;

  protected:
    LSigHeader m_lSig; //!< the L-SIG PHY header

  private:
    WifiTxVector DoGetTxVector() const override;

    /**
     * Fill in the PHY headers.
     *
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param psduSize the size duration of the PHY payload (PSDU)
     */
    void SetPhyHeaders(const WifiTxVector& txVector, std::size_t psduSize);

    /**
     * Fill in the L-SIG header.
     *
     * @param lSig the L-SIG header to fill in
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param psduSize the size duration of the PHY payload (PSDU)
     */
    void SetLSigHeader(LSigHeader& lSig, const WifiTxVector& txVector, std::size_t psduSize) const;

    /**
     * Fill in the TXVECTOR from L-SIG header.
     *
     * @param txVector the TXVECTOR to fill in
     * @param lSig the L-SIG header
     */
    virtual void SetTxVectorFromLSigHeader(WifiTxVector& txVector, const LSigHeader& lSig) const;

    MHz_u m_channelWidth; //!< the channel width used to transmit that PPDU
                          //!< (needed to distinguish 5 MHz, 10 MHz or 20 MHz PPDUs)
};

} // namespace ns3

#endif /* OFDM_PPDU_H */
