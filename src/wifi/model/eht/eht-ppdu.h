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
     * Create an EHT PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param txCenterFreq the center frequency (MHz) that was used for this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     * \param band the WifiPhyBand used for the transmission of this PPDU
     * \param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * \param flag the flag indicating the type of Tx PSD to build
     */
    EhtPpdu(const WifiConstPsduMap& psdus,
            const WifiTxVector& txVector,
            uint16_t txCenterFreq,
            Time ppduDuration,
            WifiPhyBand band,
            uint64_t uid,
            TxPsdFlag flag);

    WifiPpduType GetType() const override;
    Ptr<WifiPpdu> Copy() const override;

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                   const LSigHeader& lSig,
                                   const HeSigHeader& heSig) const override;

    uint8_t m_ehtSuMcs{0};      //!< EHT-MCS for EHT SU transmissions
    uint8_t m_ehtSuNStreams{1}; //!< Number of streams for EHT SU transmissions
};                              // class EhtPpdu

} // namespace ns3

#endif /* EHT_PPDU_H */
