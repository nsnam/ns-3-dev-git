/*
 * Copyright (c) 2009 Phillip Sitbon
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Phillip Sitbon <phillip@sitbon.net>
 */
#ifndef WAYPOINT_MOBILITY_MODEL_H
#define WAYPOINT_MOBILITY_MODEL_H

#include "mobility-model.h"
#include "waypoint.h"

#include "ns3/vector.h"

#include <deque>
#include <stdint.h>

class WaypointMobilityModelNotifyTest;

namespace ns3
{

/**
 * @ingroup mobility
 * @brief Waypoint-based mobility model.
 *
 * Each object determines its velocity and position at a given time
 * from a set of ns3::Waypoint objects.  Past waypoints are discarded
 * after the current simulation time greater than their time value.
 *
 * By default, the initial position of each object corresponds to the
 * position of the first waypoint, and the initial velocity of each
 * object is zero.  Upon reaching the last waypoint, object position
 * becomes static and velocity is zero.
 *
 * When a node is in between waypoint times, it moves with a constant
 * velocity between the position at the previous waypoint and the position
 * at the current waypoint. To make a node hold a certain position for a
 * time interval, two waypoints with the same position (but different times)
 * should be inserted sequentially.
 *
 * Waypoints can be added at any time, and setting the current position
 * of an object will set its velocity to zero until the next waypoint time
 * (at which time the object jumps to the next waypoint), unless there are
 * no more waypoints in which case it will not change without user
 * intervention.
 *
 * The model has two attributes with methods that allow clients to get
 * the next waypoint value (NextWaypoint) and the number of waypoints left
 * (WaypointsLeft) beyond (but not including) the next waypoint.
 *
 * In addition, there are two attributes that govern model behavior.  The
 * first, LazyNotify, governs how the model calls the CourseChange trace.
 * By default, LazyNotify is false, which means that each time that a
 * waypoint time is hit, an Update() is forced and the CourseChange
 * callback will be called.  When LazyNotify is true, Update() is suppressed
 * at waypoint times, and CourseChange callbacks will only occur when
 * there later are actual calls to Update () (typically when calling
 * GetPosition ()).  This option may be enabled for execution run-time
 * performance improvements, but when enabled, users should note that
 * course change listeners will in general not be notified at waypoint
 * times but instead at the next Update() following a waypoint time,
 * and some waypoints may not be notified to course change listeners.
 *
 * The second, InitialPositionIsWaypoint, is false by default.  Recall
 * that the first waypoint will set the initial position and set velocity
 * equal to 0 until the first waypoint time.  In such a case, the
 * call to SetPosition(), such as from a PositionAllocator, will be
 * ignored.  However, if InitialPositionIsWaypoint is set to true
 * and SetPosition() is called before any waypoints have been added,
 * the SetPosition() call is treated as an initial waypoint at time zero.
 * In such a case, when SetPosition() is treated as an initial waypoint,
 * it should be noted that attempts to add a waypoint at the same time
 * will cause the program to fail.
 */
class WaypointMobilityModel : public MobilityModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create a path with no waypoints at location (0,0,0).
     */
    WaypointMobilityModel();
    ~WaypointMobilityModel() override;

    /**
     * @param waypoint waypoint to append to the object path.
     *
     * Add a waypoint to the path of the object. The time must
     * be greater than the previous waypoint added, otherwise
     * a fatal error occurs. The first waypoint is set as the
     * current position with a velocity of zero.
     *
     */
    void AddWaypoint(const Waypoint& waypoint);

    /**
     * Get the waypoint that this object is traveling towards.
     * @returns The waypoint
     */
    Waypoint GetNextWaypoint() const;

    /**
     * Get the number of waypoints left for this object, excluding
     * the next one.
     * @returns The number of waypoints left
     */
    uint32_t WaypointsLeft() const;

    /**
     * Clear any existing waypoints and set the current waypoint
     * time to infinity. Calling this is only an optimization and
     * not required. After calling this function, adding waypoints
     * behaves as it would for a new object.
     */
    void EndMobility();

  private:
    friend class ::WaypointMobilityModelNotifyTest; // To allow Update() calls and access to
                                                    // m_current

    /**
     * Update the underlying state corresponding to the stored waypoints
     */
    virtual void Update() const;
    /**
     * @brief The dispose method.
     *
     * Subclasses must override this method.
     */
    void DoDispose() override;
    /**
     * @brief Get current position.
     * @return A vector with the current position of the node.
     */
    Vector DoGetPosition() const override;
    /**
     * @brief Sets a new position for the node
     * @param position A vector to be added as the new position
     */
    void DoSetPosition(const Vector& position) override;
    /**
     * @brief Returns the current velocity of a node
     * @return The velocity vector of a node.
     */
    Vector DoGetVelocity() const override;

  protected:
    /**
     * @brief This variable is set to true if there are no waypoints in the std::deque
     */
    bool m_first;
    /**
     * @brief If true, course change updates are only notified when position
     * is calculated.
     */
    bool m_lazyNotify;
    /**
     * @brief If true, calling SetPosition with no waypoints creates a waypoint
     */
    bool m_initialPositionIsWaypoint;
    /**
     * @brief The double ended queue containing the ns3::Waypoint objects
     */
    mutable std::deque<Waypoint> m_waypoints;
    /**
     * @brief The ns3::Waypoint currently being used
     */
    mutable Waypoint m_current;
    /**
     * @brief The next ns3::Waypoint in the deque
     */
    mutable Waypoint m_next;
    /**
     * @brief The current velocity vector
     */
    mutable Vector m_velocity;
    /**
     * @brief Update event
     */
    EventId m_event;
};

} // namespace ns3

#endif /* WAYPOINT_MOBILITY_MODEL_H */
