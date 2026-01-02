/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 */

#ifndef GEOCENTRIC_ECEF_MOBILITY_MODEL_H
#define GEOCENTRIC_ECEF_MOBILITY_MODEL_H

#include "geocentric-constant-position-mobility-model.h"

namespace ns3
{

/**
 * @ingroup mobility
 * @brief Geocentric mobility model that returns ECEF coordinates from GetPosition()
 *
 * This class extends GeocentricConstantPositionMobilityModel to override the default behavior.
 * While the parent class returns ENU (East-North-Up) coordinates from GetPosition(),
 * this derived class returns ECEF (Earth-Centered Earth-Fixed) coordinates instead.
 *
 * This is useful for scenarios where:
 * - Both satellite and ground station need to use the same coordinate system (ECEF)
 * - Distance calculations are performed using GetPosition() in propagation models
 * - The type checking requires GeocentricConstantPositionMobilityModel (e.g., NTN scenarios)
 *
 * The class still maintains all the functionality of the parent class, including:
 * - GetGeocentricPosition() which returns ECEF coordinates
 * - GetGeographicPosition() which returns Geographic coordinates
 * - Type compatibility with GeocentricConstantPositionMobilityModel
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
    GeocentricEcefMobilityModel();

    /**
     * Destructor
     */
    ~GeocentricEcefMobilityModel() override;

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
