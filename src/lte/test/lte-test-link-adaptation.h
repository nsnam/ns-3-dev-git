/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_TEST_LINK_ADAPTATION_H
#define LTE_TEST_LINK_ADAPTATION_H

#include "ns3/lte-common.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief Test 1.3 Link adaptation
 */
class LteLinkAdaptationTestSuite : public TestSuite
{
  public:
    LteLinkAdaptationTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief Test that LTE link adaptation works according to the theoretical model.
 */
class LteLinkAdaptationTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     * @param snrDb the SNR in dB
     * @param loss the loss
     * @param mcsIndex the DL se
     */
    LteLinkAdaptationTestCase(std::string name, double snrDb, double loss, uint16_t mcsIndex);
    LteLinkAdaptationTestCase();
    ~LteLinkAdaptationTestCase() override;

    /**
     * @brief DL scheduling function
     * @param dlInfo the DL info
     */
    void DlScheduling(DlSchedulingCallbackInfo dlInfo);

  private:
    void DoRun() override;

    double m_snrDb;      ///< the SNR in dB
    double m_loss;       ///< the loss
    uint16_t m_mcsIndex; ///< the MCS index
};

#endif /* LTE_TEST_LINK_ADAPTATION_H */
