/*
 * Copyright (c) 2026 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef BLOCK_ACK_PARAMETER_SET_H
#define BLOCK_ACK_PARAMETER_SET_H

#include "ns3/buffer.h"
#include "ns3/wifi-export.h"

namespace ns3
{

/**
 * Block Ack Parameter Set (IEEE 802.11-2024 9.4.1.13)
 */
struct WIFI_EXPORT BlockAckParameterSet
{
    /**
     * @returns the expected size of the Block Ack Parameter Set field in bytes
     *
     * This method should return the number of bytes which are needed to store
     * the Block Ack Parameter Set data by Serialize.
     */
    uint32_t GetSerializedSize() const;

    /**
     * @param start an iterator which points to where the Block Ack Parameter Set should be written
     *
     * @return Buffer::Iterator
     *
     * This method is used to store a Block Ack Parameter Set into the byte buffer.
     * The data written is expected to match bit-for-bit the representation of this
     * header in a real network.
     */
    Buffer::Iterator Serialize(Buffer::Iterator start) const;

    /**
     * @param start an iterator which points to where the Block Ack Parameter Set should be read.
     *
     * @returns the number of bytes read.
     *
     * This method is used to re-create a Block Ack Parameter Set from the byte buffer.
     * The data read is expected to match bit-for-bit the representation of this
     * header in real networks.
     */
    Buffer::Iterator Deserialize(Buffer::Iterator start);

    bool m_amsduSupport{true};      //!< A-MSDU supported subfield
    uint8_t m_policy : 1 {1};       //!< Block Ack Policy subfield
    uint8_t m_tid : 4 {0};          //!< TID subfield
    uint16_t m_bufferSize : 10 {0}; //!< Buffer Size subfield

    /**
     * @brief Equality operator for Block Ack Parameter Set
     * @param rhs right hand side
     * @return true if the Block Ack Parameter Sets are equal, false otherwise
     */
    bool operator==(const BlockAckParameterSet& rhs) const = default;
};

/**
 * Serialize Block Ack Parameter Set to the given ostream.
 *
 * @param os the output stream
 * @param params the Block Ack Parameter Set to serialize
 *
 * @return std::ostream
 */
WIFI_EXPORT std::ostream& operator<<(std::ostream& os, const BlockAckParameterSet& params);

} // namespace ns3

#endif /* BLOCK_ACK_PARAMETER_SET_H */
