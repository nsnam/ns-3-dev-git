/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_HELPER_H
#define DHCP6_HELPER_H

#include "ns3/application-container.h"
#include "ns3/dhcp6-server.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Helper
 * @brief The helper class used to configure and install DHCPv6 applications on nodes
 */
class Dhcp6Helper
{
  public:
    /**
     * @brief Default constructor.
     */
    Dhcp6Helper();

    /**
     * @brief Set DHCPv6 client attributes
     * @param name Name of the attribute
     * @param value Value to be set
     */
    void SetClientAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Set DHCPv6 server attributes
     * @param name Name of the attribute
     * @param value Value to be set
     */
    void SetServerAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Install DHCPv6 client on a set of nodes
     *
     * If there is already a DHCPv6 client on the node, the app is not installed,
     * and the already existing one is returned.
     *
     * @param clientNodes Nodes on which the DHCPv6 client is installed
     * @return The application container with DHCPv6 client installed
     */
    ApplicationContainer InstallDhcp6Client(NodeContainer clientNodes) const;

    /**
     * @brief Install DHCPv6 server on a node / NetDevice. Also updates the
     * interface -> server map.
     * @param netDevices The NetDevices on which DHCPv6 server application has to be installed
     * @return The application container with DHCPv6 server installed
     */
    ApplicationContainer InstallDhcp6Server(NetDeviceContainer netDevices);

  private:
    /**
     * @brief Helper method that iterates through the installed applications
     * on a node to look for a Dhcp6Client application. It either installs a
     * new Dhcp6Client or returns an existing one.
     * @param clientNode The node on which the application is to be installed.
     * @return Pointer to the Dhcp6Client application on the node.
     */
    Ptr<Application> InstallDhcp6ClientInternal(Ptr<Node> clientNode) const;

    ObjectFactory m_clientFactory; //!< DHCPv6 client factory.
    ObjectFactory m_serverFactory; //!< DHCPv6 server factory.
};

} // namespace ns3

#endif /* DHCP6_HELPER_H */
