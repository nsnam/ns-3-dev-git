/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "basic-energy-harvester-helper.h"

#include "ns3/energy-harvester.h"

namespace ns3
{

BasicEnergyHarvesterHelper::BasicEnergyHarvesterHelper()
{
    m_basicEnergyHarvester.SetTypeId("ns3::energy::BasicEnergyHarvester");
}

BasicEnergyHarvesterHelper::~BasicEnergyHarvesterHelper()
{
}

void
BasicEnergyHarvesterHelper::Set(std::string name, const AttributeValue& v)
{
    m_basicEnergyHarvester.Set(name, v);
}

Ptr<energy::EnergyHarvester>
BasicEnergyHarvesterHelper::DoInstall(Ptr<energy::EnergySource> source) const
{
    NS_ASSERT(source);
    Ptr<Node> node = source->GetNode();

    // Create a new Basic Energy Harvester
    Ptr<energy::EnergyHarvester> harvester =
        m_basicEnergyHarvester.Create<energy::EnergyHarvester>();
    NS_ASSERT(harvester);

    // Connect the Basic Energy Harvester to the Energy Source
    source->ConnectEnergyHarvester(harvester);
    harvester->SetNode(node);
    harvester->SetEnergySource(source);
    return harvester;
}

} // namespace ns3
