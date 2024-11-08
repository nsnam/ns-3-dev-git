/*
 * Copyright (c) 2009 Phillip Sitbon
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Phillip Sitbon <phillip@sitbon.net>
 */
#ifndef WAYPOINT_H
#define WAYPOINT_H

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"

namespace ns3
{

/**
 * @ingroup mobility
 * @brief a (time, location) pair.
 * @see attribute_Waypoint
 */
class Waypoint
{
  public:
    /**
     * @param waypointTime time of waypoint.
     * @param waypointPosition position of waypoint corresponding to the given time.
     *
     * Create a waypoint.
     */
    Waypoint(const Time& waypointTime, const Vector& waypointPosition);

    /**
     * Create a waypoint at time 0 and position (0,0,0).
     */
    Waypoint();
    /**
     * @brief The waypoint time
     */
    Time time;
    /**
     * @brief The position of the waypoint
     */
    Vector position;
};

ATTRIBUTE_HELPER_HEADER(Waypoint);

std::ostream& operator<<(std::ostream& os, const Waypoint& waypoint);
std::istream& operator>>(std::istream& is, Waypoint& waypoint);

} // namespace ns3

#endif /* WAYPOINT_H */
