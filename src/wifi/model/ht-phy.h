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

#ifndef HT_PHY_H
#define HT_PHY_H

#include "ofdm-phy.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HtPhy class.
 */

namespace ns3 {

/**
 * This defines the BSS membership value for HT PHY.
 */
#define HT_PHY 127

/**
 * \brief PHY entity for HT (11n)
 * \ingroup wifi
 *
 * HT PHY is based on OFDM PHY.
 * HT-Mixed and HT-Greenfield PPDU formats are
 * supported. Only HT MCSs up to 31 are supported.
 *
 * Refer to IEEE 802.11-2016, clause 19.
 */
class HtPhy : public OfdmPhy
{
public:
  /**
   * Constructor for HT PHY
   *
   * \param maxNss the maximum number of spatial streams
   * \param buildModeList flag used to add HT modes to list (disabled
   *                      by child classes to only add child classes' modes)
   */
  HtPhy (uint8_t maxNss = 1, bool buildModeList = true);
  /**
   * Destructor for HT PHY
   */
  virtual ~HtPhy ();

  // Inherited
  WifiMode GetMcs (uint8_t index) const override;
  bool IsMcsSupported (uint8_t index) const override;
  bool HandlesMcsModes (void) const override;
  virtual WifiMode GetSigMode (WifiPpduField field, WifiTxVector txVector) const override;
  virtual const PpduFormats & GetPpduFormats (void) const override;
  virtual Time GetDuration (WifiPpduField field, WifiTxVector txVector) const override;
  Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                           bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                           uint16_t staId) const override;
  virtual Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector,
                                   Time ppduDuration, WifiPhyBand band, uint64_t uid) const override;

  /**
   * \return the WifiMode used for the L-SIG (non-HT header) field
   */
  static WifiMode GetLSigMode (void);
  /**
   * \return the WifiMode used for the HT-SIG field
   */
  virtual WifiMode GetHtSigMode (void) const;

  /**
   * \return the BSS membership selector for this PHY entity
   */
  uint8_t GetBssMembershipSelector (void) const;

  /**
   * Set the maximum supported MCS index __per spatial stream__.
   * For HT, this results in non-continuous indices for supported MCSs.
   *
   * \return the maximum MCS index per spatial stream supported by this entity
   */
  uint8_t GetMaxSupportedMcsIndexPerSs (void) const;
  /**
   * Set the maximum supported MCS index __per spatial stream__.
   * For HT, this results in non-continuous indices for supported MCSs.
   *
   * \param maxIndex the maximum MCS index per spatial stream supported by this entity
   *
   * The provided value should not be greater than maximum standard-defined value.
   */
  void SetMaxSupportedMcsIndexPerSs (uint8_t maxIndex);
  /**
   * Configure the maximum number of spatial streams supported
   * by this HT PHY.
   *
   * \param maxNss the maximum number of spatial streams
   */
  void SetMaxSupportedNss (uint8_t maxNss);

  /**
   * \param preamble the type of preamble
   * \return the duration of the L-SIG (non-HT header) field
   *
   * \see WIFI_PPDU_FIELD_NON_HT_HEADER
   */
  virtual Time GetLSigDuration (WifiPreamble preamble) const;
  /**
   * \param txVector the transmission parameters
   * \param nDataLtf the number of data LTF fields (excluding those in preamble)
   * \param nExtensionLtf the number of extension LTF fields
   * \return the duration of the training field
   *
   * \see WIFI_PPDU_FIELD_TRAINING
   */
  virtual Time GetTrainingDuration (WifiTxVector txVector,
                                    uint8_t nDataLtf, uint8_t nExtensionLtf = 0) const;
  /**
   * \return the duration of the HT-SIG field
   */
  virtual Time GetHtSigDuration (void) const;

  /**
   * Initialize all HT modes.
   */
  static void InitializeModes (void);
  /**
   * Return the HT MCS corresponding to
   * the provided index.
   *
   * \param index the index of the MCS
   * \return an HT MCS
   */
  static WifiMode GetHtMcs (uint8_t index);

  /**
   * Return MCS 0 from HT MCS values.
   *
   * \return MCS 0 from HT MCS values
   */
  static WifiMode GetHtMcs0 (void);
  /**
   * Return MCS 1 from HT MCS values.
   *
   * \return MCS 1 from HT MCS values
   */
  static WifiMode GetHtMcs1 (void);
  /**
   * Return MCS 2 from HT MCS values.
   *
   * \return MCS 2 from HT MCS values
   */
  static WifiMode GetHtMcs2 (void);
  /**
   * Return MCS 3 from HT MCS values.
   *
   * \return MCS 3 from HT MCS values
   */
  static WifiMode GetHtMcs3 (void);
  /**
   * Return MCS 4 from HT MCS values.
   *
   * \return MCS 4 from HT MCS values
   */
  static WifiMode GetHtMcs4 (void);
  /**
   * Return MCS 5 from HT MCS values.
   *
   * \return MCS 5 from HT MCS values
   */
  static WifiMode GetHtMcs5 (void);
  /**
   * Return MCS 6 from HT MCS values.
   *
   * \return MCS 6 from HT MCS values
   */
  static WifiMode GetHtMcs6 (void);
  /**
   * Return MCS 7 from HT MCS values.
   *
   * \return MCS 7 from HT MCS values
   */
  static WifiMode GetHtMcs7 (void);
  /**
   * Return MCS 8 from HT MCS values.
   *
   * \return MCS 8 from HT MCS values
   */
  static WifiMode GetHtMcs8 (void);
  /**
   * Return MCS 9 from HT MCS values.
   *
   * \return MCS 9 from HT MCS values
   */
  static WifiMode GetHtMcs9 (void);
  /**
   * Return MCS 10 from HT MCS values.
   *
   * \return MCS 10 from HT MCS values
   */
  static WifiMode GetHtMcs10 (void);
  /**
   * Return MCS 11 from HT MCS values.
   *
   * \return MCS 11 from HT MCS values
   */
  static WifiMode GetHtMcs11 (void);
  /**
   * Return MCS 12 from HT MCS values.
   *
   * \return MCS 12 from HT MCS values
   */
  static WifiMode GetHtMcs12 (void);
  /**
   * Return MCS 13 from HT MCS values.
   *
   * \return MCS 13 from HT MCS values
   */
  static WifiMode GetHtMcs13 (void);
  /**
   * Return MCS 14 from HT MCS values.
   *
   * \return MCS 14 from HT MCS values
   */
  static WifiMode GetHtMcs14 (void);
  /**
   * Return MCS 15 from HT MCS values.
   *
   * \return MCS 15 from HT MCS values
   */
  static WifiMode GetHtMcs15 (void);
  /**
   * Return MCS 16 from HT MCS values.
   *
   * \return MCS 16 from HT MCS values
   */
  static WifiMode GetHtMcs16 (void);
  /**
   * Return MCS 17 from HT MCS values.
   *
   * \return MCS 17 from HT MCS values
   */
  static WifiMode GetHtMcs17 (void);
  /**
   * Return MCS 18 from HT MCS values.
   *
   * \return MCS 18 from HT MCS values
   */
  static WifiMode GetHtMcs18 (void);
  /**
   * Return MCS 19 from HT MCS values.
   *
   * \return MCS 19 from HT MCS values
   */
  static WifiMode GetHtMcs19 (void);
  /**
   * Return MCS 20 from HT MCS values.
   *
   * \return MCS 20 from HT MCS values
   */
  static WifiMode GetHtMcs20 (void);
  /**
   * Return MCS 21 from HT MCS values.
   *
   * \return MCS 21 from HT MCS values
   */
  static WifiMode GetHtMcs21 (void);
  /**
   * Return MCS 22 from HT MCS values.
   *
   * \return MCS 22 from HT MCS values
   */
  static WifiMode GetHtMcs22 (void);
  /**
   * Return MCS 23 from HT MCS values.
   *
   * \return MCS 23 from HT MCS values
   */
  static WifiMode GetHtMcs23 (void);
  /**
   * Return MCS 24 from HT MCS values.
   *
   * \return MCS 24 from HT MCS values
   */
  static WifiMode GetHtMcs24 (void);
  /**
   * Return MCS 25 from HT MCS values.
   *
   * \return MCS 25 from HT MCS values
   */
  static WifiMode GetHtMcs25 (void);
  /**
   * Return MCS 26 from HT MCS values.
   *
   * \return MCS 26 from HT MCS values
   */
  static WifiMode GetHtMcs26 (void);
  /**
   * Return MCS 27 from HT MCS values.
   *
   * \return MCS 27 from HT MCS values
   */
  static WifiMode GetHtMcs27 (void);
  /**
   * Return MCS 28 from HT MCS values.
   *
   * \return MCS 28 from HT MCS values
   */
  static WifiMode GetHtMcs28 (void);
  /**
   * Return MCS 29 from HT MCS values.
   *
   * \return MCS 29 from HT MCS values
   */
  static WifiMode GetHtMcs29 (void);
  /**
   * Return MCS 30 from HT MCS values.
   *
   * \return MCS 30 from HT MCS values
   */
  static WifiMode GetHtMcs30 (void);
  /**
   * Return MCS 31 from HT MCS values.
   *
   * \return MCS 31 from HT MCS values
   */
  static WifiMode GetHtMcs31 (void);

protected:
  // Inherited
  virtual PhyFieldRxStatus DoEndReceiveField (WifiPpduField field, Ptr<Event> event) override;
  virtual bool IsAllConfigSupported (WifiPpduField field, Ptr<const WifiPpdu> ppdu) const override;
  virtual bool IsConfigSupported (Ptr<const WifiPpdu> ppdu) const override;

  /**
   * Build mode list.
   * Should be redone whenever the maximum MCS index per spatial stream
   * ,or any other important parameter having an impact on the MCS index
   * (e.g. number of spatial streams for HT), changes.
   */
  virtual void BuildModeList (void);

  /**
   * \param txVector the transmission parameters
   * \return the number of BCC encoders used for data encoding
   */
  virtual uint8_t GetNumberBccEncoders (WifiTxVector txVector) const;
  /**
   * \param txVector the transmission parameters
   * \return the symbol duration (including GI)
   */
  virtual Time GetSymbolDuration (WifiTxVector txVector) const;

  uint8_t m_maxMcsIndexPerSs;          //!< the maximum MCS index per spatial stream as defined by the standard
  uint8_t m_maxSupportedMcsIndexPerSs; //!< the maximum supported MCS index per spatial stream
  uint8_t m_bssMembershipSelector;     //!< the BSS membership selector

private:
  /**
   * End receiving the HT-SIG, perform HT-specific actions, and
   * provide the status of the reception.
   *
   * \param event the event holding incoming PPDU's information
   * \return status of the reception of the HT-SIG
   */
  PhyFieldRxStatus EndReceiveHtSig (Ptr<Event> event);

  uint8_t m_maxSupportedNss; //!< Maximum supported number of spatial streams (used to build HT MCS indices)

  static const PpduFormats m_htPpduFormats; //!< HT PPDU formats
}; //class HtPhy

} //namespace ns3

#endif /* HT_PHY_H */
