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

#ifndef OFDM_PPDU_H
#define OFDM_PPDU_H

#include "wifi-phy-header.h"
#include "wifi-phy-band.h"
#include "wifi-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::OfdmPpdu class.
 */

namespace ns3 {

class WifiPsdu;

/**
 * \brief OFDM PPDU (11a)
 * \ingroup wifi
 *
 * OfdmPpdu stores a preamble, PHY headers and a PSDU of a PPDU with non-HT header,
 * i.e., PPDU that uses OFDM modulation.
 */
class OfdmPpdu : public WifiPpdu
{
public:
  /**
   * Create an OFDM PPDU.
   *
   * \param psdu the PHY payload (PSDU)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   * \param uid the unique ID of this PPDU
   * \param instantiateLSig flag used to instantiate LSigHeader (set LSigHeader's
   *                        rate and length), should be disabled by child classes
   */
  OfdmPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, WifiPhyBand band, uint64_t uid,
            bool instantiateLSig = true);
  /**
   * Destructor for OfdmPpdu.
   */
  virtual ~OfdmPpdu ();

  // Inherited
  virtual Time GetTxDuration (void) const override;
  virtual Ptr<WifiPpdu> Copy (void) const override;

protected:
  WifiPhyBand m_band;       //!< the WifiPhyBand used to transmit that PPDU
  uint16_t m_channelWidth;  //!< the channel width used to transmit that PPDU in MHz
  LSigHeader m_lSig;        //!< the L-SIG PHY header

private:
  // Inherited
  virtual WifiTxVector DoGetTxVector (void) const override;
}; //class OfdmPpdu

} //namespace ns3

#endif /* OFDM_PPDU_H */
