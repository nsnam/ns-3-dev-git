/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-noise-model-default.h"

#include "ns3/double.h"

#include <cmath>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanNoiseModelDefault);

UanNoiseModelDefault::UanNoiseModelDefault()
{
}

UanNoiseModelDefault::~UanNoiseModelDefault()
{
}

TypeId
UanNoiseModelDefault::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanNoiseModelDefault")
                            .SetParent<UanNoiseModel>()
                            .SetGroupName("Uan")
                            .AddConstructor<UanNoiseModelDefault>()
                            .AddAttribute("Wind",
                                          "Wind speed in m/s.",
                                          DoubleValue(1),
                                          MakeDoubleAccessor(&UanNoiseModelDefault::m_wind),
                                          MakeDoubleChecker<double>(0))
                            .AddAttribute("Shipping",
                                          "Shipping contribution to noise between 0 and 1.",
                                          DoubleValue(0),
                                          MakeDoubleAccessor(&UanNoiseModelDefault::m_shipping),
                                          MakeDoubleChecker<double>(0, 1));
    return tid;
}

// Common acoustic noise formulas.  These can be found
// in "Principles of Underwater Sound" by Robert J. Urick
double
UanNoiseModelDefault::GetNoiseDbHz(double fKhz) const
{
    double turb;
    double wind;
    double ship;
    double thermal;
    double turbDb;
    double windDb;
    double shipDb;
    double thermalDb;
    double noiseDb;

    double log_fKhz = std::log10(fKhz);

    turbDb = 17.0 - 30.0 * log_fKhz;
    turb = std::pow(10.0, turbDb * 0.1);

    shipDb = 40.0 + 20.0 * (m_shipping - 0.5) + 26.0 * log_fKhz - 60.0 * std::log10(fKhz + 0.03);
    ship = std::pow(10.0, (shipDb * 0.1));

    windDb = 50.0 + 7.5 * std::pow(m_wind, 0.5) + 20.0 * log_fKhz - 40.0 * std::log10(fKhz + 0.4);
    wind = std::pow(10.0, windDb * 0.1);

    thermalDb = -15 + 20 * log_fKhz;
    thermal = std::pow(10, thermalDb * 0.1);

    noiseDb = 10 * std::log10(turb + ship + wind + thermal);

    return noiseDb;
}

} // namespace ns3
