/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */

#ifndef PROPAGATION_ENVIRONMENT_H
#define PROPAGATION_ENVIRONMENT_H

namespace ns3
{

/**
 * @ingroup propagation
 *
 * The type of propagation environment
 *
 */
enum EnvironmentType
{
    UrbanEnvironment,
    SubUrbanEnvironment,
    OpenAreasEnvironment
};

/**
 * @ingroup propagation
 *
 * The size of the city in which propagation takes place
 *
 */
enum CitySize
{
    SmallCity,
    MediumCity,
    LargeCity
};

} // namespace ns3

#endif // PROPAGATION_ENVIRONMENT_H
