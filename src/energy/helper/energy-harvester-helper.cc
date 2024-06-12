/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "energy-harvester-helper.h"

#include "ns3/config.h"
#include "ns3/names.h"

namespace ns3
{

/*
 * EnergyHarvesterHelper
 */
EnergyHarvesterHelper::~EnergyHarvesterHelper()
{
}

energy::EnergyHarvesterContainer
EnergyHarvesterHelper::Install(Ptr<energy::EnergySource> source) const
{
    return Install(energy::EnergySourceContainer(source));
}

energy::EnergyHarvesterContainer
EnergyHarvesterHelper::Install(energy::EnergySourceContainer sourceContainer) const
{
    energy::EnergyHarvesterContainer container;
    for (auto i = sourceContainer.Begin(); i != sourceContainer.End(); ++i)
    {
        Ptr<energy::EnergyHarvester> harvester = DoInstall(*i);
        container.Add(harvester);
        Ptr<Node> node = (*i)->GetNode();
        /*
         * Check if EnergyHarvesterContainer is already aggregated to target node. If
         * not, create a new EnergyHarvesterContainer and aggregate it to the node.
         */
        Ptr<energy::EnergyHarvesterContainer> EnergyHarvesterContainerOnNode =
            node->GetObject<energy::EnergyHarvesterContainer>();
        if (!EnergyHarvesterContainerOnNode)
        {
            ObjectFactory fac;
            fac.SetTypeId("ns3::energy::EnergyHarvesterContainer");
            EnergyHarvesterContainerOnNode = fac.Create<energy::EnergyHarvesterContainer>();
            EnergyHarvesterContainerOnNode->Add(harvester);
            node->AggregateObject(EnergyHarvesterContainerOnNode);
        }
        else
        {
            EnergyHarvesterContainerOnNode->Add(harvester); // append new EnergyHarvester
        }
    }
    return container;
}

energy::EnergyHarvesterContainer
EnergyHarvesterHelper::Install(std::string sourceName) const
{
    Ptr<energy::EnergySource> source = Names::Find<energy::EnergySource>(sourceName);
    return Install(source);
}

} // namespace ns3
