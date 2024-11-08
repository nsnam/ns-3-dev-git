/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef CONSTANT_VELOCITY_HELPER_H
#define CONSTANT_VELOCITY_HELPER_H

#include "box.h"

#include "ns3/nstime.h"
#include "ns3/vector.h"

namespace ns3
{

class Rectangle;

/**
 * @ingroup mobility
 *
 * @brief Utility class used to move node with constant velocity.
 */
class ConstantVelocityHelper
{
  public:
    ConstantVelocityHelper();
    /**
     * Create object and set position
     * @param position the position vector
     */
    ConstantVelocityHelper(const Vector& position);
    /**
     * Create object and set position and velocity
     * @param position the position vector
     * @param vel the velocity vector
     */
    ConstantVelocityHelper(const Vector& position, const Vector& vel);

    /**
     * Set position vector
     * @param position Position vector
     */
    void SetPosition(const Vector& position);
    /**
     * Get current position vector
     * @return Position vector
     */
    Vector GetCurrentPosition() const;
    /**
     * Get velocity; if paused, will return a zero vector
     * @return Velocity vector
     */
    Vector GetVelocity() const;
    /**
     * Set new velocity vector
     * @param vel Velocity vector
     */
    void SetVelocity(const Vector& vel);
    /**
     * Pause mobility at current position
     */
    void Pause();
    /**
     * Resume mobility from current position at current velocity
     */
    void Unpause();

    /**
     * Update position, if not paused, from last position and time of last update
     * @param rectangle 2D bounding rectangle for resulting position; object will not move outside
     * the rectangle
     */
    void UpdateWithBounds(const Rectangle& rectangle) const;
    /**
     * Update position, if not paused, from last position and time of last update
     * @param bounds 3D bounding box for resulting position; object will not move outside the box
     */
    void UpdateWithBounds(const Box& bounds) const;
    /**
     * Update position, if not paused, from last position and time of last update
     */
    void Update() const;

  private:
    mutable Time m_lastUpdate; //!< time of last update
    mutable Vector m_position; //!< state variable for current position
    Vector m_velocity;         //!< state variable for velocity
    bool m_paused;             //!< state variable for paused
};

} // namespace ns3

#endif /* CONSTANT_VELOCITY_HELPER_H */
