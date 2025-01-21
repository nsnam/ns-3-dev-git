/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 */

#ifndef ERP_OFDM_PPDU_H
#define ERP_OFDM_PPDU_H

#include "ofdm-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::ErpOfdmPpdu class.
 */

namespace ns3
{

class WifiPsdu;

/**
 * @brief ERP-OFDM PPDU (11g)
 * @ingroup wifi
 *
 * ErpOfdmPpdu stores a preamble, PHY headers and a PSDU of a PPDU with non-HT header,
 * i.e., PPDU that uses ERP-OFDM modulation.
 */
class ErpOfdmPpdu : public OfdmPpdu
{
  public:
    /**
     * Create an ERP-OFDM PPDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param uid the unique ID of this PPDU
     */
    ErpOfdmPpdu(Ptr<const WifiPsdu> psdu,
                const WifiTxVector& txVector,
                const WifiPhyOperatingChannel& channel,
                uint64_t uid);

    Ptr<WifiPpdu> Copy() const override;

  private:
    void SetTxVectorFromLSigHeader(WifiTxVector& txVector, const LSigHeader& lSig) const override;
};

} // namespace ns3

#endif /* ERP_OFDM_PPDU_H */
