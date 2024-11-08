/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef TIME_DATA_CALCULATORS_H
#define TIME_DATA_CALCULATORS_H

#include "data-calculator.h"
#include "data-output-interface.h"

#include "ns3/nstime.h"

namespace ns3
{

//------------------------------------------------------------
//--------------------------------------------
/**
 * @ingroup stats
 *
 * Unfortunately, templating the base MinMaxAvgTotalCalculator to
 * operate over Time values isn't straightforward.  The main issues
 * are setting the maximum value, which can be worked around easily
 * as it done here, and dividing to get the average, which is not as
 * easily worked around.
 */
class TimeMinMaxAvgTotalCalculator : public DataCalculator
{
  public:
    TimeMinMaxAvgTotalCalculator();
    ~TimeMinMaxAvgTotalCalculator() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Updates all variables of TimeMinMaxAvgTotalCalculator
     * @param i value of type Time to use for updating the calculator
     */
    void Update(const Time i);

    /**
     * Outputs data based on the provided callback
     * @param callback
     */
    void Output(DataOutputCallback& callback) const override;

  protected:
    void DoDispose() override;

    uint32_t m_count; //!< Count value of TimeMinMaxAvgTotalCalculator
    Time m_total;     //!< Total value of TimeMinMaxAvgTotalCalculator
    Time m_min;       //!< Minimum value of TimeMinMaxAvgTotalCalculator
    Time m_max;       //!< Maximum value of TimeMinMaxAvgTotalCalculator

    // end class TimeMinMaxAvgTotalCalculator
};

// end namespace ns3
}; // namespace ns3

#endif /* TIME_DATA_CALCULATORS_H */
