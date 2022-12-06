/*
 * Copyright (c) 2022 Universita' di Firenze, Italy
 * Copyright (c) 2008-2009 Strasbourg University (original Ping6 helper)
 * Copyright (c) 2008 INRIA (original v4Ping helper)
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *
 * Derived from original v4Ping helper (author: Mathieu Lacage)
 * Derived from original ping6 helper (author: Sebastien Vincent)
 */

#include "ping-helper.h"

#include "ns3/names.h"
#include "ns3/ping.h"

namespace ns3
{

PingHelper::PingHelper()
{
    m_factory.SetTypeId("ns3::Ping");
}

PingHelper::PingHelper(Address remote, Address local)
{
    m_factory.SetTypeId("ns3::Ping");
    m_factory.Set("Destination", AddressValue(remote));
    m_factory.Set("InterfaceAddress", AddressValue(local));
}

void
PingHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
PingHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
PingHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
PingHelper::Install(NodeContainer c) const
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }

    return apps;
}

Ptr<Application>
PingHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<Ping> app = m_factory.Create<Ping>();
    node->AddApplication(app);

    return app;
}

} // namespace ns3
