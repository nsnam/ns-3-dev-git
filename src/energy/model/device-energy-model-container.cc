/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "device-energy-model-container.h"

#include "ns3/log.h"
#include "ns3/names.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("DeviceEnergyModelContainer");

DeviceEnergyModelContainer::DeviceEnergyModelContainer()
{
    NS_LOG_FUNCTION(this);
}

DeviceEnergyModelContainer::DeviceEnergyModelContainer(Ptr<DeviceEnergyModel> model)
{
    NS_LOG_FUNCTION(this << model);
    NS_ASSERT(model);
    m_models.push_back(model);
}

DeviceEnergyModelContainer::DeviceEnergyModelContainer(std::string modelName)
{
    NS_LOG_FUNCTION(this << modelName);
    Ptr<DeviceEnergyModel> model = Names::Find<DeviceEnergyModel>(modelName);
    NS_ASSERT(model);
    m_models.push_back(model);
}

DeviceEnergyModelContainer::DeviceEnergyModelContainer(const DeviceEnergyModelContainer& a,
                                                       const DeviceEnergyModelContainer& b)
{
    NS_LOG_FUNCTION(this << &a << &b);
    *this = a;
    Add(b);
}

DeviceEnergyModelContainer::Iterator
DeviceEnergyModelContainer::Begin() const
{
    NS_LOG_FUNCTION(this);
    return m_models.begin();
}

DeviceEnergyModelContainer::Iterator
DeviceEnergyModelContainer::End() const
{
    NS_LOG_FUNCTION(this);
    return m_models.end();
}

uint32_t
DeviceEnergyModelContainer::GetN() const
{
    NS_LOG_FUNCTION(this);
    return m_models.size();
}

Ptr<DeviceEnergyModel>
DeviceEnergyModelContainer::Get(uint32_t i) const
{
    NS_LOG_FUNCTION(this << i);
    return m_models[i];
}

void
DeviceEnergyModelContainer::Add(DeviceEnergyModelContainer container)
{
    NS_LOG_FUNCTION(this << &container);
    for (auto i = container.Begin(); i != container.End(); i++)
    {
        m_models.push_back(*i);
    }
}

void
DeviceEnergyModelContainer::Add(Ptr<DeviceEnergyModel> model)
{
    NS_LOG_FUNCTION(this << model);
    NS_ASSERT(model);
    m_models.push_back(model);
}

void
DeviceEnergyModelContainer::Add(std::string modelName)
{
    NS_LOG_FUNCTION(this << modelName);
    Ptr<DeviceEnergyModel> model = Names::Find<DeviceEnergyModel>(modelName);
    NS_ASSERT(model);
    m_models.push_back(model);
}

void
DeviceEnergyModelContainer::Clear()
{
    NS_LOG_FUNCTION(this);
    m_models.clear();
}

} // namespace energy
} // namespace ns3
