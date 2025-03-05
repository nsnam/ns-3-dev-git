/*
 * Copyright (c) 2013 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "radvd-helper.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/radvd-interface.h"
#include "ns3/radvd-prefix.h"
#include "ns3/radvd.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RadvdHelper");

RadvdHelper::RadvdHelper()
    : ApplicationHelper(Radvd::GetTypeId())
{
}

void
RadvdHelper::AddAnnouncedPrefix(uint32_t interface,
                                const Ipv6Address& prefix,
                                uint32_t prefixLength,
                                bool slaac)
{
    NS_LOG_FUNCTION(this << int(interface) << prefix << int(prefixLength));
    if (prefixLength != 64)
    {
        NS_LOG_WARN(
            "Adding a non-64 prefix is generally a bad idea. Autoconfiguration might not work.");
    }

    bool prefixFound = false;
    if (m_radvdInterfaces.find(interface) == m_radvdInterfaces.end())
    {
        m_radvdInterfaces[interface] = Create<RadvdInterface>(interface);
    }
    else
    {
        RadvdInterface::RadvdPrefixList prefixList = m_radvdInterfaces[interface]->GetPrefixes();
        RadvdInterface::RadvdPrefixListCI iter;
        for (iter = prefixList.begin(); iter != prefixList.end(); iter++)
        {
            if ((*iter)->GetNetwork() == prefix)
            {
                NS_LOG_LOGIC("Not adding the same prefix twice, skipping " << prefix << " "
                                                                           << int(prefixLength));
                prefixFound = true;
                break;
            }
        }
    }
    if (!prefixFound)
    {
        Ptr<RadvdPrefix> routerPrefix = Create<RadvdPrefix>(prefix, prefixLength);
        routerPrefix->SetAutonomousFlag(slaac);
        m_radvdInterfaces[interface]->AddPrefix(routerPrefix);
    }
}

void
RadvdHelper::EnableDefaultRouterForInterface(uint32_t interface)
{
    if (m_radvdInterfaces.find(interface) == m_radvdInterfaces.end())
    {
        m_radvdInterfaces[interface] = Create<RadvdInterface>(interface);
    }
    uint32_t maxRtrAdvInterval = m_radvdInterfaces[interface]->GetMaxRtrAdvInterval();
    m_radvdInterfaces[interface]->SetDefaultLifeTime(3 * maxRtrAdvInterval / 1000);
}

void
RadvdHelper::DisableDefaultRouterForInterface(uint32_t interface)
{
    if (m_radvdInterfaces.find(interface) == m_radvdInterfaces.end())
    {
        m_radvdInterfaces[interface] = Create<RadvdInterface>(interface);
    }
    m_radvdInterfaces[interface]->SetDefaultLifeTime(0);
}

Ptr<RadvdInterface>
RadvdHelper::GetRadvdInterface(uint32_t interface)
{
    if (m_radvdInterfaces.find(interface) == m_radvdInterfaces.end())
    {
        m_radvdInterfaces[interface] = Create<RadvdInterface>(interface);
    }
    return m_radvdInterfaces[interface];
}

void
RadvdHelper::ClearPrefixes()
{
    m_radvdInterfaces.clear();
}

Ptr<Application>
RadvdHelper::DoInstall(Ptr<Node> node)
{
    auto radvd = m_factory.Create<Radvd>();
    for (auto [index, interface] : m_radvdInterfaces)
    {
        if (!interface->GetPrefixes().empty())
        {
            radvd->AddConfiguration(interface);
        }
    }
    node->AddApplication(radvd);
    return radvd;
}

} /* namespace ns3 */
