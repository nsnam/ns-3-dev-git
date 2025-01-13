/*
 * Copyright (c) 2017 Alexander Krotov
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alexander Krotov <krotov@iitp.ru>
 *
 */

#ifndef LTE_AGGREGATION_THROUGHPUT_SCALE_H
#define LTE_AGGREGATION_THROUGHPUT_SCALE_H

#include "ns3/test.h"

using namespace ns3;

/**
 * @brief Test suite for executing carrier aggregation throughput scaling test case.
 *
 * \sa ns3::LteAggregationThroughputScaleTestCase
 */
class LteAggregationThroughputScaleTestSuite : public TestSuite
{
  public:
    LteAggregationThroughputScaleTestSuite();
};

/**
 * @ingroup lte
 *
 * @brief Testing that UE throughput scales linearly with number of component carriers.
 *        Also attaches UE to last component carrier to make sure no code assumes
 *        that primary carrier is first.
 */
class LteAggregationThroughputScaleTestCase : public TestCase
{
  public:
    /**
     * @brief Creates an instance of the carrier aggregation throughput scaling test case.
     * @param name name of this test
     */
    LteAggregationThroughputScaleTestCase(std::string name);

    ~LteAggregationThroughputScaleTestCase() override;

  private:
    /**
     * @brief Setup the simulation, run it, and verify the result.
     */
    void DoRun() override;

    /**
     * @brief Get throughput function
     *
     * @param numberOfComponentCarriers Number of component carriers
     * @return The total data received (in Megabits)
     */
    double GetThroughput(uint8_t numberOfComponentCarriers);

    uint16_t m_expectedCellId; ///< Cell ID UE is expected to attach to
    uint16_t m_actualCellId;   ///< Cell ID UE has attached to
};

#endif /* LTE_AGGREGATION_THROUGHPUT_SCALE_H */
