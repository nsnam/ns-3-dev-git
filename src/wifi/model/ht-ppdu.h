/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 */

#ifndef HT_PPDU_H
#define HT_PPDU_H

#include "ofdm-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HtPpdu class.
 */

namespace ns3 {

class WifiPsdu;

/**
 * \brief HT  PPDU (11n)
 * \ingroup wifi
 *
 * HtPpdu stores a preamble, PHY headers and a PSDU of a PPDU with HT header
 */
class HtPpdu : public OfdmPpdu
{
public:
  /**
   * Create an HT PPDU.
   *
   * \param psdu the PHY payload (PSDU)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   * \param uid the unique ID of this PPDU
   */
  HtPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration,
          WifiPhyBand band, uint64_t uid);
  /**
   * Destructor for HtPpdu.
   */
  virtual ~HtPpdu ();

  // Inherited
  Time GetTxDuration (void) const override;
  Ptr<WifiPpdu> Copy (void) const override;

private:
  // Inherited
  WifiTxVector DoGetTxVector (void) const override;

  HtSigHeader m_htSig;  //!< the HT-SIG PHY header
}; //class HtPpdu

} //namespace ns3

#endif /* HT_PPDU_H */
