/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_TEST_RLC_AM_TRANSMITTER_H
#define LTE_TEST_RLC_AM_TRANSMITTER_H

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
 * @brief TestSuite 4.1.1 RLC AM: Only transmitter functionality.
 */
class LteRlcAmTransmitterTestSuite : public TestSuite
{
  public:
    LteRlcAmTransmitterTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief Test case used by LteRlcAmTransmitterOneSduTestCase to create topology
 * and to implement functionalities and check if data received corresponds to
 * data sent.
 */
class LteRlcAmTransmitterTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcAmTransmitterTestCase(std::string name);
    LteRlcAmTransmitterTestCase();
    ~LteRlcAmTransmitterTestCase() override;

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
 * @brief Test 4.1.1.1 Test that SDU transmitted at PDCP corresponds to PDU
 * received by MAC.
 */
class LteRlcAmTransmitterOneSduTestCase : public LteRlcAmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcAmTransmitterOneSduTestCase(std::string name);
    LteRlcAmTransmitterOneSduTestCase();
    ~LteRlcAmTransmitterOneSduTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.2 Test the correct functionality of the Segmentation.
 * Test check that single SDU is properly segmented to n PDUs.
 */
class LteRlcAmTransmitterSegmentationTestCase : public LteRlcAmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcAmTransmitterSegmentationTestCase(std::string name);
    LteRlcAmTransmitterSegmentationTestCase();
    ~LteRlcAmTransmitterSegmentationTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.3 Test that concatenation functionality works properly.
 * Test check if n SDUs are correctly contactenate to single PDU.
 */
class LteRlcAmTransmitterConcatenationTestCase : public LteRlcAmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcAmTransmitterConcatenationTestCase(std::string name);
    LteRlcAmTransmitterConcatenationTestCase();
    ~LteRlcAmTransmitterConcatenationTestCase() override;

  private:
    void DoRun() override;
};

/**
 * @ingroup lte-test
 *
 * @brief Test 4.1.1.4 Test checks functionality of Report Buffer Status by
 * testing primitive parameters.
 */
class LteRlcAmTransmitterReportBufferStatusTestCase : public LteRlcAmTransmitterTestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     */
    LteRlcAmTransmitterReportBufferStatusTestCase(std::string name);
    LteRlcAmTransmitterReportBufferStatusTestCase();
    ~LteRlcAmTransmitterReportBufferStatusTestCase() override;

  private:
    void DoRun() override;
};

#endif // LTE_TEST_RLC_AM_TRANSMITTER_H
