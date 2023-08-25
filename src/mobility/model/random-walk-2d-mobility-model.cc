/*
 * Copyright (c) 2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
                          TimeValue(Seconds(1.0)),
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
                          MakeEnumAccessor(&RandomWalk2dMobilityModel::m_mode),
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

void
RandomWalk2dMobilityModel::DoInitialize()
{
    DoInitializePrivate();
    MobilityModel::DoInitialize();
}

void
RandomWalk2dMobilityModel::DoInitializePrivate()
{
    m_helper.Update();
    Vector position = m_helper.GetCurrentPosition();

    double speed = m_speed->GetValue();
    double direction = 0;
    if (!m_bounds.IsOnTheBorder(position))
    {
        direction = m_direction->GetValue();
    }
    else
    {
        Ptr<UniformRandomVariable> directionOverride = CreateObject<UniformRandomVariable>();
        switch (m_bounds.GetClosestSideOrCorner(position))
        {
        case Rectangle::RIGHTSIDE:
            direction = directionOverride->GetValue(M_PI_2, M_PI + M_PI_2);
            break;
        case Rectangle::LEFTSIDE:
            direction = directionOverride->GetValue(-M_PI_2, M_PI - M_PI_2);
            break;
        case Rectangle::TOPSIDE:
            direction = directionOverride->GetValue(M_PI, 2 * M_PI);
            break;
        case Rectangle::BOTTOMSIDE:
            direction = directionOverride->GetValue(0, M_PI);
            break;
        case Rectangle::TOPRIGHTCORNER:
            direction = directionOverride->GetValue(M_PI, M_PI + M_PI_2);
            break;
        case Rectangle::TOPLEFTCORNER:
            direction = directionOverride->GetValue(M_PI + M_PI_2, 2 * M_PI);
            break;
        case Rectangle::BOTTOMRIGHTCORNER:
            direction = directionOverride->GetValue(0, M_PI_2);
            direction += M_PI / 2;
            break;
        case Rectangle::BOTTOMLEFTCORNER:
            direction = directionOverride->GetValue(M_PI_2, M_PI);
            break;
        }
    }
    Vector vector(std::cos(direction) * speed, std::sin(direction) * speed, 0.0);
    m_helper.SetVelocity(vector);
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
    DoWalk(delayLeft);
}

void
RandomWalk2dMobilityModel::DoWalk(Time delayLeft)
{
    Vector position = m_helper.GetCurrentPosition();
    Vector speed = m_helper.GetVelocity();
    Vector nextPosition = position;
    nextPosition.x += speed.x * delayLeft.GetSeconds();
    nextPosition.y += speed.y * delayLeft.GetSeconds();
    m_event.Cancel();
    if (m_bounds.IsInside(nextPosition))
    {
        m_event =
            Simulator::Schedule(delayLeft, &RandomWalk2dMobilityModel::DoInitializePrivate, this);
    }
    else
    {
        nextPosition = m_bounds.CalculateIntersection(position, speed);
        double delaySeconds = std::numeric_limits<double>::max();
        if (speed.x != 0)
        {
            delaySeconds =
                std::min(delaySeconds, std::abs((nextPosition.x - position.x) / speed.x));
        }
        else if (speed.y != 0)
        {
            delaySeconds =
                std::min(delaySeconds, std::abs((nextPosition.y - position.y) / speed.y));
        }
        else
        {
            NS_ABORT_MSG("RandomWalk2dMobilityModel::DoWalk: unable to calculate the rebound time "
                         "(the node is stationary).");
        }
        Time delay = Seconds(delaySeconds);
        m_event = Simulator::Schedule(delay,
                                      &RandomWalk2dMobilityModel::Rebound,
                                      this,
                                      delayLeft - delay);
    }
    NotifyCourseChange();
}

void
RandomWalk2dMobilityModel::Rebound(Time delayLeft)
{
    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    Vector speed = m_helper.GetVelocity();
    switch (m_bounds.GetClosestSideOrCorner(position))
    {
    case Rectangle::RIGHTSIDE:
    case Rectangle::LEFTSIDE:
        speed.x = -speed.x;
        break;
    case Rectangle::TOPSIDE:
    case Rectangle::BOTTOMSIDE:
        speed.y = -speed.y;
        break;
    case Rectangle::TOPRIGHTCORNER:
    case Rectangle::BOTTOMRIGHTCORNER:
    case Rectangle::TOPLEFTCORNER:
    case Rectangle::BOTTOMLEFTCORNER:
        auto temp = speed.x;
        speed.x = -speed.y;
        speed.y = -temp;
        break;
    }
    m_helper.SetVelocity(speed);
    m_helper.Unpause();
    DoWalk(delayLeft);
}

void
RandomWalk2dMobilityModel::DoDispose()
{
    // chain up
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
    NS_ASSERT(m_bounds.IsInside(position));
    m_helper.SetPosition(position);
    m_event.Cancel();
    m_event = Simulator::ScheduleNow(&RandomWalk2dMobilityModel::DoInitializePrivate, this);
}

Vector
RandomWalk2dMobilityModel::DoGetVelocity() const
{
    return m_helper.GetVelocity();
}

int64_t
RandomWalk2dMobilityModel::DoAssignStreams(int64_t stream)
{
    m_speed->SetStream(stream);
    m_direction->SetStream(stream + 1);
    return 2;
}

} // namespace ns3
