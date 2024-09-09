/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "device-energy-model.h"

#include "ns3/log.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("DeviceEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(DeviceEnergyModel);

TypeId
DeviceEnergyModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::energy::DeviceEnergyModel")
                            .AddDeprecatedName("ns3::DeviceEnergyModel")
                            .SetParent<Object>()
                            .SetGroupName("Energy");
    return tid;
}

DeviceEnergyModel::DeviceEnergyModel()
{
    NS_LOG_FUNCTION(this);
}

DeviceEnergyModel::~DeviceEnergyModel()
{
    NS_LOG_FUNCTION(this);
}

double
DeviceEnergyModel::GetCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return DoGetCurrentA();
}

/*
 * Private function starts here.
 */

double
DeviceEnergyModel::DoGetCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return 0.0;
}

} // namespace energy
} // namespace ns3
