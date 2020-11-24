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
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#ifndef DSSS_PHY_H
#define DSSS_PHY_H

#include "phy-entity.h"
#include <vector>

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::DsssPhy class.
 */

namespace ns3 {

/**
 * \brief PHY entity for HR/DSSS (11b)
 * \ingroup wifi
 *
 * Refer to IEEE 802.11-2016, clause 16 (HR/DSSS).
 * Note that DSSS rates (clause 15) are a subset
 * of HR/DSSS rates.
 */
class DsssPhy : public PhyEntity
{
public:
  /**
   * Constructor for HR/DSSS PHY
   */
  DsssPhy ();
  /**
   * Destructor for HR/DSSS PHY
   */
  virtual ~DsssPhy ();

  // Inherited
  WifiMode GetSigMode (WifiPpduField field, WifiTxVector txVector) const override;
  const PpduFormats & GetPpduFormats (void) const override;
  Time GetDuration (WifiPpduField field, WifiTxVector txVector) const override;
  Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                           bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                           uint16_t staId) const override;
  Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector,
                           Time ppduDuration, WifiPhyBand band, uint64_t uid) const override;

  /**
   * Initialize all HR/DSSS modes.
   */
  static void InitializeModes (void);
  /**
   * Return a WifiMode for HR/DSSS
   * corresponding to the provided rate.
   *
   * \param rate the rate in bps
   * \return a WifiMode for HR/DSSS
   */
  static WifiMode GetDsssRate (uint64_t rate);
  /**
   * Return the list of rates (in bps) achievable with
   * HR/DSSS.
   *
   * \return a vector containing the achievable rates in bps
   */
  static std::vector<uint64_t> GetDsssRatesBpsList (void);

  /**
   * Return a WifiMode for DSSS at 1 Mbps.
   *
   * \return a WifiMode for DSSS at 1 Mbps
   */
  static WifiMode GetDsssRate1Mbps (void);
  /**
   * Return a WifiMode for DSSS at 2 Mbps.
   *
   * \return a WifiMode for DSSS at 2 Mbps
   */
  static WifiMode GetDsssRate2Mbps (void);
  /**
   * Return a WifiMode for HR/DSSS at 5.5 Mbps.
   *
   * \return a WifiMode for HR/DSSS at 5.5 Mbps
   */
  static WifiMode GetDsssRate5_5Mbps (void);
  /**
   * Return a WifiMode for HR/DSSS at 11 Mbps.
   *
   * \return a WifiMode for HR/DSSS at 11 Mbps
   */
  static WifiMode GetDsssRate11Mbps (void);

private:
  /**
   * \param txVector the transmission parameters
   * \return the WifiMode used for the PHY header field
   */
  WifiMode GetHeaderMode (WifiTxVector txVector) const;

  /**
   * \param txVector the transmission parameters
   * \return the duration of the PHY preamble field
   *
   * \see WIFI_PPDU_FIELD_PREAMBLE
   */
  Time GetPreambleDuration (WifiTxVector txVector) const;
  /**
   * \param txVector the transmission parameters
   * \return the duration of the PHY header field
   */
  Time GetHeaderDuration (WifiTxVector txVector) const;

  static const PpduFormats m_dsssPpduFormats; //!< DSSS and HR/DSSS PPDU formats
}; //class DsssPhy

} //namespace ns3

#endif /* DSSS_PHY_H */
