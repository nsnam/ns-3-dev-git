/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 *         Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#ifndef LENA_TEST_TDTBFQ_FF_MAC_SCHEDULER_H
#define LENA_TEST_TDTBFQ_FF_MAC_SCHEDULER_H

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
 * is equal among users is consistent with the definition of token bank fair
 * queue scheduling
 */
class LenaTdTbfqFfMacSchedulerTestCase1 : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param nUser the number of UE nodes
     * @param dist the distance between nodes
     * @param thrRefDl the DL throughput reference
     * @param thrRefUl the UL throughput reference
     * @param packetSize the packet size
     * @param interval the interval time
     * @param errorModelEnabled if true the error model is enabled
     */
    LenaTdTbfqFfMacSchedulerTestCase1(uint16_t nUser,
                                      double dist,
                                      double thrRefDl,
                                      double thrRefUl,
                                      uint16_t packetSize,
                                      uint16_t interval,
                                      bool errorModelEnabled);
    ~LenaTdTbfqFfMacSchedulerTestCase1() override;

  private:
    /**
     * Builds the test name string based on provided parameter values
     * @param nUser the number of UE nodes
     * @param dist the distance between UE nodes and eNodeB
     * @returns the name string
     */
    static std::string BuildNameString(uint16_t nUser, double dist);
    void DoRun() override;
    uint16_t m_nUser;         ///< number of UE nodes
    double m_dist;            ///< the distance between nodes
    uint16_t m_packetSize;    ///< the packet size in bytes
    uint16_t m_interval;      ///< the interval time in ms
    double m_thrRefDl;        ///< the DL throughput reference
    double m_thrRefUl;        ///< the UL throughput reference
    bool m_errorModelEnabled; ///< whether the error model is enabled
};

/**
 * @ingroup lte-test
 *
 * @brief Lena TdTbfq Ff Mac Scheduler Test Case 2
 */
class LenaTdTbfqFfMacSchedulerTestCase2 : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param dist the distance between nodes
     * @param estThrTdTbfqDl the estimated DL throughput TdTbfq
     * @param packetSize the packet size
     * @param interval the interval time
     * @param errorModelEnabled if true the error model is enabled
     */
    LenaTdTbfqFfMacSchedulerTestCase2(std::vector<double> dist,
                                      std::vector<uint32_t> estThrTdTbfqDl,
                                      std::vector<uint16_t> packetSize,
                                      uint16_t interval,
                                      bool errorModelEnabled);
    ~LenaTdTbfqFfMacSchedulerTestCase2() override;

  private:
    /**
     * Builds the test name string based on provided parameter values
     * @param nUser the number of UE nodes
     * @param dist the distance between nodes
     * @returns the name string
     */
    static std::string BuildNameString(uint16_t nUser, std::vector<double> dist);
    void DoRun() override;
    uint16_t m_nUser;                       ///< number of UE nodes
    std::vector<double> m_dist;             ///< the distance between nodes
    std::vector<uint16_t> m_packetSize;     ///< the packet size in bytes
    uint16_t m_interval;                    ///< the packet interval time in ms
    std::vector<uint32_t> m_estThrTdTbfqDl; ///< the estimated downlink throughput
    bool m_errorModelEnabled;               ///< whether the error model is enabled
};

/**
 * @ingroup lte-test
 *
 * @brief Test suite for TdTbfqFfMacScheduler test.
 */
class LenaTestTdTbfqFfMacSchedulerSuite : public TestSuite
{
  public:
    LenaTestTdTbfqFfMacSchedulerSuite();
};

#endif /* LENA_TEST_TDTBFQ_FF_MAC_SCHEDULER_H */
