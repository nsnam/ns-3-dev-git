/*
 * Copyright (c) 2019 Ritsumeikan University, Shiga, Japan
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
 *
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 *
 */

#ifndef V4TRACEROUTE_HELPER_H
#define V4TRACEROUTE_HELPER_H

#include <ns3/application-helper.h>

namespace ns3
{

class OutputStreamWrapper;

/**
 * \ingroup v4traceroute
 * \brief Create a IPv4 traceroute application and associate it to a node
 *
 * This class creates one or multiple instances of ns3::V4TraceRoute and associates
 * it/them to one/multiple node(s).
 */
class V4TraceRouteHelper : public ApplicationHelper
{
  public:
    /**
     * Create a V4TraceRouteHelper which is used to make life easier for people wanting
     * to use TraceRoute
     *
     * \param remote The address which should be traced
     */
    V4TraceRouteHelper(const Ipv4Address& remote);

    /**
     * \brief Print the resulting trace routes from given node.
     * \param node The origin node where the traceroute is initiated.
     * \param stream The outputstream used to print the resulting traced routes.
     */
    static void PrintTraceRouteAt(Ptr<Node> node, Ptr<OutputStreamWrapper> stream);
};

} // namespace ns3

#endif /* V4TRACEROUTE_HELPER_H */
