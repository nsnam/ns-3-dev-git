/*
 * Copyright (c) 2009 INRIA
 * Copyright (c) 2016 Universita' di Firenze (added echo fields)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef SEQ_TS_ECHO_HEADER_H
#define SEQ_TS_ECHO_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{
/**
 * @ingroup applications
 * @class SeqTsEchoHeader
 * @brief Packet header to carry sequence number and two timestamps
 *
 * The header is made of a 32bits sequence number followed by
 * two 64bits time stamps (Transmit and Receive).
 */
class SeqTsEchoHeader : public Header
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief constructor
     */
    SeqTsEchoHeader();

    /**
     * @param seq the sequence number
     */
    void SetSeq(uint32_t seq);

    /**
     * @return the sequence number
     */
    uint32_t GetSeq() const;

    /**
     * @return A time value set by the sender
     */
    Time GetTsValue() const;

    /**
     * @return A time value echoing the received timestamp
     */
    Time GetTsEchoReply() const;

    /**
     * @brief Set the sender's time value
     * @param ts Time value to set
     */
    void SetTsValue(Time ts);

    /**
     * @brief Upon SeqTsEchoHeader reception, the host answers via echoing
     * back the received timestamp
     * @param ts received timestamp. If not called, will contain 0
     */
    void SetTsEchoReply(Time ts);

    // Inherited
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint32_t m_seq;     //!< Sequence number
    Time m_tsValue;     //!< Sender's timestamp
    Time m_tsEchoReply; //!< Receiver's timestamp
};

} // namespace ns3

#endif /* SEQ_TS_ECHO_HEADER_H */
