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

#ifndef WIFI_ACK_POLICY_SELECTOR_H
#define WIFI_ACK_POLICY_SELECTOR_H

#include "ns3/object.h"
#include "mac-low-transmission-parameters.h"
#include "qos-txop.h"
#include "wifi-psdu.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * WifiAckPolicySelector is in charge of selecting the acknowledgment policy
 * for PSDUs containing QoS Data frames.
 */
class WifiAckPolicySelector : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~WifiAckPolicySelector();

  virtual void DoDispose (void);

  /**
   * Set the QoS Txop associated with this ack policy selector.
   *
   * \param qosTxop the QoS Txop.
   */
  void SetQosTxop (Ptr<QosTxop> qosTxop);

  /**
   * Get the QoS Txop associated with this ack policy selector.
   *
   * \return the QoS Txop.
   */
  Ptr<QosTxop> GetQosTxop (void) const;

  /**
   * Set the QoS Ack Policy for the QoS Data frames contained in the given PSDU
   * according to the given MacLow transmission parameters. Only single-TID A-MPDUs
   * are supported at the moment, hence it is expected that all the QoS Data frames
   * contained in the given PSDU have the same TID. This function is typically
   * called by MacLow before forwarding the PSDU down to the PHY layer.
   *
   * \param psdu the given PSDU.
   * \param params the given MacLow transmission parameters.
   */
  static void SetAckPolicy (Ptr<WifiPsdu> psdu, const MacLowTransmissionParameters & params);

  /**
   * Update the transmission parameters related to the acknowledgment policy for
   * the given PSDU. This method is typically called by the MPDU aggregator when
   * trying to aggregate another MPDU to the current A-MPDU. In fact, the
   * AckPolicySelector may switch to a different acknowledgment policy when a
   * new MPDU is aggregated to an A-MPDU.
   *
   * \param psdu the given PSDU.
   * \param params the MacLow parameters to update.
   */
  virtual void UpdateTxParams (Ptr<WifiPsdu> psdu, MacLowTransmissionParameters & params) = 0;

protected:
  Ptr<QosTxop> m_qosTxop;      //!< the QoS Txop this selector is associated with
};

} //namespace ns3

#endif /* WIFI_ACK_POLICY_SELECTOR_H */
