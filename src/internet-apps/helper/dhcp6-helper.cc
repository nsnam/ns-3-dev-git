/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#include "dhcp6-helper.h"

#include "ns3/dhcp6-client.h"
#include "ns3/dhcp6-server.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Helper");

Dhcp6Helper::Dhcp6Helper()
{
    m_clientFactory.SetTypeId(Dhcp6Client::GetTypeId());
    m_serverFactory.SetTypeId(Dhcp6Server::GetTypeId());
}

void
Dhcp6Helper::SetClientAttribute(std::string name, const AttributeValue& value)
{
    m_clientFactory.Set(name, value);
}

void
Dhcp6Helper::SetServerAttribute(std::string name, const AttributeValue& value)
{
    m_serverFactory.Set(name, value);
}

ApplicationContainer
Dhcp6Helper::InstallDhcp6Client(NodeContainer clientNodes) const
{
    ApplicationContainer installedApps;
    for (auto iter = clientNodes.Begin(); iter != clientNodes.End(); iter++)
    {
        Ptr<Application> app = InstallDhcp6ClientInternal((*iter));
        installedApps.Add(app);
    }

    return installedApps;
}

ApplicationContainer
Dhcp6Helper::InstallDhcp6Server(NetDeviceContainer netDevices)
{
    ApplicationContainer installedApps;
    for (auto itr = netDevices.Begin(); itr != netDevices.End(); itr++)
    {
        Ptr<NetDevice> netDevice = *itr;
        Ptr<Node> node = netDevice->GetNode();
        NS_ASSERT_MSG(node, "Dhcp6Helper: NetDevice is not associated with any node -> fail");

        Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
        NS_ASSERT_MSG(ipv6,
                      "Dhcp6Helper: NetDevice is associated"
                      " with a node without IPv6 stack installed -> fail "
                      "(maybe need to use InternetStackHelper?)");

        uint32_t nApplications = node->GetNApplications();

        for (uint32_t i = 0; i < nApplications; i++)
        {
            Ptr<Dhcp6Server> server = DynamicCast<Dhcp6Server>(node->GetApplication(i));
            if (!server)
            {
                Ptr<Dhcp6Server> app = m_serverFactory.Create<Dhcp6Server>();
                node->AddApplication(app);
                app->SetDhcp6ServerNetDevice(netDevices);
                installedApps.Add(app);
            }
        }

        if (nApplications == 0)
        {
            Ptr<Dhcp6Server> app = m_serverFactory.Create<Dhcp6Server>();
            node->AddApplication(app);
            app->SetDhcp6ServerNetDevice(netDevices);
            installedApps.Add(app);
        }
    }

    return installedApps;
}

Ptr<Application>
Dhcp6Helper::InstallDhcp6ClientInternal(Ptr<Node> clientNode) const
{
    Ptr<Application> app;
    for (uint32_t index = 0; index < clientNode->GetNApplications(); index++)
    {
        app = clientNode->GetApplication(index);
        if (app->GetInstanceTypeId() == Dhcp6Client::GetTypeId())
        {
            return app;
        }
    }

    app = m_clientFactory.Create<Dhcp6Client>();
    clientNode->AddApplication(app);

    return app;
}

} // namespace ns3
