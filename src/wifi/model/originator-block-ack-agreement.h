/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#ifndef ORIGINATOR_BLOCK_ACK_AGREEMENT_H
#define ORIGINATOR_BLOCK_ACK_AGREEMENT_H

#include "block-ack-agreement.h"

namespace ns3 {

/**
 * \ingroup wifi
 * Maintains the state and information about transmitted MPDUs with ack policy block ack
 * for an originator station. The state diagram is as follows:
 *
   \verbatim
                                    --------------
                                    |  REJECTED  |
                                    --------------
                                          ^
                   receive ADDBAResponse  |
                   status code = failure  |
                                          |
                                          |           receive ADDBAResponse
   /------------\ send ADDBARequest ----------------  status code = success  ----------------
   |   START    |------------------>|   PENDING    |------------------------>|  ESTABLISHED |-----
   \------------/                   ----------------                         ----------------     |
         ^                                |                                    /   ^    ^         |
         |              no ADDBAResponse  |                receive BlockAck   /    |    |         | receive BlockAck
         |                                v           retryPkts + queuePkts  /     |    |         | retryPkts + queuePkts
         |                          ----------------            <           /      |    |         |           >=
         |--------------------------|   NO_REPLY   |   blockAckThreshold   /       |    |         | blockAckThreshold
            Reset after timeout     ----------------                      /        |    |         |
                                                                         v         |    ----------|
                                                            ---------------        |
                                                            |  INACTIVE   |        |
                                                            ---------------        |
                                        send a MPDU (Normal Ack)   |               |
                                        retryPkts + queuePkts      |               |
                                                  >=               |               |
                                         blockAckThreshold         |----------------
   \endverbatim
 *
 * See also OriginatorBlockAckAgreement::State
 */
class OriginatorBlockAckAgreement : public BlockAckAgreement
{
  /// allow BlockAckManager class access
  friend class BlockAckManager;


public:
  /**
   * Constructor
   *
   * \param recipient MAC address
   * \param tid Traffic ID
   */
  OriginatorBlockAckAgreement (Mac48Address recipient, uint8_t tid);
  ~OriginatorBlockAckAgreement ();
  /**
  * Represents the state for this agreement.
  *
  *  PENDING:
  *    If an agreement is in PENDING state it means that an ADDBARequest frame was sent to
  *    recipient in order to setup the block ack and the originator is waiting for the relative
  *    ADDBAResponse frame.
  *
  *  ESTABLISHED:
  *    The block ack is active and all packets relative to this agreement are transmitted
  *    with ack policy set to block ack.
  *
  *  INACTIVE:
  *    In our implementation, block ack tear-down happens only if an inactivity timeout occurs
  *    so we could have an active block ack but a number of packets that doesn't reach the value of
  *    m_blockAckThreshold (see ns3::BlockAckManager). In these conditions the agreement becomes
  *    INACTIVE until that the number of packets reaches the value of m_blockAckThreshold again.
  *
  *  NO_REPLY
  *    No reply after an ADDBA request. In this state the originator will send the rest of packets
  *    in queue using normal MPDU.
  *
  *  RESET
  *    A transient state to mark the agreement for reinitialzation after failed ADDBA request.
  *    Since it is a temporary state, it is not included in the state diagram above. In this
  *    state the next transmission will be treated as if the BA agreement is not created yet.
  *
  *  REJECTED (not used for now):
  *    The agreement's state becomes REJECTED if an ADDBAResponse frame is received from recipient 
  *    and the Status Code field is set to failure.
  */
  /// State enumeration
  enum State
  {
    PENDING,
    ESTABLISHED,
    INACTIVE,
    NO_REPLY,
    RESET,
    REJECTED
  };
  /**
   * Set the current state.
   *
   * \param state to set
   */
  void SetState (State state);
  /**
   * Check if the current state of this agreement is PENDING.
   *
   * \return true if the current state of this agreement is PENDING,
   *         false otherwise
   */
  bool IsPending (void) const;
  /**
   * Check if the current state of this agreement is ESTABLISHED.
   *
   * \return true if the current state of this agreement is ESTABLISHED,
   *         false otherwise
   */
  bool IsEstablished (void) const;
  /**
   * Check if the current state of this agreement is INACTIVE.
   *
   * \return true if the current state of this agreement is INACTIVE,
   *         false otherwise
   */
  bool IsInactive (void) const;
  /**
   * Check if the current state of this agreement is NO_REPLY.
   *
   * \return true if the current state of this agreement is NO_REPLY,
   *         false otherwise
   */
  bool IsNoReply (void) const;
  /**
   * Check if the current state of this agreement is RESET.
   *
   * \return true if the current state of this agreement is RESET,
   *         false otherwise
   */
  bool IsReset (void) const;
  /**
   * Check if the current state of this agreement is REJECTED.
   *
   * \return true if the current state of this agreement is REJECTED,
   *         false otherwise
   */
  bool IsRejected (void) const;
  /**
   * Notifies a packet's transmission with ack policy Block Ack.
   *
   * \param nextSeqNumber
   */
  void NotifyMpduTransmission (uint16_t nextSeqNumber);
  /**
   * Returns true if all packets for which a block ack was negotiated have been transmitted so
   * a block ack request is needed in order to acknowledge them.
   *
   * \return  true if all packets for which a block ack was negotiated have been transmitted,
   * false otherwise
   */
  bool IsBlockAckRequestNeeded (void) const;
  /// Complete exchange function
  void CompleteExchange (void);


private:
  State m_state; ///< state
  uint16_t m_sentMpdus; ///< sent MPDUs
  bool m_needBlockAckReq; ///< flag whether it needs a Block ACK request
};

} //namespace ns3

#endif /* ORIGINATOR_BLOCK_ACK_AGREEMENT_H */
