/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef BLOCK_ACK_AGREEMENT_H
#define BLOCK_ACK_AGREEMENT_H

#include "block-ack-type.h"

#include "ns3/event-id.h"
#include "ns3/mac48-address.h"

#include <optional>

namespace ns3
{
/**
 * @brief Maintains information for a block ack agreement.
 * @ingroup wifi
 */
class BlockAckAgreement
{
    friend class HtFrameExchangeManager;

  public:
    /**
     * Constructor for BlockAckAgreement with given peer and TID.
     *
     * @param peer the peer station
     * @param tid the TID
     */
    BlockAckAgreement(Mac48Address peer, uint8_t tid);
    virtual ~BlockAckAgreement();
    /**
     * Set buffer size.
     *
     * @param bufferSize the buffer size (in number of MPDUs)
     */
    void SetBufferSize(uint16_t bufferSize);
    /**
     * Set timeout.
     *
     * @param timeout the timeout value
     */
    void SetTimeout(uint16_t timeout);
    /**
     * Set starting sequence number.
     *
     * @param seq the starting sequence number
     */
    void SetStartingSequence(uint16_t seq);
    /**
     * Set starting sequence control.
     *
     * @param seq the starting sequence control
     */
    void SetStartingSequenceControl(uint16_t seq);
    /**
     * Set block ack policy to immediate Ack.
     */
    void SetImmediateBlockAck();
    /**
     * Set block ack policy to delayed Ack.
     */
    void SetDelayedBlockAck();
    /**
     * Enable or disable A-MSDU support.
     *
     * @param supported enable or disable A-MSDU support
     */
    void SetAmsduSupport(bool supported);
    /**
     * Return the Traffic ID (TID).
     *
     * @return TID
     */
    uint8_t GetTid() const;
    /**
     * Return the peer address.
     *
     * @return the peer MAC address
     */
    Mac48Address GetPeer() const;
    /**
     * Return the buffer size.
     *
     * @return the buffer size (in number of MPDUs)
     */
    uint16_t GetBufferSize() const;
    /**
     * Return the timeout.
     *
     * @return the timeout
     */
    uint16_t GetTimeout() const;
    /**
     * Return the starting sequence number.
     *
     * @return starting sequence number
     */
    virtual uint16_t GetStartingSequence() const;
    /**
     * Return the starting sequence control
     *
     * @return starting sequence control
     */
    uint16_t GetStartingSequenceControl() const;
    /**
     * Return the last sequence number covered by the ack window
     *
     * @return ending sequence number
     */
    uint16_t GetWinEnd() const;
    /**
     * Check whether the current ack policy is immediate BlockAck.
     *
     * @return true if the current ack policy is immediate BlockAck,
     *         false otherwise
     */
    bool IsImmediateBlockAck() const;
    /**
     * Check whether A-MSDU is supported
     *
     * @return true if A-MSDU is supported,
     *         false otherwise
     */
    bool IsAmsduSupported() const;
    /**
     * Enable or disable HT support.
     *
     * @param htSupported enable or disable HT support
     */
    void SetHtSupported(bool htSupported);
    /**
     * Check whether HT is supported
     *
     * @return true if HT is supported,
     *         false otherwise
     */
    bool IsHtSupported() const;
    /**
     * Get the type of the Block Acks sent by the recipient of this agreement.
     *
     * @return the type of the Block Acks sent by the recipient of this agreement
     */
    BlockAckType GetBlockAckType() const;
    /**
     * Get the type of the Block Ack Requests sent by the originator of this agreement.
     *
     * @return the type of the Block Ack Requests sent by the originator of this agreement
     */
    BlockAckReqType GetBlockAckReqType() const;
    /**
     * Get the distance between the given starting sequence number and the
     * given sequence number.
     *
     * @param seqNumber the given sequence number
     * @param startingSeqNumber the given starting sequence number
     * @return the distance of the given sequence number from the given starting sequence number
     */
    static std::size_t GetDistance(uint16_t seqNumber, uint16_t startingSeqNumber);
    /**
     * Set the GCR group address for this agreement.
     *
     * @param gcrGroupAddress the GCR group address
     */
    void SetGcrGroupAddress(const Mac48Address& gcrGroupAddress);
    /**
     * Get the GCR group address of this agreement if it is a GCR Block Ack agreement.
     *
     * @return the GCR group address of this agreement if it is a GCR Block Ack agreement
     */
    const std::optional<Mac48Address>& GetGcrGroupAddress() const;

  protected:
    Mac48Address m_peer;                           //!< Peer address
    bool m_amsduSupported;                         //!< Flag whether MSDU aggregation is supported
    uint8_t m_blockAckPolicy;                      //!< Type of block ack: immediate or delayed
    uint8_t m_tid;                                 //!< Traffic ID
    uint16_t m_bufferSize;                         //!< Buffer size
    uint16_t m_timeout;                            //!< Timeout
    uint16_t m_startingSeq;                        //!< Starting sequence control
    uint16_t m_winEnd;                             //!< Ending sequence number
    bool m_htSupported;                            //!< Flag whether HT is supported
    std::optional<Mac48Address> m_gcrGroupAddress; //!< the optional GCR group address
    mutable EventId m_inactivityEvent;             //!< inactivity event
};

} // namespace ns3

#endif /* BLOCK_ACK_AGREEMENT_H */
