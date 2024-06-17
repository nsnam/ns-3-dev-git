/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "propagation-delay-model.h"

#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(PropagationDelayModel);

TypeId
PropagationDelayModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PropagationDelayModel").SetParent<Object>().SetGroupName("Propagation");
    return tid;
}

PropagationDelayModel::~PropagationDelayModel()
{
}

int64_t
PropagationDelayModel::AssignStreams(int64_t stream)
{
    return DoAssignStreams(stream);
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(RandomPropagationDelayModel);

TypeId
RandomPropagationDelayModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RandomPropagationDelayModel")
            .SetParent<PropagationDelayModel>()
            .SetGroupName("Propagation")
            .AddConstructor<RandomPropagationDelayModel>()
            .AddAttribute("Variable",
                          "The random variable which generates random delays (s).",
                          StringValue("ns3::UniformRandomVariable"),
                          MakePointerAccessor(&RandomPropagationDelayModel::m_variable),
                          MakePointerChecker<RandomVariableStream>());
    return tid;
}

RandomPropagationDelayModel::RandomPropagationDelayModel()
{
}

RandomPropagationDelayModel::~RandomPropagationDelayModel()
{
}

Time
RandomPropagationDelayModel::GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    return Seconds(m_variable->GetValue());
}

int64_t
RandomPropagationDelayModel::DoAssignStreams(int64_t stream)
{
    m_variable->SetStream(stream);
    return 1;
}

NS_OBJECT_ENSURE_REGISTERED(ConstantSpeedPropagationDelayModel);

TypeId
ConstantSpeedPropagationDelayModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ConstantSpeedPropagationDelayModel")
            .SetParent<PropagationDelayModel>()
            .SetGroupName("Propagation")
            .AddConstructor<ConstantSpeedPropagationDelayModel>()
            .AddAttribute("Speed",
                          "The propagation speed (m/s) in the propagation medium being considered. "
                          "The default value is the propagation speed of light in the vacuum.",
                          DoubleValue(299792458),
                          MakeDoubleAccessor(&ConstantSpeedPropagationDelayModel::m_speed),
                          MakeDoubleChecker<double>());
    return tid;
}

ConstantSpeedPropagationDelayModel::ConstantSpeedPropagationDelayModel()
{
}

Time
ConstantSpeedPropagationDelayModel::GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    double distance = a->GetDistanceFrom(b);
    double seconds = distance / m_speed;
    return Seconds(seconds);
}

void
ConstantSpeedPropagationDelayModel::SetSpeed(double speed)
{
    m_speed = speed;
}

double
ConstantSpeedPropagationDelayModel::GetSpeed() const
{
    return m_speed;
}

int64_t
ConstantSpeedPropagationDelayModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
