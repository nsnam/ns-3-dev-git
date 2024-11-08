/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <davide@magr.in>
 */

#include "ns3/assert.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/tim.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PowerSaveTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test TIM Information element serialization and deserialization
 */
class TimInformationElementTest : public HeaderSerializationTestCase
{
  public:
    /**
     * @brief Constructor
     */
    TimInformationElementTest();

    void DoRun() override;
    /**
     * Reset the passed TIM to have the provided parameters.
     *
     * @param tim the TIM element to set
     * @param dtimCount the DTIM count value
     * @param dtimPeriod the DTIM period value
     * @param multicastPending whether group addressed frames are queued
     * @param aidValues the AID values to set
     */
    void SetTim(Tim& tim,
                uint8_t dtimCount,
                uint8_t dtimPeriod,
                bool multicastPending,
                const std::list<uint16_t>& aidValues);

    /**
     * Test that the Bitmap Control and the Partial Virtual Bitmap
     * fields of the provided TIM match the passed bufferContents.
     *
     * @param tim the provided TIM
     * @param bufferContents the expected content of the buffer
     */
    void CheckSerializationAgainstBuffer(Tim& tim, const std::vector<uint8_t>& bufferContents);

    /**
     * Test that the GetAidSet() method return the expected set of AID values.
     *
     * @param tim the TIM element
     * @param aid the AID value passed to GetAidSet()
     * @param expectedSet the expected set of AID values returned by GetAidSet()
     */
    void CheckAidSet(const Tim& tim, uint16_t aid, const std::set<uint16_t>& expectedSet);
};

TimInformationElementTest::TimInformationElementTest()
    : HeaderSerializationTestCase("Test for the TIM Information Element implementation")
{
}

void
TimInformationElementTest::SetTim(Tim& tim,
                                  uint8_t dtimCount,
                                  uint8_t dtimPeriod,
                                  bool multicastPending,
                                  const std::list<uint16_t>& aidValues)
{
    tim = Tim();
    tim.m_dtimCount = dtimCount;
    tim.m_dtimPeriod = dtimPeriod;
    tim.m_hasMulticastPending = multicastPending;
    tim.AddAid(aidValues.begin(), aidValues.end());
}

void
TimInformationElementTest::CheckSerializationAgainstBuffer(
    Tim& tim,
    const std::vector<uint8_t>& bufferContents)
{
    // Serialize the TIM
    Buffer buffer;
    buffer.AddAtStart(tim.GetSerializedSize());
    tim.Serialize(buffer.Begin());

    // Check the two Buffer instances
    Buffer::Iterator bufferIterator = buffer.Begin();
    for (uint32_t j = 0; j < buffer.GetSize(); j++)
    {
        // We skip the first four bytes, since they contain known information
        if (j > 3)
        {
            NS_TEST_EXPECT_MSG_EQ(bufferIterator.ReadU8(),
                                  bufferContents.at(j - 4),
                                  "Serialization is different than provided known serialization");
        }
        else
        {
            // Advance the serialized buffer, which also contains
            // the Element ID, Length, DTIM Count, DTIM Period fields
            bufferIterator.ReadU8();
        }
    }
}

void
TimInformationElementTest::CheckAidSet(const Tim& tim,
                                       uint16_t aid,
                                       const std::set<uint16_t>& expectedSet)
{
    auto ret = tim.GetAidSet(aid);

    {
        std::vector<uint16_t> diff;

        // Expected set minus returned set provides expected elements that are not returned
        std::set_difference(expectedSet.cbegin(),
                            expectedSet.cend(),
                            ret.cbegin(),
                            ret.cend(),
                            std::back_inserter(diff));

        std::stringstream ss;
        std::copy(diff.cbegin(), diff.cend(), std::ostream_iterator<uint16_t>(ss, " "));

        NS_TEST_EXPECT_MSG_EQ(diff.size(),
                              0,
                              "Expected elements not returned by GetAidSet(): " << ss.str());
    }
    {
        std::vector<uint16_t> diff;

        // Returned set minus expected set provides returned elements that are not expected
        std::set_difference(ret.cbegin(),
                            ret.cend(),
                            expectedSet.cbegin(),
                            expectedSet.cend(),
                            std::back_inserter(diff));

        std::stringstream ss;
        std::copy(diff.cbegin(), diff.cend(), std::ostream_iterator<uint16_t>(ss, " "));

        NS_TEST_EXPECT_MSG_EQ(diff.size(),
                              0,
                              "Returned elements not expected by GetAidSet(): " << ss.str());
    }
}

void
TimInformationElementTest::DoRun()
{
    Tim tim;

    // The first three examples from 802.11-2020, Annex L
    //
    // 1. No group addressed MSDUs, but there is traffic for STAs with AID 2 and AID 7
    SetTim(tim, 0, 3, false, {2, 7});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000000, 0b10000100});
    CheckAidSet(tim, 0, {2, 7});
    CheckAidSet(tim, 1, {2, 7});
    CheckAidSet(tim, 2, {7});
    CheckAidSet(tim, 7, {});
    //
    // 2. There are group addressed MSDUs, DTIM count = 0, the nodes
    // with AID 2, 7, 22, and 24 have data buffered in the AP
    SetTim(tim, 0, 3, true, {2, 7, 22, 24});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim,
                                    {
                                        0b00000001,
                                        // NOTE The following byte is different from the example
                                        // in the standard. This is because the example sets the
                                        // AID 0 bit in the partial virtual bitmap to 1. Our code
                                        // and the example code provided in the Annex, instead, do
                                        // not set this bit. Relevant Note from 802.11-2020,
                                        // Section 9.4.2.5.1: "The bit numbered 0 in the traffic
                                        // indication virtual bitmap need not be included in the
                                        // Partial Virtual Bitmap field even if that bit is set."
                                        0b10000100,
                                        0b00000000,
                                        0b01000000,
                                        0b00000001,
                                    });
    CheckAidSet(tim, 0, {2, 7, 22, 24});
    CheckAidSet(tim, 2, {7, 22, 24});
    CheckAidSet(tim, 7, {22, 24});
    CheckAidSet(tim, 22, {24});
    CheckAidSet(tim, 24, {});
    //
    // 3. There are group addressed MSDUs, DTIM count = 0, only the node
    // with AID 24 has data buffered in the AP
    SetTim(tim, 0, 3, true, {24});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000011, 0b00000000, 0b00000001});

    // Other arbitrary examples just to make sure
    // Serialization -> Deserialization -> Serialization works
    SetTim(tim, 0, 3, false, {2000});
    TestHeaderSerialization(tim);
    SetTim(tim, 1, 3, true, {1, 134});
    TestHeaderSerialization(tim);
    SetTim(tim, 1, 3, false, {1, 2});
    TestHeaderSerialization(tim);

    // Edge cases
    //
    // What if there is group addressed data only?
    //
    // In this case, we should still have an empty byte in the Partial Virtual Bitmap.
    // From 802.11-2020: in the event that all bits other than bit 0 in the traffic indication
    // virtual bitmap are 0, the Partial Virtual Bitmap field is encoded as a single octet
    // equal to 0, the Bitmap Offset subfield is 0, and the Length field is 4.
    SetTim(tim, 0, 3, true, {});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000001, 0b00000000});
    NS_TEST_EXPECT_MSG_EQ(tim.GetSerializedSize() - 2, 4, "Unexpected TIM Length");
    //
    // What if there is no group addressed data and no unicast data?
    //
    // From 802.11-2020: When the TIM is carried in a non-S1G PPDU, in the event that all bits
    // other than bit 0 in the traffic indication virtual bitmap are 0, the Partial Virtual Bitmap
    // field is encoded as a single octet equal to 0, the Bitmap Offset subfield is 0, and the
    // Length field is 4.
    SetTim(tim, 0, 3, false, {});
    TestHeaderSerialization(tim);
    CheckSerializationAgainstBuffer(tim, {0b00000000, 0b00000000});
    NS_TEST_EXPECT_MSG_EQ(tim.GetSerializedSize() - 2, 4, "Unexpected TIM Length");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Power Save Test Suite
 */
class PowerSaveTestSuite : public TestSuite
{
  public:
    PowerSaveTestSuite();
};

PowerSaveTestSuite::PowerSaveTestSuite()
    : TestSuite("wifi-power-save", Type::UNIT)
{
    AddTestCase(new TimInformationElementTest, TestCase::Duration::QUICK);
}

static PowerSaveTestSuite g_powerSaveTestSuite; ///< the test suite
