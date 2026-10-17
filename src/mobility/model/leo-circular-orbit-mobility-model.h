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

namespace ns3
{

/**
 * @ingroup leo
 * @brief Keep track of the orbital position and velocity of a satellite.
 *
 * This uses simple circular orbits based on the inclination of the orbital
 * plane and the altitude of the satellite. Assumes earth is a perfect sphere.
 *
 * Note: Position in `DoSetPosition()` is in degrees, not the usual meters.
 * Note: Altitude is stored in km, _not_ the usual ns-3 m.
 *
 * This model is distinct from `GeocentricEcefMobilityModel` in that it
 * implements a dynamic circular orbital model, whereas `GeocentricEcefMobilityModel`
 * is intended for ECEF coordinates that do not evolve according to orbital dynamics.
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
    /// destructor
    ~LeoCircularOrbitMobilityModel() override;

    /**
     * @brief Gets the speed of the node
     * @return the speed in m/s
     */
    double GetSpeed() const;

    /**
     * @brief Get the orbital angular velocity.
     *
     * Computed from Keplerian mechanics: omega = sqrt(GM / r^3).
     * Negated for retrograde orbits (inclination > 90 degrees).
     *
     * If the user set an orbital angular velocity >= 0,
     * it will be used instead.
     *
     * @return angular velocity in rad/s
     */
    double GetAngularVelocity() const;

    /**
     * @brief Gets the altitude in m
     *
     * @return the altitude in m (above Earth surface)
     */
    double GetAltitude() const;
    /**
     * @brief Sets the altitude in m
     *
     * @param h the altitude in m (above Earth surface)
     */
    void SetAltitude(double h);

    /**
     * @brief Gets the altitude in km (not the usual meters used in ns-3)
     *
     * @return the altitude in km (above Earth surface)
     */
    double GetAltitudeKm() const;
    /**
     * @brief Sets the altitude in km (not the usual meters used in ns-3)
     *
     * @param h the altitude in km (above Earth surface)
     */
    void SetAltitudeKm(double h);

    /**
     * @brief Gets the inclination
     * @return the inclination in degrees
     */
    double GetInclination() const;

    /**
     * @brief Sets the inclination
     * @param incl the inclination in degrees
     */
    void SetInclination(double incl);

    /**
     * @brief Returns the Geocentric Position of the Node in ECEF (cartesian)
     * @return ECEF position in meters
     */
    Vector DoGetGeocentricPosition() const override;

    // Inherited from MobilityModel
    Ptr<MobilityModel> Copy() const override
    {
        return CreateObject<LeoCircularOrbitMobilityModel>(*this);
    }

  private:
    void DoInitialize() override;
    void DoDispose() override;

    /// Orbit height in km
    double m_orbitHeight;

    /// Inclination in rad
    double m_inclination;

    /// Longitudinal offset in rad
    double m_longitude;

    /// Argument of latitude in rad
    double m_argumentOfLatitude;

    /// Orbital speed in m/s; negative means compute from gravity
    double m_orbitalSpeed;

    /// Time interval between CourseChange notifications; zero disables
    /// periodic notifications
    Time m_resolutionTimeStep;

    /// Event for course change notification
    EventId m_courseChangeEvent;

    /**
     * @brief Fire NotifyCourseChange and reschedule at m_resolutionTimeStep.
     */
    void NotifyCourseChangeAndReschedule();

    /**
     * @brief Returns the node current position.
     * @return ECEF position in meters
     */
    Vector DoGetPosition() const override;

    /**
     * @brief Sets the node position via argument.
     *
     * Note: This method deviates from the `MobilityModel` interface which typically
     * expects position in meters. For this model, position is interpreted as angular
     * coordinates (degrees) because specifying orbital parameters (longitude of
     * ascending node and offset) is more intuitive in degrees than in meters
     * relative to the earth's center.
     *
     * @param position position.x is the longitude of the ascending node
     *        in degrees; position.y is the argument of latitude on the orbital plane
     *        in degrees.  Both are converted to radians internally.
     */
    void DoSetPosition(const Vector& position) override;

    /**
     * @brief Returns the current velocity of the node.
     * @return velocity vector in m/s (ECEF)
     */
    Vector DoGetVelocity() const override;

    /**
     * @brief Get the normal vector of the orbital plane
     * @param t the amount of time passed since the start of the simulation
     * @return the normal vector.
     */
    Vector3D PlaneNorm(Time t) const;

    /**
     * @brief Rotates a position vector by angle 'a' around the orbital plane
     * normal, using the Rodrigues rotation formula
     * (see https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula ).
     *
     * @param theta angle by which to rotate, in radians
     * @param v position vector to rotate, in meters (ECEF)
     * @param t time passed since simulation start (used to compute the
     *        current orbital plane normal)
     * @return rotated position vector in meters (ECEF)
     */
    Vector3D RotatePlane(double theta, const Vector3D& v, Time t) const;

    /**
     * @brief Calculate the position at time t
     *
     * The returned Vector contains Cartesian ECEF coordinates in meters.
     * Note that this differs from DoSetPosition(), where the Vector
     * encodes angular orbital parameters (longitude and offset in
     * degrees); that discrepancy is a consequence of the MobilityModel
     * base class interface, which uses a single Vector type for both
     * input and output.
     *
     * @param t simulation time
     * @return ECEF position in meters
     */
    Vector CalcPosition(Time t) const;

    /**
     * @brief Calculate the ECEF longitude of the ascending node at simulation time t
     *
     * The orbital plane drifts westward in the ECEF frame because the Earth
     * rotates eastward, so this returns m_longitude minus the Earth-rotation
     * angle accumulated over time t.
     *
     * @param t time
     * @return longitude of the ascending node in radians (ECEF)
     */
    double CalcLongitude(Time t) const;
};
} // namespace ns3

#endif
