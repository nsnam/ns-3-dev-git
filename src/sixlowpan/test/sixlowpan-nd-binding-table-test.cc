/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#include "ns3/ipv6-address.h"
#include "ns3/log.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-nd-binding-table.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SixLowPanNdBindingTableTest");

/**
 * @ingroup sixlowpan-nd-binding-table-tests
 *
 * @brief 6LoWPAN-ND binding table test case for basic entry creation and manipulation
 */
class SixLowPanNdBindingTableBasicTest : public TestCase
{
  public:
    SixLowPanNdBindingTableBasicTest()
        : TestCase("SixLowPanNdBindingTable Basic Operations Test")
    {
    }

    void DoRun() override
    {
        // Create a binding table
        Ptr<SixLowPanNdBindingTable> bindingTable = CreateObject<SixLowPanNdBindingTable>();

        // Test IPv6 addresses
        Ipv6Address addr1("2001:db8::1");
        Ipv6Address addr2("2001:db8::2");
        Ipv6Address addr3("2001:db8::3");

        // Test 1: Add entries
        auto entry1 = bindingTable->Add(addr1);
        auto entry2 = bindingTable->Add(addr2);

        NS_TEST_ASSERT_MSG_NE(entry1, nullptr, "Failed to create first entry");
        NS_TEST_ASSERT_MSG_NE(entry2, nullptr, "Failed to create second entry");

        // Simple ROVR sanity check: set a canonical 16-byte ROVR and verify it is stored
        std::vector<uint8_t> rovr(16);
        for (uint8_t i = 0; i < 16; ++i)
        {
            rovr[i] = i;
        }
        entry1->SetRovr(rovr);
        auto gotRovr = entry1->GetRovr();
        NS_TEST_ASSERT_MSG_EQ(gotRovr.size(), rovr.size(), "ROVR size mismatch");
        NS_TEST_ASSERT_MSG_EQ(std::equal(gotRovr.begin(), gotRovr.end(), rovr.begin()),
                              true,
                              "ROVR content mismatch");

        // Test 2: Lookup entries
        auto foundEntry1 = bindingTable->Lookup(addr1);
        auto foundEntry2 = bindingTable->Lookup(addr2);
        auto notFoundEntry = bindingTable->Lookup(addr3);

        NS_TEST_ASSERT_MSG_EQ(foundEntry1, entry1, "Lookup should return the same entry1");
        NS_TEST_ASSERT_MSG_EQ(foundEntry2, entry2, "Lookup should return the same entry2");
        NS_TEST_ASSERT_MSG_EQ(notFoundEntry,
                              nullptr,
                              "Lookup for non-existent entry should return nullptr");

        // Test 3: Try to add duplicate entry (should return existing entry)
        auto duplicateEntry = bindingTable->Add(addr1);
        NS_TEST_ASSERT_MSG_EQ(duplicateEntry,
                              entry1,
                              "Adding duplicate should return existing entry");

        // Test 4: Mark entry as REACHABLE
        entry1->MarkReachable(10); // 10 minutes lifetime
        NS_TEST_ASSERT_MSG_EQ(entry1->IsReachable(), true, "Entry1 should be REACHABLE");
        NS_TEST_ASSERT_MSG_EQ(entry1->IsStale(), false, "Entry1 should not be STALE");

        // Test 5: Mark entry as STALE
        entry2->MarkStale();
        NS_TEST_ASSERT_MSG_EQ(entry2->IsStale(), true, "Entry2 should be STALE");
        NS_TEST_ASSERT_MSG_EQ(entry2->IsReachable(), false, "Entry2 should not be REACHABLE");

        // Cleanup
        Simulator::Destroy();
    }
};

/**
 * @ingroup sixlowpan-nd-binding-table-tests
 *
 * @brief Test REACHABLE to STALE state transition after timer expiry
 */
class SixLowPanNdBindingTableReachableToStaleTest : public TestCase
{
  public:
    SixLowPanNdBindingTableReachableToStaleTest()
        : TestCase("SixLowPanNdBindingTable REACHABLE to STALE Transition Test"),
          m_entryBecameStale(false)
    {
    }

    void DoRun() override
    {
        // Create a binding table with a mock device
        Ptr<SixLowPanNdBindingTable> bindingTable = CreateObject<SixLowPanNdBindingTable>();
        Ptr<SimpleNetDevice> device = CreateObject<SimpleNetDevice>();

        Ipv6Address testAddr("2001:db8::1");
        auto entry = bindingTable->Add(testAddr);

        // Mark as REACHABLE with 1 minute lifetime (for faster testing)
        entry->MarkReachable(1); // 1 minute
        NS_TEST_ASSERT_MSG_EQ(entry->IsReachable(), true, "Entry should be REACHABLE");

        // Schedule check at 70 seconds (after 1 minute timer should expire)
        Simulator::Schedule(Seconds(70),
                            &SixLowPanNdBindingTableReachableToStaleTest::CheckStaleState,
                            this,
                            bindingTable,
                            testAddr);

        // Run simulation
        Simulator::Stop(Seconds(75));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_entryBecameStale, true, "Entry should be STALE after timer expiry");
    }

  private:
    /**
     * @brief Verify that the binding table entry for addr is in STALE state.
     * @param bindingTable the binding table to query
     * @param addr the IPv6 address to look up
     */
    void CheckStaleState(Ptr<SixLowPanNdBindingTable> bindingTable, Ipv6Address addr)
    {
        // Safely look up the entry (it should still exist but be STALE)
        auto entry = bindingTable->Lookup(addr);

        if (entry != nullptr)
        {
            m_entryBecameStale = entry->IsStale();

            NS_TEST_ASSERT_MSG_EQ(entry->IsStale(),
                                  true,
                                  "Entry should be STALE after timer expiry");
            NS_TEST_ASSERT_MSG_EQ(entry->IsReachable(),
                                  false,
                                  "Entry should not be REACHABLE after timer expiry");

            // Print Binding Table State for debugging
            NS_LOG_DEBUG("Entry found and is STALE: " << entry->IsStale());
        }
        else
        {
            NS_TEST_ASSERT_MSG_NE(entry, nullptr, "Entry should still exist at 70 seconds");
        }
    }

    bool m_entryBecameStale; //!< True if the entry transitioned to STALE during the test.
};

/**
 * @ingroup sixlowpan-nd-binding-table-tests
 *
 * @brief Test STALE entry removal after stale duration
 */
class SixLowPanNdBindingTableStaleRemovalTest : public TestCase
{
  public:
    SixLowPanNdBindingTableStaleRemovalTest()
        : TestCase("SixLowPanNdBindingTable STALE Entry Removal Test")
    {
    }

    void DoRun() override
    {
        // Create a binding table
        Ptr<SixLowPanNdBindingTable> bindingTable = CreateObject<SixLowPanNdBindingTable>();

        // Set a short stale duration for testing (2 seconds instead of 24 hours)
        bindingTable->SetAttribute("StaleDuration", TimeValue(Seconds(2)));

        Ipv6Address testAddr("2001:db8::1");
        auto entry = bindingTable->Add(testAddr);

        // Mark as STALE immediately (this starts the stale timer with the short duration)
        entry->MarkStale();
        NS_TEST_ASSERT_MSG_EQ(entry->IsStale(), true, "Entry should be STALE");

        // Verify entry exists before timer expiry
        auto foundEntry = bindingTable->Lookup(testAddr);
        NS_TEST_ASSERT_MSG_EQ(foundEntry, entry, "Entry should exist before stale timer expiry");

        // Schedule a check just before the timer expires to ensure entry still exists
        Simulator::Schedule(Seconds(1.5),
                            &SixLowPanNdBindingTableStaleRemovalTest::CheckEntryExists,
                            this,
                            bindingTable,
                            testAddr);

        // Schedule a check after the timer expires to ensure entry is removed
        Simulator::Schedule(Seconds(3),
                            &SixLowPanNdBindingTableStaleRemovalTest::CheckEntryRemoved,
                            this,
                            bindingTable,
                            testAddr);

        Simulator::Stop(Seconds(4));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_entryStillExisted, true, "Entry should exist before timeout");
        NS_TEST_ASSERT_MSG_EQ(m_entryWasRemoved, true, "Entry should be removed after timeout");
    }

  private:
    /**
     * @brief Verify that the binding table entry for addr still exists.
     * @param bindingTable the binding table to query
     * @param addr the IPv6 address to look up
     */
    void CheckEntryExists(Ptr<SixLowPanNdBindingTable> bindingTable, Ipv6Address addr)
    {
        auto entry = bindingTable->Lookup(addr);
        m_entryStillExisted = (entry != nullptr);
        NS_TEST_ASSERT_MSG_NE(entry, nullptr, "Entry should still exist before timer expiry");
    }

    /**
     * @brief Verify that the binding table entry for addr has been removed.
     * @param bindingTable the binding table to query
     * @param addr the IPv6 address to look up
     */
    void CheckEntryRemoved(Ptr<SixLowPanNdBindingTable> bindingTable, Ipv6Address addr)
    {
        auto entry = bindingTable->Lookup(addr);
        m_entryWasRemoved = (entry == nullptr);
        NS_TEST_ASSERT_MSG_EQ(entry, nullptr, "Entry should be removed after timer expiry");
    }

    bool m_entryStillExisted; //!< True if the entry was present before the stale timer expired.
    bool m_entryWasRemoved;   //!< True if the entry was removed after the stale timer expired.
};

/**
 * @ingroup sixlowpan-nd-binding-table-tests
 *
 * @brief Test state transitions and timer cancellations
 */
class SixLowPanNdBindingTableStateTransitionTest : public TestCase
{
  public:
    SixLowPanNdBindingTableStateTransitionTest()
        : TestCase("SixLowPanNdBindingTable State Transition Test")
    {
    }

    void DoRun() override
    {
        Ptr<SixLowPanNdBindingTable> bindingTable = CreateObject<SixLowPanNdBindingTable>();
        Ipv6Address testAddr("2001:db8::1");
        auto entry = bindingTable->Add(testAddr);

        // Test 1: Mark REACHABLE
        entry->MarkReachable(5); // 5 minutes
        NS_TEST_ASSERT_MSG_EQ(entry->IsReachable(), true, "Entry should be REACHABLE");
        NS_TEST_ASSERT_MSG_EQ(entry->IsStale(), false, "Entry should not be STALE");

        // Test 2: REACHABLE -> STALE
        entry->MarkStale();
        NS_TEST_ASSERT_MSG_EQ(entry->IsStale(), true, "Entry should be STALE");
        NS_TEST_ASSERT_MSG_EQ(entry->IsReachable(), false, "Entry should no longer be REACHABLE");

        // Test 3: STALE -> REACHABLE (stale timer should be cancelled)
        entry->MarkReachable(10);
        NS_TEST_ASSERT_MSG_EQ(entry->IsReachable(), true, "Entry should be REACHABLE again");
        NS_TEST_ASSERT_MSG_EQ(entry->IsStale(), false, "Entry should no longer be STALE");

        Simulator::Destroy();
    }
};

/**
 * @ingroup sixlowpan-nd-binding-table-tests
 *
 * @brief 6LoWPAN-ND binding table test suite
 */
class SixLowPanNdBindingTableTestSuite : public TestSuite
{
  public:
    SixLowPanNdBindingTableTestSuite()
        : TestSuite("sixlowpan-nd-binding-table-test", Type::UNIT)
    {
        AddTestCase(new SixLowPanNdBindingTableBasicTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdBindingTableReachableToStaleTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdBindingTableStaleRemovalTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdBindingTableStateTransitionTest(), TestCase::Duration::QUICK);
    }
};

/**
 * @brief Static variable for registering the 6LoWPAN-ND binding table test suite
 */
static SixLowPanNdBindingTableTestSuite g_sixlowpanNdBindingTableTestSuite;
