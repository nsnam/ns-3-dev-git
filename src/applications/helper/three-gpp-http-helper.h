/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2013 Magister Solutions
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
 * Original work author (from packet-sink-helper.cc):
 *   - Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *
 * Converted to HTTP web browsing traffic models by:
 *   - Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef THREE_GPP_HTTP_HELPER_H
#define THREE_GPP_HTTP_HELPER_H

#include <ns3/application-helper.h>

namespace ns3
{

/**
 * \ingroup applications
 * Helper to make it easier to instantiate an ThreeGppHttpClient on a set of nodes.
 */
class ThreeGppHttpClientHelper : public ApplicationHelper
{
  public:
    /**
     * Create a ThreeGppHttpClientHelper to make it easier to work with ThreeGppHttpClient
     * applications.
     * \param address The address of the remote server node to send traffic to.
     */
    ThreeGppHttpClientHelper(const Address& address);
}; // end of `class ThreeGppHttpClientHelper`

/**
 * \ingroup http
 * Helper to make it easier to instantiate an ThreeGppHttpServer on a set of nodes.
 */
class ThreeGppHttpServerHelper : public ApplicationHelper
{
  public:
    /**
     * Create a ThreeGppHttpServerHelper to make it easier to work with
     * ThreeGppHttpServer applications.
     * \param address The address of the server.
     */
    ThreeGppHttpServerHelper(const Address& address);
}; // end of `class ThreeGppHttpServerHelper`

} // namespace ns3

#endif /* THREE_GPP_HTTP_HELPER_H */
