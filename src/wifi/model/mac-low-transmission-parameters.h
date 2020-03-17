/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2006 INRIA
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
 */

#ifndef MAC_LOW_TRANSMISSION_PARAMETERS_H
#define MAC_LOW_TRANSMISSION_PARAMETERS_H

#include "ns3/uinteger.h"
#include "block-ack-type.h"

namespace ns3 {

/**
 * \brief control how a packet is transmitted.
 * \ingroup wifi
 *
 * The ns3::MacLow::StartTransmission method expects
 * an instance of this class to describe how the packet
 * should be transmitted.
 */
class MacLowTransmissionParameters
{
public:
  MacLowTransmissionParameters ();

  /**
   * Wait ACKTimeout for an Ack. If we get an Ack
   * on time, call MacLowTransmissionListener::GotAck.
   * Call MacLowTransmissionListener::MissedAck otherwise.
   */
  void EnableAck (void);
  /**
   * Wait the timeout corresponding to the given BlockAck response type.
   *
   * \param type the BlockAck response type
   */
  void EnableBlockAck (BlockAckType type);
  /**
   * Schedule the transmission of a BlockAckRequest of the given type.
   *
   * \param type the BlockAckRequest type
   */
  void EnableBlockAckRequest (BlockAckType type);
  /**
   * Send a RTS, and wait CTSTimeout for a CTS. If we get a
   * CTS on time, call MacLowTransmissionListener::GotCts
   * and send data. Otherwise, call MacLowTransmissionListener::MissedCts
   * and do not send data.
   */
  void EnableRts (void);
  /**
   * \param size size of next data to send after current packet is
   *        sent in bytes.
   *
   * Add the transmission duration of the next data to the
   * durationId of the outgoing packet and call
   * MacLowTransmissionListener::StartNextFragment at the end of
   * the current transmission + SIFS.
   */
  void EnableNextData (uint32_t size);
  /**
   * Do not wait for Ack after data transmission. Typically
   * used for Broadcast and multicast frames.
   */
  void DisableAck (void);
  /**
   * Do not send BlockAckRequest after data transmission
   */
  void DisableBlockAckRequest (void);
  /**
   * Do not send RTS and wait for CTS before sending data.
   */
  void DisableRts (void);
  /**
   * Do not attempt to send data burst after current transmission
   */
  void DisableNextData (void);
  /**
   * \returns true if normal ack protocol should be used, false
   *          otherwise.
   *
   * \sa EnableAck
   */
  bool MustWaitNormalAck (void) const;
  /**
   * \returns true if block ack mechanism is used, false otherwise.
   *
   * \sa EnableBlockAck
   */
  bool MustWaitBlockAck (void) const;
  /**
   * \returns the selected BlockAck variant.
   *
   * Only call this method if the block ack mechanism is used.
   */
  BlockAckType GetBlockAckType (void) const;
  /**
   * \returns true if a BlockAckRequest must be sent, false otherwise.
   *
   * Return true if a BlockAckRequest must be sent, false otherwise.
   */
  bool MustSendBlockAckRequest (void) const;
  /**
   * \returns the selected BlockAckRequest variant.
   *
   * Only call this method if a BlockAckRequest must be sent.
   */
  BlockAckType GetBlockAckRequestType (void) const;
  /**
   * \returns true if RTS should be sent and CTS waited for before
   *          sending data, false otherwise.
   */
  bool MustSendRts (void) const;
  /**
   * \returns true if EnableNextData was called, false otherwise.
   */
  bool HasNextPacket (void) const;
  /**
   * \returns the size specified by EnableNextData.
   */
  uint32_t GetNextPacketSize (void) const;

private:
  friend std::ostream &operator << (std::ostream &os, const MacLowTransmissionParameters &params);
  uint32_t m_nextSize; //!< the next size
  /// wait Ack enumerated type
  enum
  {
    ACK_NONE,
    ACK_NORMAL,
    BLOCK_ACK_BASIC,
    BLOCK_ACK_COMPRESSED,
    BLOCK_ACK_EXTENDED_COMPRESSED,
    BLOCK_ACK_MULTI_TID
  } m_waitAck; //!< wait Ack
  /// send BAR enumerated type
  enum
  {
    BLOCK_ACK_REQUEST_NONE,
    BLOCK_ACK_REQUEST_BASIC,
    BLOCK_ACK_REQUEST_COMPRESSED,
    BLOCK_ACK_REQUEST_EXTENDED_COMPRESSED,
    BLOCK_ACK_REQUEST_MULTI_TID
  } m_sendBar; //!< send BAR
  bool m_sendRts; //!< send an RTS?
};

/**
 * Serialize MacLowTransmissionParameters to ostream in a human-readable form.
 *
 * \param os std::ostream
 * \param params MacLowTransmissionParameters
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const MacLowTransmissionParameters &params);

} //namespace ns3

#endif /* MAC_LOW_TRANSMISSION_PARAMETERS_H */
