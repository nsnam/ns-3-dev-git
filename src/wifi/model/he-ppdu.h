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

#ifndef HE_PPDU_H
#define HE_PPDU_H

#include "ofdm-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HePpdu class.
 */

namespace ns3 {

class WifiPsdu;

/**
 * \brief HE PPDU (11ax)
 * \ingroup wifi
 *
 * HePpdu stores a preamble, PHY headers and a map of PSDUs of a PPDU with HE header
 */
class HePpdu : public OfdmPpdu
{
public:
  /**
   * Create an SU HE PPDU, storing a PSDU.
   *
   * \param psdu the PHY payload (PSDU)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   * \param uid the unique ID of this PPDU
   */
  HePpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration,
          WifiPhyBand band, uint64_t uid);
  /**
   * Create an MU HE PPDU, storing a map of PSDUs.
   *
   * This PPDU can either be UL or DL.
   *
   * \param psdus the PHY payloads (PSDUs)
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   * \param band the WifiPhyBand used for the transmission of this PPDU
   * \param uid the unique ID of this PPDU or of the triggering PPDU if this is an HE TB PPDU
   */
  HePpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration,
          WifiPhyBand band, uint64_t uid);
  /**
   * Destructor for HePpdu.
   */
  virtual ~HePpdu ();

  // Inherited
  Time GetTxDuration (void) const override;
  Ptr<WifiPpdu> Copy (void) const override;
  WifiPpduType GetType (void) const override;
  uint16_t GetStaId (void) const override;

  /**
   * Get the payload of the PPDU.
   *
   * \param bssColor the BSS color of the PHY calling this function.
   * \param staId the STA-ID of the PHY calling this function.
   * \return the PSDU
   */
  Ptr<const WifiPsdu> GetPsdu (uint8_t bssColor, uint16_t staId = SU_STA_ID) const;

protected:
  // Inherited
  std::string PrintPayload (void) const override;

  /**
   * Return true if the PPDU is a MU PPDU
   * \return true if the PPDU is a MU PPDU
   */
  bool IsMu (void) const;
  /**
   * Return true if the PPDU is a DL MU PPDU
   * \return true if the PPDU is a DL MU PPDU
   */
  bool IsDlMu (void) const;
  /**
   * Return true if the PPDU is an UL MU PPDU
   * \return true if the PPDU is an UL MU PPDU
   */
  bool IsUlMu (void) const;

  WifiTxVector::HeMuUserInfoMap m_muUserInfos; //!< the HE MU specific per-user information (to be removed once HE-SIG-B headers are implemented)

private:
  // Inherited
  WifiTxVector DoGetTxVector (void) const override;

  /**
   * Fill in the HE PHY headers.
   *
   * \param txVector the TXVECTOR that was used for this PPDU
   * \param ppduDuration the transmission duration of this PPDU
   */
  void SetPhyHeaders (WifiTxVector txVector, Time ppduDuration);

  HeSigHeader m_heSig;  //!< the HE-SIG PHY header
}; //class HePpdu

} //namespace ns3

#endif /* HE_PPDU_H */
