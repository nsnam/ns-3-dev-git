/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#ifndef LTE_TEST_PATHLOSS_MODEL_H
#define LTE_TEST_PATHLOSS_MODEL_H

#include "ns3/buildings-propagation-loss-model.h"
#include "ns3/lte-common.h"
#include "ns3/spectrum-value.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief Test 1.1 pathloss calculation
 */
class LtePathlossModelTestSuite : public TestSuite
{
  public:
    LtePathlossModelTestSuite();
};

/**
 * @ingroup lte-test
 *
 * @brief  Tests that the BuildingPathlossModel works according to
 * the expected theoretical values. Theoretical reference values
 * are obtained with the octave script src/lte/test/reference/lte_pathloss.m
 */
class LtePathlossModelSystemTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the reference name
     * @param snrDb the SNR in dB
     * @param dist the distance
     * @param mcsIndex the MCS index
     */
    LtePathlossModelSystemTestCase(std::string name, double snrDb, double dist, uint16_t mcsIndex);
    LtePathlossModelSystemTestCase();
    ~LtePathlossModelSystemTestCase() override;

    /**
     * @brief DL scheduling function
     * @param dlInfo the DL info
     */
    void DlScheduling(DlSchedulingCallbackInfo dlInfo);

  private:
    void DoRun() override;

    double m_snrDb;      ///< the SNR in dB
    double m_distance;   ///< the distance
    uint16_t m_mcsIndex; ///< the MCS index
};

#endif /* LTE_TEST_PATHLOSS_MODEL_H */
