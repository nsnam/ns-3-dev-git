/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#include "constant-acceleration-mobility-model.h"

#include "ns3/simulator.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(ConstantAccelerationMobilityModel);

TypeId
ConstantAccelerationMobilityModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ConstantAccelerationMobilityModel")
                            .SetParent<MobilityModel>()
                            .SetGroupName("Mobility")
                            .AddConstructor<ConstantAccelerationMobilityModel>();
    return tid;
}

ConstantAccelerationMobilityModel::ConstantAccelerationMobilityModel()
{
}

ConstantAccelerationMobilityModel::~ConstantAccelerationMobilityModel()
{
}

inline Vector
ConstantAccelerationMobilityModel::DoGetVelocity() const
{
    double t = (Simulator::Now() - m_baseTime).GetSeconds();
    return Vector(m_baseVelocity.x + m_acceleration.x * t,
                  m_baseVelocity.y + m_acceleration.y * t,
                  m_baseVelocity.z + m_acceleration.z * t);
}

inline Vector
ConstantAccelerationMobilityModel::DoGetPosition() const
{
    double t = (Simulator::Now() - m_baseTime).GetSeconds();
    double half_t_square = t * t * 0.5;
    return Vector(m_basePosition.x + m_baseVelocity.x * t + m_acceleration.x * half_t_square,
                  m_basePosition.y + m_baseVelocity.y * t + m_acceleration.y * half_t_square,
                  m_basePosition.z + m_baseVelocity.z * t + m_acceleration.z * half_t_square);
}

void
ConstantAccelerationMobilityModel::DoSetPosition(const Vector& position)
{
    m_baseVelocity = DoGetVelocity();
    m_baseTime = Simulator::Now();
    m_basePosition = position;
    NotifyCourseChange();
}

void
ConstantAccelerationMobilityModel::SetVelocityAndAcceleration(const Vector& velocity,
                                                              const Vector& acceleration)
{
    m_basePosition = DoGetPosition();
    m_baseTime = Simulator::Now();
    m_baseVelocity = velocity;
    m_acceleration = acceleration;
    NotifyCourseChange();
}

} // namespace ns3
