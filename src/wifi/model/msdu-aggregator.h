/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef MSDU_AGGREGATOR_H
#define MSDU_AGGREGATOR_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "wifi-mode.h"
#include "qos-utils.h"
#include <map>

namespace ns3 {

class AmsduSubframeHeader;
class Packet;
class QosTxop;
class WifiMacQueueItem;
class WifiTxVector;

/**
 * \brief Aggregator used to construct A-MSDUs
 * \ingroup wifi
 */
class MsduAggregator : public Object
{
public:
  /// DeaggregatedMsdus typedef
  typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader> > DeaggregatedMsdus;
  /// DeaggregatedMsdusCI typedef
  typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader> >::const_iterator DeaggregatedMsdusCI;
  /// EDCA queues typedef
  typedef std::map<AcIndex, Ptr<QosTxop> > EdcaQueues;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  MsduAggregator ();
  virtual ~MsduAggregator ();

  /**
   * Compute the size of the A-MSDU resulting from the aggregation of an MSDU of
   * size <i>msduSize</i> and an A-MSDU of size <i>amsduSize</i>.
   * Note that only the basic A-MSDU subframe format (section 9.3.2.2.2 of IEEE
   * 802.11-2016) is supported.
   *
   * \param msduSize the MSDU size in bytes.
   * \param amsduSize the A-MSDU size in bytes.
   * \return the size of the resulting A-MSDU in bytes.
   */
  static uint16_t GetSizeIfAggregated (uint16_t msduSize, uint16_t amsduSize);

  /**
   * Dequeue MSDUs to be transmitted to a given station and belonging to a
   * given TID from the corresponding EDCA queue and aggregate them to form
   * an A-MSDU that meets the following constraints:
   *
   * - the A-MSDU size does not exceed the maximum A-MSDU size as determined for
   * the modulation class indicated by the given TxVector
   *
   * - the size of the A-MPDU resulting from the aggregation of the MPDU in which
   * the A-MSDU will be embedded and an existing A-MPDU of the given size
   * (possibly null) does not exceed the maximum A-MPDU size as determined for
   * the modulation class indicated by the given TxVector
   *
   * - the time to transmit the resulting PPDU, according to the given TxVector,
   * does not exceed both the maximum PPDU duration allowed by the corresponding
   * modulation class (if any) and the given PPDU duration limit (if distinct from
   * Time::Min ())
   *
   * If it is not possible to aggregate at least two MSDUs, no MSDU is dequeued
   * from the EDCA queue and a null pointer is returned.
   *
   * \param recipient the receiver station address.
   * \param tid the TID.
   * \param txVector the TxVector used to transmit the frame
   * \param ampduSize the size of the existing A-MPDU in bytes
   * \param ppduDurationLimit the limit on the PPDU duration
   * \return the resulting A-MSDU, if aggregation is possible, 0 otherwise.
   */
  Ptr<WifiMacQueueItem> GetNextAmsdu (Mac48Address recipient, uint8_t tid,
                                      WifiTxVector txVector, uint32_t ampduSize = 0,
                                      Time ppduDurationLimit = Time::Min ()) const;

  /**
   * Determine the maximum size for an A-MSDU of the given TID that can be sent
   * to the given receiver when using the given modulation class.
   *
   * \param recipient the receiver station address.
   * \param tid the TID.
   * \param modulation the modulation class.
   * \return the maximum A-MSDU size in bytes.
   */
  uint16_t GetMaxAmsduSize (Mac48Address recipient, uint8_t tid,
                            WifiModulationClass modulation) const;

  /**
   *
   * \param aggregatedPacket the aggregated packet.
   * \returns DeaggregatedMsdus.
   */
  static DeaggregatedMsdus Deaggregate (Ptr<Packet> aggregatedPacket);

  /**
   * Set the map of EDCA queues.
   *
   * \param map the map of EDCA queues.
   */
  void SetEdcaQueues (EdcaQueues map);

  /**
   * Calculate how much padding must be added to the end of an A-MSDU of the
   * given size if a new MSDU is added.
   * Each A-MSDU subframe is padded so that its length is multiple of 4 octets.
   *
   * \param amsduSize the size of the A-MSDU
   *
   * \return the number of octets required for padding
   */
  static uint8_t CalculatePadding (uint16_t amsduSize);

private:
  EdcaQueues m_edca;   //!< the map of EDCA queues
};

} //namespace ns3

#endif /* MSDU_AGGREGATOR_H */
