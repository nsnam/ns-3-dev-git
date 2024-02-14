/*
 * Copyright (c) 2006,2007 INRIA
 * Copyright (c) 2019 University of Padova
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */
#include "random-walk-2d-outdoor-mobility-model.h"

#include "building-list.h"
#include "building.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <cmath>
#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RandomWalk2dOutdoor");

NS_OBJECT_ENSURE_REGISTERED(RandomWalk2dOutdoorMobilityModel);

TypeId
RandomWalk2dOutdoorMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RandomWalk2dOutdoorMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<RandomWalk2dOutdoorMobilityModel>()
            .AddAttribute("Bounds",
                          "Bounds of the area to cruise.",
                          RectangleValue(Rectangle(0.0, 100.0, 0.0, 100.0)),
                          MakeRectangleAccessor(&RandomWalk2dOutdoorMobilityModel::m_bounds),
                          MakeRectangleChecker())
            .AddAttribute("Time",
                          "Change current direction and speed after moving for this delay.",
                          TimeValue(Seconds(20.0)),
                          MakeTimeAccessor(&RandomWalk2dOutdoorMobilityModel::m_modeTime),
                          MakeTimeChecker())
            .AddAttribute("Distance",
                          "Change current direction and speed after moving for this distance.",
                          DoubleValue(30.0),
                          MakeDoubleAccessor(&RandomWalk2dOutdoorMobilityModel::m_modeDistance),
                          MakeDoubleChecker<double>())
            .AddAttribute("Mode",
                          "The mode indicates the condition used to "
                          "change the current speed and direction",
                          EnumValue(RandomWalk2dOutdoorMobilityModel::MODE_DISTANCE),
                          MakeEnumAccessor<Mode>(&RandomWalk2dOutdoorMobilityModel::m_mode),
                          MakeEnumChecker(RandomWalk2dOutdoorMobilityModel::MODE_DISTANCE,
                                          "Distance",
                                          RandomWalk2dOutdoorMobilityModel::MODE_TIME,
                                          "Time"))
            .AddAttribute("Direction",
                          "A random variable used to pick the direction (radians).",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"),
                          MakePointerAccessor(&RandomWalk2dOutdoorMobilityModel::m_direction),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute(
                "Speed",
                "A random variable used to pick the speed (m/s)."
                "The default value is taken from Figure 1 of the paper"
                "Henderson, L.F., 1971. The statistics of crowd fluids. nature, 229(5284), p.381.",
                StringValue("ns3::NormalRandomVariable[Mean=1.53|Variance=0.040401]"),
                MakePointerAccessor(&RandomWalk2dOutdoorMobilityModel::m_speed),
                MakePointerChecker<RandomVariableStream>())
            .AddAttribute("Tolerance",
                          "Tolerance for the intersection point with buildings (m)."
                          "It represents a small distance from where the building limit"
                          "is actually placed, for example to represent a sidewalk.",
                          DoubleValue(1e-6),
                          MakeDoubleAccessor(&RandomWalk2dOutdoorMobilityModel::m_epsilon),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxIterations",
                          "Maximum number of attempts to find an alternative next position"
                          "if the original one is inside a building.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&RandomWalk2dOutdoorMobilityModel::m_maxIter),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

void
RandomWalk2dOutdoorMobilityModel::DoInitialize()
{
    DoInitializePrivate();
    MobilityModel::DoInitialize();
}

void
RandomWalk2dOutdoorMobilityModel::DoInitializePrivate()
{
    m_helper.Update();
    double speed = m_speed->GetValue();
    double direction = m_direction->GetValue();
    Vector velocity(std::cos(direction) * speed, std::sin(direction) * speed, 0.0);
    m_helper.SetVelocity(velocity);
    m_helper.Unpause();

    Time delayLeft;
    if (m_mode == RandomWalk2dOutdoorMobilityModel::MODE_TIME)
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
RandomWalk2dOutdoorMobilityModel::DoWalk(Time delayLeft)
{
    if (delayLeft.IsNegative())
    {
        NS_LOG_INFO(this << " Ran out of time");
        return;
    }
    NS_LOG_FUNCTION(this << delayLeft.GetSeconds());

    Vector position = m_helper.GetCurrentPosition();
    Vector velocity = m_helper.GetVelocity();
    Vector nextPosition = position;
    nextPosition.x += velocity.x * delayLeft.GetSeconds();
    nextPosition.y += velocity.y * delayLeft.GetSeconds();
    m_event.Cancel();

    // check if the nextPosition is inside a building, or if the line
    // from position to the next position intersects a building
    auto outdoorBuilding = IsLineClearOfBuildings(position, nextPosition);
    bool outdoor = std::get<0>(outdoorBuilding);
    Ptr<Building> building = std::get<1>(outdoorBuilding);

    if (m_bounds.IsInside(nextPosition))
    {
        if (outdoor)
        {
            m_event = Simulator::Schedule(delayLeft,
                                          &RandomWalk2dOutdoorMobilityModel::DoInitializePrivate,
                                          this);
        }
        else
        {
            NS_LOG_LOGIC("NextPosition would lead into a building");
            nextPosition =
                CalculateIntersectionFromOutside(position, nextPosition, building->GetBoundaries());

            double delaySecondsX = std::numeric_limits<double>::max();
            double delaySecondsY = std::numeric_limits<double>::max();
            if (velocity.x != 0)
            {
                delaySecondsX = std::abs((nextPosition.x - position.x) / velocity.x);
            }
            if (velocity.y != 0)
            {
                delaySecondsY = std::abs((nextPosition.y - position.y) / velocity.y);
            }
            Time delay = Seconds(std::min(delaySecondsX, delaySecondsY));
            m_event = Simulator::Schedule(delay,
                                          &RandomWalk2dOutdoorMobilityModel::AvoidBuilding,
                                          this,
                                          delayLeft - delay,
                                          nextPosition);
        }
    }
    else
    {
        NS_LOG_LOGIC("Out of bounding box");
        nextPosition = m_bounds.CalculateIntersection(position, velocity);
        // check that this nextPosition is outdoor
        auto outdoorBuilding = IsLineClearOfBuildings(position, nextPosition);
        bool outdoor = std::get<0>(outdoorBuilding);
        Ptr<Building> building = std::get<1>(outdoorBuilding);

        if (outdoor)
        {
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
                NS_ABORT_MSG("RandomWalk2dOutdoorMobilityModel::DoWalk: unable to calculate the "
                             "rebound time "
                             "(the node is stationary).");
            }
            Time delay = Seconds(delaySeconds);
            m_event = Simulator::Schedule(delay,
                                          &RandomWalk2dOutdoorMobilityModel::Rebound,
                                          this,
                                          delayLeft - delay);
        }
        else
        {
            NS_LOG_LOGIC("NextPosition would lead into a building");
            nextPosition =
                CalculateIntersectionFromOutside(position, nextPosition, building->GetBoundaries());

            double delaySecondsX = std::numeric_limits<double>::max();
            double delaySecondsY = std::numeric_limits<double>::max();
            if (velocity.x != 0)
            {
                delaySecondsX =
                    std::min(delaySecondsX, std::abs((nextPosition.x - position.x) / velocity.x));
            }
            if (velocity.y != 0)
            {
                delaySecondsY =
                    std::min(delaySecondsY, std::abs((nextPosition.y - position.y) / velocity.y));
            }
            if (delaySecondsX == std::numeric_limits<double>::max() &&
                delaySecondsY == std::numeric_limits<double>::max())
            {
                NS_ABORT_MSG("RandomWalk2dOutdoorMobilityModel::DoWalk: unable to calculate the "
                             "rebound time "
                             "(the node is stationary).");
            }

            Time delay = Seconds(std::min(delaySecondsX, delaySecondsY));
            m_event = Simulator::Schedule(delay,
                                          &RandomWalk2dOutdoorMobilityModel::AvoidBuilding,
                                          this,
                                          delayLeft - delay,
                                          nextPosition);
        }
    }
    NS_LOG_LOGIC("Position " << position << " NextPosition " << nextPosition);

    // store the previous position
    m_prevPosition = position;
    NotifyCourseChange();
}

std::pair<bool, Ptr<Building>>
RandomWalk2dOutdoorMobilityModel::IsLineClearOfBuildings(Vector currentPosition,
                                                         Vector nextPosition) const
{
    NS_LOG_FUNCTION(this << currentPosition << nextPosition);

    bool intersectBuilding = false;
    double minIntersectionDistance = std::numeric_limits<double>::max();
    Ptr<Building> minIntersectionDistanceBuilding;

    for (auto bit = BuildingList::Begin(); bit != BuildingList::End(); ++bit)
    {
        // check if this building intersects the line between the current and next positions
        // this checks also if the next position is inside the building
        if ((*bit)->IsIntersect(currentPosition, nextPosition))
        {
            NS_LOG_LOGIC("Building " << (*bit)->GetBoundaries() << " intersects the line between "
                                     << currentPosition << " and " << nextPosition);
            auto intersection = CalculateIntersectionFromOutside(currentPosition,
                                                                 nextPosition,
                                                                 (*bit)->GetBoundaries());
            double distance = CalculateDistance(intersection, currentPosition);
            intersectBuilding = true;
            if (distance < minIntersectionDistance)
            {
                minIntersectionDistance = distance;
                minIntersectionDistanceBuilding = (*bit);
            }
        }
    }

    return std::make_pair(!intersectBuilding, minIntersectionDistanceBuilding);
}

Vector
RandomWalk2dOutdoorMobilityModel::CalculateIntersectionFromOutside(const Vector& current,
                                                                   const Vector& next,
                                                                   Box boundaries) const
{
    NS_LOG_FUNCTION(this << " current " << current << " next " << next);
    bool inside = boundaries.IsInside(current);
    NS_ASSERT(!inside);

    // get the closest side
    Rectangle rect = Rectangle(boundaries.xMin, boundaries.xMax, boundaries.yMin, boundaries.yMax);
    NS_LOG_DEBUG("rect " << rect);
    Rectangle::Side closestSide = rect.GetClosestSideOrCorner(current);

    double xIntersect = 0;
    double yIntersect = 0;

    switch (closestSide)
    {
    case Rectangle::RIGHTSIDE:
        NS_LOG_DEBUG("The closest side is RIGHT");
        xIntersect = boundaries.xMax + m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect =
            (next.y - current.y) / (next.x - current.x) * (xIntersect - current.x) + current.y;
        break;
    case Rectangle::LEFTSIDE:
        NS_LOG_DEBUG("The closest side is LEFT");
        xIntersect = boundaries.xMin - m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect =
            (next.y - current.y) / (next.x - current.x) * (xIntersect - current.x) + current.y;
        break;
    case Rectangle::TOPSIDE:
        NS_LOG_DEBUG("The closest side is TOP");
        yIntersect = boundaries.yMax + m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        xIntersect =
            (next.x - current.x) / (next.y - current.y) * (yIntersect - current.y) + current.x;
        break;
    case Rectangle::BOTTOMSIDE:
        NS_LOG_DEBUG("The closest side is BOTTOM");
        yIntersect = boundaries.yMin - m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        xIntersect =
            (next.x - current.x) / (next.y - current.y) * (yIntersect - current.y) + current.x;
        break;
    case Rectangle::TOPRIGHTCORNER:
        NS_LOG_DEBUG("The closest side is TOPRIGHT");
        xIntersect = boundaries.xMax + m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect = boundaries.yMax + m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        break;
    case Rectangle::TOPLEFTCORNER:
        NS_LOG_DEBUG("The closest side is TOPLEFT");
        xIntersect = boundaries.xMin - m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect = boundaries.yMax + m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        break;
    case Rectangle::BOTTOMRIGHTCORNER:
        NS_LOG_DEBUG("The closest side is BOTTOMRIGHT");
        xIntersect = boundaries.xMax + m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect = boundaries.yMin - m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        break;
    case Rectangle::BOTTOMLEFTCORNER:
        NS_LOG_DEBUG("The closest side is BOTTOMLEFT");
        xIntersect = boundaries.xMin - m_epsilon;
        NS_ABORT_MSG_IF(next.x - current.x == 0, "x position not updated");
        yIntersect = boundaries.yMin - m_epsilon;
        NS_ABORT_MSG_IF(next.y - current.y == 0, "y position not updated");
        break;
    }
    NS_LOG_DEBUG("xIntersect " << xIntersect << " yIntersect " << yIntersect);
    return Vector(xIntersect, yIntersect, 0);
}

void
RandomWalk2dOutdoorMobilityModel::Rebound(Time delayLeft)
{
    NS_LOG_FUNCTION(this << delayLeft.GetSeconds());
    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    Vector velocity = m_helper.GetVelocity();
    switch (m_bounds.GetClosestSideOrCorner(position))
    {
    case Rectangle::RIGHTSIDE:
    case Rectangle::LEFTSIDE:
        NS_LOG_DEBUG("The closest side is RIGHT or LEFT");
        velocity.x = -velocity.x;
        break;
    case Rectangle::TOPSIDE:
    case Rectangle::BOTTOMSIDE:
        NS_LOG_DEBUG("The closest side is TOP or BOTTOM");
        velocity.y = -velocity.y;
        break;
    case Rectangle::TOPRIGHTCORNER:
    case Rectangle::BOTTOMRIGHTCORNER:
    case Rectangle::TOPLEFTCORNER:
    case Rectangle::BOTTOMLEFTCORNER:
        NS_LOG_DEBUG("The closest side is a corner");
        velocity.x = -velocity.x;
        velocity.y = -velocity.y;
        break;
    }
    m_helper.SetVelocity(velocity);
    m_helper.Unpause();
    DoWalk(delayLeft);
}

void
RandomWalk2dOutdoorMobilityModel::AvoidBuilding(Time delayLeft, Vector intersectPosition)
{
    NS_LOG_FUNCTION(this << delayLeft.GetSeconds());
    m_helper.Update();

    bool nextWouldBeInside = true;
    uint32_t iter = 0;

    while (nextWouldBeInside && iter < m_maxIter)
    {
        NS_LOG_INFO("The next position would be inside a building, compute an alternative");
        iter++;
        double speed = m_speed->GetValue();
        double direction = m_direction->GetValue();
        Vector velocityVector(std::cos(direction) * speed, std::sin(direction) * speed, 0.0);
        m_helper.SetVelocity(velocityVector);

        Vector nextPosition = intersectPosition;
        nextPosition.x += velocityVector.x * delayLeft.GetSeconds();
        nextPosition.y += velocityVector.y * delayLeft.GetSeconds();

        // check if this is inside the current buildingBox
        auto outdoorBuilding = IsLineClearOfBuildings(intersectPosition, nextPosition);
        bool outdoor = std::get<0>(outdoorBuilding);

        if (!outdoor)
        {
            NS_LOG_LOGIC("inside loop intersect " << intersectPosition << " nextPosition "
                                                  << nextPosition << " " << outdoor << " building "
                                                  << std::get<1>(outdoorBuilding)->GetBoundaries());
        }
        else
        {
            NS_LOG_LOGIC("inside loop intersect " << intersectPosition << " nextPosition "
                                                  << nextPosition << " " << outdoor);
        }

        if (outdoor && m_bounds.IsInside(nextPosition))
        {
            nextWouldBeInside = false;
        }
    }

    // after m_maxIter iterations, the positions tested are all inside
    // to avoid increasing m_maxIter too much, it is possible to perform a step back
    // to the previous position and continue from there
    if (iter >= m_maxIter)
    {
        NS_LOG_INFO("Move back to the previous position");

        // compute the difference between the previous position and the intersection
        Vector posDiff = m_prevPosition - intersectPosition;
        // compute the distance
        double distance = CalculateDistance(m_prevPosition, intersectPosition);
        double speed = distance / delayLeft.GetSeconds(); // compute the speed

        NS_LOG_LOGIC("prev " << m_prevPosition << " intersectPosition " << intersectPosition
                             << " diff " << posDiff << " dist " << distance);

        Vector velocityVector(posDiff.x / distance * speed, posDiff.y / distance * speed, 0.0);
        m_helper.SetVelocity(velocityVector);

        Vector nextPosition = intersectPosition;
        nextPosition.x += velocityVector.x * delayLeft.GetSeconds();
        nextPosition.y += velocityVector.y * delayLeft.GetSeconds();

        // check if the path is clear
        auto outdoorBuilding = IsLineClearOfBuildings(intersectPosition, nextPosition);
        bool outdoor = std::get<0>(outdoorBuilding);
        if (!outdoor)
        {
            NS_LOG_LOGIC("The position is still inside after "
                         << m_maxIter + 1 << " iterations, loop intersect " << intersectPosition
                         << " nextPosition " << nextPosition << " " << outdoor << " building "
                         << std::get<1>(outdoorBuilding)->GetBoundaries());
            // This error may be due to buildings being attached to one another, or to the boundary
            // of the scenario.
            NS_FATAL_ERROR(
                "Not able to find an outdoor position. Try to increase the attribute MaxIterations "
                "and check the position of the buildings in the scenario.");
        }
        else
        {
            NS_LOG_LOGIC("inside loop intersect " << intersectPosition << " nextPosition "
                                                  << nextPosition << " " << outdoor);
        }
    }

    m_helper.Unpause();

    DoWalk(delayLeft);
}

void
RandomWalk2dOutdoorMobilityModel::DoDispose()
{
    // chain up
    MobilityModel::DoDispose();
}

Vector
RandomWalk2dOutdoorMobilityModel::DoGetPosition() const
{
    m_helper.UpdateWithBounds(m_bounds);
    return m_helper.GetCurrentPosition();
}

void
RandomWalk2dOutdoorMobilityModel::DoSetPosition(const Vector& position)
{
    NS_ASSERT(m_bounds.IsInside(position));
    m_helper.SetPosition(position);
    Simulator::Remove(m_event);
    m_event = Simulator::ScheduleNow(&RandomWalk2dOutdoorMobilityModel::DoInitializePrivate, this);
}

Vector
RandomWalk2dOutdoorMobilityModel::DoGetVelocity() const
{
    return m_helper.GetVelocity();
}

int64_t
RandomWalk2dOutdoorMobilityModel::DoAssignStreams(int64_t stream)
{
    m_speed->SetStream(stream);
    m_direction->SetStream(stream + 1);
    return 2;
}

} // namespace ns3
