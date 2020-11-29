/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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

#ifndef WIFI_DEFAULT_ACK_MANAGER_H
#define WIFI_DEFAULT_ACK_MANAGER_H

#include "wifi-ack-manager.h"


namespace ns3 {

class WifiTxParameters;
class WifiMacQueueItem;

/**
 * \ingroup wifi
 *
 * WifiDefaultAckManager is the default ack manager.
 */
class WifiDefaultAckManager : public WifiAckManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WifiDefaultAckManager ();
  virtual ~WifiDefaultAckManager ();

  // Overridden from WifiAckManager
  virtual std::unique_ptr<WifiAcknowledgment> TryAddMpdu (Ptr<const WifiMacQueueItem> mpdu,
                                                          const WifiTxParameters& txParams);
  virtual std::unique_ptr<WifiAcknowledgment> TryAggregateMsdu (Ptr<const WifiMacQueueItem> msdu,
                                                                const WifiTxParameters& txParams);

  /**
   * Get the maximum distance between the starting sequence number of the Block
   * Ack agreement which the given MPDU belongs to and each of the sequence numbers
   * of the given MPDU and of all the QoS data frames included in the given TX
   * parameters.
   *
   * \param mpdu the given MPDU
   * \param txParams the given TX parameters
   * \return the maximum distance between the starting sequence number of the Block
   *         Ack agreement which the given MPDU belongs to and each of the sequence
   *         numbers of the given MPDU and of all the QoS data frames included in
   *         the given TX parameters
   */
  uint16_t GetMaxDistFromStartingSeq (Ptr<const WifiMacQueueItem> mpdu,
                                      const WifiTxParameters& txParams) const;

protected:
  /**
   * Determine whether the (A-)MPDU containing the given MPDU and the MPDUs (if any)
   * included in the given TX parameters requires an immediate response (Normal Ack,
   * Block Ack or Block Ack Request followed by Block Ack).
   *
   * \param mpdu the given MPDU.
   * \param txParams the given TX parameters.
   * \return true if the given PSDU requires an immediate response
   */
  bool IsResponseNeeded (Ptr<const WifiMacQueueItem> mpdu,
                         const WifiTxParameters& txParams) const;
private:
  bool m_useExplicitBar;                    //!< true for sending BARs, false for using Implicit BAR policy
  double m_baThreshold;                     //!< Threshold to determine when a BlockAck must be requested
};



} //namespace ns3

#endif /* WIFI_DEFAULT_ACK_MANAGER_H */
