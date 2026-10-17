/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ipv6-global-routing-helper.h"

#include "ns3/global-router-interface.h"
#include "ns3/global-routing.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6GlobalRoutingHelper");

Ipv6GlobalRoutingHelper::Ipv6GlobalRoutingHelper()
{
}

Ipv6GlobalRoutingHelper::Ipv6GlobalRoutingHelper(const Ipv6GlobalRoutingHelper& o)
{
}

Ipv6GlobalRoutingHelper*
Ipv6GlobalRoutingHelper::Copy() const
{
    return new Ipv6GlobalRoutingHelper(*this);
}

Ptr<Ipv6RoutingProtocol>
Ipv6GlobalRoutingHelper::Create(Ptr<Node> node) const
{
    NS_LOG_LOGIC("Adding GlobalRouter interface to node " << node->GetId());

    Ptr<GlobalRouter<Ipv6Manager>> globalRouter = CreateObject<GlobalRouter<Ipv6Manager>>();
    node->AggregateObject(globalRouter);

    NS_LOG_LOGIC("Adding GlobalRouting Protocol to node " << node->GetId());
    Ptr<GlobalRouting<Ipv6RoutingProtocol>> globalRouting =
        CreateObject<GlobalRouting<Ipv6RoutingProtocol>>();
    globalRouter->SetRoutingProtocol(globalRouting);

    return globalRouting;
}

void
Ipv6GlobalRoutingHelper::PopulateRoutingTables()
{
    GlobalRouteManager<Ipv6Manager>::InitializeRouters();
    GlobalRouteManager<Ipv6Manager>::BuildGlobalRoutingDatabase();
    GlobalRouteManager<Ipv6Manager>::InitializeRoutes();
}

void
Ipv6GlobalRoutingHelper::RecomputeRoutingTables()
{
    GlobalRouteManager<Ipv6Manager>::DeleteGlobalRoutes();
    GlobalRouteManager<Ipv6Manager>::BuildGlobalRoutingDatabase();
    GlobalRouteManager<Ipv6Manager>::InitializeRoutes();
}

} // namespace ns3
