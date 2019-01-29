/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_ITEM_H
#define WIFI_MAC_QUEUE_ITEM_H

#include "ns3/nstime.h"
#include "wifi-mac-header.h"

namespace ns3 {

class QosBlockedDestinations;
class Packet;

/**
 * \ingroup wifi
 *
 * WifiMacQueueItem stores (const) packets along with their Wifi MAC headers
 * and the time when they were enqueued.
 */
class WifiMacQueueItem : public SimpleRefCount<WifiMacQueueItem>
{
public:
  /**
   * \brief Create a Wifi MAC queue item containing a packet and a Wifi MAC header.
   * \param p the const packet included in the created item.
   * \param header the Wifi Mac header included in the created item.
   */
  WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header);

  /**
   * \brief Create a Wifi MAC queue item containing a packet and a Wifi MAC header.
   * \param p the const packet included in the created item.
   * \param header the Wifi Mac header included in the created item.
   * \param tstamp the timestamp associated with the created item.
   */
  WifiMacQueueItem (Ptr<const Packet> p, const WifiMacHeader & header, Time tstamp);

  virtual ~WifiMacQueueItem ();

  /**
   * \brief Get the packet stored in this item
   * \return the packet stored in this item.
   */
  Ptr<const Packet> GetPacket (void) const;

  /**
   * \brief Get the header stored in this item
   * \return the header stored in this item.
   */
  const WifiMacHeader & GetHeader (void) const;

  /**
   * \brief Get the header stored in this item
   * \return the header stored in this item.
   */
  WifiMacHeader & GetHeader (void);

  /**
   * \brief Return the destination address present in the header
   * \return the destination address
   */
  Mac48Address GetDestinationAddress (void) const;

  /**
   * \brief Get the timestamp included in this item
   * \return the timestamp included in this item.
   */
  Time GetTimeStamp (void) const;

  /**
   * \brief Return the size of the packet stored by this item, including header
   *        size and trailer size
   *
   * \return the size of the packet stored by this item.
   */
  uint32_t GetSize (void) const;

  /**
   * \brief Print the item contents.
   * \param os output stream in which the data should be printed.
   */
  virtual void Print (std::ostream &os) const;

private:
  Ptr<const Packet> m_packet;  //!< The packet contained in this queue item
  WifiMacHeader m_header;      //!< Wifi MAC header associated with the packet
  Time m_tstamp;               //!< timestamp when the packet arrived at the queue
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param item the item
 * \returns a reference to the stream
 */
std::ostream& operator<< (std::ostream& os, const WifiMacQueueItem &item);

} //namespace ns3

#endif /* WIFI_MAC_QUEUE_ITEM_H */
