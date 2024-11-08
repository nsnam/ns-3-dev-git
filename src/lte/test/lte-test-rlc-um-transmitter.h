/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_TEST_RLC_UM_TRANSMITTER_H
#define LTE_TEST_RLC_UM_TRANSMITTER_H

#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/test.h"

namespace ns3
{

class LteTestRrc;
class LteTestMac;
class LteTestPdcp;
class LteRlc;

} // namespace ns3

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief TestSuite 4.1.1 for RLC UM: Only transmitter part.
 */
class LteRlcUmTransmitterTestSuite : public TestSuite
{
  public:
    LteRlcUmTransmitterTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief Test case used by LteRlcUmTransmitterOneSduTestCase to create topology
 * and to implement functionalities and check if data received corresponds to
 * data sent.
 */
class LteRlcUmTransmitterTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the test name
     */
    LteRlcUmTransmitterTestCase(std::string name);
    LteRlcUmTransmitterTestCase();
    ~LteRlcUmTransmitterTestCase() override;

    /**
     * Check data received function
     * @param time the time to check
     * @param shouldReceived should have received indicator
     * @param assertMsg the assert message
     */
    void CheckDataReceived(Time time, std::string shouldReceived, std::string assertMsg);

  protected:
    void DoRun() override;

    Ptr<LteTestPdcp> txPdcp; ///< the transmit PDCP
    Ptr<LteRlc> txRlc;       ///< the RLC
    Ptr<LteTestMac> txMac;   ///< the MAC

  private:
    /**
     * Check data received function
     * @param shouldReceived should have received indicator
     * @param assertMsg the assert message
     */
    void DoCheckDataReceived(std::string shouldReceived, std::string assertMsg);
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.1 One SDU, One PDU
 */
class LteRlcUmTransmitterOneSduTestCase : public LteRlcUmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the test name
     */
    LteRlcUmTransmitterOneSduTestCase(std::string name);
    LteRlcUmTransmitterOneSduTestCase();
    ~LteRlcUmTransmitterOneSduTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.2 Segmentation (One SDU => n PDUs)
 */
class LteRlcUmTransmitterSegmentationTestCase : public LteRlcUmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcUmTransmitterSegmentationTestCase(std::string name);
    LteRlcUmTransmitterSegmentationTestCase();
    ~LteRlcUmTransmitterSegmentationTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.3 Concatenation (n SDUs => One PDU)
 */
class LteRlcUmTransmitterConcatenationTestCase : public LteRlcUmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcUmTransmitterConcatenationTestCase(std::string name);
    LteRlcUmTransmitterConcatenationTestCase();
    ~LteRlcUmTransmitterConcatenationTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.4 Report Buffer Status (test primitive parameters)
 */
class LteRlcUmTransmitterReportBufferStatusTestCase : public LteRlcUmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcUmTransmitterReportBufferStatusTestCase(std::string name);
    LteRlcUmTransmitterReportBufferStatusTestCase();
    ~LteRlcUmTransmitterReportBufferStatusTestCase() override;

  private:
    void DoRun() override;
};

#endif /* LTE_TEST_RLC_UM_TRANSMITTER_H */
