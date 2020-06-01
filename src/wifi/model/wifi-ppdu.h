/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Orange Labs
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
 */

#ifndef WIFI_PPDU_H
#define WIFI_PPDU_H

#include <list>
#include "ns3/nstime.h"
#include "wifi-tx-vector.h"
#include "wifi-phy-header.h"
#include "wifi-phy-band.h"

namespace ns3 {

class WifiPsdu;

/**
 * \ingroup wifi
 *
 * WifiPpdu stores a preamble, a modulation class, PHY headers and a PSDU.
 * This class should be extended later on to handle MU PPDUs.
 */
class WifiPpdu : public SimpleRefCount<WifiPpdu>
{
public:
  /**
   * Create a single user PPDU storing a PSDU.
   *
   * \param psdu the PHY payload (PSDU)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   */
  WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, WifiPhyBand band);

  virtual ~WifiPpdu ();

  /**
   * Get the TXVECTOR used to send the PPDU.
   * \return the TXVECTOR of the PPDU.
   */
  WifiTxVector GetTxVector (void) const;
  /**
   * Get the payload of the PPDU.
   * \return the PSDU
   */
  Ptr<const WifiPsdu> GetPsdu (void) const;
  /**
   * Return true if the PPDU's transmission was aborted due to transmitter switch off
   * \return true if the PPDU's transmission was aborted due to transmitter switch off
   */
  bool IsTruncatedTx (void) const;
  /**
   * Indicate that the PPDU's transmission was aborted due to transmitter switch off.
   */
  void SetTruncatedTx (void);
  /**
   * Get the total transmission duration of the PPDU.
   * \return the transmission duration of the PPDU
   */
  Time GetTxDuration () const;

  /**
   * \brief Print the PPDU contents.
   * \param os output stream in which the data should be printed.
   */
  void Print (std::ostream &os) const;

private:
  DsssSigHeader m_dsssSig;          //!< the DSSS SIG PHY header
  LSigHeader m_lSig;                //!< the L-SIG PHY header
  HtSigHeader m_htSig;              //!< the HT-SIG PHY header
  VhtSigHeader m_vhtSig;            //!< the VHT-SIG PHY header
  HeSigHeader m_heSig;              //!< the HE-SIG PHY header
  WifiPreamble m_preamble;          //!< the PHY preamble
  WifiModulationClass m_modulation; //!< the modulation used for the transmission of this PPDU
  Ptr<const WifiPsdu> m_psdu;       //!< the PSDU contained in this PPDU
  bool m_truncatedTx;               //!< flag indicating whether the frame's transmission was aborted due to transmitter switch off
  WifiPhyBand m_band;               //!< the WifiPhyBand used to transmit that PPDU
  uint16_t m_channelWidth;          //!< the channel width used to transmit that PPDU in MHz
  uint8_t m_txPowerLevel;           //!< the transmission power level (used only for TX and initializing the returned WifiTxVector)
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param ppdu the PPDU
 * \returns a reference to the stream
 */
std::ostream& operator<< (std::ostream& os, const WifiPpdu &ppdu);

} //namespace ns3

#endif /* WIFI_PPDU_H */
