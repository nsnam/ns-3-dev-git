/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */
#include "ns3/aodv-id-cache.h"
#include "ns3/test.h"

namespace ns3
{
namespace aodv
{

/**
 * @defgroup aodv-test AODV module tests
 * @ingroup aodv
 * @ingroup tests
 */

/**
 * @ingroup aodv-test
 *
 * @brief Unit test for id cache
 */
class IdCacheTest : public TestCase
{
  public:
    IdCacheTest()
        : TestCase("Id Cache"),
          cache(Seconds(10))
    {
    }

    void DoRun() override;

  private:
    /// Timeout test function #1
    void CheckTimeout1();
    /// Timeout test function #2
    void CheckTimeout2();
    /// Timeout test function #3
    void CheckTimeout3();

    /// ID cache
    IdCache cache;
};

void
IdCacheTest::DoRun()
{
    NS_TEST_EXPECT_MSG_EQ(cache.GetLifeTime(), Seconds(10), "Lifetime");
    NS_TEST_EXPECT_MSG_EQ(cache.IsDuplicate(Ipv4Address("1.2.3.4"), 3),
                          false,
                          "Unknown ID & address");
    NS_TEST_EXPECT_MSG_EQ(cache.IsDuplicate(Ipv4Address("1.2.3.4"), 4), false, "Unknown ID");
    NS_TEST_EXPECT_MSG_EQ(cache.IsDuplicate(Ipv4Address("4.3.2.1"), 3), false, "Unknown address");
    NS_TEST_EXPECT_MSG_EQ(cache.IsDuplicate(Ipv4Address("1.2.3.4"), 3), true, "Known address & ID");
    cache.SetLifetime(Seconds(15));
    NS_TEST_EXPECT_MSG_EQ(cache.GetLifeTime(), Seconds(15), "New lifetime");
    cache.IsDuplicate(Ipv4Address("1.1.1.1"), 4);
    cache.IsDuplicate(Ipv4Address("1.1.1.1"), 4);
    cache.IsDuplicate(Ipv4Address("2.2.2.2"), 5);
    cache.IsDuplicate(Ipv4Address("3.3.3.3"), 6);
    NS_TEST_EXPECT_MSG_EQ(cache.GetSize(), 6, "trivial");

    Simulator::Schedule(Seconds(5), &IdCacheTest::CheckTimeout1, this);
    Simulator::Schedule(Seconds(11), &IdCacheTest::CheckTimeout2, this);
    Simulator::Schedule(Seconds(30), &IdCacheTest::CheckTimeout3, this);
    Simulator::Run();
    Simulator::Destroy();
}

void
IdCacheTest::CheckTimeout1()
{
    NS_TEST_EXPECT_MSG_EQ(cache.GetSize(), 6, "Nothing expire");
}

void
IdCacheTest::CheckTimeout2()
{
    NS_TEST_EXPECT_MSG_EQ(cache.GetSize(), 3, "3 records left");
}

void
IdCacheTest::CheckTimeout3()
{
    NS_TEST_EXPECT_MSG_EQ(cache.GetSize(), 0, "All records expire");
}

/**
 * @ingroup aodv-test
 *
 * @brief Id Cache Test Suite
 */
class IdCacheTestSuite : public TestSuite
{
  public:
    IdCacheTestSuite()
        : TestSuite("aodv-routing-id-cache", Type::UNIT)
    {
        AddTestCase(new IdCacheTest, TestCase::Duration::QUICK);
    }
} g_idCacheTestSuite; ///< the test suite

} // namespace aodv
} // namespace ns3
