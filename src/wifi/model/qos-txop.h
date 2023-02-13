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

#include "block-ack-manager.h"
#include "qos-utils.h"
#include "txop.h"

#include "ns3/traced-value.h"

namespace ns3
{

class MgtAddBaResponseHeader;
class MgtDelBaHeader;
class AggregationCapableTransmissionListener;
class WifiTxVector;
class QosFrameExchangeManager;
class WifiTxParameters;

/**
 * \brief Handle packet fragmentation and retransmissions for QoS data frames as well
 * as MSDU aggregation (A-MSDU) and block ack sessions, for a given access class.
 * \ingroup wifi
 *
 * This class implements the packet fragmentation and retransmission policy for
 * QoS data frames. It uses the ns3::ChannelAccessManager helper class to decide
 * when to send a packet. Packets are stored in a ns3::WifiMacQueue until they can be sent.
 *
 * This queue contains packets for a particular access class.
 * Possibles access classes are:
 *   - AC_VO : voice, TID = 6,7
 *   - AC_VI : video, TID = 4,5
 *   - AC_BE : best-effort, TID = 0,3
 *   - AC_BK : background, TID = 1,2
 *
 * This class also implements block ack sessions and MSDU aggregation (A-MSDU).
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
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     *
     * \param ac the Access Category
     */
    QosTxop(AcIndex ac = AC_UNDEF);

    ~QosTxop() override;

    bool IsQosTxop() const override;
    bool HasFramesToTransmit(uint8_t linkId) override;
    void NotifyChannelAccessed(uint8_t linkId, Time txopDuration) override;
    void NotifyChannelReleased(uint8_t linkId) override;
    void SetDroppedMpduCallback(DroppedMpdu callback) override;

    /**
     * Get the access category of this object.
     *
     * \return the access category.
     */
    AcIndex GetAccessCategory() const;

    /**
     * Return true if an explicit BlockAckRequest is sent after a missed BlockAck
     *
     * \return true if an explicit BlockAckRequest is sent after a missed BlockAck
     */
    bool UseExplicitBarAfterMissedBlockAck() const;

    /**
     * Get the Block Ack Manager associated with this QosTxop.
     *
     * \returns the Block Ack Manager
     */
    Ptr<BlockAckManager> GetBaManager();
    /**
     * \param address recipient address of the peer station
     * \param tid traffic ID.
     *
     * \return the negotiated buffer size during ADDBA handshake.
     *
     * Returns the negotiated buffer size during ADDBA handshake with station addressed by
     * <i>recipient</i> for TID <i>tid</i>.
     */
    uint16_t GetBaBufferSize(Mac48Address address, uint8_t tid) const;
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
    uint16_t GetBaStartingSequence(Mac48Address address, uint8_t tid) const;
    /**
     * \param recipient Address of recipient.
     * \param tid traffic ID.
     * \return the BlockAckRequest header and the MAC header for the BlockAckReq
     *
     * Prepare a BlockAckRequest to be sent to <i>recipient</i> for Traffic ID
     * <i>tid</i>. The header for the BlockAckRequest is requested to the QosTxop
     * corresponding to the given TID. A block ack agreement with the given recipient
     * for the given TID must have been established by such QosTxop.
     */
    std::pair<CtrlBAckRequestHeader, WifiMacHeader> PrepareBlockAckRequest(Mac48Address recipient,
                                                                           uint8_t tid) const;

    /* Event handlers */
    /**
     * Event handler when an ADDBA response is received.
     *
     * \param respHdr ADDBA response header.
     * \param recipient address of the recipient.
     */
    void GotAddBaResponse(const MgtAddBaResponseHeader& respHdr, Mac48Address recipient);
    /**
     * Event handler when a DELBA frame is received.
     *
     * \param delBaHdr DELBA header.
     * \param recipient address of the recipient.
     */
    void GotDelBaFrame(const MgtDelBaHeader* delBaHdr, Mac48Address recipient);
    /**
     * Take action upon notification of ADDBA_REQUEST frame being discarded
     * (likely due to exceeded max retry limit).
     *
     * \param recipient the intended recipient of the ADDBA_REQUEST frame
     * \param tid the TID
     */
    void NotifyOriginatorAgreementNoReply(const Mac48Address& recipient, uint8_t tid);
    /**
     * Callback when ADDBA response is not received after timeout.
     *
     * \param recipient MAC address of recipient
     * \param tid traffic ID
     */
    void AddBaResponseTimeout(Mac48Address recipient, uint8_t tid);
    /**
     * Reset BA agreement after BA negotiation failed.
     *
     * \param recipient MAC address of recipient
     * \param tid traffic ID
     */
    void ResetBa(Mac48Address recipient, uint8_t tid);

    /**
     * Set threshold for block ack mechanism. If number of packets in the
     * queue reaches the threshold, block ack mechanism is used.
     *
     * \param threshold block ack threshold value.
     */
    void SetBlockAckThreshold(uint8_t threshold);
    /**
     * Return the current threshold for block ack mechanism.
     *
     * \return the current threshold for block ack mechanism.
     */
    uint8_t GetBlockAckThreshold() const;

    /**
     * Set the BlockAck inactivity timeout.
     *
     * \param timeout the BlockAck inactivity timeout.
     */
    void SetBlockAckInactivityTimeout(uint16_t timeout);
    /**
     * Get the BlockAck inactivity timeout.
     *
     * \return the BlockAck inactivity timeout.
     */
    uint16_t GetBlockAckInactivityTimeout() const;
    /**
     * Stores an MPDU (part of an A-MPDU) in block ack agreement (i.e. the sender is waiting
     * for a BlockAck containing the sequence number of this MPDU).
     *
     * \param mpdu received MPDU.
     */
    void CompleteMpduTx(Ptr<WifiMpdu> mpdu);
    /**
     * Set the timeout to wait for ADDBA response.
     *
     * \param addBaResponseTimeout the timeout to wait for ADDBA response
     */
    void SetAddBaResponseTimeout(Time addBaResponseTimeout);
    /**
     * Get the timeout for ADDBA response.
     *
     * \returns the timeout to wait for ADDBA response
     */
    Time GetAddBaResponseTimeout() const;
    /**
     * Set the timeout for failed BA agreement. During the timeout period,
     * all packets will be transmitted using normal MPDU.
     *
     * \param failedAddBaTimeout the timeout for failed BA agreement
     */
    void SetFailedAddBaTimeout(Time failedAddBaTimeout);
    /**
     * Get the timeout for failed BA agreement.
     *
     * \returns the timeout for failed BA agreement
     */
    Time GetFailedAddBaTimeout() const;

    /**
     * Return the next sequence number for the given header.
     *
     * \param hdr Wi-Fi header.
     *
     * \return the next sequence number.
     */
    uint16_t GetNextSequenceNumberFor(const WifiMacHeader* hdr);
    /**
     * Return the next sequence number for the Traffic ID and destination, but do not pick it (i.e.
     * the current sequence number remains unchanged).
     *
     * \param hdr Wi-Fi header.
     *
     * \return the next sequence number.
     */
    uint16_t PeekNextSequenceNumberFor(const WifiMacHeader* hdr);
    /**
     * Peek the next frame to transmit on the given link to the given receiver and of the given TID
     * from the EDCA queue. If <i>tid</i> is equal to 8 (invalid value) and <i>recipient</i>
     * is the broadcast address, the first available frame is returned. If <i>mpdu</i>
     * is not a null pointer, the search starts from the MPDU following <i>mpdu</i>
     * in the queue; otherwise, the search starts from the head of the queue.
     * Note that A-MSDU aggregation is never attempted. If the frame has never been
     * transmitted, it is assigned a sequence number peeked from MacTxMiddle.
     * Also note that multiple links are only available since 802.11be.
     *
     * \param linkId the ID of the given link
     * \param tid traffic ID.
     * \param recipient the receiver station address.
     * \param mpdu the MPDU after which the search starts from
     * \returns the peeked frame.
     */
    Ptr<WifiMpdu> PeekNextMpdu(uint8_t linkId,
                               uint8_t tid = 8,
                               Mac48Address recipient = Mac48Address::GetBroadcast(),
                               Ptr<const WifiMpdu> mpdu = nullptr);
    /**
     * Prepare the frame to transmit on the given link starting from the MPDU that has been
     * previously peeked by calling PeekNextMpdu. A frame is only returned if it meets the
     * constraint on the maximum A-MPDU size (by assuming that the frame has to be
     * aggregated to an existing A-MPDU as specified by the TX parameters) and its
     * transmission time does not exceed the given PPDU duration limit (if distinct from
     *  Time::Min ()). If the peeked MPDU is a unicast QoS Data frame stored in the EDCA queue,
     * attempt to perform A-MSDU aggregation (while meeting the constraints mentioned
     * above) and assign a sequence number to the dequeued frame.
     *
     * \param linkId the ID of the given link
     * \param peekedItem the peeked frame.
     * \param txParams the TX parameters for the frame
     * \param availableTime the time available for the transmission of the frame
                            (including protection and acknowledgment); a value of
     *                      Time::Min() indicates no time constraint
     * \param initialFrame true if the frame is the initial PPDU of a TXOP
     * \return the frame to transmit or a null pointer if no frame meets the time constraints
     */
    Ptr<WifiMpdu> GetNextMpdu(uint8_t linkId,
                              Ptr<WifiMpdu> peekedItem,
                              WifiTxParameters& txParams,
                              Time availableTime,
                              bool initialFrame);

    /**
     * Assign a sequence number to the given MPDU, if it is not a fragment
     * and it is not a retransmitted frame.
     *
     * \param mpdu the MPDU
     */
    void AssignSequenceNumber(Ptr<WifiMpdu> mpdu) const;

    /**
     * Get the value for the Queue Size subfield of the QoS Control field of a
     * QoS data frame of the given TID and addressed to the given receiver.
     *
     * \param tid the Traffic ID
     * \param receiver the address of the given receiver
     * \return the value for the Queue Size subfield
     */
    uint8_t GetQosQueueSize(uint8_t tid, Mac48Address receiver) const;

    /**
     * Return true if a TXOP has started on the given link.
     *
     * \param linkId the ID of the given link
     * \return true if a TXOP has started, false otherwise.
     */
    virtual bool IsTxopStarted(uint8_t linkId) const;
    /**
     * Return the remaining duration in the current TXOP on the given link.
     *
     * \param linkId the ID of the given link
     * \return the remaining duration in the current TXOP.
     */
    virtual Time GetRemainingTxop(uint8_t linkId) const;

    /**
     * Set the minimum contention window size to use while the MU EDCA Timer
     * is running for the given link.
     *
     * \param cwMin the minimum contention window size.
     * \param linkId the ID of the given link
     */
    void SetMuCwMin(uint16_t cwMin, uint8_t linkId);
    /**
     * Set the maximum contention window size to use while the MU EDCA Timer
     * is running for the given link.
     *
     * \param cwMax the maximum contention window size.
     * \param linkId the ID of the given link
     */
    void SetMuCwMax(uint16_t cwMax, uint8_t linkId);
    /**
     * Set the number of slots that make up an AIFS while the MU EDCA Timer
     * is running for the given link.
     *
     * \param aifsn the number of slots that make up an AIFS.
     * \param linkId the ID of the given link
     */
    void SetMuAifsn(uint8_t aifsn, uint8_t linkId);
    /**
     * Set the MU EDCA Timer for the given link.
     *
     * \param timer the timer duration.
     * \param linkId the ID of the given link
     */
    void SetMuEdcaTimer(Time timer, uint8_t linkId);
    /**
     * Start the MU EDCA Timer for the given link.
     *
     * \param linkId the ID of the given link
     */
    void StartMuEdcaTimerNow(uint8_t linkId);
    /**
     * Return true if the MU EDCA Timer is running for the given link, false otherwise.
     *
     * \param linkId the ID of the given link
     * \return whether the MU EDCA Timer is running
     */
    bool MuEdcaTimerRunning(uint8_t linkId) const;
    /**
     * Return true if the EDCA is disabled (the MU EDCA Timer is running and the
     * MU AIFSN is zero) for the given link, false otherwise.
     *
     * \param linkId the ID of the given link
     * \return whether the EDCA is disabled
     */
    bool EdcaDisabled(uint8_t linkId) const;
    /**
     * For the given link, return the minimum contention window size from the
     * EDCA Parameter Set or the MU EDCA Parameter Set, depending on whether the
     * MU EDCA Timer is running or not.
     *
     * \param linkId the ID of the given link
     * \return the currently used minimum contention window size.
     */
    uint32_t GetMinCw(uint8_t linkId) const override;
    /**
     * For the given link, return the maximum contention window size from the
     * EDCA Parameter Set or the MU EDCA Parameter Set, depending on whether the
     * MU EDCA Timer is running or not.
     *
     * \param linkId the ID of the given link
     * \return the currently used maximum contention window size.
     */
    uint32_t GetMaxCw(uint8_t linkId) const override;
    /**
     * For the given link, return the number of slots that make up an AIFS according
     * to the EDCA Parameter Set or the MU EDCA Parameter Set, depending on whether
     * the MU EDCA Timer is running or not.
     *
     * \param linkId the ID of the given link
     * \return the number of slots that currently make up an AIFS.
     */
    uint8_t GetAifsn(uint8_t linkId) const override;

  protected:
    /**
     * Structure holding information specific to a single link. Here, the meaning of
     * "link" is that of the 11be amendment which introduced multi-link devices. For
     * previous amendments, only one link can be created.
     */
    struct QosLinkEntity : public Txop::LinkEntity
    {
        /// Destructor (a virtual method is needed to make this struct polymorphic)
        ~QosLinkEntity() override = default;

        Time startTxop{0};            //!< the start TXOP time
        Time txopDuration{0};         //!< the duration of a TXOP
        uint32_t muCwMin{0};          //!< the MU CW minimum
        uint32_t muCwMax{0};          //!< the MU CW maximum
        uint8_t muAifsn{0};           //!< the MU AIFSN
        Time muEdcaTimer{0};          //!< the MU EDCA Timer
        Time muEdcaTimerStartTime{0}; //!< last start time of the MU EDCA Timer
    };

    void DoDispose() override;

    /**
     * Get a reference to the link associated with the given ID.
     *
     * \param linkId the given link ID
     * \return a reference to the link associated with the given ID
     */
    QosLinkEntity& GetLink(uint8_t linkId) const;

  private:
    /// allow AggregationCapableTransmissionListener class access
    friend class AggregationCapableTransmissionListener;

    std::unique_ptr<LinkEntity> CreateLinkEntity() const override;

    /**
     * Check if the given MPDU is to be considered old according to the current
     * starting sequence number of the transmit window, provided that a block ack
     * agreement has been established with the recipient for the given TID.
     *
     * \param mpdu the given MPDU
     * \return true if the MPDU is to be considered old, false otherwise
     */
    bool IsQosOldPacket(Ptr<const WifiMpdu> mpdu);

    AcIndex m_ac;                     //!< the access category
    Ptr<BlockAckManager> m_baManager; //!< the block ack manager
    uint8_t m_blockAckThreshold; /**< the block ack threshold (use BA mechanism if number of packets
                                    in queue reaches this value. If this value is 0, block ack is
                                    never used. When A-MPDU is enabled, block ack mechanism is used
                                    regardless of this value) */
    uint16_t m_blockAckInactivityTimeout; //!< the BlockAck inactivity timeout value (in TUs, i.e.
                                          //!< blocks of 1024 microseconds)
    Time m_addBaResponseTimeout;          //!< timeout for ADDBA response
    Time m_failedAddBaTimeout;            //!< timeout after failed BA agreement
    bool m_useExplicitBarAfterMissedBlockAck; //!< flag whether explicit BlockAckRequest should be
                                              //!< sent upon missed BlockAck Response
    uint8_t m_nMaxInflights;                  //!< the maximum number of links on which
                                              //!< an MPDU can be in-flight at the same
                                              //!< time

    /// TracedCallback for TXOP trace typedef
    typedef TracedCallback<Time /* start time */, Time /* duration */, uint8_t /* link ID*/>
        TxopTracedCallback;

    TxopTracedCallback m_txopTrace; //!< TXOP trace callback
};

} // namespace ns3

#endif /* QOS_TXOP_H */
