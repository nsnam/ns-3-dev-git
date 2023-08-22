/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef BLOCK_ACK_TYPE_H
#define BLOCK_ACK_TYPE_H

#include <cstdint>
#include <ostream>
#include <vector>

namespace ns3
{

/**
 * @ingroup wifi
 * The different BlockAck variants.
 */
struct BlockAckType
{
    /**
     * @enum Variant
     * @brief The BlockAck variants
     */
    enum Variant : uint8_t
    {
        BASIC,
        COMPRESSED,
        EXTENDED_COMPRESSED,
        MULTI_TID,
        GCR,
        MULTI_STA
    };

    Variant m_variant;                //!< Block Ack variant
    std::vector<uint8_t> m_bitmapLen; //!< Length (bytes) of included bitmaps

    /**
     * Default constructor for BlockAckType.
     */
    BlockAckType();
    /**
     * Constructor for BlockAckType with given variant.
     *
     * @param v the Block Ack variant
     */
    BlockAckType(Variant v);
    /**
     * Constructor for BlockAckType with given variant
     * and bitmap length.
     *
     * @param v the Block Ack variant
     * @param l the length (bytes) of included bitmaps
     */
    BlockAckType(Variant v, std::vector<uint8_t> l);
};

/**
 * @ingroup wifi
 * The different BlockAckRequest variants.
 */
struct BlockAckReqType
{
    /**
     * @enum Variant
     * @brief The BlockAckReq variants
     */
    enum Variant : uint8_t
    {
        BASIC,
        COMPRESSED,
        EXTENDED_COMPRESSED,
        MULTI_TID,
        GCR
    };

    Variant m_variant;      //!< Block Ack Request variant
    uint8_t m_nSeqControls; //!< Number of included Starting Sequence Control fields.
                            //!< This member is added for future support of Multi-TID BARs

    /**
     * Default constructor for BlockAckReqType.
     */
    BlockAckReqType();
    /**
     * Constructor for BlockAckReqType with given variant.
     *
     * @param v the Block Ack Request variant
     */
    BlockAckReqType(Variant v);
    /**
     * Constructor for BlockAckReqType with given variant
     * and number of SSC fields.
     *
     * @param v the Block Ack Request variant
     * @param nSeqControls the number of included Starting Sequence Control fields
     */
    BlockAckReqType(Variant v, uint8_t nSeqControls);
};

/**
 * Serialize BlockAckType to ostream in a human-readable form.
 *
 * @param os std::ostream
 * @param type block ack type
 * @return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const BlockAckType& type);

/**
 * Serialize BlockAckReqType to ostream in a human-readable form.
 *
 * @param os std::ostream
 * @param type block ack request type
 * @return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const BlockAckReqType& type);

} // namespace ns3

#endif /* BLOCK_ACK_TYPE_H */
