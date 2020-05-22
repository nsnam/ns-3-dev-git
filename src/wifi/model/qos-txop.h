/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
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

#ifndef QOS_TXOP_H
#define QOS_TXOP_H

#include "ns3/traced-value.h"
#include "block-ack-manager.h"
#include "txop.h"
#include "qos-utils.h"

class AmpduAggregationTest;
class TwoLevelAggregationTest;
class HeAggregationTest;

namespace ns3 {

class QosBlockedDestinations;
class MgtAddBaResponseHeader;
class MgtDelBaHeader;
class AggregationCapableTransmissionListener;
class WifiTxVector;
class WifiAckPolicySelector;

/**
 * Enumeration for type of station
 */
enum TypeOfStation
{
  STA,
  AP,
  ADHOC_STA,
  MESH,
  HT_STA,
  HT_AP,
  HT_ADHOC_STA,
  OCB
};

/**
 * \brief Handle packet fragmentation and retransmissions for QoS data frames as well
 * as MSDU aggregation (A-MSDU) and block ack sessions, for a given access class.
 * \ingroup wifi
 *
 * This class implements the packet fragmentation and retransmission policy for
 * QoS data frames. It uses the ns3::MacLow and ns3::ChannelAccessManager helper classes
 * to respectively send packets and decide when to send them. Packets are stored
 * in a ns3::WifiMacQueue until they can be sent.
 *
 * This queue contains packets for a particular access class.
 * Possibles access classes are:
 *   - AC_VO : voice, TID = 6,7
 *   - AC_VI : video, TID = 4,5
 *   - AC_BE : best-effort, TID = 0,3
 *   - AC_BK : background, TID = 1,2
 *
 * This class also implements block ack sessions and MSDU aggregation (A-MSDU).
 * If A-MSDU is enabled for that access class, it picks several packets from the
 * queue instead of a single one and sends the aggregated packet to ns3::MacLow.
 *
 * The fragmentation policy currently implemented uses a simple
 * threshold: any packet bigger than this threshold is fragmented
 * in fragments whose size is smaller than the threshold.
 *
 * The retransmission policy is also very simple: every packet is
 * retransmitted until it is either successfully transmitted or
 * it has been retransmitted up until the SSRC or SLRC thresholds.
 *
 * The RTS/CTS policy is similar to the fragmentation policy: when
 * a packet is bigger than a threshold, the RTS/CTS protocol is used.
 */

class QosTxop : public Txop
{
public:
  /// allow AmpduAggregationTest class access
  friend class ::AmpduAggregationTest;
  /// allow TwoLevelAggregationTest class access
  friend class ::TwoLevelAggregationTest;
  /// allow HeAggregationTest class access
  friend class ::HeAggregationTest;

  std::map<Mac48Address, bool> m_aMpduEnabled; //!< list containing flags whether A-MPDU is enabled for a given destination address

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  QosTxop ();
  virtual ~QosTxop ();

  // Overridden from Txop
  bool IsQosTxop (void) const;
  void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> remoteManager);
  virtual bool HasFramesToTransmit (void);
  void NotifyAccessGranted (void);
  void NotifyInternalCollision (void);
  void GotAck (void);
  void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient,
                    double rxSnr, double dataSnr, WifiTxVector dataTxVector);
  void MissedBlockAck (uint8_t nMpdus);
  void MissedAck (void);
  void StartNextPacket (void);
  void EndTxNoAck (void);
  void RestartAccessIfNeeded (void);
  void StartAccessIfNeeded (void);
  Time GetTxopRemaining (void) const;
  bool NeedFragmentation (void) const;
  Ptr<Packet> GetFragmentPacket (WifiMacHeader *hdr);

  /**
   * Set the ack policy selector.
   *
   * \param ackSelector the ack policy selector.
   */
  void SetAckPolicySelector (Ptr<WifiAckPolicySelector> ackSelector);
  /**
   * Return the ack policy selector.
   *
   * \return the ack policy selector.
   */
  Ptr<WifiAckPolicySelector> GetAckPolicySelector (void) const;
  /**
   * Set type of station with the given type.
   *
   * \param type the type of station.
   */
  void SetTypeOfStation (TypeOfStation type);
  /**
   * Return type of station.
   *
   * \return type of station.
   */
  TypeOfStation GetTypeOfStation (void) const;

  /**
   * \param address recipient address of the peer station
   * \param tid traffic ID.
   *
   * \return true if a block ack agreement is established, false otherwise.
   *
   * Checks if a block ack agreement is established with station addressed by
   * <i>recipient</i> for TID <i>tid</i>.
   */
  bool GetBaAgreementEstablished (Mac48Address address, uint8_t tid) const;
  /**
   * \param address recipient address of the peer station
   * \param tid traffic ID.
   *
   * \return the negotiated buffer size during ADDBA handshake.
   *
   * Returns the negotiated buffer size during ADDBA handshake with station addressed by
   * <i>recipient</i> for TID <i>tid</i>.
   */
  uint16_t GetBaBufferSize (Mac48Address address, uint8_t tid) const;
  /**
   * \param address recipient address of the peer station
   * \param tid traffic ID.
   *
   * \return the starting sequence number of the originator transmit window.
   *
   * Returns the current starting sequence number of the transmit window on the
   * originator (WinStartO) of the block ack agreement established with the given
   * recipient for the given TID.
   */
  uint16_t GetBaStartingSequence (Mac48Address address, uint8_t tid) const;
  /**
   * \param address recipient address
   * \param tid traffic ID
   *
   * Prepare a BlockAckRequest to be sent to <i>recipient</i> for Traffic ID
   * <i>tid</i>. The header for the BlockAckRequest is requested to the QosTxop
   * corresponding to the given TID. A block ack agreement with the given recipient
   * for the given TID must have been established by such QosTxop.
   */
  Ptr<const WifiMacQueueItem> PrepareBlockAckRequest (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param bar the BlockAckRequest to schedule
   * \param skipIfNoDataQueued do not send if there is no data queued
   *
   * Request the block ack manager to schedule the transmission of the given
   * BlockAckRequest.
   */
  void ScheduleBar (Ptr<const WifiMacQueueItem> bar, bool skipIfNoDataQueued = false);

  /* Event handlers */
  /**
   * Event handler when a CTS timeout has occurred.
   *
   * \param mpduList the list of MPDUs that were not transmitted
   */
  void NotifyMissedCts (std::list<Ptr<WifiMacQueueItem>> mpduList);

  /**
   * Event handler when an ADDBA response is received.
   *
   * \param respHdr ADDBA response header.
   * \param recipient address of the recipient.
   */
  void GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient);
  /**
   * Event handler when a DELBA frame is received.
   *
   * \param delBaHdr DELBA header.
   * \param recipient address of the recipient.
   */
  void GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient);

  /**
   * Check if BlockAckRequest should be re-transmitted.
   *
   * \return true if BAR should be re-transmitted,
   *         false otherwise.
   */
  bool NeedBarRetransmission (void);

  /**
   * Set the access category of this EDCAF.
   *
   * \param ac access category.
   */
  void SetAccessCategory (AcIndex ac);

  /**
   * \param packet packet to send.
   * \param hdr header of packet to send.
   *
   * Store the packet in the front of the internal queue until it
   * can be sent safely.
   */
  void PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr);

  /**
   * Complete block ack configuration.
   */
  void CompleteConfig (void);

  /**
   * Set threshold for block ack mechanism. If number of packets in the
   * queue reaches the threshold, block ack mechanism is used.
   *
   * \param threshold block ack threshold value.
   */
  void SetBlockAckThreshold (uint8_t threshold);
  /**
   * Return the current threshold for block ack mechanism.
   *
   * \return the current threshold for block ack mechanism.
   */
  uint8_t GetBlockAckThreshold (void) const;

  /**
   * Set the BlockAck inactivity timeout.
   *
   * \param timeout the BlockAck inactivity timeout.
   */
  void SetBlockAckInactivityTimeout (uint16_t timeout);
  /**
   * Sends DELBA frame to cancel a block ack agreement with STA
   * addressed by <i>addr</i> for TID <i>tid</i>.
   *
   * \param addr address of the recipient.
   * \param tid traffic ID.
   * \param byOriginator flag to indicate whether this is set by the originator.
   */
  void SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator);
  /**
   * Stores an MPDU (part of an A-MPDU) in block ack agreement (i.e. the sender is waiting
   * for a BlockAck containing the sequence number of this MPDU).
   *
   * \param mpdu received MPDU.
   */
  void CompleteMpduTx (Ptr<WifiMacQueueItem> mpdu);
  /**
   * Return whether A-MPDU is used to transmit data to a peer station.
   *
   * \param dest address of peer station
   * \returns true if A-MPDU is used by the peer station
   */
  bool GetAmpduExist (Mac48Address dest) const;
  /**
   * Set indication whether A-MPDU is used to transmit data to a peer station.
   *
   * \param dest address of peer station.
   * \param enableAmpdu flag whether A-MPDU is used or not.
   */
  void SetAmpduExist (Mac48Address dest, bool enableAmpdu);
  /**
   * Set the timeout to wait for ADDBA response.
   *
   * \param addBaResponseTimeout the timeout to wait for ADDBA response
   */
  void SetAddBaResponseTimeout (Time addBaResponseTimeout);
  /**
   * Get the timeout for ADDBA response.
   *
   * \returns the timeout to wait for ADDBA response
   */
  Time GetAddBaResponseTimeout (void) const;
  /**
   * Set the timeout for failed BA agreement. During the timeout period,
   * all packets will be transmitted using normal MPDU.
   *
   * \param failedAddBaTimeout the timeout for failed BA agreement
   */
  void SetFailedAddBaTimeout (Time failedAddBaTimeout);
  /**
   * Get the timeout for failed BA agreement.
   *
   * \returns the timeout for failed BA agreement
   */
  Time GetFailedAddBaTimeout (void) const;

  /**
   * Return the next sequence number for the given header.
   *
   * \param hdr Wi-Fi header.
   *
   * \return the next sequence number.
   */
  uint16_t GetNextSequenceNumberFor (const WifiMacHeader *hdr);
  /**
   * Return the next sequence number for the Traffic ID and destination, but do not pick it (i.e. the current sequence number remains unchanged).
   *
   * \param hdr Wi-Fi header.
   *
   * \return the next sequence number.
   */
  uint16_t PeekNextSequenceNumberFor (const WifiMacHeader *hdr);
  /**
   * Peek the next frame to transmit to the given receiver and of the given
   * TID from the block ack manager retransmit queue first and, if not found, from
   * the EDCA queue. If <i>tid</i> is equal to 8 (invalid value) and <i>recipient</i>
   * is the broadcast address, the first available frame is returned.
   * Note that A-MSDU aggregation is never attempted (this is relevant if the
   * frame is peeked from the EDCA queue). If the frame is peeked from the EDCA
   * queue, it is assigned a sequence number peeked from MacTxMiddle.
   *
   * \param tid traffic ID.
   * \param recipient the receiver station address.
   * \returns the peeked frame.
   */
  Ptr<const WifiMacQueueItem> PeekNextFrame (uint8_t tid = 8, Mac48Address recipient = Mac48Address::GetBroadcast ());
  /**
   * Dequeue the frame that has been previously peeked by calling PeekNextFrame.
   * If the peeked frame is a QoS Data frame, it is actually dequeued if it meets
   * the constraint on the maximum A-MPDU size (by assuming that the frame has to
   * be aggregated to an existing A-MPDU of the given size) and its transmission
   * time does not exceed the given PPDU duration limit (if distinct from Time::Min ()).
   * If the peeked frame is a unicast QoS Data frame stored in the EDCA queue,
   * attempt to perform A-MSDU aggregation (while meeting the constraints mentioned
   * above) if <i>aggregate</i> is true and assign a sequence number to the
   * dequeued frame.
   *
   * \param peekedItem the peeked frame.
   * \param txVector the TX vector used to transmit the peeked frame
   * \param aggregate whether to attempt A-MSDU aggregation
   * \param ampduSize the size of the existing A-MPDU in bytes, if any
   * \param ppduDurationLimit the limit on the PPDU duration
   * \returns the dequeued frame.
   */
  Ptr<WifiMacQueueItem> DequeuePeekedFrame (Ptr<const WifiMacQueueItem> peekedItem, WifiTxVector txVector,
                                            bool aggregate = true, uint32_t ampduSize = 0,
                                            Time ppduDurationLimit = Time::Min ());
  /**
   * Compute the MacLow transmission parameters for the given frame. Allowed frames
   * are those handled by a QosTxop (QoS data frames, BlockAckReq frames, ADDBA
   * Request/Response, DELBA Request).
   *
   * \param frame the given frame
   * \return the MacLow transmission parameters.
   */
  MacLowTransmissionParameters GetTransmissionParameters (Ptr<const WifiMacQueueItem> frame) const;

  /**
   * Update the current packet this QosTxop is trying to transmit. This method
   * is typically called by MacLow when it changes (i.e., by performing A-MSDU
   * aggregation) the packet received from this QosTxop.
   *
   * \param mpdu the MPDU that MacLow is forwarding down to the PHY.
   */
  void UpdateCurrentPacket (Ptr<WifiMacQueueItem> mpdu);
  /**
   * The packet we sent was successfully received by the receiver.
   *
   * \param hdr the header of the packet that we successfully sent.
   */
  void BaTxOk (const WifiMacHeader &hdr);
  /**
   * The packet we sent was successfully received by the receiver.
   *
   * \param hdr the header of the packet that we failed to sent.
   */
  void BaTxFailed (const WifiMacHeader &hdr);

  /**
   * This functions are used only to correctly set source address in A-MSDU subframes.
   * If aggregating STA is a non-AP STA (in an infrastructure network):
   *   SA = Address2
   * If aggregating STA is an AP
   *   SA = Address3
   *
   * \param hdr Wi-Fi header
   * \return Mac48Address
   */
  Mac48Address MapSrcAddressForAggregation (const WifiMacHeader &hdr);
  /**
   * This functions are used only to correctly set destination address in A-MSDU subframes.
   * If aggregating STA is a non-AP STA (in an infrastructure network):
   *   DA = Address3
   * If aggregating STA is an AP
   *   DA = Address1
   *
   * \param hdr Wi-Fi header
   * \return Mac48Address
   */
  Mac48Address MapDestAddressForAggregation (const WifiMacHeader &hdr);


private:
  /// allow AggregationCapableTransmissionListener class access
  friend class AggregationCapableTransmissionListener;

  // Overridden from Txop
  void DoDispose (void);
  void DoInitialize (void);
  void TerminateTxop (void);
  uint32_t GetNextFragmentSize (void) const;
  uint32_t GetFragmentSize (void) const;
  uint32_t GetFragmentOffset (void) const;
  bool IsLastFragment (void) const;

  /**
   * If number of packets in the queue reaches m_blockAckThreshold value, an ADDBA Request frame
   * is sent to destination in order to setup a block ack.
   *
   * \return true if we tried to set up block ack, false otherwise.
   */
  bool SetupBlockAckIfNeeded (void);
  /**
   * Sends an ADDBA Request to establish a block ack agreement with STA
   * addressed by <i>recipient</i> for TID <i>tid</i>.
   *
   * \param recipient address of the recipient.
   * \param tid traffic ID.
   * \param startSeq starting sequence.
   * \param timeout timeout value.
   * \param immediateBAck flag to indicate whether immediate BlockAck is used.
   */
  void SendAddBaRequest (Mac48Address recipient, uint8_t tid, uint16_t startSeq,
                         uint16_t timeout, bool immediateBAck);
  /**
   * Check if the given MPDU is to be considered old according to the current
   * starting sequence number of the transmit window, provided that a block ack
   * agreement has been established with the recipient for the given TID.
   *
   * \param mpdu the given MPDU
   * \return true if the MPDU is to be considered old, false otherwise
   */
  bool IsQosOldPacket (Ptr<const WifiMacQueueItem> mpdu);

  /**
   * Check if the current packet is fragmented because of an exceeded TXOP duration.
   *
   * \return true if the current packet is fragmented because of an exceeded TXOP duration,
   *         false otherwise
   */
  bool IsTxopFragmentation (void) const;
  /**
   * Calculate the size of the current TXOP fragment.
   *
   * \return the size of the current TXOP fragment in bytes
   */
  uint32_t GetTxopFragmentSize (void) const;
  /**
   * Calculate the number of TXOP fragments needed for the transmission of the current packet.
   *
   * \return the number of TXOP fragments needed for the transmission of the current packet
   */
  uint32_t GetNTxopFragment (void) const;
  /**
   * Calculate the size of the next TXOP fragment.
   *
   * \param fragmentNumber number of the next fragment
   * \returns the next TXOP fragment size in bytes
   */
  uint32_t GetNextTxopFragmentSize (uint32_t fragmentNumber) const;
  /**
   * Calculate the offset for the fragment.
   *
   * \param fragmentNumber number of the fragment
   * \returns the TXOP fragment offset in bytes
   */
  uint32_t GetTxopFragmentOffset (uint32_t fragmentNumber) const;
  /**
   * Callback when ADDBA response is not received after timeout.
   *
   * \param recipient MAC address of recipient
   * \param tid traffic ID
   */
  void AddBaResponseTimeout (Mac48Address recipient, uint8_t tid);
  /**
   * Reset BA agreement after BA negotiation failed.
   *
   * \param recipient MAC address of recipient
   * \param tid traffic ID
   */
  void ResetBa (Mac48Address recipient, uint8_t tid);

  AcIndex m_ac;                                         //!< the access category
  TypeOfStation m_typeOfStation;                        //!< the type of station
  Ptr<WifiAckPolicySelector> m_ackPolicySelector;       //!< the ack policy selector
  Ptr<QosBlockedDestinations> m_qosBlockedDestinations; //!< the QoS blocked destinations
  Ptr<BlockAckManager> m_baManager;                     //!< the block ack manager
  uint8_t m_blockAckThreshold;                          /**< the block ack threshold (use BA mechanism if number of packets in queue reaches
                                                             this value. If this value is 0, block ack is never used. When A-MPDU is enabled,
                                                             block ack mechanism is used regardless of this value) */
  BlockAckType m_blockAckType;                          //!< the BlockAck type
  Time m_currentPacketTimestamp;                        //!< the current packet timestamp
  uint16_t m_blockAckInactivityTimeout;                 //!< the BlockAck inactivity timeout value (in TUs, i.e. blocks of 1024 microseconds)
  Time m_startTxop;                                     //!< the start TXOP time
  bool m_isAccessRequestedForRts;                       //!< flag whether access is requested to transmit a RTS frame
  bool m_currentIsFragmented;                           //!< flag whether current packet is fragmented
  Time m_addBaResponseTimeout;                          //!< timeout for ADDBA response
  Time m_failedAddBaTimeout;                            //!< timeout after failed BA agreement
  bool m_useExplicitBarAfterMissedBlockAck;             //!< flag whether explicit BlockAckRequest should be sent upon missed BlockAck Response

  TracedCallback<Time, Time> m_txopTrace; //!< TXOP trace callback
};

} //namespace ns3

#endif /* QOS_TXOP_H */
