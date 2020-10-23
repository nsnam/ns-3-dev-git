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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 */

#ifndef HE_PHY_H
#define HE_PHY_H

#include "vht-phy.h"
#include "wifi-phy-band.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HePhy class.
 */

namespace ns3 {

/**
 * This defines the BSS membership value for HE PHY.
 */
#define HE_PHY 125

/**
 * \brief PHY entity for HE (11ax)
 * \ingroup wifi
 *
 * HE PHY is based on VHT PHY.
 *
 * Refer to P802.11ax/D4.0, clause 27.
 */
class HePhy : public VhtPhy
{
public:
  /**
   * Constructor for HE PHY
   *
   * \param buildModeList flag used to add HE modes to list (disabled
   *                      by child classes to only add child classes' modes)
   */
  HePhy (bool buildModeList = true);
  /**
   * Destructor for HE PHY
   */
  virtual ~HePhy ();

  // Inherited
  WifiMode GetSigAMode (void) const override;
  WifiMode GetSigBMode (WifiTxVector txVector) const override;
  virtual const PpduFormats & GetPpduFormats (void) const override;
  Time GetLSigDuration (WifiPreamble preamble) const override;
  virtual Time GetTrainingDuration (WifiTxVector txVector,
                                    uint8_t nDataLtf, uint8_t nExtensionLtf = 0) const override;
  Time GetSigADuration (WifiPreamble preamble) const override;
  Time GetSigBDuration (WifiTxVector txVector) const override;

  /**
   * \param ppduDuration the duration of the HE TB PPDU
   * \param band the frequency band being used
   *
   * \return the L-SIG length value corresponding to that HE TB PPDU duration.
   */
  uint16_t ConvertHeTbPpduDurationToLSigLength (Time ppduDuration, WifiPhyBand band) const;
  /**
   * \param length the L-SIG length value
   * \param txVector the TXVECTOR used for the transmission of this HE TB PPDU
   * \param band the frequency band being used
   *
   * \return the duration of the HE TB PPDU corresponding to that L-SIG length value.
   */
  Time ConvertLSigLengthToHeTbPpduDuration (uint16_t length, WifiTxVector txVector, WifiPhyBand band) const;
  /**
   * \param txVector the transmission parameters used for the HE TB PPDU
   *
   * \return the duration of the non-OFDMA portion of the HE TB PPDU.
   */
  Time CalculateNonOfdmaDurationForHeTb (WifiTxVector txVector) const;

  /**
   * Initialize all HE modes.
   */
  static void InitializeModes (void);
  /**
   * Return the HE MCS corresponding to
   * the provided index.
   *
   * \param index the index of the MCS
   * \return an HE MCS
   */
  static WifiMode GetHeMcs (uint8_t index);

  /**
   * Return MCS 0 from HE MCS values.
   *
   * \return MCS 0 from HE MCS values
   */
  static WifiMode GetHeMcs0 (void);
  /**
   * Return MCS 1 from HE MCS values.
   *
   * \return MCS 1 from HE MCS values
   */
  static WifiMode GetHeMcs1 (void);
  /**
   * Return MCS 2 from HE MCS values.
   *
   * \return MCS 2 from HE MCS values
   */
  static WifiMode GetHeMcs2 (void);
  /**
   * Return MCS 3 from HE MCS values.
   *
   * \return MCS 3 from HE MCS values
   */
  static WifiMode GetHeMcs3 (void);
  /**
   * Return MCS 4 from HE MCS values.
   *
   * \return MCS 4 from HE MCS values
   */
  static WifiMode GetHeMcs4 (void);
  /**
   * Return MCS 5 from HE MCS values.
   *
   * \return MCS 5 from HE MCS values
   */
  static WifiMode GetHeMcs5 (void);
  /**
   * Return MCS 6 from HE MCS values.
   *
   * \return MCS 6 from HE MCS values
   */
  static WifiMode GetHeMcs6 (void);
  /**
   * Return MCS 7 from HE MCS values.
   *
   * \return MCS 7 from HE MCS values
   */
  static WifiMode GetHeMcs7 (void);
  /**
   * Return MCS 8 from HE MCS values.
   *
   * \return MCS 8 from HE MCS values
   */
  static WifiMode GetHeMcs8 (void);
  /**
   * Return MCS 9 from HE MCS values.
   *
   * \return MCS 9 from HE MCS values
   */
  static WifiMode GetHeMcs9 (void);
  /**
   * Return MCS 10 from HE MCS values.
   *
   * \return MCS 10 from HE MCS values
   */
  static WifiMode GetHeMcs10 (void);
  /**
   * Return MCS 11 from HE MCS values.
   *
   * \return MCS 11 from HE MCS values
   */
  static WifiMode GetHeMcs11 (void);

private:
  // Inherited
  virtual void BuildModeList (void) override;

  static const PpduFormats m_hePpduFormats; //!< HE PPDU formats
}; //class HePhy

} //namespace ns3

#endif /* HE_PHY_H */
