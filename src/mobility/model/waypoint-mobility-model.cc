/*
 * Copyright (c) 2009 Phillip Sitbon
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Phillip Sitbon <phillip@sitbon.net>
 */
#include "waypoint-mobility-model.h"

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WaypointMobilityModel");

NS_OBJECT_ENSURE_REGISTERED(WaypointMobilityModel);

TypeId
WaypointMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WaypointMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<WaypointMobilityModel>()
            .AddAttribute("NextWaypoint",
                          "The next waypoint used to determine position.",
                          TypeId::ATTR_GET,
                          WaypointValue(),
                          MakeWaypointAccessor(&WaypointMobilityModel::GetNextWaypoint),
                          MakeWaypointChecker())
            .AddAttribute("WaypointsLeft",
                          "The number of waypoints remaining.",
                          TypeId::ATTR_GET,
                          UintegerValue(0),
                          MakeUintegerAccessor(&WaypointMobilityModel::WaypointsLeft),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("LazyNotify",
                          "Only call NotifyCourseChange when position is calculated.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WaypointMobilityModel::m_lazyNotify),
                          MakeBooleanChecker())
            .AddAttribute("InitialPositionIsWaypoint",
                          "Calling SetPosition with no waypoints creates a waypoint.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&WaypointMobilityModel::m_initialPositionIsWaypoint),
                          MakeBooleanChecker());
    return tid;
}

WaypointMobilityModel::WaypointMobilityModel()
    : m_first(true),
      m_lazyNotify(false),
      m_initialPositionIsWaypoint(false)
{
}

WaypointMobilityModel::~WaypointMobilityModel()
{
    m_event.Cancel();
}

void
WaypointMobilityModel::DoDispose()
{
    MobilityModel::DoDispose();
}

void
WaypointMobilityModel::AddWaypoint(const Waypoint& waypoint)
{
    if (m_first)
    {
        m_first = false;
        m_current = m_next = waypoint;
    }
    else
    {
        NS_ABORT_MSG_IF(!m_waypoints.empty() && (m_waypoints.back().time >= waypoint.time),
                        "Waypoints must be added in ascending time order");
        m_waypoints.push_back(waypoint);
    }

    if (!m_lazyNotify)
    {
        m_event = Simulator::Schedule(waypoint.time - Simulator::Now(),
                                      &WaypointMobilityModel::Update,
                                      this);
    }
}

Waypoint
WaypointMobilityModel::GetNextWaypoint() const
{
    Update();
    return m_next;
}

uint32_t
WaypointMobilityModel::WaypointsLeft() const
{
    Update();
    return m_waypoints.size();
}

void
WaypointMobilityModel::Update() const
{
    const Time now = Simulator::Now();
    bool newWaypoint = false;

    if (now < m_current.time)
    {
        return;
    }

    while (now >= m_next.time)
    {
        if (m_waypoints.empty())
        {
            if (m_current.time <= m_next.time)
            {
                /*
                  Set m_next.time = -1 to make sure this doesn't happen more than once.
                  The comparison here still needs to be '<=' in the case of mobility with one
                  waypoint.
                */
                m_next.time = Seconds(-1.0);
                m_current.position = m_next.position;
                m_current.time = now;
                m_velocity = Vector(0, 0, 0);
                NotifyCourseChange();
            }
            else
            {
                m_current.time = now;
            }

            return;
        }

        m_current = m_next;
        m_next = m_waypoints.front();
        m_waypoints.pop_front();
        newWaypoint = true;

        const double t_span = (m_next.time - m_current.time).GetSeconds();
        NS_ASSERT(t_span > 0);
        m_velocity.x = (m_next.position.x - m_current.position.x) / t_span;
        m_velocity.y = (m_next.position.y - m_current.position.y) / t_span;
        m_velocity.z = (m_next.position.z - m_current.position.z) / t_span;
    }

    if (now > m_current.time) // Won't ever be less, but may be equal
    {
        const double t_diff = (now - m_current.time).GetSeconds();
        m_current.position.x += m_velocity.x * t_diff;
        m_current.position.y += m_velocity.y * t_diff;
        m_current.position.z += m_velocity.z * t_diff;
        m_current.time = now;
    }

    if (newWaypoint)
    {
        NotifyCourseChange();
    }
}

Vector
WaypointMobilityModel::DoGetPosition() const
{
    Update();
    return m_current.position;
}

void
WaypointMobilityModel::DoSetPosition(const Vector& position)
{
    const Time now = Simulator::Now();

    if (m_first && m_initialPositionIsWaypoint)
    {
        AddWaypoint(Waypoint(now, position));
        return;
    }

    Update();
    m_current.time = std::max(now, m_next.time);
    m_current.position = position;
    m_velocity = Vector(0, 0, 0);

    if (!m_first && (now >= m_current.time))
    {
        // This is only a course change if the node is actually moving
        NotifyCourseChange();
    }
}

void
WaypointMobilityModel::EndMobility()
{
    m_waypoints.clear();
    m_current.time = Time(std::numeric_limits<uint64_t>::infinity());
    m_next.time = m_current.time;
    m_first = true;
}

Vector
WaypointMobilityModel::DoGetVelocity() const
{
    return m_velocity;
}

} // namespace ns3
