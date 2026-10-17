/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/callback.h"
#include "ns3/csma-channel.h"
#include "ns3/csma-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup csma-test
 * @ingroup tests
 *
 * @brief Verify that detaching a CsmaNetDevice from its channel fires the
 * link-change callback and marks the link down (@issueid{234}).
 */
class CsmaLinkChangeTestCase : public TestCase
{
  public:
    CsmaLinkChangeTestCase();

  private:
    void DoRun() override;
    /// Link-change callback; counts invocations.
    void LinkChange();
    uint32_t m_linkChangeCount{0}; //!< number of link-change notifications
};

CsmaLinkChangeTestCase::CsmaLinkChangeTestCase()
    : TestCase("CsmaChannel::Detach fires the link-change callback (issue #234)")
{
}

void
CsmaLinkChangeTestCase::LinkChange()
{
    m_linkChangeCount++;
}

void
CsmaLinkChangeTestCase::DoRun()
{
    // --- Detach by device pointer ---
    {
        m_linkChangeCount = 0;
        Ptr<Node> node = CreateObject<Node>();
        Ptr<CsmaChannel> channel = CreateObject<CsmaChannel>();
        Ptr<CsmaNetDevice> dev = CreateObject<CsmaNetDevice>();
        dev->SetAddress(Mac48Address::Allocate());
        node->AddDevice(dev);

        dev->Attach(channel);
        NS_TEST_ASSERT_MSG_EQ(dev->IsLinkUp(), true, "link should be up after Attach");

        dev->AddLinkChangeCallback(MakeCallback(&CsmaLinkChangeTestCase::LinkChange, this));

        bool detached = channel->Detach(dev);
        NS_TEST_ASSERT_MSG_EQ(detached, true, "Detach(device) should succeed");
        NS_TEST_ASSERT_MSG_EQ(m_linkChangeCount,
                              1,
                              "link-change callback should fire on Detach(device)");
        NS_TEST_ASSERT_MSG_EQ(dev->IsLinkUp(), false, "link should be down after Detach(device)");
    }

    // --- Detach by device id ---
    {
        m_linkChangeCount = 0;
        Ptr<Node> node = CreateObject<Node>();
        Ptr<CsmaChannel> channel = CreateObject<CsmaChannel>();
        Ptr<CsmaNetDevice> dev = CreateObject<CsmaNetDevice>();
        dev->SetAddress(Mac48Address::Allocate());
        node->AddDevice(dev);

        dev->Attach(channel);
        dev->AddLinkChangeCallback(MakeCallback(&CsmaLinkChangeTestCase::LinkChange, this));

        // The first (and only) attached device has channel device id 0.
        bool detached = channel->Detach(uint32_t(0));
        NS_TEST_ASSERT_MSG_EQ(detached, true, "Detach(deviceId) should succeed");
        NS_TEST_ASSERT_MSG_EQ(m_linkChangeCount,
                              1,
                              "link-change callback should fire on Detach(deviceId)");
        NS_TEST_ASSERT_MSG_EQ(dev->IsLinkUp(), false, "link should be down after Detach(deviceId)");
    }

    // The nodes register themselves with the NodeList on construction; this
    // clears it and disposes them along with their devices and channels.
    Simulator::Destroy();
}

/**
 * @ingroup csma-test
 * @ingroup tests
 *
 * @brief CSMA link-change test suite.
 */
class CsmaLinkChangeTestSuite : public TestSuite
{
  public:
    CsmaLinkChangeTestSuite()
        : TestSuite("csma-link-change", Type::UNIT)
    {
        AddTestCase(new CsmaLinkChangeTestCase(), TestCase::Duration::QUICK);
    }
};

static CsmaLinkChangeTestSuite g_csmaLinkChangeTestSuite; //!< Static variable for test init
