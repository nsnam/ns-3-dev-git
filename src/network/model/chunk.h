/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#ifndef CHUNK_H
#define CHUNK_H

#include "buffer.h"

#include "ns3/object-base.h"

namespace ns3
{

/**
 * @ingroup packet
 *
 * @brief abstract base class for ns3::Header and ns3::Trailer
 */
class Chunk : public ObjectBase
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Deserialize the object from a buffer iterator
     *
     * This version of Deserialize can be used when the Chunk has a fixed
     * size.  It should not be called for variable-sized Chunk derived types
     * (but must be implemented, for historical reasons).
     *
     * @param start the buffer iterator
     * @returns the number of deserialized bytes
     */
    virtual uint32_t Deserialize(Buffer::Iterator start) = 0;

    /**
     * @brief Deserialize the object from a buffer iterator
     *
     * This version of Deserialize must be used when the Chunk has a variable
     * size, because the bounds of the Chunk may not be known at the point
     * of deserialization (e.g. a sequence of TLV fields).
     *
     * The size of the chunk should be start.GetDistanceFrom (end);
     *
     * @param start the starting point
     * @param end the ending point
     * @returns the number of deserialized bytes
     */
    virtual uint32_t Deserialize(Buffer::Iterator start, Buffer::Iterator end);

    /**
     * @brief Print the object contents
     * @param os the output stream
     */
    virtual void Print(std::ostream& os) const = 0;
};

} // namespace ns3

#endif /* CHUNK_H */
