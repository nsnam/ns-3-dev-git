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

#ifndef PING_HELPER_H
#define PING_HELPER_H

#include "ns3/application-container.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ping.h"

#include <stdint.h>

namespace ns3
{

/**
 * \ingroup ping
 * \brief Create a ping application and associate it to a node
 *
 * This class creates one or multiple instances of ns3::Ping and associates
 * it/them to one/multiple node(s).
 */
class PingHelper
{
  public:
    /**
     * Create a PingHelper which is used to make life easier for people wanting
     * to use ping Applications.
     */
    PingHelper();

    /**
     * Create a PingHelper which is used to make life easier for people wanting
     * to use ping Applications.
     *
     * \param remote The address which should be pinged
     * \param local The source address
     */
    PingHelper(Address remote, Address local = Address());

    /**
     * Install a Ping application on each Node in the provided NodeContainer.
     *
     * \param nodes The NodeContainer containing all of the nodes to get a Ping
     *              application.
     *
     * \returns A list of Ping applications, one for each input node
     */
    ApplicationContainer Install(NodeContainer nodes) const;

    /**
     * Install a Ping application on the provided Node.  The Node is specified
     * directly by a Ptr<Node>
     *
     * \param node The node to install the Ping application on.
     *
     * \returns An ApplicationContainer holding the Ping application created.
     */
    ApplicationContainer Install(Ptr<Node> node) const;

    /**
     * Install a Ping application on the provided Node.  The Node is specified
     * by a string that must have previously been associated with a Node using the
     * Object Name Service.
     *
     * \param nodeName The node to install the Ping application on.
     *
     * \returns An ApplicationContainer holding the Ping application created.
     */
    ApplicationContainer Install(std::string nodeName) const;

    /**
     * \brief Configure ping applications attribute
     * \param name   attribute's name
     * \param value  attribute's value
     */
    void SetAttribute(std::string name, const AttributeValue& value);

  private:
    /**
     * \brief Do the actual application installation in the node
     * \param node the node
     * \returns a Smart pointer to the installed application
     */
    Ptr<Application> InstallPriv(Ptr<Node> node) const;
    /// Object factory
    ObjectFactory m_factory;
};

} // namespace ns3

#endif /* PING_HELPER_H */
