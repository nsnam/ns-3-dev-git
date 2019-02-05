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

#ifndef WIFI_PSDU_H
#define WIFI_PSDU_H

#include "ns3/nstime.h"
#include "wifi-mac-header.h"
#include <vector>
#include <set>

namespace ns3 {

class WifiMacQueueItem;
class Packet;

/**
 * \ingroup wifi
 *
 * WifiPsdu stores an MPDU, S-MPDU or A-MPDU, by keeping header(s) and
 * payload(s) separate for each constituent MPDU.
 */
class WifiPsdu : public SimpleRefCount<WifiPsdu>
{
public:
  /**
   * Create a PSDU storing an MPDU. Typically used for control and management
   * frames that do not have to keep an associated lifetime and are not stored
   * in an S-MPDU.
   *
   * \param p the payload of the MPDU.
   * \param header the Wifi Mac header of the MPDU.
   */
  WifiPsdu (Ptr<const Packet> p, const WifiMacHeader & header);

  /**
   * Create a PSDU storing an MPDU or S-MPDU. Typically used for QoS data
   * frames that have to keep an associated lifetime.
   *
   * \param mpdu the MPDU.
   * \param isSingle true for an S-MPDU
   */
  WifiPsdu (Ptr<WifiMacQueueItem> mpdu, bool isSingle);

  /**
   * Create a PSDU storing an MPDU or S-MPDU. Typically used for QoS data
   * frames that have to keep an associated lifetime.
   *
   * \param mpdu the MPDU.
   * \param isSingle true for an S-MPDU
   */
  WifiPsdu (Ptr<const WifiMacQueueItem> mpdu, bool isSingle);

  /**
   * Create a PSDU storing an S-MPDU or A-MPDU.
   *
   * \param mpduList the list of constituent MPDUs.
   */
  WifiPsdu (std::vector<Ptr<WifiMacQueueItem>> mpduList);

  virtual ~WifiPsdu ();

  /**
   * Return true if the PSDU is an S-MPDU
   * \return true if the PSDU is an S-MPDU.
   */
  bool IsSingle (void) const;

  /**
   * Return true if the PSDU is an S-MPDU or A-MPDU
   * \return true if the PSDU is an S-MPDU or A-MPDU.
   */
  bool IsAggregate (void) const;

  /**
   * \brief Get the PSDU as a single packet
   * \return the PSDU.
   */
  Ptr<const Packet> GetPacket (void) const;

  /**
   * \brief Get the header of the i-th MPDU
   * \return the header of the i-th MPDU.
   */
  const WifiMacHeader & GetHeader (std::size_t i) const;

  /**
   * \brief Get the header of the i-th MPDU
   * \return the header of the i-th MPDU.
   */
  WifiMacHeader & GetHeader (std::size_t i);

  /**
   * \brief Get the payload of the i-th MPDU
   * \return the payload of the i-th MPDU.
   */
  Ptr<const Packet> GetPayload (std::size_t i) const;

  /**
   * \brief Get the timestamp of the i-th MPDU
   * \return the timestamp of the i-th MPDU.
   */
  Time GetTimeStamp (std::size_t i) const;

  /**
   * Get the Receiver Address (RA), which is common to all the MPDUs
   * \return the Receiver Address.
   */
  Mac48Address GetAddr1 (void) const;

  /**
   * Get the Transmitter Address (TA), which is common to all the MPDUs
   * \return the Transmitter Address.
   */
  Mac48Address GetAddr2 (void) const;

  /**
   * Get the duration from the Duration/ID field, which is common to all the MPDUs
   * \return the duration from the Duration/ID field.
   */
  Time GetDuration (void) const;

  /**
   * Set the Duration/ID field on all the MPDUs
   * \param duration the value for the Duration/ID field.
   */
  void SetDuration (Time duration);

  /**
   * Get the set of TIDs of the QoS Data frames included in the PSDU. Note that
   * only single-TID A-MPDUs are currently supported, hence the returned set
   * contains at most one TID value.
   *
   * \return the set of TIDs of the QoS Data frames included in the PSDU.
   */
  std::set<uint8_t> GetTids (void) const;

  /**
   * Get the QoS Ack Policy of the QoS Data frames included in the PSDU that
   * have the given TID. Also, check that all the QoS Data frames having the
   * given TID have the same QoS Ack Policy. Do not call this method if there
   * is no QoS Date frame in the PSDU.
   *
   * \param tid the given TID
   * \return the QoS Ack Policy common to all QoS Data frames having the given TID.
   */
  WifiMacHeader::QosAckPolicy GetAckPolicyForTid (uint8_t tid) const;

  /**
   * Set the QoS Ack Policy of the QoS Data frames included in the PSDU that
   * have the given TID to the given policy.
   *
   * \param tid the given TID
   * \param policy the given QoS Ack policy
   */
  void SetAckPolicyForTid (uint8_t tid, WifiMacHeader::QosAckPolicy policy);

  /**
   * \brief Return the size of the PSDU
   *
   * \return the size of the PSDU.
   */
  uint32_t GetSize (void) const;

  /**
   * \brief Return the number of MPDUs constituting the PSDU
   *
   * \return the number of MPDUs constituting the PSDU.
   */
  std::size_t GetNMpdus (void) const;

  /**
   * \brief Return a const iterator to the first MPDU
   *
   * \return a const iterator to the first MPDU.
   */
  std::vector<Ptr<WifiMacQueueItem>>::const_iterator begin (void) const;

  /**
   * \brief Return an iterator to the first MPDU
   *
   * \return an iterator to the first MPDU.
   */
  std::vector<Ptr<WifiMacQueueItem>>::iterator begin (void);

  /**
   * \brief Return a const iterator to past-the-last MPDU
   *
   * \return a const iterator to past-the-last MPDU.
   */
  std::vector<Ptr<WifiMacQueueItem>>::const_iterator end (void) const;

  /**
   * \brief Return an iterator to past-the-last MPDU
   *
   * \return an iterator to past-the-last MPDU.
   */
  std::vector<Ptr<WifiMacQueueItem>>::iterator end (void);

private:
  bool m_isSingle;                                //!< true for an S-MPDU
  std::vector<Ptr<WifiMacQueueItem>> m_mpduList;  //!< list of constituent MPDUs
  uint32_t m_size;                                //!< the size of the PSDU
};


} //namespace ns3

#endif /* WIFI_PSDU_H */
