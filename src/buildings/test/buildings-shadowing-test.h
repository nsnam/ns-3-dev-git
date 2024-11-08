/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#ifndef BUILDINGS_SHADOWING_TEST_H
#define BUILDINGS_SHADOWING_TEST_H

#include "ns3/ptr.h"
#include "ns3/test.h"

namespace ns3
{
class MobilityModel;
}

using namespace ns3;

/**
 * @ingroup building-test
 *
 * Shadowing compound test
 *
 * This TestSuite tests the shadowing model of BuildingPathlossModel
 * by reproducing several communication scenarios
 */
class BuildingsShadowingTestSuite : public TestSuite
{
  public:
    BuildingsShadowingTestSuite();
};

/**
 * @ingroup building-test
 *
 * Shadowing test
 */
class BuildingsShadowingTestCase : public TestCase
{
  public:
    /**
     * Constructor
     * @param m1 First MobilityModel Index
     * @param m2 Second MobilityModel Index
     * @param refValue Theoretical loss
     * @param sigmaRef Theoretical loss standard deviation
     * @param name Test name
     */
    BuildingsShadowingTestCase(uint16_t m1,
                               uint16_t m2,
                               double refValue,
                               double sigmaRef,
                               std::string name);
    ~BuildingsShadowingTestCase() override;

  private:
    void DoRun() override;
    /**
     * Create a mobility model based on its index
     * @param index MobilityModel index
     * @return The MobilityModel
     */
    Ptr<MobilityModel> CreateMobilityModel(uint16_t index);

    uint16_t m_mobilityModelIndex1; //!< First MobilityModel Index
    uint16_t m_mobilityModelIndex2; //!< Second MobilityModel Index
    double m_lossRef;               //!< pathloss value (without shadowing)
    double m_sigmaRef;              //!< pathloss standard deviation value reference value
};

#endif /*BUILDINGS_SHADOWING_TEST_H*/
