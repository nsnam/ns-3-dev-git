/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef IPV4_LIST_ROUTING_HELPER_H
#define IPV4_LIST_ROUTING_HELPER_H

#include "ns3/ipv4-routing-helper.h"

#include <list>
#include <stdint.h>

namespace ns3
{

/**
 * \ingroup ipv4Helpers
 *
 * \brief Helper class that adds ns3::Ipv4ListRouting objects
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class Ipv4ListRoutingHelper : public Ipv4RoutingHelper
{
  public:
    /*
     * Construct an Ipv4ListRoutingHelper used to make installing routing
     * protocols easier.
     */
    Ipv4ListRoutingHelper();

    /*
     * Destroy an Ipv4ListRoutingHelper.
     */
    ~Ipv4ListRoutingHelper() override;

    /**
     * \brief Construct an Ipv4ListRoutingHelper from another previously
     * initialized instance (Copy Constructor).
     * \param o object to be copied
     */
    Ipv4ListRoutingHelper(const Ipv4ListRoutingHelper& o);

    // Delete assignment operator to avoid misuse
    Ipv4ListRoutingHelper& operator=(const Ipv4ListRoutingHelper&) = delete;

    /**
     * \returns pointer to clone of this Ipv4ListRoutingHelper
     *
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    Ipv4ListRoutingHelper* Copy() const override;

    /**
     * \param routing a routing helper
     * \param priority the priority of the associated helper
     *
     * Store in the internal list a reference to the input routing helper
     * and associated priority. These helpers will be used later by
     * the ns3::Ipv4ListRoutingHelper::Create method to create
     * an ns3::Ipv4ListRouting object and add in it routing protocols
     * created with the helpers.
     */
    void Add(const Ipv4RoutingHelper& routing, int16_t priority);
    /**
     * \param node the node on which the routing protocol will run
     * \returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     */
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;

  private:
    /**
     * \brief Container for pairs of Ipv4RoutingHelper pointer / priority.
     */
    std::list<std::pair<const Ipv4RoutingHelper*, int16_t>> m_list;
};

} // namespace ns3

#endif /* IPV4_LIST_ROUTING_HELPER_H */
