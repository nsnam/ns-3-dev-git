/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "random-walk-2d-mobility-model.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RandomWalk2d");

NS_OBJECT_ENSURE_REGISTERED(RandomWalk2dMobilityModel);

TypeId
RandomWalk2dMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RandomWalk2dMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<RandomWalk2dMobilityModel>()
            .AddAttribute("Bounds",
                          "Bounds of the area to cruise.",
                          RectangleValue(Rectangle(0.0, 100.0, 0.0, 100.0)),
                          MakeRectangleAccessor(&RandomWalk2dMobilityModel::m_bounds),
                          MakeRectangleChecker())
            .AddAttribute("Time",
                          "Change current direction and speed after moving for this delay.",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&RandomWalk2dMobilityModel::m_modeTime),
                          MakeTimeChecker())
            .AddAttribute("Distance",
                          "Change current direction and speed after moving for this distance.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&RandomWalk2dMobilityModel::m_modeDistance),
                          MakeDoubleChecker<double>())
            .AddAttribute("Mode",
                          "The mode indicates the condition used to "
                          "change the current speed and direction",
                          EnumValue(RandomWalk2dMobilityModel::MODE_DISTANCE),
                          MakeEnumAccessor<RandomWalk2dMobilityModel::Mode>(
                              &RandomWalk2dMobilityModel::m_mode),
                          MakeEnumChecker(RandomWalk2dMobilityModel::MODE_DISTANCE,
                                          "Distance",
                                          RandomWalk2dMobilityModel::MODE_TIME,
                                          "Time"))
            .AddAttribute("Direction",
                          "A random variable used to pick the direction (radians).",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"),
                          MakePointerAccessor(&RandomWalk2dMobilityModel::m_direction),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("Speed",
                          "A random variable used to pick the speed (m/s).",
                          StringValue("ns3::UniformRandomVariable[Min=2.0|Max=4.0]"),
                          MakePointerAccessor(&RandomWalk2dMobilityModel::m_speed),
                          MakePointerChecker<RandomVariableStream>());
    return tid;
}

RandomWalk2dMobilityModel::~RandomWalk2dMobilityModel()
{
    m_event.Cancel();
}

void
RandomWalk2dMobilityModel::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    DrawRandomVelocityAndDistance();
    MobilityModel::DoInitialize();
}

// Set new velocity and distance to travel, and call DoWalk() to initiate movement
void
RandomWalk2dMobilityModel::DrawRandomVelocityAndDistance()
{
    NS_LOG_FUNCTION(this);
    m_helper.Update();
    Vector position = m_helper.GetCurrentPosition();

    double speed = m_speed->GetValue();
    double direction = m_direction->GetValue();
    Vector velocity(std::cos(direction) * speed, std::sin(direction) * speed, 0.0);
    if (m_bounds.IsOnTheBorder(position))
    {
        switch (m_bounds.GetClosestSideOrCorner(position))
        {
        case Rectangle::RIGHTSIDE:
            velocity.x = -1 * std::abs(velocity.x);
            break;
        case Rectangle::LEFTSIDE:
            velocity.x = std::abs(velocity.x);
            break;
        case Rectangle::TOPSIDE:
            velocity.y = -1 * std::abs(velocity.y);
            break;
        case Rectangle::BOTTOMSIDE:
            velocity.y = std::abs(velocity.y);
            break;
        case Rectangle::TOPRIGHTCORNER:
            velocity.x = -1 * std::abs(velocity.x);
            velocity.y = -1 * std::abs(velocity.y);
            break;
        case Rectangle::TOPLEFTCORNER:
            velocity.x = std::abs(velocity.x);
            velocity.y = -1 * std::abs(velocity.y);
            break;
        case Rectangle::BOTTOMRIGHTCORNER:
            velocity.x = -1 * std::abs(velocity.x);
            velocity.y = std::abs(velocity.y);
            break;
        case Rectangle::BOTTOMLEFTCORNER:
            velocity.x = std::abs(velocity.x);
            velocity.y = std::abs(velocity.y);
            break;
        }
    }
    NS_LOG_INFO("Setting new velocity to " << velocity);
    m_helper.SetVelocity(velocity);
    m_helper.Unpause();

    Time delayLeft;
    if (m_mode == RandomWalk2dMobilityModel::MODE_TIME)
    {
        delayLeft = m_modeTime;
    }
    else
    {
        delayLeft = Seconds(m_modeDistance / speed);
    }
    NS_LOG_INFO("Setting delayLeft for DoWalk() to " << delayLeft.As(Time::S));
    DoWalk(delayLeft);
}

// Notify course change and schedule event for either a wall rebound or a new velocity
void
RandomWalk2dMobilityModel::DoWalk(Time delayLeft)
{
    NS_LOG_FUNCTION(this << delayLeft.As(Time::S));
    Vector position = m_helper.GetCurrentPosition();
    Vector velocity = m_helper.GetVelocity();
    Vector nextPosition = position;
    nextPosition.x += velocity.x * delayLeft.GetSeconds();
    nextPosition.y += velocity.y * delayLeft.GetSeconds();
    m_event.Cancel();
    if (m_bounds.IsInside(nextPosition))
    {
        NS_LOG_INFO("Scheduling new velocity in " << delayLeft.As(Time::S));
        m_event = Simulator::Schedule(delayLeft,
                                      &RandomWalk2dMobilityModel::DrawRandomVelocityAndDistance,
                                      this);
    }
    else
    {
        nextPosition = m_bounds.CalculateIntersection(position, velocity);
        double delaySeconds = std::numeric_limits<double>::max();
        if (velocity.x != 0)
        {
            delaySeconds =
                std::min(delaySeconds, std::abs((nextPosition.x - position.x) / velocity.x));
        }
        else if (velocity.y != 0)
        {
            delaySeconds =
                std::min(delaySeconds, std::abs((nextPosition.y - position.y) / velocity.y));
        }
        else
        {
            NS_ABORT_MSG("RandomWalk2dMobilityModel::DoWalk: unable to calculate the rebound time "
                         "(the node is stationary).");
        }
        Time delay = Seconds(delaySeconds);
        NS_LOG_INFO("Scheduling a rebound event in " << (delayLeft - delay).As(Time::S));
        m_event = Simulator::Schedule(delay,
                                      &RandomWalk2dMobilityModel::Rebound,
                                      this,
                                      delayLeft - delay);
    }
    NotifyCourseChange();
}

// Set a velocity from the previous velocity, and start motion with remaining time
void
RandomWalk2dMobilityModel::Rebound(Time delayLeft)
{
    NS_LOG_FUNCTION(this << delayLeft.As(Time::S));
    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    Vector velocity = m_helper.GetVelocity();
    switch (m_bounds.GetClosestSideOrCorner(position))
    {
    case Rectangle::RIGHTSIDE:
    case Rectangle::LEFTSIDE:
        velocity.x = -velocity.x;
        break;
    case Rectangle::TOPSIDE:
    case Rectangle::BOTTOMSIDE:
        velocity.y = -velocity.y;
        break;
    case Rectangle::TOPRIGHTCORNER:
    case Rectangle::BOTTOMRIGHTCORNER:
    case Rectangle::TOPLEFTCORNER:
    case Rectangle::BOTTOMLEFTCORNER:
        velocity.x = -velocity.x;
        velocity.y = -velocity.y;
        break;
    }
    m_helper.SetVelocity(velocity);
    m_helper.Unpause();
    NS_LOG_INFO("Rebounding with new velocity " << velocity);
    DoWalk(delayLeft);
}

void
RandomWalk2dMobilityModel::DoDispose()
{
    // chain up
    NS_LOG_FUNCTION(this);
    MobilityModel::DoDispose();
}

Vector
RandomWalk2dMobilityModel::DoGetPosition() const
{
    m_helper.UpdateWithBounds(m_bounds);
    return m_helper.GetCurrentPosition();
}

void
RandomWalk2dMobilityModel::DoSetPosition(const Vector& position)
{
    NS_LOG_FUNCTION(this << position);
    NS_ASSERT(m_bounds.IsInside(position));
    m_helper.SetPosition(position);
    m_event.Cancel();
    m_event =
        Simulator::ScheduleNow(&RandomWalk2dMobilityModel::DrawRandomVelocityAndDistance, this);
}

Vector
RandomWalk2dMobilityModel::DoGetVelocity() const
{
    return m_helper.GetVelocity();
}

int64_t
RandomWalk2dMobilityModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_speed->SetStream(stream);
    m_direction->SetStream(stream + 1);
    return 2;
}

} // namespace ns3
