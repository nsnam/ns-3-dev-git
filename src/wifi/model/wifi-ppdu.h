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
#include <unordered_map>
#include "ns3/nstime.h"
#include "wifi-tx-vector.h"
#include "wifi-phy-header.h"
#include "wifi-phy-band.h"

namespace ns3 {

class WifiPsdu;

typedef std::unordered_map <uint16_t /* staId */, Ptr<const WifiPsdu> /* PSDU */> WifiConstPsduMap;

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
   * Create a SU PPDU storing a PSDU.
   *
   * \param psdu the PHY payload (PSDU)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   */
  WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, WifiPhyBand band);

  /**
   * Create a MU PPDU storing a vector of PSDUs.
   *
   * \param psdus the PHY payloads (PSDUs)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   */
  WifiPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration, WifiPhyBand band);

  virtual ~WifiPpdu ();

  /**
   * Get the TXVECTOR used to send the PPDU.
   * \return the TXVECTOR of the PPDU.
   */
  WifiTxVector GetTxVector (void) const;

  /**
   * Get the payload of the PPDU.
   * \param bssColor the BSS color of the PHY calling this function.
   * \param staId the staId of the PHY calling this function.
   * \return the PSDU
   */
  Ptr<const WifiPsdu> GetPsdu (uint8_t bssColor = 64, uint16_t staId = SU_STA_ID) const;

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
   * Return true if the PPDU is a MU PPDU
   * \return true if the PPDU is a MU PPDU
   */
  bool IsMu (void) const;

  /**
   * Get the modulation used for the PPDU.
   * \return the modulation used for the PPDU
   */
  WifiModulationClass GetModulation (void) const;

  /**
   * \brief Print the PPDU contents.
   * \param os output stream in which the data should be printed.
   */
  void Print (std::ostream &os) const;


private:
  /**
   * Fill in the PHY headers.
   *
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   */
  void SetPhyHeaders (WifiTxVector txVector, Time ppduDuration, WifiPhyBand band);

  DsssSigHeader m_dsssSig;                     //!< the DSSS SIG PHY header
  LSigHeader m_lSig;                           //!< the L-SIG PHY header
  HtSigHeader m_htSig;                         //!< the HT-SIG PHY header
  VhtSigHeader m_vhtSig;                       //!< the VHT-SIG PHY header
  HeSigHeader m_heSig;                         //!< the HE-SIG PHY header
  WifiPreamble m_preamble;                     //!< the PHY preamble
  WifiModulationClass m_modulation;            //!< the modulation used for the transmission of this PPDU
  WifiConstPsduMap m_psdus;                    //!< the PSDUs contained in this PPDU
  bool m_truncatedTx;                          //!< flag indicating whether the frame's transmission was aborted due to transmitter switch off
  WifiPhyBand m_band;                          //!< the WifiPhyBand used to transmit that PPDU
  uint16_t m_channelWidth;                     //!< the channel width used to transmit that PPDU in MHz
  uint8_t m_txPowerLevel;                      //!< the transmission power level (used only for TX and initializing the returned WifiTxVector)
  WifiTxVector::HeMuUserInfoMap m_muUserInfos; //!< the HE MU specific per-user information (to be removed once HE-SIG-B headers are implemented)
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param ppdu the PPDU
 * \returns a reference to the stream
 */
std::ostream& operator<< (std::ostream& os, const WifiPpdu &ppdu);

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param psdus the PSDUs
 * \returns a reference to the stream
 */
std::ostream & operator << (std::ostream &os, const WifiConstPsduMap &psdus);

} //namespace ns3

#endif /* WIFI_PPDU_H */
