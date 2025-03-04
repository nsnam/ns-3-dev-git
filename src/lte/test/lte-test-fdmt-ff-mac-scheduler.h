/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 *         Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#ifndef LENA_TEST_FDMT_FF_MAC_SCHEDULER_H
#define LENA_TEST_FDMT_FF_MAC_SCHEDULER_H

#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief This system test program creates different test cases with a single eNB and
 * several UEs, all having the same Radio Bearer specification. In each test
 * case, the UEs see the same SINR from the eNB; different test cases are
 * implemented obtained by using different SINR values and different numbers of
 * UEs. The test consists on checking that the obtained throughput performance
 * is consistent with the definition of maximum throughput
 * scheduling
 */
class LenaFdMtFfMacSchedulerTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param nUser number of UE nodes
     * @param dist distance between nodes
     * @param thrRefDl DL throughput reference
     * @param thrRefUl UL throughput reference
     * @param errorModelEnabled error model enabled?
     */
    LenaFdMtFfMacSchedulerTestCase(uint16_t nUser,
                                   double dist,
                                   double thrRefDl,
                                   double thrRefUl,
                                   bool errorModelEnabled);
    ~LenaFdMtFfMacSchedulerTestCase() override;

  private:
    /**
     *  Builds the test name string based on provided parameter values
     *
     * @param nUser number of UE nodes
     * @param dist distance between nodes
     * @returns name string
     */
    static std::string BuildNameString(uint16_t nUser, double dist);
    void DoRun() override;
    uint16_t m_nUser;         ///< number of UE nodes
    double m_dist;            ///< distance between the nodes
    double m_thrRefDl;        ///< DL throughput reference
    double m_thrRefUl;        ///< UL throughput reference
    bool m_errorModelEnabled; ///< error model enabled?
};

/**
 * @ingroup lte-test
 *
 * @brief Test suite for LenaFdMtFfMacSchedulerTestCase test case.
 */
class LenaTestFdMtFfMacSchedulerSuite : public TestSuite
{
  public:
    LenaTestFdMtFfMacSchedulerSuite();
};

#endif /* LENA_TEST_FDMT_FF_MAC_SCHEDULER_H */
