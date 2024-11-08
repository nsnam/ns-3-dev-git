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
     */
    template <typename T, typename... Args>
    void TestHeaderSerialization(const T& hdr, Args&&... args);
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <typename T, typename... Args>
void
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

    NS_TEST_ASSERT_MSG_EQ(buffer.GetSize(), otherBuffer.GetSize(), "Size of buffers differs");

    Buffer::Iterator bufferIterator = buffer.Begin();
    Buffer::Iterator otherBufferIterator = otherBuffer.Begin();
    for (uint32_t j = 0; j < buffer.GetSize(); j++)
    {
        uint8_t bufferVal = bufferIterator.ReadU8();
        uint8_t otherBufferVal = otherBufferIterator.ReadU8();
        NS_TEST_EXPECT_MSG_EQ(+bufferVal,
                              +otherBufferVal,
                              "Serialization -> Deserialization -> Serialization "
                                  << "is different than Serialization at byte " << j);
    }
}

} // namespace ns3

#endif /* NS3_TEST_HDR_SERIALIZE_H */
