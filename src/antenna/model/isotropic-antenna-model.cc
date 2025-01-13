/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "isotropic-antenna-model.h"

#include "antenna-model.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IsotropicAntennaModel");

NS_OBJECT_ENSURE_REGISTERED(IsotropicAntennaModel);

TypeId
IsotropicAntennaModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IsotropicAntennaModel")
                            .SetParent<AntennaModel>()
                            .SetGroupName("Antenna")
                            .AddConstructor<IsotropicAntennaModel>()
                            .AddAttribute("Gain",
                                          "The gain of the antenna in dB",
                                          DoubleValue(0),
                                          MakeDoubleAccessor(&IsotropicAntennaModel::m_gainDb),
                                          MakeDoubleChecker<double>());
    return tid;
}

IsotropicAntennaModel::IsotropicAntennaModel()
{
    NS_LOG_FUNCTION(this);
}

double
IsotropicAntennaModel::GetGainDb(Angles a)
{
    NS_LOG_FUNCTION(this << a);
    return m_gainDb;
}

} // namespace ns3
