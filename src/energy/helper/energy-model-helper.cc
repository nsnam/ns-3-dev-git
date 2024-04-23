/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "energy-model-helper.h"

#include "ns3/config.h"
#include "ns3/names.h"

namespace ns3
{

/*
 * EnergySourceHelper
 */
EnergySourceHelper::~EnergySourceHelper()
{
}

energy::EnergySourceContainer
EnergySourceHelper::Install(Ptr<Node> node) const
{
    return Install(NodeContainer(node));
}

energy::EnergySourceContainer
EnergySourceHelper::Install(NodeContainer c) const
{
    energy::EnergySourceContainer container;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<energy::EnergySource> src = DoInstall(*i);
        container.Add(src);
        /*
         * Check if EnergySourceContainer is already aggregated to target node. If
         * not, create a new EnergySourceContainer and aggregate it to node.
         */
        Ptr<energy::EnergySourceContainer> EnergySourceContainerOnNode =
            (*i)->GetObject<energy::EnergySourceContainer>();
        if (!EnergySourceContainerOnNode)
        {
            ObjectFactory fac;
            fac.SetTypeId("ns3::EnergySourceContainer");
            EnergySourceContainerOnNode = fac.Create<energy::EnergySourceContainer>();
            EnergySourceContainerOnNode->Add(src);
            (*i)->AggregateObject(EnergySourceContainerOnNode);
        }
        else
        {
            EnergySourceContainerOnNode->Add(src); // append new EnergySource
        }
    }
    return container;
}

energy::EnergySourceContainer
EnergySourceHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

energy::EnergySourceContainer
EnergySourceHelper::InstallAll() const
{
    return Install(NodeContainer::GetGlobal());
}

/*
 * DeviceEnergyModelHelper
 */
DeviceEnergyModelHelper::~DeviceEnergyModelHelper()
{
}

energy::DeviceEnergyModelContainer
DeviceEnergyModelHelper::Install(Ptr<NetDevice> device, Ptr<energy::EnergySource> source) const
{
    NS_ASSERT(device);
    NS_ASSERT(source);
    // check to make sure source and net device are on the same node
    NS_ASSERT(device->GetNode() == source->GetNode());
    energy::DeviceEnergyModelContainer container(DoInstall(device, source));
    return container;
}

energy::DeviceEnergyModelContainer
DeviceEnergyModelHelper::Install(NetDeviceContainer deviceContainer,
                                 energy::EnergySourceContainer sourceContainer) const
{
    NS_ASSERT(deviceContainer.GetN() <= sourceContainer.GetN());
    energy::DeviceEnergyModelContainer container;
    auto dev = deviceContainer.Begin();
    auto src = sourceContainer.Begin();
    while (dev != deviceContainer.End())
    {
        // check to make sure source and net device are on the same node
        NS_ASSERT((*dev)->GetNode() == (*src)->GetNode());
        Ptr<energy::DeviceEnergyModel> model = DoInstall(*dev, *src);
        container.Add(model);
        dev++;
        src++;
    }
    return container;
}

} // namespace ns3
