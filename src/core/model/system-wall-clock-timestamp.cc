/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "system-wall-clock-timestamp.h"

/**
 * @file
 * @ingroup system
 * ns3::SystemWallClockTimestamp implementation.
 */

namespace ns3
{

SystemWallClockTimestamp::SystemWallClockTimestamp()
    : m_last(0),
      m_diff(0)
{
    Stamp();
}

void
SystemWallClockTimestamp::Stamp()
{
    std::time_t seconds = std::time(nullptr);
    m_diff = seconds - m_last;
    m_last = seconds;
}

std::string
SystemWallClockTimestamp::ToString() const
{
    std::string now = std::ctime(&m_last);
    now.resize(now.length() - 1); // trim trailing newline
    return now;
}

std::time_t
SystemWallClockTimestamp::GetLast() const
{
    return m_last;
}

std::time_t
SystemWallClockTimestamp::GetInterval() const
{
    return m_diff;
}

} // namespace ns3
