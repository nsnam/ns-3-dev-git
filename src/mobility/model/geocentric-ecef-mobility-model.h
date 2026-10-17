/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jesse Chiu <jessest94106@gmail.com>
 */

#ifndef GEOCENTRIC_ECEF_MOBILITY_MODEL_H
#define GEOCENTRIC_ECEF_MOBILITY_MODEL_H

#include "geocentric-constant-position-mobility-model.h"

namespace ns3
{

/**
 * @ingroup mobility
 * @ingroup leo
 * @brief Fixed geocentric mobility model whose generic position is ECEF
 *
 * This class adapts GeocentricConstantPositionMobilityModel for NTN scenarios
 * where fixed ground nodes and satellite nodes must report positions in the
 * same coordinate frame through the generic MobilityModel API.
 *
 * GeocentricConstantPositionMobilityModel provides GetGeocentricPosition() for
 * ECEF (Earth-Centered Earth-Fixed) coordinates, but its GetPosition() reports
 * the local topocentric/ENU (East-North-Up) position. LEO satellite mobility
 * models report ECEF coordinates from GetPosition(). Some NR, 3GPP propagation,
 * spectrum, and antenna geometry code queries nodes only through
 * MobilityModel::GetPosition(), so changing those call sites to use
 * GetGeocentricPosition() would require broader API changes.
 *
 * GeocentricEcefMobilityModel therefore overrides DoGetPosition() so
 * GetPosition() returns the same ECEF coordinates as GetGeocentricPosition(),
 * while retaining the geographic and geocentric accessors from the parent class.
 *
 * @internal
 * TODO: Consider if a unified units/coordinate system framework could
 *       eliminate the need for specialized mobility model classes like this one
 *       by allowing positions to carry their coordinate system metadata.
 */
class GeocentricEcefMobilityModel : public GeocentricConstantPositionMobilityModel
{
  public:
    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create a geocentric ECEF mobility model
     */
    GeocentricEcefMobilityModel() = default;

    /**
     * Destructor
     */
    ~GeocentricEcefMobilityModel() override = default;

  private:
    /**
     * @brief Override to return ECEF coordinates instead of ENU
     * @return the position in ECEF (Earth-Centered Earth-Fixed) coordinates
     *
     * This overrides the parent class's DoGetPosition() which returns ENU coordinates.
     * Instead, this returns the same ECEF coordinates as DoGetGeocentricPosition().
     */
    Vector DoGetPosition() const override;
};

} // namespace ns3

#endif /* GEOCENTRIC_ECEF_MOBILITY_MODEL_H */
