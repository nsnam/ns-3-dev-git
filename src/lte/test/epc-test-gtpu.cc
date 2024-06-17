/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "epc-test-gtpu.h"

#include "ns3/epc-gtpu-header.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/packet.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EpcGtpuTest");

/**
 * TestSuite
 */

EpsGtpuTestSuite::EpsGtpuTestSuite()
    : TestSuite("epc-gtpu", Type::SYSTEM)
{
    AddTestCase(new EpsGtpuHeaderTestCase(), TestCase::Duration::QUICK);
}

static EpsGtpuTestSuite epsGtpuTestSuite;

/**
 * TestCase
 */

EpsGtpuHeaderTestCase::EpsGtpuHeaderTestCase()
    : TestCase("Check header coding and decoding")
{
    NS_LOG_INFO("Creating EpsGtpuHeaderTestCase");
}

EpsGtpuHeaderTestCase::~EpsGtpuHeaderTestCase()
{
}

void
EpsGtpuHeaderTestCase::DoRun()
{
    auto logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

    LogComponentEnable("EpcGtpuTest", logLevel);
    GtpuHeader h1;
    h1.SetExtensionHeaderFlag(true);
    h1.SetLength(1234);
    h1.SetMessageType(123);
    h1.SetNPduNumber(123);
    h1.SetNPduNumberFlag(true);
    h1.SetNextExtensionType(123);
    h1.SetProtocolType(true);
    h1.SetSequenceNumber(1234);
    h1.SetSequenceNumberFlag(true);
    h1.SetTeid(1234567);
    h1.SetVersion(123);

    Packet p;
    GtpuHeader h2;
    p.AddHeader(h1);
    p.RemoveHeader(h2);

    NS_TEST_ASSERT_MSG_EQ(h1, h2, "Wrong value!");
}
