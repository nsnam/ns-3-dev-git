/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 *         Dizhi Zhou <dizhi.zhou@gmail.com>
 */

#ifndef LENA_TEST_PSS_FF_MAC_SCHEDULER_H
#define LENA_TEST_PSS_FF_MAC_SCHEDULER_H

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
class LenaPssFfMacSchedulerTestCase1 : public TestCase
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
    LenaPssFfMacSchedulerTestCase1(uint16_t nUser,
                                   double dist,
                                   double thrRefDl,
                                   double thrRefUl,
                                   uint16_t packetSize,
                                   uint16_t interval,
                                   bool errorModelEnabled);
    ~LenaPssFfMacSchedulerTestCase1() override;

  private:
    /**
     * Builds the test name string based on provided parameter values
     * @param nUser the number of UE nodes
     * @param dist the distance between nodes
     * @returns the name string
     */
    static std::string BuildNameString(uint16_t nUser, double dist);
    void DoRun() override;
    uint16_t m_nUser;         ///< number of UE nodes
    double m_dist;            ///< the distance between nodes
    uint16_t m_packetSize;    ///< the packet size in bytes
    uint16_t m_interval;      ///< the interval time in ms
    double m_thrRefDl;        ///< the DL throughput reference value
    double m_thrRefUl;        ///< the UL throughput reference value
    bool m_errorModelEnabled; ///< indicates whether the error model is enabled
};

/**
 * @ingroup lte-test
 *
 * @brief Similar to the LenaPssFfMacSchedulerTestCase1 with the difference that
 * UEs are places in such a way to experience different SINRs. Test checks if the
 * achieved throughput in such conditions has expected value.
 */
class LenaPssFfMacSchedulerTestCase2 : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param dist the distance between nodes
     * @param estThrPssDl the estimated DL throughput PSS
     * @param packetSize the packet size
     * @param interval the interval time
     * @param errorModelEnabled if true the error model is enabled
     */
    LenaPssFfMacSchedulerTestCase2(std::vector<double> dist,
                                   std::vector<uint32_t> estThrPssDl,
                                   std::vector<uint16_t> packetSize,
                                   uint16_t interval,
                                   bool errorModelEnabled);
    ~LenaPssFfMacSchedulerTestCase2() override;

  private:
    /**
     * Builds the test name string based on provided parameter values
     * @param nUser the number of UE nodes
     * @param dist the distance between nodes
     * @returns the name string
     */
    static std::string BuildNameString(uint16_t nUser, std::vector<double> dist);
    void DoRun() override;
    uint16_t m_nUser;                    ///< number of UE nodes
    std::vector<double> m_dist;          ///< the distance between nodes
    std::vector<uint16_t> m_packetSize;  ///< the packet size in bytes
    uint16_t m_interval;                 ///< the interval time in ms
    std::vector<uint32_t> m_estThrPssDl; ///< the DL estimated throughput PSS
    bool m_errorModelEnabled;            ///< indicates whether the error model is enabled
};

/**
 * @ingroup lte-test
 *
 * @brief Lena Pss Ff Mac Scheduler Test Suite
 */
class LenaTestPssFfMacSchedulerSuite : public TestSuite
{
  public:
    LenaTestPssFfMacSchedulerSuite();
};

#endif /* LENA_TEST_PSS_FF_MAC_SCHEDULER_H */
