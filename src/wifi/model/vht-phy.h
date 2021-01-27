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

#ifndef VHT_PHY_H
#define VHT_PHY_H

#include "ht-phy.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::VhtPhy class.
 */

namespace ns3 {

/**
 * This defines the BSS membership value for VHT PHY.
 */
#define VHT_PHY 126

/**
 * \brief PHY entity for VHT (11ac)
 * \ingroup wifi
 *
 * VHT PHY is based on HT PHY.
 *
 * Refer to IEEE 802.11-2016, clause 21.
 */
class VhtPhy : public HtPhy
{
public:
  /**
   * Constructor for VHT PHY
   *
   * \param buildModeList flag used to add VHT modes to list (disabled
   *                      by child classes to only add child classes' modes)
   */
  VhtPhy (bool buildModeList = true);
  /**
   * Destructor for VHT PHY
   */
  virtual ~VhtPhy ();

  // Inherited
  virtual WifiMode GetSigMode (WifiPpduField field, WifiTxVector txVector) const override;
  virtual const PpduFormats & GetPpduFormats (void) const override;

  /**
   * \return the WifiMode used for the SIG-A field
   */
  virtual WifiMode GetSigAMode (void) const;
  /**
   * \param txVector the transmission parameters
   * \return the WifiMode used for the SIG-B field
   */
  virtual WifiMode GetSigBMode (WifiTxVector txVector) const;

  /**
   * Initialize all VHT modes.
   */
  static void InitializeModes (void);
  /**
   * Return the VHT MCS corresponding to
   * the provided index.
   *
   * \param index the index of the MCS
   * \return an VHT MCS
   */
  static WifiMode GetVhtMcs (uint8_t index);

  /**
   * Return MCS 0 from VHT MCS values.
   *
   * \return MCS 0 from VHT MCS values
   */
  static WifiMode GetVhtMcs0 (void);
  /**
   * Return MCS 1 from VHT MCS values.
   *
   * \return MCS 1 from VHT MCS values
   */
  static WifiMode GetVhtMcs1 (void);
  /**
   * Return MCS 2 from VHT MCS values.
   *
   * \return MCS 2 from VHT MCS values
   */
  static WifiMode GetVhtMcs2 (void);
  /**
   * Return MCS 3 from VHT MCS values.
   *
   * \return MCS 3 from VHT MCS values
   */
  static WifiMode GetVhtMcs3 (void);
  /**
   * Return MCS 4 from VHT MCS values.
   *
   * \return MCS 4 from VHT MCS values
   */
  static WifiMode GetVhtMcs4 (void);
  /**
   * Return MCS 5 from VHT MCS values.
   *
   * \return MCS 5 from VHT MCS values
   */
  static WifiMode GetVhtMcs5 (void);
  /**
   * Return MCS 6 from VHT MCS values.
   *
   * \return MCS 6 from VHT MCS values
   */
  static WifiMode GetVhtMcs6 (void);
  /**
   * Return MCS 7 from VHT MCS values.
   *
   * \return MCS 7 from VHT MCS values
   */
  static WifiMode GetVhtMcs7 (void);
  /**
   * Return MCS 8 from VHT MCS values.
   *
   * \return MCS 8 from VHT MCS values
   */
  static WifiMode GetVhtMcs8 (void);
  /**
   * Return MCS 9 from VHT MCS values.
   *
   * \return MCS 9 from VHT MCS values
   */
  static WifiMode GetVhtMcs9 (void);

protected:
  // Inherited
  WifiMode GetHtSigMode (void) const override;

private:
  // Inherited
  virtual void BuildModeList (void) override;

  static const PpduFormats m_vhtPpduFormats; //!< VHT PPDU formats
}; //class VhtPhy

} //namespace ns3

#endif /* VHT_PHY_H */
