/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef CONSTANT_WIFI_ACK_POLICY_SELECTOR_H
#define CONSTANT_WIFI_ACK_POLICY_SELECTOR_H

#include "wifi-ack-policy-selector.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * A constant ack policy selector operating based on the values of its attributes.
 */
class ConstantWifiAckPolicySelector : public WifiAckPolicySelector
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ConstantWifiAckPolicySelector ();
  virtual ~ConstantWifiAckPolicySelector();

  /**
   * Update the transmission parameters related to the acknowledgment policy for
   * the given PSDU. This method is typically called by the MPDU aggregator when
   * trying to aggregate another MPDU to the current A-MPDU. In fact, the
   * AckPolicySelector may switch to a different acknowledgment policy when a
   * new MPDU is aggregated to an A-MPDU.
   * Note that multi-TID A-MPDUs are currently not supported by this method.
   *
   * \param psdu the given PSDU.
   * \param params the MacLow parameters to update.
   */
  virtual void UpdateTxParams (Ptr<WifiPsdu> psdu, MacLowTransmissionParameters & params);

private:
  bool m_useExplicitBar; //!< true for sending BARs, false for using Implicit BAR ack policy
  double m_baThreshold;  //!< Threshold to determine when a BlockAck must be requested
};

} //namespace ns3

#endif /* CONSTANT_WIFI_ACK_POLICY_SELECTOR_H */
