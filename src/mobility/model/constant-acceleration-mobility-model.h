/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#ifndef CONSTANT_ACCELERATION_MOBILITY_MODEL_H
#define CONSTANT_ACCELERATION_MOBILITY_MODEL_H

#include "mobility-model.h"

#include "ns3/nstime.h"

namespace ns3
{

/**
 * @ingroup mobility
 *
 * @brief Mobility model for which the current acceleration does not change once it has been set and
 * until it is set again explicitly to a new value.
 */
class ConstantAccelerationMobilityModel : public MobilityModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Create position located at coordinates (0,0,0) with
     * speed (0,0,0).
     */
    ConstantAccelerationMobilityModel();
    ~ConstantAccelerationMobilityModel() override;
    /**
     * Set the model's velocity and acceleration
     * @param velocity the velocity (m/s)
     * @param acceleration the acceleration (m/s^2)
     */
    void SetVelocityAndAcceleration(const Vector& velocity, const Vector& acceleration);

  private:
    Vector DoGetPosition() const override;
    void DoSetPosition(const Vector& position) override;
    Vector DoGetVelocity() const override;

    Time m_baseTime;       //!< the base time
    Vector m_basePosition; //!< the base position
    Vector m_baseVelocity; //!< the base velocity
    Vector m_acceleration; //!< the acceleration
};

} // namespace ns3

#endif /* CONSTANT_ACCELERATION_MOBILITY_MODEL_H */
