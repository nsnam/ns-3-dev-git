/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tom Henderson (tomhend@u.washington.edu)
 */

#include "global-route-manager.h"

#include "global-route-manager-impl.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulation-singleton.h"

#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GlobalRouteManager");

// ---------------------------------------------------------------------------
//
// GlobalRouteManager Implementation
//
// ---------------------------------------------------------------------------

template <typename T>
uint32_t GlobalRouteManager<T>::routerId = 0; //!< Router ID counter

template <typename T>
void
GlobalRouteManager<T>::DeleteGlobalRoutes()
{
    NS_LOG_FUNCTION_NOARGS();
    SimulationSingleton<GlobalRouteManagerImpl<typename GlobalRouteManager<T>::IpManager>>::Get()
        ->DeleteGlobalRoutes();
}

template <typename T>
void
GlobalRouteManager<T>::BuildGlobalRoutingDatabase()
{
    NS_LOG_FUNCTION_NOARGS();
    SimulationSingleton<GlobalRouteManagerImpl<typename GlobalRouteManager<T>::IpManager>>::Get()
        ->BuildGlobalRoutingDatabase();
}

template <typename T>
void
GlobalRouteManager<T>::InitializeRoutes()
{
    NS_LOG_FUNCTION_NOARGS();
    SimulationSingleton<GlobalRouteManagerImpl<typename GlobalRouteManager<T>::IpManager>>::Get()
        ->InitializeRoutes();
}

template <typename T>
uint32_t
GlobalRouteManager<T>::AllocateRouterId()
{
    NS_LOG_FUNCTION_NOARGS();
    return routerId++;
}

template <typename T>
void
GlobalRouteManager<T>::ResetRouterId()
{
    routerId = 0;
}

template <typename T>
void
GlobalRouteManager<T>::PrintRoute(Ptr<Node> sourceNode,
                                  IpAddress dest,
                                  Ptr<OutputStreamWrapper> stream,
                                  bool nodeIdLookup,
                                  Time::Unit unit)
{
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "PrintRoute at Time: " << Now().As(unit);
    *os << " from Node " << sourceNode->GetId() << " to address " << dest;
    SimulationSingleton<GlobalRouteManagerImpl<IpManager>>::Get()->PrintRoute(sourceNode,
                                                                              dest,
                                                                              stream,
                                                                              nodeIdLookup,
                                                                              unit);
    (*os).copyfmt(oldState);
}

template <typename T>
void
GlobalRouteManager<T>::PrintRoute(Ptr<Node> sourceNode,
                                  IpAddress dest,
                                  bool nodeIdLookup,
                                  Time::Unit unit)
{
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    GlobalRouteManager<T>::PrintRoute(sourceNode, dest, stream, nodeIdLookup, unit);
}

template <typename T>
void
GlobalRouteManager<T>::PrintRoute(Ptr<Node> sourceNode,
                                  Ptr<Node> dest,
                                  Ptr<OutputStreamWrapper> stream,
                                  bool nodeIdLookup,
                                  Time::Unit unit)
{
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "PrintRoute at Time: " << Now().As(unit);
    *os << " from Node " << sourceNode->GetId() << " to Node " << dest->GetId();
    SimulationSingleton<GlobalRouteManagerImpl<IpManager>>::Get()->PrintRoute(sourceNode,
                                                                              dest,
                                                                              stream,
                                                                              nodeIdLookup,
                                                                              unit);
    (*os).copyfmt(oldState);
}

template <typename T>
void
GlobalRouteManager<T>::PrintRoute(Ptr<Node> sourceNode,
                                  Ptr<Node> dest,
                                  bool nodeIdLookup,
                                  Time::Unit unit)
{
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    GlobalRouteManager<T>::PrintRoute(sourceNode, dest, stream, nodeIdLookup, unit);
}

template <typename T>
void
GlobalRouteManager<T>::InitializeRouters()
{
    SimulationSingleton<GlobalRouteManagerImpl<typename GlobalRouteManager<T>::IpManager>>::Get()
        ->InitializeRouters();
}

template class ns3::GlobalRouteManager<ns3::Ipv4Manager>;
template class ns3::GlobalRouteManager<ns3::Ipv6Manager>;

} // namespace ns3
