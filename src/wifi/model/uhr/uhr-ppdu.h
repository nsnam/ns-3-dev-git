/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_PPDU_H
#define UHR_PPDU_H

#include "ns3/eht-ppdu.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::UhrPpdu class.
 */

namespace ns3
{

/**
 * @brief UHR PPDU (11bn)
 * @ingroup wifi
 *
 * TODO: UhrPpdu is currently identical to EhtPpdu
 */
class UhrPpdu : public EhtPpdu
{
  public:
    /**
     * Create an UHR PPDU, storing a map of PSDUs.
     *
     * @param psdus the PHY payloads (PSDUs)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param ppduDuration the transmission duration of this PPDU
     * @param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * @param flag the flag indicating the type of Tx PSD to build
     */
    UhrPpdu(const WifiConstPsduMap& psdus,
            const WifiTxVector& txVector,
            const WifiPhyOperatingChannel& channel,
            Time ppduDuration,
            uint64_t uid,
            TxPsdFlag flag);

    WifiPpduType GetType() const override;
    Ptr<WifiPpdu> Copy() const override;

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    WifiMode GetMcs(uint8_t mcs) const override;
}; // class UhrPpdu

} // namespace ns3

#endif /* UHR_PPDU_H */
