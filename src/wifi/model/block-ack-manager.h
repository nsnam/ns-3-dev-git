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

#include "wifi-mac-header.h"
#include "originator-block-ack-agreement.h"
#include "ctrl-headers.h"
#include "qos-utils.h"
#include "wifi-remote-station-manager.h"
#include <map>

namespace ns3 {

class MgtAddBaResponseHeader;
class MgtAddBaRequestHeader;
class MacTxMiddle;
class WifiMacQueue;

/**
 * \ingroup wifi
 * \brief Block Ack Request
 *
 */
struct Bar
{
  Bar ();
  /**
   * Construct Block ACK request for a given packet,
   * receiver address, Traffic ID, and ACK policy.
   *
   * \param packet
   * \param recipient
   * \param tid
   * \param immediate
   */
  Bar (Ptr<const Packet> packet,
       Mac48Address recipient,
       uint8_t tid,
       bool immediate);
  Ptr<const Packet> bar; ///< block ack request
  Mac48Address recipient; ///< recipient
  uint8_t tid; ///< TID
  bool immediate; ///< immediate
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
  /// assignment operator
  BlockAckManager& operator= (const BlockAckManager&);


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
   * <i>recipient</i> for tid <i>tid</i>.
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
   * station addressed by <i>recipient</i> for tid <i>tid</i>.
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
   * \param tid Tid Traffic id of transmitted packet.
   *
   * Invoked when a recipient reject a block ack agreement or when a Delba frame
   * is Received/Transmitted.
   */
  void DestroyAgreement (Mac48Address recipient, uint8_t tid);
  /**
   * \param respHdr Relative Add block ack response (action frame).
   * \param recipient Address of peer station involved in block ack mechanism.
   *
   * Invoked upon receipt of a ADDBA response frame from <i>recipient</i>.
   */
  void UpdateAgreement (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient);
  /**
   * \param packet Packet to store.
   * \param hdr 802.11 header for packet.
   * \param tStamp time stamp for packet
   *
   * Stores <i>packet</i> for a possible future retransmission. Retransmission occurs
   * if the packet, in a block ack frame, is indicated by recipient as not received.
   */
  void StorePacket (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time tStamp);
  /**
   * \param hdr 802.11 header of returned packet (if exists).
   *
   * \return the packet
   *
   * This methods returns a packet (if exists) indicated as not received in
   * corresponding block ack bitmap.
   */
  Ptr<const Packet> GetNextPacket (WifiMacHeader &hdr);
  /**
   * \param hdr 802.11 header of returned packet (if exists).
   *
   * \return the packet
   *
   * This methods returns a packet (if exists) indicated as not received in
   * corresponding block ack bitmap. This method doesn't remove the packet from this queue.
   */
  Ptr<const Packet> PeekNextPacket (WifiMacHeader &hdr);
  /**
   * Returns true if the BAR is scheduled. Returns false otherwise.
   *
   * \param bar
   *
   * \return true if a BAR is scheduled, false otherwise
   */
  bool HasBar (Bar &bar);
  /**
   * Returns true if there are packets that need of retransmission or at least a
   * BAR is scheduled. Returns false otherwise.
   *
   * \return true if there are packets that need of retransmission or at least a BAR is scheduled,
   *         false otherwise
   */
  bool HasPackets (void) const;
  /**
   * \param blockAck The received block ack frame.
   * \param recipient Sender of block ack frame.
   * \param rxSnr received SNR of the block ack frame itself
   * \param txMode mode of block ack frame.
   * \param dataSnr data SNR reported by remote station
   *
   * Invoked upon receipt of a block ack frame. Typically, this function, is called
   * by ns3::EdcaTxopN object. Performs a check on which MPDUs, previously sent
   * with ack policy set to Block Ack, were correctly received by the recipient.
   * An acknowledged MPDU is removed from the buffer, retransmitted otherwise.
   */
  void NotifyGotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID.
   *
   * \return the number of packets buffered for a specified agreement
   *
   * Returns number of packets buffered for a specified agreement. This methods doesn't return
   * number of buffered MPDUs but number of buffered MSDUs.
   */
  uint32_t GetNBufferedPackets (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID.
   *
   * \return the number of packets for a specific agreement that need retransmission
   *
   * Returns number of packets for a specific agreement that need retransmission.
   * This method doesn't return number of MPDUs that need retransmission but number of MSDUs.
   */
  uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const;
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
   * Marks an agreement as unsuccessful. This happens if <i>recipient</i> station reject block ack setup
   * by an ADDBA Response frame with a failure status code. For now we assume that every QoS station accepts
   * a block ack setup.
   */
  void NotifyAgreementUnsuccessful (Mac48Address recipient, uint8_t tid);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   * \param nextSeqNumber Sequence number of the next packet that would be trasmitted by EdcaTxopN.
   * \param policy ack policy of the transmitted packet.
   *
   * This method is typically invoked by ns3::EdcaTxopN object every time that a MPDU
   * with ack policy subfield in Qos Control field set to Block Ack is transmitted.
   * The <i>nextSeqNumber</i> parameter is used to block transmission of packets that are out of bitmap.
   */
  void NotifyMpduTransmission (Mac48Address recipient, uint8_t tid, uint16_t nextSeqNumber, WifiMacHeader::QosAckPolicy policy);
  /**
   * \param recipient Address of peer station involved in block ack mechanism.
   * \param tid Traffic ID of transmitted packet.
   *
   * This method to set the number of packets waitin for blockAck = 0 since the receiver will send the blockAck right away
   */
  void CompleteAmpduExchange (Mac48Address recipient, uint8_t tid);
  /**
   * \param nPackets Minimum number of packets for use of block ack.
   *
   * Upon receipt of a block ack frame, if total number of packets (packets in WifiMacQueue
   * and buffered packets) is greater of <i>nPackets</i>, they are transmitted using block ack mechanism.
   */
  void SetBlockAckThreshold (uint8_t nPackets);

  /**
   * \param queue The WifiMacQueue object.
   */
  void SetQueue (const Ptr<WifiMacQueue> queue);
  /**
   * Set the MacTxMiddle
   * \param txMiddle the MacTxMiddle
   */
  void SetTxMiddle (const Ptr<MacTxMiddle> txMiddle);

  /**
   * \param bAckType Type of block ack
   *
   * See ctrl-headers.h for more details.
   */
  void SetBlockAckType (BlockAckType bAckType);
  /**
   * \param recipient Address of station involved in block ack mechanism.
   * \param tid Traffic ID.
   *
   * This method is invoked by EdcaTxopN object upon receipt of a DELBA frame
   * from recipient. The relative block ack agreement is destroyed.
   */
  void TearDownBlockAck (Mac48Address recipient, uint8_t tid);
  /**
   * \param sequenceNumber Sequence number of the packet which fragment is
   * part of.
   * \return true if another fragment with the given sequence number is scheduled
   * for retransmission.
   *
   * Returns true if another fragment with sequence number <i>sequenceNumber</i> is scheduled
   * for retransmission.
   */
  bool HasOtherFragments (uint16_t sequenceNumber) const;
  /**
   * \return the size of the next packet that needs retransmission
   *
   * Returns size of the next packet that needs retransmission.
   */
  uint32_t GetNextPacketSize (void) const;
  /**
   * \param maxDelay Max delay for a buffered packet.
   *
   * This method is always called by ns3::WifiMacQueue object and sets max delay equals
   * to ns3:WifiMacQueue delay value.
   */
  void SetMaxPacketDelay (Time maxDelay);

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
   * \param recipient
   * \param tid
   * \param startingSeq
   *
   * \return true if there are packets in the queue that could be sent under block ACK,
   *         false otherwise
   *
   * Checks if there are in the queue other packets that could be send under block ack.
   * If yes adds these packets in current block ack exchange.
   * However, number of packets exchanged in the current block ack, will not exceed
   * the value of BufferSize in the corresponding OriginatorBlockAckAgreement object.
   */
  bool SwitchToBlockAckIfNeeded (Mac48Address recipient, uint8_t tid, uint16_t startingSeq);
  /**
   * \param recipient
   * \param tid
   *
   * \return the sequence number of the next retry packet for a specific agreement
   *
   * Returns the sequence number of the next retry packet for a specific agreement.
   * If there are no packets that need retransmission for the specified agreement or
   * the agreement doesn't exist the function returns 4096;
   */
  uint16_t GetSeqNumOfNextRetryPacket (Mac48Address recipient, uint8_t tid) const;
  /**
   * Checks if the packet already exists in the retransmit queue or not if it does then it doesn't add it again
   */
  bool AlreadyExists (uint16_t currentSeq, Mac48Address recipient, uint8_t tid) const;
  /**
   * Remove a packet after you peek in the queue and get it
   */
  bool RemovePacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber);
  /**
   * Peek in retransmit queue and get the next packet having address indicated
   * by <i>type</i> equals to <i>addr</i>, and tid equals to <i>tid</i>.
   * This method doesn't remove the packet from this queue.
   *
   * \param hdr wifi mac header
   * \param recipient mac address
   * \param tid Traffic ID
   * \param timestamp timestamp
   *
   * \returns Ptr<const Packet>
   */
  Ptr<const Packet> PeekNextPacketByTidAndAddress (WifiMacHeader &hdr, Mac48Address recipient, uint8_t tid, Time *timestamp);
  /**
   * This function returns true if the lifetime of the packets a BAR refers to didn't expire yet else it returns false.
   * If it return false then the BAR will be discarded (i.e. will not be re-transmitted)
   *
   * \param tid Traffic ID
   * \param seqNumber sequence number
   * \param recipient mac address
   *
   * \returns true if BAR retransmission needed
   */
  bool NeedBarRetransmission (uint8_t tid, uint16_t seqNumber, Mac48Address recipient);

  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, const WifiMacHeader&> TxOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was failed.
   */
  typedef Callback <void, const WifiMacHeader&> TxFailed;
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


private:
  /**
   * \param recipient
   * \param tid
   *
   * \return a packet
   *
   * Checks if all packets, for which a block ack agreement was established or refreshed,
   * have been transmitted. If yes, adds a pair in m_bAckReqs to indicate that
   * at next channel access a block ack request (for established agreement
   * <i>recipient</i>,<i>tid</i>) is needed.
   */
  Ptr<Packet> ScheduleBlockAckReqIfNeeded (Mac48Address recipient, uint8_t tid);

  /**
   * This method removes packets whose lifetime was exceeded.
   */
  void CleanupBuffers (void);
  /**
   * Inactivity timeout function
   * \param recipient the recipient MAC address
   * \param tid Traffic ID
   */
  void InactivityTimeout (Mac48Address recipient, uint8_t tid);

  struct Item;
  /**
   * typedef for a list of Item struct.
   */
  typedef std::list<Item> PacketQueue;
  /**
   * typedef for an iterator for PacketQueue.
   */
  typedef std::list<Item>::iterator PacketQueueI;
  /**
   * typedef for a const iterator for PacketQueue.
   */
  typedef std::list<Item>::const_iterator PacketQueueCI;
  /**
   * typedef for a map between MAC address and block ACK agreement.
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
   * A struct for packet, Wifi header, and timestamp.
   * Used in queue by block ACK manager.
   */
  struct Item
  {
    Item ();
    /**
     * Constructor
     *
     * \param packet packet
     * \param hdr packet header
     * \param tStamp timestamp
     */
    Item (Ptr<const Packet> packet,
          const WifiMacHeader &hdr,
          Time tStamp);
    Ptr<const Packet> packet; ///< packet
    WifiMacHeader hdr; ///< header
    Time timestamp; ///< timestamp
  };
  /**
   * \param item
   *
   * Insert item in retransmission queue.
   * This method ensures packets are retransmitted in the correct order.
   */
  void InsertInRetryQueue (PacketQueueI item);

  /**
   * Remove items from retransmission queue.
   * This method should be called when packets are acknowledged.
   *
   * \param address recipient mac address of the packet to be removed
   * \param tid Traffic ID of the packet to be removed
   * \param seq sequence number of the packet to be removed
   */
  void RemoveFromRetryQueue (Mac48Address address, uint8_t tid, uint16_t seq);

  /**
   * This data structure contains, for each block ack agreement (recipient, tid), a set of packets
   * for which an ack by block ack is requested.
   * Every packet or fragment indicated as correctly received in block ack frame is
   * erased from this data structure. Pushed back in retransmission queue otherwise.
   */
  Agreements m_agreements;

  /**
   * This list contains all iterators to stored packets that need to be retransmitted.
   * A packet needs retransmission if it's indicated as not correctly received in a block ack
   * frame.
   */
  std::list<PacketQueueI> m_retryPackets;
  std::list<Bar> m_bars; ///< list of BARs

  uint8_t m_blockAckThreshold; ///< bock ack threshold
  BlockAckType m_blockAckType; ///< bock ack type
  Time m_maxDelay; ///< maximum delay
  Ptr<MacTxMiddle> m_txMiddle; ///< the MacTxMiddle
  Mac48Address m_address; ///< address
  Ptr<WifiMacQueue> m_queue; ///< queue
  Callback<void, Mac48Address, uint8_t, bool> m_blockAckInactivityTimeout; ///< block ack inactivity timeout callback
  Callback<void, Mac48Address, uint8_t> m_blockPackets; ///< block packets callback
  Callback<void, Mac48Address, uint8_t> m_unblockPackets; ///< unblock packets callback
  TxOk m_txOkCallback; ///< transmit ok callback
  TxFailed m_txFailedCallback; ///< transmit failed callback
  Ptr<WifiRemoteStationManager> m_stationManager; ///< the station manager
};

} //namespace ns3

#endif /* BLOCK_ACK_MANAGER_H */
