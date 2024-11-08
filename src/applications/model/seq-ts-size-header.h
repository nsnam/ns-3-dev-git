/*
 *   Copyright (c) 2009 INRIA
 *   Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
 *                      (added timestamp and size fields)
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef SEQ_TS_SIZE_HEADER_H
#define SEQ_TS_SIZE_HEADER_H

#include "seq-ts-header.h"

namespace ns3
{
/**
 * @ingroup applications
 * @brief Header with a sequence, a timestamp, and a "size" attribute
 *
 * This header adds a size attribute to the sequence number and timestamp
 * of class \c SeqTsHeader.  The size attribute can be used to track
 * application data units for stream-based sockets such as TCP.
 *
 * \sa ns3::SeqTsHeader
 */
class SeqTsSizeHeader : public SeqTsHeader
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
    SeqTsSizeHeader();

    /**
     * @brief Set the size information that the header will carry
     * @param size the size
     */
    void SetSize(uint64_t size);

    /**
     * @brief Get the size information that the header is carrying
     * @return the size
     */
    uint64_t GetSize() const;

    // Inherited
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint64_t m_size{0}; //!< The 'size' information that the header is carrying
};

} // namespace ns3

#endif /* SEQ_TS_SIZE_HEADER */
