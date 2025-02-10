/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (DsssSigHeader)
 */

#ifndef DSSS_PPDU_H
#define DSSS_PPDU_H

#include "ns3/wifi-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::DsssPpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * @brief DSSS (HR/DSSS) PPDU (11b)
 * @ingroup wifi
 *
 * DsssPpdu stores a preamble, PHY headers and a PSDU of a PPDU with DSSS modulation.
 */
class DsssPpdu : public WifiPpdu
{
  public:
    /**
     * DSSS SIG PHY header.
     * See section 16.2.2 in IEEE 802.11-2016.
     */
    class DsssSigHeader
    {
      public:
        DsssSigHeader();

        /**
         * Fill the RATE field of L-SIG (in bit/s).
         *
         * @param rate the RATE field of L-SIG expressed in bit/s
         */
        void SetRate(uint64_t rate);
        /**
         * Return the RATE field of L-SIG (in bit/s).
         *
         * @return the RATE field of L-SIG expressed in bit/s
         */
        uint64_t GetRate() const;
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

        // end of class DsssSigHeader
    };

    /**
     * Create a DSSS (HR/DSSS) PPDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU
     */
    DsssPpdu(Ptr<const WifiPsdu> psdu,
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
     */
    void SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration);

    /**
     * Fill in the DSSS header.
     *
     * @param dsssSig the DSSS header to fill in
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     */
    void SetDsssHeader(DsssSigHeader& dsssSig,
                       const WifiTxVector& txVector,
                       Time ppduDuration) const;

    /**
     * Fill in the TXVECTOR from DSSS header.
     *
     * @param txVector the TXVECTOR to fill in
     * @param dsssSig the DSSS header
     */
    virtual void SetTxVectorFromDsssHeader(WifiTxVector& txVector,
                                           const DsssSigHeader& dsssSig) const;

    DsssSigHeader m_dsssSig; //!< the DSSS SIG PHY header
};

} // namespace ns3

#endif /* DSSS_PPDU_H */
