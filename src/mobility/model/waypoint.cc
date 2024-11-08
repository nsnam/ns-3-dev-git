/*
 * Copyright (c) 2009 Phillip Sitbon
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Phillip Sitbon <phillip@sitbon.net>
 */
#include "waypoint.h"

namespace ns3
{

ATTRIBUTE_HELPER_CPP(Waypoint);

Waypoint::Waypoint(const Time& waypointTime, const Vector& waypointPosition)
    : time(waypointTime),
      position(waypointPosition)
{
}

Waypoint::Waypoint()
    : time(),
      position(0, 0, 0)
{
}

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param waypoint the waypoint
 * @returns a reference to the stream
 */
std::ostream&
operator<<(std::ostream& os, const Waypoint& waypoint)
{
    os << waypoint.time.GetSeconds() << "$" << waypoint.position;
    return os;
}

/**
 * @brief Stream extraction operator.
 *
 * @param is the stream
 * @param waypoint the waypoint
 * @returns a reference to the stream
 */
std::istream&
operator>>(std::istream& is, Waypoint& waypoint)
{
    char separator;
    is >> waypoint.time >> separator >> waypoint.position;
    if (separator != '$')
    {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

} // namespace ns3
