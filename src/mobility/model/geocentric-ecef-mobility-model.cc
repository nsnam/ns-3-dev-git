/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 */

#include "geocentric-ecef-mobility-model.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GeocentricEcefMobilityModel");

NS_OBJECT_ENSURE_REGISTERED(GeocentricEcefMobilityModel);

TypeId
GeocentricEcefMobilityModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GeocentricEcefMobilityModel")
                            .SetParent<GeocentricConstantPositionMobilityModel>()
                            .SetGroupName("Mobility")
                            .AddConstructor<GeocentricEcefMobilityModel>();
    return tid;
}

GeocentricEcefMobilityModel::GeocentricEcefMobilityModel()
{
    NS_LOG_FUNCTION(this);
}

GeocentricEcefMobilityModel::~GeocentricEcefMobilityModel()
{
    NS_LOG_FUNCTION(this);
}

Vector
GeocentricEcefMobilityModel::DoGetPosition() const
{
    NS_LOG_FUNCTION(this);

    // Return ECEF coordinates instead of ENU
    // Use the public GetGeocentricPosition() method which returns ECEF
    return GetGeocentricPosition();
}

} // namespace ns3
