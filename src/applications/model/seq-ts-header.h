/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef SEQ_TS_HEADER_H
#define SEQ_TS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{
/**
 * @ingroup applications
 *
 * @brief Packet header to carry sequence number and timestamp
 *
 * The header is used as a payload in applications (typically UDP) to convey
 * a 32 bit sequence number followed by a 64 bit timestamp (12 bytes total).
 *
 * The timestamp is not set explicitly but automatically set to the
 * simulation time upon creation.
 *
 * If you need space for an application data unit size field (e.g. for
 * stream-based protocols like TCP), use ns3::SeqTsSizeHeader.
 *
 * \sa ns3::SeqTsSizeHeader
 */
class SeqTsHeader : public Header
{
  public:
    SeqTsHeader();

    /**
     * @param seq the sequence number
     */
    void SetSeq(uint32_t seq);
    /**
     * @return the sequence number
     */
    uint32_t GetSeq() const;
    /**
     * @return the time stamp
     */
    Time GetTs() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint32_t m_seq; //!< Sequence number
    uint64_t m_ts;  //!< Timestamp
};

} // namespace ns3

#endif /* SEQ_TS_HEADER_H */
