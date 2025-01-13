/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#ifndef BUILDINGS_PATHLOSS_TEST_H
#define BUILDINGS_PATHLOSS_TEST_H

#include "ns3/hybrid-buildings-propagation-loss-model.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup building-test
 *
 * Test 1.1 BuildingsPathlossModel Pathloss compound test
 *
 * This TestSuite tests the BuildingPathlossModel by reproducing
 * several communication scenarios
 */
class BuildingsPathlossTestSuite : public TestSuite
{
  public:
    BuildingsPathlossTestSuite();
};

/**
 * @ingroup building-test
 *
 * Test 1.1 BuildingsPathlossModel Pathloss test
 *
 */
class BuildingsPathlossTestCase : public TestCase
{
  public:
    /**
     * Constructor
     * @param freq Communication frequency
     * @param m1 First MobilityModel Index
     * @param m2 Second MobilityModel Index
     * @param env Environment type
     * @param city City size
     * @param refValue Theoretical loss
     * @param name Test name
     */
    BuildingsPathlossTestCase(double freq,
                              uint16_t m1,
                              uint16_t m2,
                              EnvironmentType env,
                              CitySize city,
                              double refValue,
                              std::string name);
    ~BuildingsPathlossTestCase() override;

  private:
    void DoRun() override;
    /**
     * Create a mobility model based on its index
     * @param index MobilityModel index
     * @return The MobilityModel
     */
    Ptr<MobilityModel> CreateMobilityModel(uint16_t index);

    double m_freq;                  //!< Communication frequency
    uint16_t m_mobilityModelIndex1; //!< First MobilityModel Index
    uint16_t m_mobilityModelIndex2; //!< Second MobilityModel Index
    EnvironmentType m_env;          //!< Environment type
    CitySize m_city;                //!< City size
    double m_lossRef;               //!< Theoretical loss
};

#endif /* BUILDING_PATHLOSS_TEST_H */
