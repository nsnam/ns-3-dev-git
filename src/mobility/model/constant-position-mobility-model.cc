/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "constant-position-mobility-model.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(ConstantPositionMobilityModel);

TypeId
ConstantPositionMobilityModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ConstantPositionMobilityModel")
                            .SetParent<MobilityModel>()
                            .SetGroupName("Mobility")
                            .AddConstructor<ConstantPositionMobilityModel>();
    return tid;
}

ConstantPositionMobilityModel::ConstantPositionMobilityModel()
{
}

ConstantPositionMobilityModel::~ConstantPositionMobilityModel()
{
}

Vector
ConstantPositionMobilityModel::DoGetPosition() const
{
    return m_position;
}

void
ConstantPositionMobilityModel::DoSetPosition(const Vector& position)
{
    m_position = position;
    NotifyCourseChange();
}

Vector
ConstantPositionMobilityModel::DoGetVelocity() const
{
    return Vector(0.0, 0.0, 0.0);
}

} // namespace ns3
