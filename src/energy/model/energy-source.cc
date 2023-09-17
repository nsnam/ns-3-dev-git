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
 * Author: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 *
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * Modifications made by: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "energy-source.h"

#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EnergySource");

NS_OBJECT_ENSURE_REGISTERED(EnergySource);

TypeId
EnergySource::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EnergySource").SetParent<Object>().SetGroupName("Energy");
    return tid;
}

EnergySource::EnergySource()
{
    NS_LOG_FUNCTION(this);
}

EnergySource::~EnergySource()
{
    NS_LOG_FUNCTION(this);
}

void
EnergySource::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(node);
    m_node = node;
}

Ptr<Node>
EnergySource::GetNode() const
{
    return m_node;
}

void
EnergySource::AppendDeviceEnergyModel(Ptr<DeviceEnergyModel> deviceEnergyModelPtr)
{
    NS_LOG_FUNCTION(this << deviceEnergyModelPtr);
    NS_ASSERT(deviceEnergyModelPtr); // model must exist
    m_models.Add(deviceEnergyModelPtr);
}

DeviceEnergyModelContainer
EnergySource::FindDeviceEnergyModels(TypeId tid)
{
    NS_LOG_FUNCTION(this << tid);
    DeviceEnergyModelContainer container;
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        if ((*i)->GetInstanceTypeId() == tid)
        {
            container.Add(*i);
        }
    }
    return container;
}

DeviceEnergyModelContainer
EnergySource::FindDeviceEnergyModels(std::string name)
{
    NS_LOG_FUNCTION(this << name);
    DeviceEnergyModelContainer container;
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        if ((*i)->GetInstanceTypeId().GetName() == name)
        {
            container.Add(*i);
        }
    }
    return container;
}

void
EnergySource::InitializeDeviceModels()
{
    NS_LOG_FUNCTION(this);
    /*
     * Device models are not aggregated to the node, hence we have to manually
     * call dispose method here.
     */
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        (*i)->Initialize();
    }
}

void
EnergySource::DisposeDeviceModels()
{
    NS_LOG_FUNCTION(this);
    /*
     * Device models are not aggregated to the node, hence we have to manually
     * call dispose method here.
     */
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        (*i)->Dispose();
    }
}

void
EnergySource::ConnectEnergyHarvester(Ptr<EnergyHarvester> energyHarvesterPtr)
{
    NS_LOG_FUNCTION(this << energyHarvesterPtr);
    NS_ASSERT(energyHarvesterPtr); // energy harvester must exist
    m_harvesters.push_back(energyHarvesterPtr);
}

/*
 * Private function starts here.
 */

void
EnergySource::DoDispose()
{
    NS_LOG_FUNCTION(this);
    BreakDeviceEnergyModelRefCycle();
}

/*
 * Protected functions start here.
 */

double
EnergySource::CalculateTotalCurrent()
{
    NS_LOG_FUNCTION(this);
    double totalCurrentA = 0.0;
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        totalCurrentA += (*i)->GetCurrentA();
    }

    if (!m_harvesters.empty())
    {
        double totalHarvestedPower = 0.0;

        for (auto harvester = m_harvesters.begin(); harvester != m_harvesters.end(); harvester++)
        {
            totalHarvestedPower += (*harvester)->GetPower();
        }

        double supplyVoltage = GetSupplyVoltage();

        if (supplyVoltage != 0)
        {
            double currentHarvestersA = totalHarvestedPower / supplyVoltage;
            NS_LOG_DEBUG(" Total harvested power: " << totalHarvestedPower
                                                    << "| Current from harvesters: "
                                                    << currentHarvestersA);
            totalCurrentA -= currentHarvestersA;
        }
    }

    return totalCurrentA;
}

void
EnergySource::NotifyEnergyDrained()
{
    NS_LOG_FUNCTION(this);
    // notify all device energy models installed on node
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        (*i)->HandleEnergyDepletion();
    }
}

void
EnergySource::NotifyEnergyRecharged()
{
    NS_LOG_FUNCTION(this);
    // notify all device energy models installed on node
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        (*i)->HandleEnergyRecharged();
    }
}

void
EnergySource::NotifyEnergyChanged()
{
    NS_LOG_FUNCTION(this);
    // notify all device energy models installed on node
    DeviceEnergyModelContainer::Iterator i;
    for (i = m_models.Begin(); i != m_models.End(); i++)
    {
        (*i)->HandleEnergyChanged();
    }
}

void
EnergySource::BreakDeviceEnergyModelRefCycle()
{
    NS_LOG_FUNCTION(this);
    m_models.Clear();
    m_harvesters.clear();
    m_node = nullptr;
}

} // namespace ns3
