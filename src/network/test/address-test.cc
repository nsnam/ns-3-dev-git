/*
 * Copyright (c) 2025 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/address.h"
#include "ns3/test.h"

#include <map>
#include <string>

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Address classes Test
 */
class AddressTest : public TestCase
{
  public:
    void DoRun() override;
    AddressTest();
};

AddressTest::AddressTest()
    : TestCase("Address class implementation")
{
}

void
AddressTest::DoRun()
{
    Address addrA;
    Address addrB;
    Address addrC;

    // tests comparison of non-initialized addresses
    NS_TEST_EXPECT_MSG_EQ(addrA == addrB, true, "Non-initialized addresses must be equal");

    uint8_t bufferA[10]{1, 8, 0, 0, 0, 0, 0, 0, 0, 1};
    uint8_t bufferB[10]{1, 8, 0, 0, 0, 0, 0, 0, 0, 2};
    uint8_t bufferC[10]{1, 8, 0, 0, 0, 0, 0, 0, 0, 3};

    addrA.CopyAllFrom(bufferA, 10);
    addrB.CopyAllFrom(bufferB, 10);
    addrC.CopyAllFrom(bufferC, 10);

    // tests comparison of initialized addresses
    NS_TEST_EXPECT_MSG_EQ(addrA < addrB, true, "Initialized addresses must be different, A < B");
    NS_TEST_EXPECT_MSG_EQ(addrA < addrC, true, "Initialized addresses must be different, A < C");
    NS_TEST_EXPECT_MSG_EQ(addrB < addrC, true, "Initialized addresses must be different, B < C");

    std::map<Address, int8_t> theMap;
    theMap.insert({addrC, 2});
    theMap.insert({addrB, 1});
    theMap.insert({addrA, 0});

    int8_t test = -1;
    for (auto [k, v] : theMap)
    {
        NS_TEST_EXPECT_MSG_EQ(v == test + 1, true, "Address map sorting must be correct");
        test = v;
    }
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Address TestSuite
 */
class AddressTestSuite : public TestSuite
{
  public:
    AddressTestSuite();

  private:
};

AddressTestSuite::AddressTestSuite()
    : TestSuite("address", Type::UNIT)
{
    AddTestCase(new AddressTest(), TestCase::Duration::QUICK);
}

static AddressTestSuite g_addressTestSuite; //!< Static variable for test initialization
