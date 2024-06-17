/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "net-device-container.h"

#include "ns3/names.h"

namespace ns3
{

NetDeviceContainer::NetDeviceContainer()
{
}

NetDeviceContainer::NetDeviceContainer(Ptr<NetDevice> dev)
{
    m_devices.push_back(dev);
}

NetDeviceContainer::NetDeviceContainer(std::string devName)
{
    Ptr<NetDevice> dev = Names::Find<NetDevice>(devName);
    m_devices.push_back(dev);
}

NetDeviceContainer::NetDeviceContainer(const NetDeviceContainer& a, const NetDeviceContainer& b)
{
    *this = a;
    Add(b);
}

NetDeviceContainer::Iterator
NetDeviceContainer::Begin() const
{
    return m_devices.begin();
}

NetDeviceContainer::Iterator
NetDeviceContainer::End() const
{
    return m_devices.end();
}

uint32_t
NetDeviceContainer::GetN() const
{
    return m_devices.size();
}

Ptr<NetDevice>
NetDeviceContainer::Get(uint32_t i) const
{
    return m_devices[i];
}

void
NetDeviceContainer::Add(NetDeviceContainer other)
{
    for (auto i = other.Begin(); i != other.End(); i++)
    {
        m_devices.push_back(*i);
    }
}

void
NetDeviceContainer::Add(Ptr<NetDevice> device)
{
    m_devices.push_back(device);
}

void
NetDeviceContainer::Add(std::string deviceName)
{
    Ptr<NetDevice> device = Names::Find<NetDevice>(deviceName);
    m_devices.push_back(device);
}

} // namespace ns3
