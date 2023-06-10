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

#include "block-ack-type.h"
#include "originator-block-ack-agreement.h"
#include "recipient-block-ack-agreement.h"
#include "wifi-mac-header.h"
#include "wifi-mpdu.h"
#include "wifi-tx-vector.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <map>
#include <optional>

namespace ns3
{

class MgtAddBaResponseHeader;
class MgtAddBaRequestHeader;
class CtrlBAckResponseHeader;
class CtrlBAckRequestHeader;
class WifiMacQueue;
class MacRxMiddle;

/**
 * \brief Manages all block ack agreements for an originator station.
 * \ingroup wifi
 */
class BlockAckManager : public Object
{
  private:
    /**
     * Enumeration for the statuses of a buffered MPDU
     */
    enum MpduStatus : uint8_t
    {
        STAY_INFLIGHT = 0,
        TO_RETRANSMIT,
        ACKNOWLEDGED
    };

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    BlockAckManager();
    ~BlockAckManager() override;

    // Delete copy constructor and assignment operator to avoid misuse
    BlockAckManager(const BlockAckManager&) = delete;
    BlockAckManager& operator=(const BlockAckManager&) = delete;

    /// optional const reference to OriginatorBlockAckAgreement
    using OriginatorAgreementOptConstRef =
        std::optional<std::reference_wrapper<const OriginatorBlockAckAgreement>>;
    /// optional const reference to RecipientBlockAckAgreement
    using RecipientAgreementOptConstRef =
        std::optional<std::reference_wrapper<const RecipientBlockAckAgreement>>;

    /**
     * \param recipient MAC address of the recipient
     * \param tid Traffic ID
     *
     * \return a const reference to the block ack agreement with the given recipient, if it exists
     *
     * Check if we are the originator of an existing block ack agreement with the given recipient.
     */
    OriginatorAgreementOptConstRef GetAgreementAsOriginator(const Mac48Address& recipient,
                                                            uint8_t tid) const;
    /**
     * \param originator MAC address of the originator
     * \param tid Traffic ID
     *
     * \return a const reference to the block ack agreement with the given originator, if it exists
     *
     * Check if we are the recipient of an existing block ack agreement with the given originator.
     */
    RecipientAgreementOptConstRef GetAgreementAsRecipient(const Mac48Address& originator,
                                                          uint8_t tid) const;

    /**
     * \param reqHdr Relative Add block ack request (action frame).
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param htSupported Whether both originator and recipient support HT
     *
     * Creates a new originator block ack agreement in pending state. When a ADDBA response
     * with a successful status code is received, the relative agreement becomes established.
     */
    void CreateOriginatorAgreement(const MgtAddBaRequestHeader& reqHdr,
                                   const Mac48Address& recipient,
                                   bool htSupported = true);
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid traffic ID of transmitted packet.
     *
     * Invoked when a recipient reject a block ack agreement or when a DELBA frame
     * is Received/Transmitted.
     */
    void DestroyOriginatorAgreement(const Mac48Address& recipient, uint8_t tid);
    /**
     * \param respHdr Relative Add block ack response (action frame).
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param startingSeq the updated starting sequence number
     *
     * Invoked upon receipt of a ADDBA response frame from <i>recipient</i>.
     */
    void UpdateOriginatorAgreement(const MgtAddBaResponseHeader& respHdr,
                                   const Mac48Address& recipient,
                                   uint16_t startingSeq);

    /**
     * \param respHdr Add block ack response from originator (action
     *        frame).
     * \param originator Address of peer station involved in block ack
     *        mechanism.
     * \param startingSeq Sequence number of the first MPDU of all
     *        packets for which block ack was negotiated.
     * \param htSupported whether HT support is enabled
     * \param rxMiddle the MAC RX Middle on this station
     *
     * This function is typically invoked only by ns3::WifiMac
     * when the STA (which may be non-AP in ESS, or in an IBSS) has
     * received an ADDBA Request frame and is transmitting an ADDBA
     * Response frame. At this point the frame exchange manager must
     * allocate buffers to collect all correctly received packets belonging
     * to the category for which block ack was negotiated.
     */
    void CreateRecipientAgreement(const MgtAddBaResponseHeader& respHdr,
                                  const Mac48Address& originator,
                                  uint16_t startingSeq,
                                  bool htSupported,
                                  Ptr<MacRxMiddle> rxMiddle);
    /**
     * Destroy a recipient Block Ack agreement.
     *
     * \param originator the originator MAC address
     * \param tid the TID associated with the Block Ack agreement
     */
    void DestroyRecipientAgreement(const Mac48Address& originator, uint8_t tid);

    /**
     * \param mpdu MPDU to store.
     *
     * Stores <i>mpdu</i> for a possible future retransmission. Retransmission occurs
     * if the packet, in a BlockAck frame, is indicated by recipient as not received.
     */
    void StorePacket(Ptr<WifiMpdu> mpdu);
    /**
     * Invoked upon receipt of an Ack frame on the given link after the transmission of a
     * QoS data frame sent under an established block ack agreement. Remove the acknowledged
     * frame from the outstanding packets and update the starting sequence number of the
     * transmit window, if needed.
     *
     * \param linkId the ID of the given link
     * \param mpdu The acknowledged MPDU.
     */
    void NotifyGotAck(uint8_t linkId, Ptr<const WifiMpdu> mpdu);
    /**
     * Invoked upon missed reception of an Ack frame on the given link after the
     * transmission of a QoS data frame sent under an established block ack agreement.
     * Remove the acknowledged frame from the outstanding packets and insert it in the
     * retransmission queue.
     *
     * \param linkId the ID of the given link
     * \param mpdu The unacknowledged MPDU.
     */
    void NotifyMissedAck(uint8_t linkId, Ptr<WifiMpdu> mpdu);
    /**
     * \param linkId the ID of the given link
     * \param blockAck The received BlockAck frame.
     * \param recipient Sender of BlockAck frame.
     * \param tids the set of TIDs the acknowledged MPDUs belong to
     * \param index the index of the Per AID TID Info subfield, in case of Multi-STA
     *              Block Ack, or 0, otherwise
     * \return a pair of values indicating the number of successfully received MPDUs
     *         and the number of failed MPDUs
     *
     * Invoked upon receipt of a BlockAck frame on the given link. Typically, this function
     * is called by ns3::QosTxop object. Performs a check on which MPDUs, previously sent
     * with Ack Policy set to Block Ack, were correctly received by the recipient.
     * An acknowledged MPDU is removed from the buffer, retransmitted otherwise.
     * Note that <i>tids</i> is only used if <i>blockAck</i> is a Multi-STA Block Ack
     * using All-ack context.
     */
    std::pair<uint16_t, uint16_t> NotifyGotBlockAck(uint8_t linkId,
                                                    const CtrlBAckResponseHeader& blockAck,
                                                    const Mac48Address& recipient,
                                                    const std::set<uint8_t>& tids,
                                                    size_t index = 0);
    /**
     * \param linkId the ID of the given link
     * \param recipient Sender of the expected BlockAck frame.
     * \param tid Traffic ID.
     *
     * Invoked upon missed reception of a block ack frame on the given link. Typically, this
     * function is called by ns3::QosTxop object. Performs a check on which MPDUs, previously
     * sent with ack policy set to Block Ack, should be placed in the retransmission queue.
     */
    void NotifyMissedBlockAck(uint8_t linkId, const Mac48Address& recipient, uint8_t tid);
    /**
     * \param originator MAC address of the sender of the Block Ack Request
     * \param tid Traffic ID
     * \param startingSeq the starting sequence number in the Block Ack Request
     *
     * Perform required actions upon receiving a Block Ack Request frame.
     */
    void NotifyGotBlockAckRequest(const Mac48Address& originator,
                                  uint8_t tid,
                                  uint16_t startingSeq);
    /**
     * \param mpdu the received MPDU
     *
     * Perform required actions upon receiving an MPDU.
     */
    void NotifyGotMpdu(Ptr<const WifiMpdu> mpdu);
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid Traffic ID.
     *
     * \return the number of packets buffered for a specified agreement
     *
     * Returns the number of packets buffered for a specified agreement. This methods doesn't return
     * the number of buffered MPDUs but the number of buffered MSDUs.
     */
    uint32_t GetNBufferedPackets(const Mac48Address& recipient, uint8_t tid) const;
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid Traffic ID of transmitted packet.
     * \param startingSeq starting sequence field
     *
     * Puts corresponding originator agreement in established state and updates number of packets
     * and starting sequence field. Invoked typically after a block ack refresh.
     */
    void NotifyOriginatorAgreementEstablished(const Mac48Address& recipient,
                                              uint8_t tid,
                                              uint16_t startingSeq);
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid Traffic ID of transmitted packet.
     *
     * Marks an originator agreement as rejected. This happens if <i>recipient</i> station reject
     * block ack setup by an ADDBA Response frame with a failure status code. For now we assume
     * that every QoS station accepts a block ack setup.
     */
    void NotifyOriginatorAgreementRejected(const Mac48Address& recipient, uint8_t tid);
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid Traffic ID of transmitted packet.
     *
     * Marks an originator agreement after not receiving response to ADDBA request. During this
     * state any packets in queue will be transmitted using normal MPDU. This also unblocks
     * recipient address.
     */
    void NotifyOriginatorAgreementNoReply(const Mac48Address& recipient, uint8_t tid);
    /**
     * \param recipient Address of peer station involved in block ack mechanism.
     * \param tid Traffic ID of transmitted packet.
     *
     * Set Originator BA agreement to a transitory state to reset it after not receiving response
     * to ADDBA request.
     */
    void NotifyOriginatorAgreementReset(const Mac48Address& recipient, uint8_t tid);
    /**
     * \param nPackets Minimum number of packets for use of block ack.
     *
     * Upon receipt of a BlockAck frame, if total number of packets (packets in WifiMacQueue
     * and buffered packets) is greater of <i>nPackets</i>, they are transmitted using block ack
     * mechanism.
     */
    void SetBlockAckThreshold(uint8_t nPackets);

    /**
     * \param queue The WifiMacQueue object.
     */
    void SetQueue(const Ptr<WifiMacQueue> queue);

    /**
     * Set block ack inactivity callback
     * \param callback the block ack inactivity callback function
     */
    void SetBlockAckInactivityCallback(Callback<void, Mac48Address, uint8_t, bool> callback);
    /**
     * Set block destination callback
     * \param callback the block destination callback
     */
    void SetBlockDestinationCallback(Callback<void, Mac48Address, uint8_t> callback);
    /**
     * Set unblock destination callback
     * \param callback the unblock destination callback
     */
    void SetUnblockDestinationCallback(Callback<void, Mac48Address, uint8_t> callback);

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
    bool NeedBarRetransmission(uint8_t tid, const Mac48Address& recipient);
    /**
     * This function returns the buffer size negotiated with the recipient.
     *
     * \param tid Traffic ID
     * \param recipient MAC address of the recipient
     *
     * \returns the buffer size negotiated with the recipient
     */
    uint16_t GetRecipientBufferSize(const Mac48Address& recipient, uint8_t tid) const;
    /**
     * This function returns the starting sequence number of the transmit window.
     *
     * \param tid Traffic ID
     * \param recipient MAC address of the recipient
     *
     * \returns the starting sequence number of the transmit window (WinStartO)
     */
    uint16_t GetOriginatorStartingSequence(const Mac48Address& recipient, uint8_t tid) const;

    /**
     * typedef for a callback to invoke when an MPDU is successfully ack'ed.
     */
    typedef Callback<void, Ptr<const WifiMpdu>> TxOk;
    /**
     * typedef for a callback to invoke when an MPDU is negatively ack'ed.
     */
    typedef Callback<void, Ptr<const WifiMpdu>> TxFailed;
    /**
     * typedef for a callback to invoke when an MPDU is dropped.
     */
    typedef Callback<void, Ptr<const WifiMpdu>> DroppedOldMpdu;

    /**
     * \param callback the callback to invoke when a
     * packet transmission was completed successfully.
     */
    void SetTxOkCallback(TxOk callback);
    /**
     * \param callback the callback to invoke when a
     * packet transmission was completed unsuccessfuly.
     */
    void SetTxFailedCallback(TxFailed callback);
    /**
     * \param callback the callback to invoke when an old MPDU is dropped
     */
    void SetDroppedOldMpduCallback(DroppedOldMpdu callback);

    /**
     * TracedCallback signature for state changes.
     *
     * \param [in] now Time when the \pname{state} changed.
     * \param [in] recipient MAC address of the recipient.
     * \param [in] tid the TID.
     * \param [in] state The state.
     */
    typedef void (*AgreementStateTracedCallback)(Time now,
                                                 const Mac48Address& recipient,
                                                 uint8_t tid,
                                                 OriginatorBlockAckAgreement::State state);

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
    void NotifyDiscardedMpdu(Ptr<const WifiMpdu> mpdu);

    /**
     * \param recipient the recipient
     * \param tid the TID
     * \return the BlockAckRequest header for the established BA agreement
     *
     * Get the BlockAckRequest header for the established BA agreement
     * (<i>recipient</i>,<i>tid</i>).
     */
    CtrlBAckRequestHeader GetBlockAckReqHeader(const Mac48Address& recipient, uint8_t tid) const;

    /**
     * \param reqHdr the BlockAckRequest header
     * \param hdr the 802.11 MAC header
     *
     * Enqueue the given BlockAckRequest into the queue storing the next BAR
     * frames to transmit. If a BAR for the same recipient and TID is already present
     * in the queue, it is replaced by the new one. If the given BAR is retransmitted,
     * it is placed at the head of the queue, otherwise at the tail.
     */
    void ScheduleBar(const CtrlBAckRequestHeader& reqHdr, const WifiMacHeader& hdr);
    /**
     * \param muBar the MU-BAR Trigger Frame to enqueue
     *
     * Enqueue the given MU-BAR Trigger Frame into the queue storing the next MU-BAR
     * frames to transmit. If the given MU-BAR Trigger Frame is retransmitted,
     * it is placed at the head of the queue, otherwise at the tail.
     */
    void ScheduleMuBar(Ptr<WifiMpdu> muBar);

    /// agreement key typedef (MAC address and TID)
    using AgreementKey = std::pair<Mac48Address, uint8_t>;

    /**
     * \return the list of BA agreements (identified by the recipient and TID pair) for which a BAR
     * shall only be sent if there are queued data frames belonging to those agreements
     */
    const std::list<AgreementKey>& GetSendBarIfDataQueuedList() const;
    /**
     * Add the given (recipient, TID) pair to the list of BA agreements for which a BAR
     * shall only be sent if there are queued data frames belonging to those agreements
     *
     * \param recipient the recipient
     * \param tid the TID
     */
    void AddToSendBarIfDataQueuedList(const Mac48Address& recipient, uint8_t tid);
    /**
     * Remove the given (recipient, TID) pair from the list of BA agreements for which a BAR
     * shall only be sent if there are queued data frames belonging to those agreements
     *
     * \param recipient the recipient
     * \param tid the TID
     */
    void RemoveFromSendBarIfDataQueuedList(const Mac48Address& recipient, uint8_t tid);

  protected:
    void DoDispose() override;

  private:
    /**
     * Inactivity timeout function
     * \param recipient the recipient MAC address
     * \param tid Traffic ID
     */
    void InactivityTimeout(const Mac48Address& recipient, uint8_t tid);

    /**
     * typedef for a list of WifiMpdu.
     */
    typedef std::list<Ptr<WifiMpdu>> PacketQueue;
    /**
     * typedef for an iterator for PacketQueue.
     */
    typedef std::list<Ptr<WifiMpdu>>::iterator PacketQueueI;

    /// AgreementKey-indexed map of originator block ack agreements
    using OriginatorAgreements =
        std::map<AgreementKey, std::pair<OriginatorBlockAckAgreement, PacketQueue>>;
    /// typedef for an iterator for Agreements
    using OriginatorAgreementsI = OriginatorAgreements::iterator;

    /// AgreementKey-indexed map of recipient block ack agreements
    using RecipientAgreements = std::map<AgreementKey, RecipientBlockAckAgreement>;

    /**
     * Handle the given in flight MPDU based on its given status. If the status is
     * ACKNOWLEDGED, the MPDU is removed from both the EDCA queue and the queue of
     * in flight MPDUs. If the status is TO_RETRANSMIT, the MPDU is only removed
     * from the queue of in flight MPDUs. Note that the MPDU is removed from both
     * queues (independently of the status) if the MPDU is not stored in the EDCA
     * queue, is an old packet or its lifetime expired.
     *
     * \param linkId the ID of the link on which the MPDU has been transmitted
     * \param mpduIt an iterator pointing to the MPDU in the queue of in flight MPDUs
     * \param status the status of the in flight MPDU
     * \param it iterator pointing to the Block Ack agreement
     * \param now the current time
     * \return an iterator pointing to the next MPDU in the queue of in flight MPDUs
     */
    PacketQueueI HandleInFlightMpdu(uint8_t linkId,
                                    PacketQueueI mpduIt,
                                    MpduStatus status,
                                    const OriginatorAgreementsI& it,
                                    const Time& now);

    /**
     * This data structure contains, for each originator block ack agreement (recipient, TID),
     * a set of packets for which an ack by block ack is requested.
     * Every packet or fragment indicated as correctly received in BlockAck frame is
     * erased from this data structure. Pushed back in retransmission queue otherwise.
     */
    OriginatorAgreements m_originatorAgreements;
    RecipientAgreements m_recipientAgreements; //!< Recipient Block Ack agreements

    std::list<AgreementKey> m_sendBarIfDataQueued; ///< list of BA agreements for which a BAR shall
                                                   ///< only be sent if data is queued

    uint8_t m_blockAckThreshold; ///< block ack threshold
    Ptr<WifiMacQueue> m_queue;   ///< queue
    Callback<void, Mac48Address, uint8_t, bool>
        m_blockAckInactivityTimeout;                      ///< BlockAck inactivity timeout callback
    Callback<void, Mac48Address, uint8_t> m_blockPackets; ///< block packets callback
    Callback<void, Mac48Address, uint8_t> m_unblockPackets; ///< unblock packets callback
    TxOk m_txOkCallback;                                    ///< transmit OK callback
    TxFailed m_txFailedCallback;                            ///< transmit failed callback
    DroppedOldMpdu m_droppedOldMpduCallback;                ///< the dropped MPDU callback

    /**
     * The trace source fired when a state transition occurred.
     */
    TracedCallback<Time, Mac48Address, uint8_t, OriginatorBlockAckAgreement::State>
        m_originatorAgreementState;
};

} // namespace ns3

#endif /* BLOCK_ACK_MANAGER_H */
