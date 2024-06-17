/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "random-direction-2d-mobility-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RandomDirection2dMobilityModel");

NS_OBJECT_ENSURE_REGISTERED(RandomDirection2dMobilityModel);

TypeId
RandomDirection2dMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RandomDirection2dMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<RandomDirection2dMobilityModel>()
            .AddAttribute("Bounds",
                          "The 2d bounding area",
                          RectangleValue(Rectangle(-100, 100, -100, 100)),
                          MakeRectangleAccessor(&RandomDirection2dMobilityModel::m_bounds),
                          MakeRectangleChecker())
            .AddAttribute("Speed",
                          "A random variable to control the speed (m/s).",
                          StringValue("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                          MakePointerAccessor(&RandomDirection2dMobilityModel::m_speed),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("Pause",
                          "A random variable to control the pause (s).",
                          StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                          MakePointerAccessor(&RandomDirection2dMobilityModel::m_pause),
                          MakePointerChecker<RandomVariableStream>());
    return tid;
}

RandomDirection2dMobilityModel::RandomDirection2dMobilityModel()
{
    m_direction = CreateObject<UniformRandomVariable>();
}

RandomDirection2dMobilityModel::~RandomDirection2dMobilityModel()
{
    m_event.Cancel();
}

void
RandomDirection2dMobilityModel::DoDispose()
{
    // chain up.
    MobilityModel::DoDispose();
}

void
RandomDirection2dMobilityModel::DoInitialize()
{
    DoInitializePrivate();
    MobilityModel::DoInitialize();
}

void
RandomDirection2dMobilityModel::DoInitializePrivate()
{
    double direction = m_direction->GetValue(0, 2 * M_PI);
    SetDirectionAndSpeed(direction);
}

void
RandomDirection2dMobilityModel::BeginPause()
{
    m_helper.Update();
    m_helper.Pause();
    Time pause = Seconds(m_pause->GetValue());
    m_event.Cancel();
    m_event =
        Simulator::Schedule(pause, &RandomDirection2dMobilityModel::ResetDirectionAndSpeed, this);
    NotifyCourseChange();
}

void
RandomDirection2dMobilityModel::SetDirectionAndSpeed(double direction)
{
    NS_LOG_FUNCTION_NOARGS();
    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    double speed = m_speed->GetValue();
    const Vector vector(std::cos(direction) * speed, std::sin(direction) * speed, 0.0);
    m_helper.SetVelocity(vector);
    m_helper.Unpause();
    Vector next = m_bounds.CalculateIntersection(position, vector);
    Time delay = Seconds(CalculateDistance(position, next) / speed);
    m_event.Cancel();
    m_event = Simulator::Schedule(delay, &RandomDirection2dMobilityModel::BeginPause, this);
    NotifyCourseChange();
}

void
RandomDirection2dMobilityModel::ResetDirectionAndSpeed()
{
    double direction = 0;

    m_helper.UpdateWithBounds(m_bounds);
    Vector position = m_helper.GetCurrentPosition();
    switch (m_bounds.GetClosestSideOrCorner(position))
    {
    case Rectangle::RIGHTSIDE:
        direction = m_direction->GetValue(M_PI_2, M_PI + M_PI_2);
        break;
    case Rectangle::LEFTSIDE:
        direction = m_direction->GetValue(-M_PI_2, M_PI - M_PI_2);
        break;
    case Rectangle::TOPSIDE:
        direction = m_direction->GetValue(M_PI, 2 * M_PI);
        break;
    case Rectangle::BOTTOMSIDE:
        direction = m_direction->GetValue(0, M_PI);
        break;
    case Rectangle::TOPRIGHTCORNER:
        direction = m_direction->GetValue(M_PI, M_PI + M_PI_2);
        break;
    case Rectangle::TOPLEFTCORNER:
        direction = m_direction->GetValue(M_PI + M_PI_2, 2 * M_PI);
        break;
    case Rectangle::BOTTOMRIGHTCORNER:
        direction = m_direction->GetValue(0, M_PI_2);
        direction += M_PI / 2;
        break;
    case Rectangle::BOTTOMLEFTCORNER:
        direction = m_direction->GetValue(M_PI_2, M_PI);
        break;
    }
    SetDirectionAndSpeed(direction);
}

Vector
RandomDirection2dMobilityModel::DoGetPosition() const
{
    m_helper.UpdateWithBounds(m_bounds);
    return m_helper.GetCurrentPosition();
}

void
RandomDirection2dMobilityModel::DoSetPosition(const Vector& position)
{
    m_helper.SetPosition(position);
    m_event.Cancel();
    m_event = Simulator::ScheduleNow(&RandomDirection2dMobilityModel::DoInitializePrivate, this);
}

Vector
RandomDirection2dMobilityModel::DoGetVelocity() const
{
    return m_helper.GetVelocity();
}

int64_t
RandomDirection2dMobilityModel::DoAssignStreams(int64_t stream)
{
    m_direction->SetStream(stream);
    m_speed->SetStream(stream + 1);
    m_pause->SetStream(stream + 2);
    return 3;
}

} // namespace ns3
