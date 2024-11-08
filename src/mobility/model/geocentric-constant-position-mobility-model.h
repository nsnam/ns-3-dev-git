/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mattia Sandri <mattia.sandri@unipd.it>
 */
#ifndef GEOCENTRIC_CONSTANT_POSITION_MOBILITY_MODEL_H
#define GEOCENTRIC_CONSTANT_POSITION_MOBILITY_MODEL_H

#include "geographic-positions.h"
#include "mobility-model.h"

/**
 * @file
 * @ingroup mobility
 * Class GeocentricConstantPositionMobilityModel declaration.
 */

namespace ns3
{

/**
 * @brief Mobility model using geocentric euclidean coordinates, as defined in 38.811 chapter 6.3
 */
class GeocentricConstantPositionMobilityModel : public MobilityModel
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
    GeocentricConstantPositionMobilityModel() = default;
    ~GeocentricConstantPositionMobilityModel() override = default;

    /**
     * @brief Computes elevation angle between a ground terminal and a HAPS/Satellite.
     * After calculating the plane perpendicular to a cartesian position vector,
     * the elevation angle is calculated using
     * https://www.w3schools.blog/angle-between-a-line-and-a-plane.
     * The altitude of the position passed as a parameter must be higher than that of the reference
     * point.
     * @param other pointer to the HAPS/satellite mobility model
     * @return the elevation angle
     * in degrees
     */
    virtual double GetElevationAngle(Ptr<const GeocentricConstantPositionMobilityModel> other);

    /**
     * @brief Get the position using geographic (geodetic) coordinates
     * @return Vector containing (latitude (degree), longitude (degree), altitude (meter))
     */
    virtual Vector GetGeographicPosition() const;

    /**
     * @brief Set the position using geographic coordinates
     *
     * Sets the position, using geographic coordinates and asserting
     * that the provided parameter falls within the appropriate range.
     *
     * @param latLonAlt pointer to a Vector containing (latitude (degree), longitude (degree),
     * altitude (meter)). The values are expected to be in the ranges [-90, 90], [-180, 180], [0,
     * +inf[, respectively. These assumptions are enforced with an assert for the latitude and the
     * altitude, while the longitude is normalized to the expected range.
     */
    virtual void SetGeographicPosition(const Vector& latLonAlt);

    /**
     * @brief Get the position using Geocentric Cartesian coordinates
     * @return Vector containing (x, y, z) (meter) coordinates
     */
    virtual Vector GetGeocentricPosition() const;

    /**
     * @brief Set the position using Geocentric Cartesian coordinates
     * @param position pointer to a Vector containing (x, y, z) (meter) coordinates
     */
    virtual void SetGeocentricPosition(const Vector& position);

    /**
     * @brief Set the reference point for coordinate conversion
     * @param refPoint vector containing the geographic reference point (meter)
     */
    virtual void SetCoordinateTranslationReferencePoint(const Vector& refPoint);

    /**
     * @brief Get the reference point for coordinate conversion
     * @return Vector containing geographic reference point (meter)
     */
    virtual Vector GetCoordinateTranslationReferencePoint() const;

    /**
     * @return the current position
     */
    virtual Vector GetPosition() const;

    /**
     * @param position the position to set.
     */
    virtual void SetPosition(const Vector& position);

    /**
     * @param other a reference to another mobility model
     * @return the distance between the two objects. Unit is meters.
     */
    double GetDistanceFrom(Ptr<const GeocentricConstantPositionMobilityModel> other) const;

  private:
    /** @copydoc GetPosition() */
    Vector DoGetPosition() const override;
    /** @copydoc SetPosition() */
    void DoSetPosition(const Vector& position) override;
    /** @copydoc GetVelocity() */
    Vector DoGetVelocity() const override;
    /** @copydoc GetDistanceFrom() */
    double DoGetDistanceFrom(Ptr<const GeocentricConstantPositionMobilityModel> other) const;
    /** @copydoc GetGeographicPosition() */
    virtual Vector DoGetGeographicPosition() const;
    /** @copydoc SetGeographicPosition() */
    virtual void DoSetGeographicPosition(const Vector& latLonAlt);
    /** @copydoc GetGeocentricPosition() */
    virtual Vector DoGetGeocentricPosition() const;
    /** @copydoc SetGeocentricPosition() */
    virtual void DoSetGeocentricPosition(const Vector& position);
    /** @copydoc GetElevationAngle() */
    virtual double DoGetElevationAngle(Ptr<const GeocentricConstantPositionMobilityModel> other);
    /** @copydoc SetCoordinateTranslationReferencePoint() */
    virtual void DoSetCoordinateTranslationReferencePoint(const Vector& refPoint);
    /** @copydoc GetCoordinateTranslationReferencePoint() */
    virtual Vector DoGetCoordinateTranslationReferencePoint() const;

    /**
     * the constant Geographic position,, in order: latitude (degree), longitude (degree), altitude
     * (meter).
     */
    Vector m_position{0, 0, 0};

    /**
     * This is the point (meter) taken as a reference for converting
     * from geographic to topographic (also referred to as planar Cartesian)
     */
    Vector m_geographicReferencePoint{0, 0, 0};
};

} // namespace ns3

#endif /* GEOCENTRIC_CONSTANT_POSITION_MOBILITY_MODEL_H */
