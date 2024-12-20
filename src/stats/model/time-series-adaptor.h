/*
 * Copyright (c) 2013 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#ifndef TIME_SERIES_ADAPTOR_H
#define TIME_SERIES_ADAPTOR_H

#include "data-collection-object.h"

#include "ns3/object.h"
#include "ns3/traced-value.h"
#include "ns3/type-id.h"

namespace ns3
{

/**
 * @ingroup aggregator
 *
 * @brief Takes probed values of different types and outputs the
 * current time plus the value with both converted to doubles.
 *
 * The role of the TimeSeriesAdaptor class is that of an adaptor
 * class, to take raw-valued probe data of different types, and output
 * a tuple of two double values.  The first is a timestamp which may
 * be set to different resolutions (e.g. Seconds, Milliseconds, etc.)
 * in the future, but which presently is hardcoded to Seconds.  The second
 * is the conversion of
 * a non-double value to a double value (possibly with loss of precision).
 *
 * It should be noted that time series adaptors convert
 * Simulation Time objects to double values in its output.
 */
class TimeSeriesAdaptor : public DataCollectionObject
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TimeSeriesAdaptor();
    ~TimeSeriesAdaptor() override;

    /**
     * @brief Trace sink for receiving data from double valued trace
     * sources.
     * @param oldData the original value.
     * @param newData the new value.
     *
     * This method serves as a trace sink to double valued trace
     * sources.
     */
    void TraceSinkDouble(double oldData, double newData);

    /**
     * @brief Trace sink for receiving data from bool valued trace
     * sources.
     * @param oldData the original value.
     * @param newData the new value.
     *
     * This method serves as a trace sink to bool valued trace
     * sources.
     */
    void TraceSinkBoolean(bool oldData, bool newData);

    /**
     * @brief Trace sink for receiving data from uint8_t valued trace
     * sources.
     * @param oldData the original value.
     * @param newData the new value.
     *
     * This method serves as a trace sink to uint8_t valued trace
     * sources.
     */
    void TraceSinkUinteger8(uint8_t oldData, uint8_t newData);

    /**
     * @brief Trace sink for receiving data from uint16_t valued trace
     * sources.
     * @param oldData the original value.
     * @param newData the new value.
     *
     * This method serves as a trace sink to uint16_t valued trace
     * sources.
     */
    void TraceSinkUinteger16(uint16_t oldData, uint16_t newData);

    /**
     * @brief Trace sink for receiving data from uint32_t valued trace
     * sources.
     * @param oldData the original value.
     * @param newData the new value.
     *
     * This method serves as a trace sink to uint32_t valued trace
     * sources.
     */
    void TraceSinkUinteger32(uint32_t oldData, uint32_t newData);

    /**
     * TracedCallback signature for output trace.
     *
     * @param [in] now The current time, in seconds.
     * @param [in] data The new data value.
     */
    typedef void (*OutputTracedCallback)(const double now, const double data);

  private:
    TracedCallback<double, double> m_output; //!< output trace
};

} // namespace ns3

#endif // TIME_SERIES_ADAPTOR_H
