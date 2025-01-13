/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */
#include "ns3/log.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::lrwpan;

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan PLME and PD Interfaces Test
 */
class LrWpanPlmeAndPdInterfaceTestCase : public TestCase
{
  public:
    LrWpanPlmeAndPdInterfaceTestCase();
    ~LrWpanPlmeAndPdInterfaceTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Receives a PdData indication
     * @param psduLength The PSDU length.
     * @param p The packet.
     * @param lqi The LQI.
     */
    void ReceivePdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi);
};

LrWpanPlmeAndPdInterfaceTestCase::LrWpanPlmeAndPdInterfaceTestCase()
    : TestCase("Test the PLME and PD SAP per IEEE 802.15.4")
{
}

LrWpanPlmeAndPdInterfaceTestCase::~LrWpanPlmeAndPdInterfaceTestCase()
{
}

void
LrWpanPlmeAndPdInterfaceTestCase::ReceivePdDataIndication(uint32_t psduLength,
                                                          Ptr<Packet> p,
                                                          uint8_t lqi)
{
    NS_LOG_UNCOND("At: " << Simulator::Now() << " Received frame size: " << psduLength
                         << " LQI: " << lqi);
}

void
LrWpanPlmeAndPdInterfaceTestCase::DoRun()
{
    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);

    Ptr<LrWpanPhy> sender = CreateObject<LrWpanPhy>();
    Ptr<LrWpanPhy> receiver = CreateObject<LrWpanPhy>();

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    sender->SetChannel(channel);
    receiver->SetChannel(channel);

    receiver->SetPdDataIndicationCallback(
        MakeCallback(&LrWpanPlmeAndPdInterfaceTestCase::ReceivePdDataIndication, this));

    uint32_t n = 10;
    Ptr<Packet> p = Create<Packet>(n);
    sender->PdDataRequest(p->GetSize(), p);

    Simulator::Destroy();
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan PLME and PD Interfaces TestSuite
 */
class LrWpanPlmeAndPdInterfaceTestSuite : public TestSuite
{
  public:
    LrWpanPlmeAndPdInterfaceTestSuite();
};

LrWpanPlmeAndPdInterfaceTestSuite::LrWpanPlmeAndPdInterfaceTestSuite()
    : TestSuite("lr-wpan-plme-pd-sap", Type::UNIT)
{
    AddTestCase(new LrWpanPlmeAndPdInterfaceTestCase, TestCase::Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static LrWpanPlmeAndPdInterfaceTestSuite
    g_lrWpanPlmeAndPdInterfaceTestSuite; //!< Static variable for test initialization
