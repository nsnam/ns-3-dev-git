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

#ifndef DHCP_HELPER_H
#define DHCP_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

/**
 * \ingroup dhcp
 *
 * \class DhcpClientHelper
 * \brief The helper class used to configure and install the client application on nodes
 */
class DhcpClientHelper
{
public:
  /**
   * \brief Constructor of client helper
   * \param device The interface on which DHCP client has to be installed
   */
  DhcpClientHelper (uint32_t device);

  /**
   * \brief Function to set DHCP client attributes
   * \param name Name of the attribute
   * \param value Value to be set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * \brief Function to install DHCP client of a node
   * \param node The node on which DHCP client application has to be installed
   * \return The application container with DHCP client installed
   */
  ApplicationContainer Install (Ptr<Node> node) const;

private:
  /**
   * \brief Function to install DHCP client on a node
   * \param node The node on which DHCP client application has to be installed
   * \return The pointer to the installed DHCP client
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;                 //!< The subset of ns3::object for setting attributes
};

/**
 * \ingroup dhcp
 *
 * \class DhcpServerHelper
 * \brief The helper class used to configure and install the server application on nodes
 */
class DhcpServerHelper
{
public:
  /**
   * \brief Constructor of server helper
   * \param pool_addr The Ipv4Address of the allocated pool
   * \param pool_mask The mask of the allocated pool
   * \param min_addr The lower bound of the Ipv4Address pool
   * \param max_addr The upper bound of the Ipv4Address pool
   * \param gateway The Ipv4Address of default gateway
   */
  DhcpServerHelper (Ipv4Address pool_addr, Ipv4Mask pool_mask,
                    Ipv4Address min_addr, Ipv4Address max_addr,
                    Ipv4Address gateway = Ipv4Address ());

  /**
   * \brief Function to set DHCP server attributes
   * \param name Name of the attribute
   * \param value Value to be set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * \brief Function to install DHCP server on a node
   * \param node The node on which DHCP server application has to be installed
   * \return The application container with DHCP server installed
   */
  ApplicationContainer Install (Ptr<Node> node) const;

private:
  /**
   * \brief Function to install DHCP server of a node
   * \param node The node on which DHCP server application has to be installed
   * \return The pointer to the installed DHCP server
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory;             //!< The subset of ns3::object for setting attributes
};

} // namespace ns3

#endif /* DHCP_HELPER_H */
