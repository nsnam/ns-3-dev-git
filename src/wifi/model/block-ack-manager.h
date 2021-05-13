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
 */

#ifndef BLOCK_ACK_MANAGER_H
#define BLOCK_ACK_MANAGER_H

#include <map>
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "wifi-mac-header.h"
#include "originator-block-ack-agreement.h"
#include "block-ack-type.h"
#include "wifi-mac-queue-item.h"
#include "wifi-tx-vector.h"

namespace ns3 {

class WifiRemoteStationManager;
class MgtAddBaResponseHeader;
class MgtAddBaRequestHeader;
class CtrlBAckResponseHeader;
class CtrlBAckRequestHeader;
class WifiMacQueue;
class WifiMode;
class Packet;

/**
 * \ingroup wifi
 * \brief BlockAckRequest frame information
 *
 */
struct Bar
{
  Bar ();
  /**
   * Store a BlockAckRequest along with the corresponding TID or a MU-BAR Trigger Frame.
   *
   * \param bar the BAR
   * \param tid the Traffic ID
   * \param skipIfNoDataQueued true to hold this BAR if there is no data queued
   */
  Bar (Ptr<const WifiMacQueueItem> bar, uint8_t tid, bool skipIfNoDataQueued = false);
  Ptr<const WifiMacQueueItem> bar;  ///< BlockAckRequest or MU-BAR Trigger Frame
  uint8_t tid;                      ///< TID (unused if MU-BAR)
  bool skipIfNoDataQueued;          ///< do not send if there is no data queued (unused if MU-BAR)
};


/**
 * \brief Manages all block ack agreements for an originator station.
 * \ingroup wifi
 */
class BlockAckManager : public Object
{
private:
  /// type conversion operator
  BlockAckManager (const BlockAckManager&);
  /**
   * assignment operator
   * \param block BlockAckManager to assign
   * \returns the assigned BlockAckManager
   */
  BlockAckManager& operator= (const BlockAckManager& block);


public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  BlockAckManager ();
  ~BlockAckManager ();

  /**
   * Set up WifiRemoteStationManager associated with this BlockAckManager.
   *
   * \param manager WifiRemoteStationManager associated with this BlockAckManager
   */
  void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> manager);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID.
   *
   * \return true  if a block ack agreement exists, false otherwise
   *
   * Checks if a block ack agreement exists with station addressed by
   * <i>recipient</i> for TID <i>tid</i>.
   */
  bool ExistsAgreement (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID.
   * \param state The state for block ack agreement
   *
   * \return true if a block ack agreement exists, false otherwise
   *
   * Checks if a block ack agreement with a state equals to <i>state</i> exists with
   * station addressed by <i>recipient</i> for TID <i>tid</i>.
   */
  bool ExistsAgreementInState (Mac48Address recipient, uint8_t tid,
                               OriginatorBlockAckAgreement::State state) const;
  /**
   * \param reqHdr Relative Add block ack request (action frame).
   * \param recipient Address of peer station involved in block ack mechanism.
   *
   * Creates a new block ack agreement in pending state. When a ADDBA response
   * with a successful status code is received, the relative agreement becomes established.
   */
  void CreateAgreement (const MgtAddBaRequestHeader *reqHdr, Mac48Address recipient);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid traffic ID of transmitted packet.
   *
   * Invoked when a recipient reject a block ack agreement or when a DELBA frame
   * is Received/Transmitted.
   */
  void DestroyAgreement (Mac48Address recipient, uint8_t tid);
  /**
   * \param respHdr Relative Add block ack response (action frame).
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param startingSeq the updated starting sequence number
   *
   * Invoked upon receipt of a ADDBA response frame from <i>recipient</i>.
   */
  void UpdateAgreement (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient,
                        uint16_t startingSeq);
  /**
   * \param mpdu MPDU to store.
   *
   * Stores <i>mpdu</i> for a possible future retransmission. Retransmission occurs
   * if the packet, in a BlockAck frame, is indicated by recipient as not received.
   */
  void StorePacket (Ptr<WifiMacQueueItem> mpdu);
  /**
   * Returns the next BlockAckRequest or MU-BAR Trigger Frame to send, if any.
   * If the given recipient is not the broadcast address and the given TID is less
   * than 8, then only return a BlockAckRequest, if any, addressed to that recipient
   * and for the given TID.
   *
   * \param remove true if the BAR has to be removed from the queue
   * \param tid the TID
   * \param recipient the recipient of the BAR
   *
   * \return the next BAR to be sent, if any
   */
  Ptr<const WifiMacQueueItem> GetBar (bool remove = true, uint8_t tid = 8,
                                      Mac48Address recipient = Mac48Address::GetBroadcast ());
  /**
   * Returns true if there are packets that need of retransmission or at least a
   * BAR is scheduled. Returns false otherwise.
   *
   * \return true if there are packets that need of retransmission or at least a BAR is scheduled,
   *         false otherwise
   */
  bool HasPackets (void);
  /**
   * Invoked upon receipt of an Ack frame after the transmission of a QoS data frame
   * sent under an established block ack agreement. Remove the acknowledged frame
   * from the outstanding packets and update the starting sequence number of the
   * transmit window, if needed.
   *
   * \param mpdu The acknowledged MPDU.
   */
  void NotifyGotAck (Ptr<const WifiMacQueueItem> mpdu);
  /**
   * Invoked upon missed reception of an Ack frame after the transmission of a
   * QoS data frame sent under an established block ack agreement. Remove the
   * acknowledged frame from the outstanding packets and insert it in the
   * retransmission queue.
   *
   * \param mpdu The unacknowledged MPDU.
   */
  void NotifyMissedAck (Ptr<WifiMacQueueItem> mpdu);
  /**
   * \param blockAck The received BlockAck frame.
   * \param recipient Sender of BlockAck frame.
   * \param tids the set of TIDs the acknowledged MPDUs belong to
   * \param rxSnr received SNR of the BlockAck frame itself
   * \param dataSnr data SNR reported by remote station
   * \param dataTxVector the TXVECTOR used to send the Data
   * \param index the index of the Per AID TID Info subfield, in case of Multi-STA
   *              Block Ack, or 0, otherwise
   *
   * Invoked upon receipt of a BlockAck frame. Typically, this function, is called
   * by ns3::QosTxop object. Performs a check on which MPDUs, previously sent
   * with Ack Policy set to Block Ack, were correctly received by the recipient.
   * An acknowledged MPDU is removed from the buffer, retransmitted otherwise.
   * Note that <i>tids</i> is only used if <i>blockAck</i> is a Multi-STA Block Ack
   * using All-ack context.
   */
  void NotifyGotBlockAck (const CtrlBAckResponseHeader& blockAck, Mac48Address recipient,
                          const std::set<uint8_t>& tids, double rxSnr, double dataSnr,
                          const WifiTxVector& dataTxVector, size_t index = 0);
  /**
   * \param recipient Sender of the expected BlockAck frame.
   * \param tid Traffic ID.
   *
   * Invoked upon missed reception of a block ack frame. Typically, this function, is called
   * by ns3::QosTxop object. Performs a check on which MPDUs, previously sent
   * with ack policy set to Block Ack, should be placed in the retransmission queue.
   */
  void NotifyMissedBlockAck (Mac48Address recipient, uint8_t tid);
  /**
   * \param recipient outstanding frames' receiver.
   * \param tid Traffic ID.
   *
   * Discard all the outstanding MPDUs destined to the given receiver and belonging
   * to the given TID. Typically, this function is called by ns3::QosTxop object
   * when it gives up retransmitting either a BlockAckRequest or the Data frames.
   */
  void DiscardOutstandingMpdus (Mac48Address recipient, uint8_t tid);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID.
   *
   * \return the number of packets buffered for a specified agreement
   *
   * Returns the number of packets buffered for a specified agreement. This methods doesn't return
   * the number of buffered MPDUs but the number of buffered MSDUs.
   */
  uint32_t GetNBufferedPackets (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   * \param startingSeq starting sequence field
   *
   * Puts corresponding agreement in established state and updates number of packets
   * and starting sequence field. Invoked typically after a block ack refresh.
   */
  void NotifyAgreementEstablished (Mac48Address recipient, uint8_t tid, uint16_t startingSeq);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   *
   * Marks an agreement as rejected. This happens if <i>recipient</i> station reject block ack setup
   * by an ADDBA Response frame with a failure status code. For now we assume that every QoS station accepts
   * a block ack setup.
   */
  void NotifyAgreementRejected (Mac48Address recipient, uint8_t tid);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   *
   * Marks an agreement after not receiving response to ADDBA request. During this state
   * any packets in queue will be transmitted using normal MPDU. This also unblock
   * recipient address.
   */
  void NotifyAgreementNoReply (Mac48Address recipient, uint8_t tid);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   *
   * Set BA agreement to a transitory state to reset it after not receiving response to ADDBA request.
   */
  void NotifyAgreementReset (Mac48Address recipient, uint8_t tid);
  /**
   * \param nPackets Minimum number of packets for use of block ack.
   *
   * Upon receipt of a BlockAck frame, if total number of packets (packets in WifiMacQueue
   * and buffered packets) is greater of <i>nPackets</i>, they are transmitted using block ack mechanism.
   */
  void SetBlockAckThreshold (uint8_t nPackets);
  /**
   * \return the retransmit queue.
   *
   * Return the retransmit queue.
   */
  Ptr<WifiMacQueue> GetRetransmitQueue (void);

  /**
   * \param queue The WifiMacQueue object.
   */
  void SetQueue (const Ptr<WifiMacQueue> queue);

  /**
   * Set block ack inactivity callback
   * \param callback the block ack inactivity callback function
   */
  void SetBlockAckInactivityCallback (Callback<void, Mac48Address, uint8_t, bool> callback);
  /**
   * Set block destination callback
   * \param callback the block destination callback
   */
  void SetBlockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback);
  /**
   * Set unblock destination callback
   * \param callback the unblock destination callback
   */
  void SetUnblockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback);

  /**
   * \param recipient the destination address
   * \param tid the Traffic ID
   * \param startingSeq the starting sequence number
   *
   * \return true if there are packets in the queue that could be sent under block ack,
   *         false otherwise
   *
   * Checks if there are in the queue other packets that could be send under block ack.
   * If yes adds these packets in current block ack exchange.
   * However, number of packets exchanged in the current block ack, will not exceed
   * the value of BufferSize in the corresponding OriginatorBlockAckAgreement object.
   */
  bool SwitchToBlockAckIfNeeded (Mac48Address recipient, uint8_t tid, uint16_t startingSeq);
  /**
   * This function returns true if a block ack agreement is established with the
   * given recipient for the given TID and there is at least an outstanding MPDU
   * for such agreement whose lifetime is not expired.
   *
   * \param tid Traffic ID
   * \param recipient MAC address of the recipient
   *
   * \returns true if BAR retransmission needed
   */
  bool NeedBarRetransmission (uint8_t tid, Mac48Address recipient);
  /**
   * This function returns the buffer size negotiated with the recipient.
   *
   * \param tid Traffic ID
   * \param recipient MAC address of the recipient
   *
   * \returns the buffer size negotiated with the recipient
   */
  uint16_t GetRecipientBufferSize (Mac48Address recipient, uint8_t tid) const;
  /**
   * This function returns the type of Block Acks sent to the recipient.
   *
   * \param recipient MAC address of recipient
   * \param tid Traffic ID
   *
   * \returns the type of Block Acks sent to the recipient
   */
  BlockAckReqType GetBlockAckReqType (Mac48Address recipient, uint8_t tid) const;
  /**
   * This function returns the type of Block Acks sent by the recipient.
   *
   * \param recipient MAC address
   * \param tid Traffic ID
   *
   * \returns the type of Block Acks sent by the recipient
   */
  BlockAckType GetBlockAckType (Mac48Address recipient, uint8_t tid) const;
  /**
   * This function returns the starting sequence number of the transmit window.
   *
   * \param tid Traffic ID
   * \param recipient MAC address of the recipient
   *
   * \returns the starting sequence number of the transmit window (WinStartO)
   */
  uint16_t GetOriginatorStartingSequence (Mac48Address recipient, uint8_t tid) const;

  /**
   * typedef for a callback to invoke when an MPDU is successfully ack'ed.
   */
  typedef Callback <void, Ptr<const WifiMacQueueItem>> TxOk;
  /**
   * typedef for a callback to invoke when an MPDU is negatively ack'ed.
   */
  typedef Callback <void, Ptr<const WifiMacQueueItem>> TxFailed;
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxOk callback);
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed unsuccessfully.
   */
  void SetTxFailedCallback (TxFailed callback);

  /**
   * TracedCallback signature for state changes.
   *
   * \param [in] now Time when the \pname{state} changed.
   * \param [in] recipient MAC address of the recipient.
   * \param [in] tid the TID.
   * \param [in] state The state.
   */
  typedef void (* AgreementStateTracedCallback)(Time now, Mac48Address recipient, uint8_t tid, OriginatorBlockAckAgreement::State state);

  /**
   * \param mpdu the discarded frame
   *
   * Notify the block ack manager that an MPDU has been discarded, e.g., because
   * the MSDU lifetime expired. If there is an established block ack agreement,
   * make the transmit window advance beyond the discarded frame. This also
   * involves (i) the removal of frames that consequently become old from the
   * retransmit queue and from the queue of the block ack agreement, and (ii) the
   * scheduling of a BlockAckRequest.
   */
  void NotifyDiscardedMpdu (Ptr<const WifiMacQueueItem> mpdu);

  /**
   * \param recipient the recipient
   * \param tid the TID
   * \return the BlockAckRequest header for the established BA agreement
   *
   * Get the BlockAckRequest header for the established BA agreement
   * (<i>recipient</i>,<i>tid</i>).
   */
  CtrlBAckRequestHeader GetBlockAckReqHeader (Mac48Address recipient, uint8_t tid) const;

  /**
   * \param bar the BlockAckRequest to enqueue
   * \param skipIfNoDataQueued do not send if there is no data queued
   *
   * Enqueue the given BlockAckRequest into the queue storing the next BAR
   * frames to transmit. If a BAR for the same recipient and TID is already present
   * in the queue, it is replaced by the new one. If the given BAR is retransmitted,
   * it is placed at the head of the queue, otherwise at the tail.
   */
  void ScheduleBar (Ptr<const WifiMacQueueItem> bar, bool skipIfNoDataQueued = false);

private:
  /**
   * Inactivity timeout function
   * \param recipient the recipient MAC address
   * \param tid Traffic ID
   */
  void InactivityTimeout (Mac48Address recipient, uint8_t tid);

  /**
   * Remove packets from the retransmit queue and from the queue of outstanding
   * packets that become old after setting the starting sequence number for the
   * agreement with recipient equal to <i>recipient</i> and TID equal to <i>tid</i>
   * to the given <i>startingSeq</i>.
   *
   * \param recipient the recipient MAC address
   * \param tid Traffic ID
   * \param startingSeq the new starting sequence number
   */
  void RemoveOldPackets (Mac48Address recipient, uint8_t tid, uint16_t startingSeq);

  /**
   * typedef for a list of WifiMacQueueItem.
   */
  typedef std::list<Ptr<WifiMacQueueItem>> PacketQueue;
  /**
   * typedef for an iterator for PacketQueue.
   */
  typedef std::list<Ptr<WifiMacQueueItem>>::iterator PacketQueueI;
  /**
   * typedef for a const iterator for PacketQueue.
   */
  typedef std::list<Ptr<WifiMacQueueItem>>::const_iterator PacketQueueCI;
  /**
   * typedef for a map between MAC address and block ack agreement.
   */
  typedef std::map<std::pair<Mac48Address, uint8_t>,
                   std::pair<OriginatorBlockAckAgreement, PacketQueue> > Agreements;
  /**
   * typedef for an iterator for Agreements.
   */
  typedef std::map<std::pair<Mac48Address, uint8_t>,
                   std::pair<OriginatorBlockAckAgreement, PacketQueue> >::iterator AgreementsI;
  /**
   * typedef for a const iterator for Agreements.
   */
  typedef std::map<std::pair<Mac48Address, uint8_t>,
                   std::pair<OriginatorBlockAckAgreement, PacketQueue> >::const_iterator AgreementsCI;

  /**
   * \param mpdu the packet to insert in the retransmission queue
   *
   * Insert <i>mpdu</i> in retransmission queue.
   * This method ensures packets are retransmitted in the correct order.
   */
  void InsertInRetryQueue (Ptr<WifiMacQueueItem> mpdu);

  /**
   * Remove an item from retransmission queue.
   * This method should be called when packets are acknowledged.
   *
   * \param address recipient MAC address of the packet to be removed
   * \param tid Traffic ID of the packet to be removed
   * \param seq sequence number of the packet to be removed
   */
  void RemoveFromRetryQueue (Mac48Address address, uint8_t tid, uint16_t seq);

  /**
   * Remove a range of items from retransmission queue.
   * This method should be called when packets are acknowledged.
   *
   * \param address recipient MAC address of the packet to be removed
   * \param tid Traffic ID of the packet to be removed
   * \param startSeq sequence number of the first packet to be removed
   * \param endSeq sequence number of the last packet to be removed
   */
  void RemoveFromRetryQueue (Mac48Address address, uint8_t tid, uint16_t startSeq, uint16_t endSeq);

  /**
   * This data structure contains, for each block ack agreement (recipient, TID), a set of packets
   * for which an ack by block ack is requested.
   * Every packet or fragment indicated as correctly received in BlockAck frame is
   * erased from this data structure. Pushed back in retransmission queue otherwise.
   */
  Agreements m_agreements;

  /**
   * This list contains all iterators to stored packets that need to be retransmitted.
   * A packet needs retransmission if it's indicated as not correctly received in a BlockAck
   * frame.
   */
  Ptr<WifiMacQueue> m_retryPackets;
  std::list<Bar> m_bars; ///< list of BARs

  uint8_t m_blockAckThreshold; ///< block ack threshold
  Ptr<WifiMacQueue> m_queue;   ///< queue
  Callback<void, Mac48Address, uint8_t, bool> m_blockAckInactivityTimeout; ///< BlockAck inactivity timeout callback
  Callback<void, Mac48Address, uint8_t> m_blockPackets;   ///< block packets callback
  Callback<void, Mac48Address, uint8_t> m_unblockPackets; ///< unblock packets callback
  TxOk m_txOkCallback;                                    ///< transmit OK callback
  TxFailed m_txFailedCallback;                            ///< transmit failed callback
  Ptr<WifiRemoteStationManager> m_stationManager;         ///< the station manager

  /**
   * The trace source fired when a state transition occurred.
   */
  TracedCallback<Time, Mac48Address, uint8_t, OriginatorBlockAckAgreement::State> m_agreementState;
};

} //namespace ns3

#endif /* BLOCK_ACK_MANAGER_H */
