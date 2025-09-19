/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Davide Magrin <magrin.davide@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef NS3_TEST_HDR_SERIALIZE_H
#define NS3_TEST_HDR_SERIALIZE_H

#include "ns3/buffer.h"
#include "ns3/test.h"

#include <algorithm>

namespace ns3
{

/**
 * Subclass of TestCase class adding the ability to test the serialization and
 * deserialization of a Header object.
 */
class HeaderSerializationTestCase : public TestCase
{
  protected:
    /**
     * @brief Constructor.
     *
     * @param [in] name The name of the new TestCase created
     */
    HeaderSerializationTestCase(std::string name)
        : TestCase(name)
    {
    }

  public:
    /**
     * Serialize the given header in a buffer, then create a new header by
     * deserializing from the buffer and serialize the new header into a new buffer.
     * Verify that the two buffers have the same size and the same content.
     *
     * @tparam T \deduced Type of the given header
     * @tparam Args \deduced Type of arguments to pass to the constructor of the header
     * @param [in] hdr the header to test
     * @param [in] args the arguments to construct the new header
     * @return the new header obtained by deserializing from the buffer in which the given header
     *         is serialized
     */
    template <typename T, typename... Args>
    T TestHeaderSerialization(const T& hdr, Args&&... args);
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <typename T, typename... Args>
T
HeaderSerializationTestCase::TestHeaderSerialization(const T& hdr, Args&&... args)
{
    Buffer buffer;
    buffer.AddAtStart(hdr.GetSerializedSize());
    hdr.Serialize(buffer.Begin());

    T otherHdr(std::forward<Args>(args)...);
    otherHdr.Deserialize(buffer.Begin());
    Buffer otherBuffer;
    otherBuffer.AddAtStart(otherHdr.GetSerializedSize());
    otherHdr.Serialize(otherBuffer.Begin());

    NS_TEST_EXPECT_MSG_EQ(buffer.GetSize(), otherBuffer.GetSize(), "Size of buffers differs");

    auto bufferIterator = buffer.Begin();
    auto otherBufferIterator = otherBuffer.Begin();
    for (uint32_t j = 0; j < std::min(buffer.GetSize(), otherBuffer.GetSize()); j++)
    {
        const auto bufferVal = bufferIterator.ReadU8();
        const auto otherBufferVal = otherBufferIterator.ReadU8();
        NS_TEST_EXPECT_MSG_EQ(+bufferVal,
                              +otherBufferVal,
                              "Serialization -> Deserialization -> Serialization "
                                  << "is different than Serialization at byte " << j);
    }

    return otherHdr;
}

} // namespace ns3

#endif /* NS3_TEST_HDR_SERIALIZE_H */
