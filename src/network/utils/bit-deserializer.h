/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef BITDESERIALIZER_H_
#define BITDESERIALIZER_H_

#include <cstdint>
#include <deque>
#include <vector>

namespace ns3
{

/**
 * @ingroup packet
 *
 * @brief Bit deserializer. See also \sa ns3::BitSerializer
 *
 * This class helps converting a variable number, variable sized
 * number of bit-boundary fields stored as byte array representation
 * and extract the original bit-fields.
 *
 * Note that once the Deserialization starts, it's not anymore
 * possible to add more data to the byte blob to deserialize.
 */

class BitDeserializer
{
  public:
    BitDeserializer();

    /**
     * Pushes some bytes into the blob to be deserialized.
     * @param bytes The bytes to add.
     */
    void PushBytes(std::vector<uint8_t> bytes);

    /**
     * Pushes some bytes into the blob to be deserialized.
     * @param bytes The bytes to add.
     * @param size The length of the array.
     */
    void PushBytes(uint8_t* bytes, uint32_t size);

    /**
     * Pushes one byte into the blob to be deserialized.
     * @param byte The byte to add.
     */
    void PushByte(uint8_t byte);

    /**
     * Pops a given number of bits from the blob front. In other terms,
     * 'size' bits are shifted from the contents of the byte blob
     * onto the return value.
     *
     * The maximum number of bits to be deserialized in one single call is 64.
     *
     * @param size The number of bits to pop.
     * @return The popped bits value
     */
    uint64_t GetBits(uint8_t size);

  private:
    /**
     * Prepare the byte array to the deserialization.
     */
    void PrepareDeserialization();

    std::deque<bool> m_blob;          //!< Blob of bits ready to be deserialized.
    std::vector<uint8_t> m_bytesBlob; //!< Blob of bytes to be deserialized.
    bool m_deserializing;             //!< True if the deserialization did start already.
};

} // namespace ns3

#endif /* BITDESERIALIZER_H_ */
