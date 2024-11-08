/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef CONSTANT_POSITION_MOBILITY_MODEL_H
#define CONSTANT_POSITION_MOBILITY_MODEL_H

#include "mobility-model.h"

namespace ns3
{

/**
 * @ingroup mobility
 *
 * @brief Mobility model for which the current position does not change once it has been set and
 * until it is set again explicitly to a new value.
 */
class ConstantPositionMobilityModel : public MobilityModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Create a position located at coordinates (0,0,0)
     */
    ConstantPositionMobilityModel();
    ~ConstantPositionMobilityModel() override;

  private:
    Vector DoGetPosition() const override;
    void DoSetPosition(const Vector& position) override;
    Vector DoGetVelocity() const override;

    Vector m_position; //!< the constant position
};

} // namespace ns3

#endif /* CONSTANT_POSITION_MOBILITY_MODEL_H */
