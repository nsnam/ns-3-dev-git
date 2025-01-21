/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef SYSTEM_WALL_CLOCK_TIMESTAMP_H
#define SYSTEM_WALL_CLOCK_TIMESTAMP_H

/**
 * @file
 * @ingroup system
 * ns3::SystemWallClockTimestamp declaration.
 */

#include <ctime> // ctime, time_t
#include <string>

namespace ns3
{

/**
 * Utility class to record the difference between two wall-clock times.
 */
class SystemWallClockTimestamp
{
  public:
    /** Constructor */
    SystemWallClockTimestamp();

    /** Record the current wall-clock time and delta since the last stamp(). */
    void Stamp();

    /**
     * Get the last time stamp as a string.
     * @return The last time stamp.
     */
    std::string ToString() const;

    /**
     * Get the last recorded raw value.
     * @returns The last time stamp recorded.
     */
    std::time_t GetLast() const;

    /**
     * Get the last recorded interval.
     * @returns The interval between the last two time stamps.
     */
    std::time_t GetInterval() const;

  private:
    /** The last time stamp. */
    std::time_t m_last;

    /** Difference between the two previous time stamps. */
    std::time_t m_diff;
};

} // namespace ns3

#endif /* SYSTEM_WALL_CLOCK_TIMESTAMP_H */
