/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 *         Ankit Deepak <adadeepak8@gmail.com>
 *         Deepti Rajagopal <deeptir96@gmail.com>
 *
 */

#include "dhcp-helper.h"
#include "ns3/dhcp-server.h"
#include "ns3/dhcp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

DhcpClientHelper::DhcpClientHelper ()
{
  m_factory.SetTypeId (DhcpClient::GetTypeId ());
}

DhcpServerHelper::DhcpServerHelper (Ipv4Address pool_addr, Ipv4Mask pool_mask,
                                    Ipv4Address min_addr, Ipv4Address max_addr,
                                    Ipv4Address gateway)
{
  m_factory.SetTypeId (DhcpServer::GetTypeId ());
  SetAttribute ("PoolAddresses", Ipv4AddressValue (pool_addr));
  SetAttribute ("PoolMask", Ipv4MaskValue (pool_mask));
  SetAttribute ("FirstAddress", Ipv4AddressValue (min_addr));
  SetAttribute ("LastAddress", Ipv4AddressValue (max_addr));
  SetAttribute ("Gateway", Ipv4AddressValue (gateway));
}

void DhcpClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void DhcpServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer DhcpClientHelper::Install (Ptr<NetDevice> netDevice) const
{
  return ApplicationContainer (InstallPriv (netDevice));
}

ApplicationContainer DhcpClientHelper::Install (NetDeviceContainer netDevices) const
{
  ApplicationContainer apps;
  for (NetDeviceContainer::Iterator i = netDevices.Begin (); i != netDevices.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }
  return apps;
}

ApplicationContainer DhcpServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

Ptr<Application> DhcpClientHelper::InstallPriv (Ptr<NetDevice> netDevice) const
{
  Ptr<Node> node = netDevice->GetNode ();
  NS_ASSERT_MSG (node != 0, "DhcpClientHelper: NetDevice must be installed in a node before adding DhcpClient.");

  Ptr<DhcpClient> app = DynamicCast <DhcpClient> (m_factory.Create<DhcpClient> ());
  app->SetDhcpClientNetDevice (netDevice);
  node->AddApplication (app);

  return app;
}

Ptr<Application> DhcpServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<DhcpServer> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
