/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef TRAILER_H
#define TRAILER_H

#include "buffer.h"
#include "chunk.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup packet
 *
 * @brief Protocol trailer serialization and deserialization.
 *
 * Every Protocol trailer which needs to be inserted or removed
 * from a Packet instance must derive from this base class and
 * implement the pure virtual methods defined here.
 */
class Trailer : public Chunk
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ~Trailer() override;
    /**
     * @returns the expected size of the trailer.
     *
     * This method is used by Packet::AddTrailer
     * to store a trailer into the byte buffer of a packet. This method
     * should return the number of bytes which are needed to store
     * the full trailer data by Serialize.
     */
    virtual uint32_t GetSerializedSize() const = 0;
    /**
     * @param start an iterator which points to where the trailer
     *        should be written.
     *
     * This method is used by Packet::AddTrailer to
     * store a header into the byte buffer of a packet.
     * The data written is expected to match bit-for-bit the
     * representation of this trailer in real networks.
     * The input iterator points to the end of the area where the
     * data shall be written. This method is thus expected to call
     * Buffer::Iterator::Prev prior to actually writing any data.
     */
    virtual void Serialize(Buffer::Iterator start) const = 0;
    /**
     * @param end an iterator which points to the end of the buffer
     *        where the trailer should be read from.
     * @returns the number of bytes read.
     *
     * This method is used by Packet::RemoveTrailer to
     * re-create a trailer from the byte buffer of a packet.
     * The data read is expected to match bit-for-bit the
     * representation of this trailer in real networks.
     * The input iterator points to the end of the area where the
     * data shall be read from. This method is thus expected to call
     * Buffer::Iterator::Prev prior to actually reading any data.
     */
    uint32_t Deserialize(Buffer::Iterator end) override = 0;
    /**
     * @param start an iterator which points to the start of the buffer
     *        where the trailer should be read from.
     * @param end an iterator which points to the end of the buffer
     *        where the trailer should be read from.
     * @returns the number of bytes read.
     *
     * This method is used by Packet::RemoveTrailer to
     * re-create a trailer from the byte buffer of a packet.
     * The data read is expected to match bit-for-bit the
     * representation of this trailer in real networks.
     * The input iterator end points to the end of the area where the
     * data shall be read from.
     *
     * This variant should be provided by any variable-sized trailer subclass
     * (i.e. if GetSerializedSize () does not return a constant).
     */
    uint32_t Deserialize(Buffer::Iterator start, Buffer::Iterator end) override;
    /**
     * @param os output stream
     * This method is used by Packet::Print to print the
     * content of a trailer as ascii data to a c++ output stream.
     * Although the trailer is free to format its output as it
     * wishes, it is recommended to follow a few rules to integrate
     * with the packet pretty printer: start with flags, small field
     * values located between a pair of parens. Values should be separated
     * by whitespace. Follow the parens with the important fields,
     * separated by whitespace.
     * i.e.: (field1 val1 field2 val2 field3 val3) field4 val4 field5 val5
     */
    void Print(std::ostream& os) const override = 0;
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param trailer the trailer
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const Trailer& trailer);

} // namespace ns3

#endif /* TRAILER_H */
