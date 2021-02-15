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
  WifiMode GetSigMode (WifiPpduField field, WifiTxVector txVector) const override;
  WifiMode GetSigAMode (void) const override;
  WifiMode GetSigBMode (WifiTxVector txVector) const override;
  virtual const PpduFormats & GetPpduFormats (void) const override;
  Time GetLSigDuration (WifiPreamble preamble) const override;
  virtual Time GetTrainingDuration (WifiTxVector txVector,
                                    uint8_t nDataLtf, uint8_t nExtensionLtf = 0) const override;
  Time GetSigADuration (WifiPreamble preamble) const override;
  Time GetSigBDuration (WifiTxVector txVector) const override;
  virtual Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector,
                                   Time ppduDuration, WifiPhyBand band, uint64_t uid) const override;
  Ptr<const WifiPsdu> GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const override;
  void StartReceivePreamble (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW,
                             Time rxDuration, TxPsdFlag psdFlag) override;
  void CancelAllEvents (void) override;
  virtual uint16_t GetStaId (const Ptr<const WifiPpdu> ppdu) const override;

  /**
   * \return the BSS color of this PHY.
   */
  uint8_t GetBssColor (void) const;

  /**
   * \param ppduDuration the duration of the HE TB PPDU
   * \param band the frequency band being used
   *
   * \return the L-SIG length value corresponding to that HE TB PPDU duration.
   */
  static uint16_t ConvertHeTbPpduDurationToLSigLength (Time ppduDuration, WifiPhyBand band);
  /**
   * \param length the L-SIG length value
   * \param txVector the TXVECTOR used for the transmission of this HE TB PPDU
   * \param band the frequency band being used
   *
   * \return the duration of the HE TB PPDU corresponding to that L-SIG length value.
   */
  static Time ConvertLSigLengthToHeTbPpduDuration (uint16_t length, WifiTxVector txVector, WifiPhyBand band);
  /**
   * \param txVector the transmission parameters used for the HE TB PPDU
   *
   * \return the duration of the non-OFDMA portion of the HE TB PPDU.
   */
  Time CalculateNonOfdmaDurationForHeTb (WifiTxVector txVector) const;

  /**
   * Get the RU band used to transmit a PSDU to a given STA in a HE MU PPDU
   *
   * \param txVector the TXVECTOR used for the transmission
   * \param staId the STA-ID of the recipient
   *
   * \return the RU band used to transmit a PSDU to a given STA in a HE MU PPDU
   */
  WifiSpectrumBand GetRuBand (WifiTxVector txVector, uint16_t staId) const;
  /**
   * Get the band used to transmit the non-OFDMA part of an HE TB PPDU.
   *
   * \param txVector the TXVECTOR used for the transmission
   * \param staId the STA-ID of the station taking part of the UL MU
   *
   * \return the spectrum band used to transmit the non-OFDMA part of an HE TB PPDU
   */
  WifiSpectrumBand GetNonOfdmaBand (WifiTxVector txVector, uint16_t staId) const;

  /**
   * \return the UID of the HE TB PPDU being received
   */
  uint64_t GetCurrentHeTbPpduUid (void) const;

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

protected:
  // Inherited
  PhyFieldRxStatus ProcessSigA (Ptr<Event> event, PhyFieldRxStatus status) override;
  PhyFieldRxStatus ProcessSigB (Ptr<Event> event, PhyFieldRxStatus status) override;
  Ptr<Event> DoGetEvent (Ptr<const WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW) override;
  virtual bool IsConfigSupported (Ptr<const WifiPpdu> ppdu) const override;
  virtual void DoStartReceivePayload (Ptr<Event> event) override;
  std::pair<uint16_t, WifiSpectrumBand> GetChannelWidthAndBand (WifiTxVector txVector, uint16_t staId) const override;
  void DoEndReceivePayload (Ptr<const WifiPpdu> ppdu) override;
  void DoResetReceive (Ptr<Event> event) override;
  void DoAbortCurrentReception (WifiPhyRxfailureReason reason) override;

  /**
   * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived) of an UL-OFDMA transmission.
   * This function is called upon the RX event corresponding to the OFDMA part of the UL MU PPDU.
   *
   * \param event the event holding incoming OFDMA part of the PPDU's information
   */
  void StartReceiveOfdmaPayload (Ptr<Event> event);

  uint64_t m_currentHeTbPpduUid;   //!< UID of the HE TB PPDU being received

  std::map <uint16_t /* STA-ID */, EventId> m_beginOfdmaPayloadRxEvents; //!< the beginning of the OFDMA payload reception events (indexed by STA-ID)

private:
  // Inherited
  virtual void BuildModeList (void) override;
  uint8_t GetNumberBccEncoders (WifiTxVector txVector) const override;
  virtual Time GetSymbolDuration (WifiTxVector txVector) const override;

  static const PpduFormats m_hePpduFormats; //!< HE PPDU formats
}; //class HePhy

} //namespace ns3

#endif /* HE_PHY_H */
