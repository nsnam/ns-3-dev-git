/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#ifndef EPC_TEST_GTPU_H
#define EPC_TEST_GTPU_H

#include "ns3/epc-gtpu-header.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup lte
 * @ingroup tests
 * @defgroup lte-test lte module tests
 */

/**
 * @ingroup lte-test
 *
 * @brief Test suite for testing GPRS tunnelling protocol header coding and decoding.
 */
class EpsGtpuTestSuite : public TestSuite
{
  public:
    EpsGtpuTestSuite();
};

/**
 * Test 1.Check header coding and decoding
 */
class EpsGtpuHeaderTestCase : public TestCase
{
  public:
    EpsGtpuHeaderTestCase();
    ~EpsGtpuHeaderTestCase() override;

  private:
    void DoRun() override;
};

#endif /* EPC_TEST_GTPU_H */
