// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_CIRCULAR_ORBIT_MOBILITY_MODEL_H
#define LEO_CIRCULAR_ORBIT_MOBILITY_MODEL_H

#include "geocentric-constant-position-mobility-model.h"
#include "mobility-model.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/vector.h"

/**
 * @file
 * @ingroup leo
 *
 * Declaration of LeoCircularOrbitMobilityModel
 */

/// geocentric gravitational constant in KM^3/s^2 https://ntrs.nasa.gov/citations/19760058274
constexpr double LEO_EARTH_GGC = 398600.7;

namespace ns3
{

/**
 * @ingroup leo
 * @brief Keep track of the orbital position and velocity of a satellite.
 *
 * This uses simple circular orbits based on the inclination of the orbital
 * plane and the height of the satellite.
 */
class LeoCircularOrbitMobilityModel : public GeocentricConstantPositionMobilityModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /// constructor
    LeoCircularOrbitMobilityModel();

    /**
     * @brief Gets the speed of the node
     * @return the speed in m/s
     */
    double GetSpeed() const;

    /**
     * @brief Gets the altitude in km
     * @return the altitude in km
     */
    double GetAltitude() const;
    /**
     * @brief Sets the altitude in km
     * @param h the altitude km
     */
    void SetAltitude(double h);

    /**
     * @brief Gets the inclination
     * @return the inclination
     */
    double GetInclination() const;

    /**
     * @brief Sets the inclination
     * @param incl the inclination
     */
    void SetInclination(double incl);

    /**
     * @brief Sets the index of the node at the Progress Vector
     * @param index the node's index at the Progress Vector
     */
    void SetNodeIndexAtProgressVector(uint64_t index);

    /**
     * @brief Links this node to a certain Progress Vector - probably shared among other nodes
     * @param ptr a pointer to the Progress Vector container
     */
    void SetProgressVectorPointer(const std::shared_ptr<std::vector<double>>& ptr);

    /**
     * @brief Orders the calculation of the node position, notifies course change, advances
     * the node index at the Progress Vector and schedules the next update event.
     * @return position that will be returned upon next call to DoGetPosition
     */
    Vector UpdateNodePositionAndScheduleEvent();

    /**
     * @brief Returns the Geocentric Position of the Node in ECEF (cartesian)
     * @return a vector representing the node position
     */
    Vector DoGetGeocentricPosition() const override;

    // Inherited from MobilityModel
    Ptr<MobilityModel> Copy() const override
    {
        return CreateObject<LeoCircularOrbitMobilityModel>(*this);
    }

  private:
    /// Orbit height in km
    double m_orbitHeight;

    /// Inclination in rad
    double m_inclination;

    /// Longitudinal offset in rad
    double m_longitude;

    /// Offset on the orbital plane in rad
    double m_offset;

    /// Current position
    Vector3D m_position;

    /// Time precision for positions
    Time m_precision;

    /// The index of the node in the Progress Vector
    uint16_t m_nodeIndexAtProgressVector = 0;

    /// A pointer to a progress vector that is shared among all nodes that have the same altitude
    std::shared_ptr<std::vector<double>> m_progressVector;

    /**
     * @brief Returns the node current position.
     * @return the current position.
     */
    Vector DoGetPosition() const override;

    /**
     * @brief Sets the node position via argument.
     * @param position the position to set.
     */
    void DoSetPosition(const Vector& position) override;

    /**
     * @brief Returns the current velocity of the node.
     * @return the current velocity.
     */
    Vector DoGetVelocity() const override;

    /**
     * @brief Get the normal vector of the orbital plane
     * @param t the amount of time passed since the start of the simulation
     * @return the normal vector.
     */
    Vector3D PlaneNorm(Time t) const;

    /**
     * @brief Advances a satellite by 'a' degrees inside the orbital plane
     * @param a angle by which to rotate
     * @param x vector to rotate
     * @param t time passed since simulation start (used to calculate rotation offset)
     * @return rotated vector.
     */
    Vector3D RotatePlane(double a, const Vector3D& x, Time t) const;

    /**
     * @brief Calculate the position at time t
     * @param t time
     * @return position at time t
     */
    Vector CalcPosition(Time t) const;

    /**
     * @brief Calculate the latitude depending on simulation time
     * @param t time
     * @return latitude
     */
    double CalcLatitude(Time t) const;
};
} // namespace ns3

#endif
