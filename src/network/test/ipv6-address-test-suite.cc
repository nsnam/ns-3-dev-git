/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/ipv6-address.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * Ipv6Address unit tests.
 */
class Ipv6AddressTestCase : public TestCase
{
  public:
    Ipv6AddressTestCase();
    ~Ipv6AddressTestCase() override;

  private:
    void DoRun() override;
};

Ipv6AddressTestCase::Ipv6AddressTestCase()
    : TestCase("serialization code")
{
}

Ipv6AddressTestCase::~Ipv6AddressTestCase()
{
}

void
Ipv6AddressTestCase::DoRun()
{
    Ipv6Address ip = Ipv6Address("2001:db8::1");
    uint8_t ipBytes[16];
    ip.Serialize(ipBytes);
    NS_TEST_ASSERT_MSG_EQ(ipBytes[0], 0x20, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[1], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[2], 0x0d, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[3], 0xb8, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[15], 1, "Failed string conversion");

    ip = Ipv6Address("2001:db8:1::1");
    ip.Serialize(ipBytes);
    NS_TEST_ASSERT_MSG_EQ(ipBytes[0], 0x20, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[1], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[2], 0x0d, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[3], 0xb8, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[5], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[15], 1, "Failed string conversion");

    // Zero padding
    ip = Ipv6Address("2001:0db8:0001::1");
    ip.Serialize(ipBytes);
    NS_TEST_ASSERT_MSG_EQ(ipBytes[0], 0x20, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[1], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[2], 0x0d, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[3], 0xb8, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[5], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[15], 1, "Failed string conversion");

    ip = Ipv6Address("2001:db8:0:1::1");
    ip.Serialize(ipBytes);
    NS_TEST_ASSERT_MSG_EQ(ipBytes[0], 0x20, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[1], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[2], 0x0d, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[3], 0xb8, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[7], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[15], 1, "Failed string conversion");

    ip = Ipv6Address("2001:db8:0:1:0:0:0:1");
    ip.Serialize(ipBytes);
    NS_TEST_ASSERT_MSG_EQ(ipBytes[0], 0x20, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[1], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[2], 0x0d, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[3], 0xb8, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[7], 0x01, "Failed string conversion");
    NS_TEST_ASSERT_MSG_EQ(ipBytes[15], 1, "Failed string conversion");

    // Please add more tests below
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Ipv6Address TestSuite
 *
 */
class Ipv6AddressTestSuite : public TestSuite
{
  public:
    Ipv6AddressTestSuite();
};

Ipv6AddressTestSuite::Ipv6AddressTestSuite()
    : TestSuite("ipv6-address", Type::UNIT)
{
    AddTestCase(new Ipv6AddressTestCase, TestCase::Duration::QUICK);
}

static Ipv6AddressTestSuite ipv6AddressTestSuite; //!< Static variable for test initialization
