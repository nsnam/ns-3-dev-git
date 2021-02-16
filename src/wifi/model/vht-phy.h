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
#include <tuple>

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
  virtual Time GetDuration (WifiPpduField field, WifiTxVector txVector) const override;
  virtual Time GetLSigDuration (WifiPreamble preamble) const override;
  virtual Time GetTrainingDuration (WifiTxVector txVector,
                                    uint8_t nDataLtf, uint8_t nExtensionLtf = 0) const override;
  virtual Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration) override;

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
   * \param preamble the type of preamble
   * \return the duration of the SIG-A field
   */
  virtual Time GetSigADuration (WifiPreamble preamble) const;
  /**
   * \param txVector the transmission parameters
   * \return the duration of the SIG-B field
   */
  virtual Time GetSigBDuration (WifiTxVector txVector) const;

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
  Time GetHtSigDuration (void) const override;
  virtual uint8_t GetNumberBccEncoders (WifiTxVector txVector) const override;
  virtual PhyFieldRxStatus DoEndReceiveField (WifiPpduField field, Ptr<Event> event) override;
  virtual bool IsAllConfigSupported (WifiPpduField field, Ptr<const WifiPpdu> ppdu) const override;

  /**
   * End receiving the SIG-A, perform VHT-specific actions, and
   * provide the status of the reception.
   *
   * Child classes can perform amendment-specific actions by specializing
   * \see ProcessSigA.
   *
   * \param event the event holding incoming PPDU's information
   * \return status of the reception of the SIG-A
   */
  PhyFieldRxStatus EndReceiveSigA (Ptr<Event> event);
  /**
   * End receiving the SIG-B, perform VHT-specific actions, and
   * provide the status of the reception.
   *
   * Child classes can perform amendment-specific actions by specializing
   * \see ProcessSigB.
   *
   * \param event the event holding incoming PPDU's information
   * \return status of the reception of the SIG-B
   */
  PhyFieldRxStatus EndReceiveSigB (Ptr<Event> event);

  /**
   * Process SIG-A, perform amendment-specific actions, and
   * provide an updated status of the reception.
   *
   * \param event the event holding incoming PPDU's information
   * \param status the status of the reception of the correctly received SIG-A after the configuration support check
   * \return the updated status of the reception of the SIG-A
   */
  virtual PhyFieldRxStatus ProcessSigA (Ptr<Event> event, PhyFieldRxStatus status);
  /**
   * Process SIG-B, perform amendment-specific actions, and
   * provide an updated status of the reception.
   *
   * \param event the event holding incoming PPDU's information
   * \param status the status of the reception of the correctly received SIG-B after the configuration support check
   * \return the updated status of the reception of the SIG-B
   */
  virtual PhyFieldRxStatus ProcessSigB (Ptr<Event> event, PhyFieldRxStatus status);

private:
  // Inherited
  virtual void BuildModeList (void) override;

  /**
   * Typedef for storing exceptions in the number of BCC encoders for VHT MCSs
   */
  typedef std::map< std::tuple<uint16_t /* channelWidth in MHz */,
                               uint8_t /* Nss */,
                               uint8_t /* MCS index */>, uint8_t /* Nes */ > NesExceptionMap;
  static const NesExceptionMap m_exceptionsMap; //!< exception map for number of BCC encoders (extracted from VHT-MCS tables)
  static const PpduFormats m_vhtPpduFormats;    //!< VHT PPDU formats
}; //class VhtPhy

} //namespace ns3

#endif /* VHT_PHY_H */
