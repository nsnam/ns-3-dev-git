/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/wifi-information-element.h"

#include <list>
#include <numeric>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiIeFragmentTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * Subelement to test fragmentation. Its content is a sequence of bytes
 * of configurable size.
 */
class TestWifiSubElement : public WifiInformationElement
{
  public:
    TestWifiSubElement() = default;

    /**
     * Construct a test subelement containing a sequence of bytes of the given
     * size and with the given initial value.
     *
     * \param count the number of bytes to append
     * \param start the initial value for the sequence of bytes to add
     */
    TestWifiSubElement(uint16_t count, uint8_t start);

    WifiInformationElementId ElementId() const override;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    std::list<uint8_t> m_content; ///< content of the IE
};

TestWifiSubElement::TestWifiSubElement(uint16_t count, uint8_t start)
{
    NS_LOG_FUNCTION(this << count << +start);
    m_content.resize(count);
    std::iota(m_content.begin(), m_content.end(), start);
}

WifiInformationElementId
TestWifiSubElement::ElementId() const
{
    return 0;
}

uint16_t
TestWifiSubElement::GetInformationFieldSize() const
{
    return m_content.size();
}

void
TestWifiSubElement::SerializeInformationField(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    for (const auto& byte : m_content)
    {
        start.WriteU8(byte);
    }
}

uint16_t
TestWifiSubElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    NS_LOG_FUNCTION(this << length);
    m_content.clear();
    for (uint16_t i = 0; i < length; i++)
    {
        m_content.push_back(start.ReadU8());
    }
    return length;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * Information Element to test IE fragmentation. Its content is one or more
 * test subelements.
 */
class TestWifiInformationElement : public WifiInformationElement
{
  public:
    /**
     * Constructor
     * \param extended whether this IE includes an Element ID Extension field
     */
    TestWifiInformationElement(bool extended);

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    /**
     * Append the given subelement.
     *
     * \param subelement the subelement to append
     */
    void AddSubelement(TestWifiSubElement&& subelement);

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    bool m_extended;                         ///< whether this IE has an Element ID Extension field
    std::list<TestWifiSubElement> m_content; ///< content of the IE
};

TestWifiInformationElement::TestWifiInformationElement(bool extended)
    : m_extended(extended)
{
    NS_LOG_FUNCTION(this << extended);
}

WifiInformationElementId
TestWifiInformationElement::ElementId() const
{
    return m_extended ? 255 : 2; // reserved in 802.11-2020
}

WifiInformationElementId
TestWifiInformationElement::ElementIdExt() const
{
    NS_ABORT_IF(!m_extended);
    return 32; // reserved in 802.11-2020
}

void
TestWifiInformationElement::AddSubelement(TestWifiSubElement&& subelement)
{
    NS_LOG_FUNCTION(this);
    m_content.push_back(std::move(subelement));
}

uint16_t
TestWifiInformationElement::GetInformationFieldSize() const
{
    uint16_t size = (m_extended ? 1 : 0);
    for (const auto& subelement : m_content)
    {
        size += subelement.GetSerializedSize();
    }
    return size;
}

void
TestWifiInformationElement::SerializeInformationField(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    for (const auto& subelement : m_content)
    {
        start = subelement.Serialize(start);
    }
}

uint16_t
TestWifiInformationElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    NS_LOG_FUNCTION(this << length);

    Buffer::Iterator i = start;
    uint16_t count = 0;

    while (count < length)
    {
        TestWifiSubElement subelement;
        i = subelement.Deserialize(i);
        m_content.push_back(std::move(subelement));
        count = i.GetDistanceFrom(start);
    }
    return count;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * Test header that can contain multiple test information elements.
 */
class TestHeader : public Header
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Append the given wifi test information element.
     *
     * \param element the given wifi test information element
     */
    void AddTestIe(TestWifiInformationElement&& element);

  private:
    std::list<TestWifiInformationElement> m_elements; ///< test information elements
};

NS_OBJECT_ENSURE_REGISTERED(TestHeader);

ns3::TypeId
TestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<TestHeader>();
    return tid;
}

TypeId
TestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TestHeader::Print(std::ostream& os) const
{
}

uint32_t
TestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    for (const auto& elem : m_elements)
    {
        size += elem.GetSerializedSize();
    }
    return size;
}

void
TestHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    for (const auto& elem : m_elements)
    {
        start = elem.Serialize(start);
    }
}

uint32_t
TestHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    m_elements.clear();

    while (!i.IsEnd())
    {
        std::optional<TestWifiInformationElement> elem;
        i = WifiInformationElement::DeserializeIfPresent(elem, i, true);
        if (!elem.has_value())
        {
            // try deserializing the test IE without Element ID Extension field
            i = WifiInformationElement::DeserializeIfPresent(elem, i, false);
        }
        if (elem.has_value())
        {
            m_elements.push_back(std::move(*elem));
        }
    }
    return i.GetDistanceFrom(start);
}

void
TestHeader::AddTestIe(TestWifiInformationElement&& element)
{
    NS_LOG_FUNCTION(this);
    m_elements.push_back(std::move(element));
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test fragmentation of Information Elements
 */
class WifiIeFragmentationTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     * \param extended whether this IE includes an Element ID Extension field
     */
    WifiIeFragmentationTest(bool extended);
    ~WifiIeFragmentationTest() override = default;

    /**
     * Serialize the given element in a buffer.
     *
     * \param element the given element
     * \return the buffer in which the given element has been serialized
     */
    Buffer SerializeIntoBuffer(const WifiInformationElement& element);

    /**
     * Check that the given buffer contains the given value at the given position.
     *
     * \param buffer the given buffer
     * \param position the given position (starting at 0)
     * \param value the given value
     */
    void CheckSerializedByte(const Buffer& buffer, uint32_t position, uint8_t value);

  private:
    void DoRun() override;

    bool m_extended; //!< whether the IE includes an Element ID Extension field
};

WifiIeFragmentationTest ::WifiIeFragmentationTest(bool extended)
    : HeaderSerializationTestCase("Check fragmentation of Information Elements"),
      m_extended(extended)
{
}

Buffer
WifiIeFragmentationTest::SerializeIntoBuffer(const WifiInformationElement& element)
{
    Buffer buffer;
    buffer.AddAtStart(element.GetSerializedSize());
    element.Serialize(buffer.Begin());
    return buffer;
}

void
WifiIeFragmentationTest::CheckSerializedByte(const Buffer& buffer, uint32_t position, uint8_t value)
{
    Buffer::Iterator it = buffer.Begin();
    it.Next(position);
    uint8_t byte = it.ReadU8();
    NS_TEST_EXPECT_MSG_EQ(+byte, +value, "Unexpected byte at pos=" << position);
}

void
WifiIeFragmentationTest::DoRun()
{
    // maximum IE size to avoid incurring IE fragmentation
    uint16_t limit = m_extended ? 254 : 255;

    TestHeader header;

    /*
     * Add an IE (containing 2 subelements). No fragmentation occurs
     */

    uint16_t sub01Size = 50;
    uint16_t sub02Size = limit - sub01Size;

    auto sub01 =
        TestWifiSubElement(sub01Size - 2, 53); // minus 2 to account for Subelement ID and Length
    auto sub02 = TestWifiSubElement(sub02Size - 2, 26);

    auto testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));
    testIe.AddSubelement(std::move(sub02));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // element length is the maximum length
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer, (m_extended ? 3 : 2) + 1, sub01Size - 2); // subelement 1 Length
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size,
                            TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 1,
                            sub02Size - 2); // subelement 2 Length
    }

    header.AddTestIe(std::move(testIe));
    uint32_t expectedHdrSize = 2 + 255;
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);

    /*
     * Add an IE (containing 2 subelements) that is fragmented into 2 fragments.
     * Subelements are not fragmented
     */
    sub01Size = 65;
    sub02Size = limit + 1 - sub01Size;

    sub01 =
        TestWifiSubElement(sub01Size - 2, 47); // minus 2 to account for Subelement ID and Length
    sub02 = TestWifiSubElement(sub02Size - 2, 71);

    testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));
    testIe.AddSubelement(std::move(sub02));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // maximum length for first element fragment
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer, (m_extended ? 3 : 2) + 1, sub01Size - 2); // subelement 1 Length
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size,
                            TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 1,
                            sub02Size - 2);                // subelement 2 Length
        CheckSerializedByte(buffer, 2 + 255, IE_FRAGMENT); // Fragment ID
        CheckSerializedByte(buffer,
                            2 + 255 + 1,
                            1); // the length of the second element fragment is 1
    }

    header.AddTestIe(std::move(testIe));
    expectedHdrSize += 2 + 255  // first fragment
                       + 2 + 1; // second fragment
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);

    /*
     * Add an IE (containing 3 subelements) that is fragmented into 2 fragments.
     * Subelements are not fragmented
     */
    sub01Size = 200;
    sub02Size = 200;
    uint16_t sub03Size = limit + 255 - sub01Size - sub02Size;

    sub01 =
        TestWifiSubElement(sub01Size - 2, 16); // minus 2 to account for Subelement ID and Length
    sub02 = TestWifiSubElement(sub02Size - 2, 83);
    auto sub03 = TestWifiSubElement(sub03Size - 2, 98);

    testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));
    testIe.AddSubelement(std::move(sub02));
    testIe.AddSubelement(std::move(sub03));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // maximum length for first element fragment
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer, (m_extended ? 3 : 2) + 1, sub01Size - 2); // subelement 1 Length
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size,
                            TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 1,
                            sub02Size - 2);                // subelement 2 Length
        CheckSerializedByte(buffer, 2 + 255, IE_FRAGMENT); // Fragment ID
        CheckSerializedByte(buffer, 2 + 255 + 1, 255); // maximum length for second element fragment
    }

    header.AddTestIe(std::move(testIe));
    expectedHdrSize += 2 + 255    // first fragment
                       + 2 + 255; // second fragment
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);

    /*
     * Add an IE (containing 3 subelements) that is fragmented into 3 fragments.
     * Subelements are not fragmented
     */
    sub01Size = 200;
    sub02Size = 200;
    sub03Size = limit + 255 + 1 - sub01Size - sub02Size;

    sub01 =
        TestWifiSubElement(sub01Size - 2, 20); // minus 2 to account for Subelement ID and Length
    sub02 = TestWifiSubElement(sub02Size - 2, 77);
    sub03 = TestWifiSubElement(sub03Size - 2, 14);

    testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));
    testIe.AddSubelement(std::move(sub02));
    testIe.AddSubelement(std::move(sub03));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // maximum length for first element fragment
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer, (m_extended ? 3 : 2) + 1, sub01Size - 2); // subelement 1 Length
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size,
                            TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 1,
                            sub02Size - 2);                // subelement 2 Length
        CheckSerializedByte(buffer, 2 + 255, IE_FRAGMENT); // Fragment ID
        CheckSerializedByte(buffer, 2 + 255 + 1, 255);     // maximum length for second fragment
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 2 + sub02Size,
                            TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + sub01Size + 2 + sub02Size + 1,
                            sub03Size - 2);                      // subelement 3 Length
        CheckSerializedByte(buffer, 2 * (2 + 255), IE_FRAGMENT); // Fragment ID
        CheckSerializedByte(buffer, 2 * (2 + 255) + 1, 1); // the length of the third fragment is 1
    }

    header.AddTestIe(std::move(testIe));
    expectedHdrSize += 2 + 255   // first fragment
                       + 2 + 255 // second fragment
                       + 2 + 1;  // third fragment
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);

    /*
     * Add an IE containing one subelement of the maximum size.
     * The IE is fragmented into 2 fragments.
     */
    sub01Size = 2 + 255;

    sub01 =
        TestWifiSubElement(sub01Size - 2, 47); // minus 2 to account for Subelement ID and Length

    testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // maximum length for first element fragment
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer, (m_extended ? 3 : 2) + 1, sub01Size - 2); // subelement 1 Length
        CheckSerializedByte(buffer, 2 + 255, IE_FRAGMENT);                    // Fragment ID
        CheckSerializedByte(buffer,
                            2 + 255 + 1,
                            (m_extended ? 3 : 2)); // length of the second element fragment
    }

    header.AddTestIe(std::move(testIe));
    expectedHdrSize += 2 + 255                     // first fragment
                       + 2 + (m_extended ? 3 : 2); // second fragment
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);

    /*
     * Add an IE containing one subelement that gets fragmented.
     * The IE is fragmented into 2 fragments as well.
     */
    sub01Size = 2 + 256;

    sub01 =
        TestWifiSubElement(sub01Size - 2, 84); // minus 2 to account for Subelement ID and Length

    testIe = TestWifiInformationElement(m_extended);
    testIe.AddSubelement(std::move(sub01));

    {
        Buffer buffer = SerializeIntoBuffer(testIe);
        CheckSerializedByte(buffer, 1, 255); // maximum length for first element fragment
        if (m_extended)
        {
            CheckSerializedByte(buffer, 2, testIe.ElementIdExt());
        }
        CheckSerializedByte(buffer, (m_extended ? 3 : 2), TestWifiSubElement().ElementId());
        CheckSerializedByte(buffer,
                            (m_extended ? 3 : 2) + 1,
                            255); // first subelement fragment Length
        CheckSerializedByte(buffer,
                            2 + 255,
                            IE_FRAGMENT); // Fragment ID for second element fragment
        // Subelement bytes in first element fragment: X = 255 - 1 (Ext ID, if any) - 1 (Sub ID) - 1
        // (Sub Length) Subelement bytes in second element fragment: Y = 256 - X = (m_extended ? 4 :
        // 3) Length of the second element fragment: Y + 2 (Fragment ID and Length for second
        // subelement fragment)
        CheckSerializedByte(buffer, 2 + 255 + 1, (m_extended ? 6 : 5));
        CheckSerializedByte(buffer,
                            2 + 255 + 2 + (m_extended ? 3 : 2),
                            IE_FRAGMENT); // Fragment ID for second subelement fragment
        CheckSerializedByte(buffer,
                            2 + 255 + 2 + (m_extended ? 3 : 2) + 1,
                            1); // Length for second subelement fragment
    }

    header.AddTestIe(std::move(testIe));
    expectedHdrSize += 2 + 255                     // first fragment
                       + 2 + (m_extended ? 6 : 5); // second fragment
    NS_TEST_EXPECT_MSG_EQ(header.GetSerializedSize(), expectedHdrSize, "Unexpected header size");
    TestHeaderSerialization(header);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi Information Element fragmentation Test Suite
 */
class WifiIeFragmentationTestSuite : public TestSuite
{
  public:
    WifiIeFragmentationTestSuite();
};

WifiIeFragmentationTestSuite::WifiIeFragmentationTestSuite()
    : TestSuite("wifi-ie-fragment", UNIT)
{
    AddTestCase(new WifiIeFragmentationTest(false), TestCase::QUICK);
    AddTestCase(new WifiIeFragmentationTest(true), TestCase::QUICK);
}

static WifiIeFragmentationTestSuite g_wifiIeFragmentationTestSuite; ///< the test suite
