/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef TIME_SPAN_H
#define TIME_SPAN_H

#include "nstime.h"
#include "struct.h"

/**
 * @file
 * @ingroup time
 * Declaration of class ns3::TimeSpan and definition of the TimeSpanValue attribute value.
 */

namespace ns3
{

/**
 * Class representing a period of time defined via a begin time and an end time. An object of this
 * class can also be set/get via the attribute system by using the @ref TimeSpanValue attribute
 * value.
 */
class TimeSpan
{
  public:
    TimeSpan() = default;

    /**
     * Construct a TimeSpan from a string.
     *
     * The string representation is "{begin, end}". Whitespace around the begin and end times
     * is allowed.
     *
     * @param s The string to parse into a TimeSpan
     */
    explicit TimeSpan(const std::string& s);

    /**
     * Construct a TimeSpan from a begin time and an end time.
     *
     * @param begin the begin time
     * @param end the end time
     */
    TimeSpan(const Time& begin, const Time& end);

    /**
     * @brief TimeSpan factory (required by @ref TimeSpanValue)
     * @param begin the begin time
     * @param end the end time
     * @return a TimeSpan object
     */
    static TimeSpan Create(const Time& begin, const Time& end);

    /**
     * @return the begin time
     */
    Time Begin() const;

    /**
     * @return the end time
     */
    Time End() const;

    /**
     * @return the duration of the timespan
     */
    Time Duration() const;

    /**
     * Compare two TimeSpan objects for equality.
     *
     * @param other The TimeSpan to compare with
     * @return true if both TimeSpan objects have the same begin and end times
     */
    bool operator==(const TimeSpan& other) const = default;

  private:
    Time m_begin; //!< begin time
    Time m_end;   //!< end time
};

/**
 * TimeSpan output streamer.
 *
 * Generates output such as "{10ms, 50ms}".
 *
 * @param [in,out] os The output stream.
 * @param [in] timespan The timespan to put on the stream.
 * @return The stream.
 */
std::ostream& operator<<(std::ostream& os, const TimeSpan& timespan);

/**
 * TimeSpan input streamer.
 *
 * The string representation is "{begin, end}". Whitespace around the begin and end times is
 * allowed.
 *
 * @param [in,out] is The input stream.
 * @param [out] timespan The TimeSpan variable to set from the stream data.
 * @return The stream.
 */
std::istream& operator>>(std::istream& is, TimeSpan& timespan);

/// @brief Definition of the TimeSpanValue attribute value
using TimeSpanValue = StructValue<TimeSpan,
                                  StructConstructor<&TimeSpan::Create>,
                                  StructField<TimeValue, &TimeSpan::Begin>,
                                  StructField<TimeValue, &TimeSpan::End>>;

} // namespace ns3

#endif /* TIME_SPAN_H */
