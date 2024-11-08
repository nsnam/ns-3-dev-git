/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#ifndef LENA_TEST_MIMO_H
#define LENA_TEST_MIMO_H

#include "ns3/simulator.h"
#include "ns3/test.h"

namespace ns3
{
class RadioBearerStatsCalculator;
}

using namespace ns3;

/**
 * @ingroup lte-test
 *
 * @brief This system test program creates different test cases with a
 * single eNB and single UE. The traffic is configured to be in saturation
 * mode. It is checked if the throughput reaches the expected values
 * when MIMO is used.
 */
class LenaMimoTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param dist the distance
     * @param estThrDl the estimated throughput DL
     * @param schedulerType the scheduler type
     * @param useIdealRrc true if use ideal RRC
     */
    LenaMimoTestCase(uint16_t dist,
                     std::vector<uint32_t> estThrDl,
                     std::string schedulerType,
                     bool useIdealRrc);
    ~LenaMimoTestCase() override;

  private:
    void DoRun() override;

    /**
     * Get RLC buffer sample
     * @param rlcStats Ptr<RadioBearerStatsCalculator>
     * @param imsi the IMSI
     * @param rnti the RNTI
     */
    void GetRlcBufferSample(Ptr<RadioBearerStatsCalculator> rlcStats, uint64_t imsi, uint8_t rnti);

    /**
     * Builds the test name string based on provided parameter values
     * @param dist the distance
     * @param schedulerType the scheduler type
     * @param useIdealRrc if true use the ideal RRC
     * @returns the name string
     */
    static std::string BuildNameString(uint16_t dist, std::string schedulerType, bool useIdealRrc);
    uint16_t m_dist;                  ///< the distance
    std::vector<uint32_t> m_estThrDl; ///< estimated throughput DL
    std::string m_schedulerType;      ///< the scheduler type
    bool m_useIdealRrc;               ///< whether to use the ideal RRC

    std::vector<uint64_t> m_dlDataRxed; ///< DL data received
};

/**
 * @ingroup lte-test
 *
 * @brief Lena Test Mimo Suite
 */

class LenaTestMimoSuite : public TestSuite
{
  public:
    LenaTestMimoSuite();
};

#endif /* LENA_TEST_MIMO_H */
