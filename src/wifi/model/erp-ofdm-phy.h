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

#ifndef ERP_OFDM_PHY_H
#define ERP_OFDM_PHY_H

#include "ofdm-phy.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::ErpOfdmPhy class.
 */

namespace ns3 {

/**
 * \brief PHY entity for ERP-OFDM (11g)
 * \ingroup wifi
 *
 * ERP-OFDM PHY is based on OFDM PHY.
 * ERP-DSSS/CCK mode is not supported.
 *
 * Refer to IEEE 802.11-2016, clause 18.
 */
class ErpOfdmPhy : public OfdmPhy
{
public:
  /**
   * Constructor for ERP-OFDM PHY
   */
  ErpOfdmPhy ();
  /**
   * Destructor for ERP-OFDM PHY
   */
  virtual ~ErpOfdmPhy ();

  //Inherited
  Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration) override;

  /**
   * Initialize all ERP-OFDM modes.
   */
  static void InitializeModes (void);
  /**
   * Return a WifiMode for ERP-OFDM
   * corresponding to the provided rate.
   *
   * \param rate the rate in bps
   * \return a WifiMode for ERP-OFDM
   */
  static WifiMode GetErpOfdmRate (uint64_t rate);
  /**
   * Return the list of rates (in bps) achievable with
   * ERP-OFDM.
   *
   * \return a vector containing the achievable rates in bps
   */
  static std::vector<uint64_t> GetErpOfdmRatesBpsList (void);

  /**
   * Return a WifiMode for ERP-OFDM at 6 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 6 Mbps
   */
  static WifiMode GetErpOfdmRate6Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 9 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 9 Mbps
   */
  static WifiMode GetErpOfdmRate9Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 12 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 12 Mbps
   */
  static WifiMode GetErpOfdmRate12Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 18 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 18 Mbps
   */
  static WifiMode GetErpOfdmRate18Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 24 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 24 Mbps
   */
  static WifiMode GetErpOfdmRate24Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 36 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 36 Mbps
   */
  static WifiMode GetErpOfdmRate36Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 48 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 48 Mbps
   */
  static WifiMode GetErpOfdmRate48Mbps (void);
  /**
   * Return a WifiMode for ERP-OFDM at 54 Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 54 Mbps
   */
  static WifiMode GetErpOfdmRate54Mbps (void);

private:
  // Inherited
  WifiMode GetHeaderMode (WifiTxVector txVector) const override;
  Time GetPreambleDuration (WifiTxVector txVector) const override;
  Time GetHeaderDuration (WifiTxVector txVector) const override;
}; //class ErpOfdmPhy

} //namespace ns3

#endif /* ERP_OFDM_PHY_H */
