/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef RANDOM_WAYPOINT_MOBILITY_MODEL_H
#define RANDOM_WAYPOINT_MOBILITY_MODEL_H

#include "constant-velocity-helper.h"
#include "mobility-model.h"
#include "position-allocator.h"

#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

/**
 * @ingroup mobility
 * @brief Random waypoint mobility model.
 *
 * Each object starts by pausing at time zero for the duration governed
 * by the random variable "Pause".  After pausing, the object will pick
 * a new waypoint (via the PositionAllocator) and a new random speed
 * via the random variable "Speed", and will begin moving towards the
 * waypoint at a constant speed.  When it reaches the destination,
 * the process starts over (by pausing).
 *
 * This mobility model enforces no bounding box by itself; the
 * PositionAllocator assigned to this object will bound the movement.
 * If the user fails to provide a pointer to a PositionAllocator to
 * be used to pick waypoints, the simulation program will assert.
 *
 * The implementation of this model is not 2d-specific. i.e. if you provide
 * a 3d random waypoint position model to this mobility model, the model
 * will still work. There is no 3d position allocator for now but it should
 * be trivial to add one.
 */
class RandomWaypointMobilityModel : public MobilityModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ~RandomWaypointMobilityModel() override;

  protected:
    void DoInitialize() override;

  private:
    /**
     * Get next position, begin moving towards it, schedule future pause event
     */
    void BeginWalk();
    /**
     * Begin current pause event, schedule future walk event
     */
    void DoInitializePrivate();
    Vector DoGetPosition() const override;
    void DoSetPosition(const Vector& position) override;
    Vector DoGetVelocity() const override;
    int64_t DoAssignStreams(int64_t) override;

    ConstantVelocityHelper m_helper;   //!< helper for velocity computations
    Ptr<PositionAllocator> m_position; //!< pointer to position allocator
    Ptr<RandomVariableStream> m_speed; //!< random variable to generate speeds
    Ptr<RandomVariableStream> m_pause; //!< random variable to generate pauses
    EventId m_event;                   //!< event ID of next scheduled event
};

} // namespace ns3

#endif /* RANDOM_WAYPOINT_MOBILITY_MODEL_H */
