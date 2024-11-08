/*
 * Copyright (c) 2008-2009 Strasbourg University
 *               2013 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef IPV6_INTERFACE_CONTAINER_H
#define IPV6_INTERFACE_CONTAINER_H

#include "ns3/ipv6-address.h"
#include "ns3/ipv6.h"

#include <stdint.h>
#include <vector>

namespace ns3
{

/**
 * @ingroup ipv6
 *
 * @brief Keep track of a set of IPv6 interfaces.
 */
class Ipv6InterfaceContainer
{
  public:
    /**
     * @brief Container Const Iterator for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv6>, uint32_t>>::const_iterator Iterator;

    /**
     * @brief Constructor.
     */
    Ipv6InterfaceContainer();

    /**
     * @returns the number of Ptr<Ipv6> and interface pairs stored in this
     * Ipv6InterfaceContainer.
     *
     * Pairs can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the direct method and is typically used to
     * define an ending condition in a for-loop that runs through the stored
     * Nodes
     *
     * @code
     *   uint32_t nNodes = container.GetN ();
     *   for (uint32_t i = 0 i < nNodes; ++i)
     *     {
     *       std::pair<Ptr<Ipv6>, uint32_t> pair = container.Get (i);
     *       method (pair.first, pair.second);  // use the pair
     *     }
     * @endcode
     */
    uint32_t GetN() const;

    /**
     * @brief Get the interface index for the specified node index.
     * @param i index of the node
     * @return interface index
     */
    uint32_t GetInterfaceIndex(uint32_t i) const;

    /**
     * @brief Get the address for the specified index.
     * @param i interface index
     * @param j address index, generally index 0 is the link-local address
     * @return IPv6 address
     */
    Ipv6Address GetAddress(uint32_t i, uint32_t j) const;

    /**
     * @brief Get the link-local address for the specified index.
     * @param i index
     * @return the link-local address, or "::" if the interface has no link local address.
     */
    Ipv6Address GetLinkLocalAddress(uint32_t i);

    /**
     * @brief Get the link-local address for the node with the specified global address.
     * @param address the address to find.
     * @return the link-local address, or "::" if the interface has no link local address.
     */
    Ipv6Address GetLinkLocalAddress(Ipv6Address address);

    /**
     * @brief Add a couple IPv6/interface.
     * @param ipv6 IPv6 address
     * @param interface interface index
     */
    void Add(Ptr<Ipv6> ipv6, uint32_t interface);

    /**
     * @brief Get an iterator which refers to the first pair in the
     * container.
     *
     * Pairs can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the iterator method and is typically used in a
     * for-loop to run through the pairs
     *
     * @code
     *   Ipv6InterfaceContainer::Iterator i;
     *   for (i = container.Begin (); i != container.End (); ++i)
     *     {
     *       std::pair<Ptr<Ipv6>, uint32_t> pair = *i;
     *       method (pair.first, pair.second);  // use the pair
     *     }
     * @endcode
     *
     * @returns an iterator which refers to the first pair in the container.
     */
    Iterator Begin() const;

    /**
     * @brief Get an iterator which indicates past-the-last Node in the
     * container.
     *
     * Nodes can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the iterator method and is typically used in a
     * for-loop to run through the Nodes
     *
     * @code
     *   NodeContainer::Iterator i;
     *   for (i = container.Begin (); i != container.End (); ++i)
     *     {
     *       std::pair<Ptr<Ipv6>, uint32_t> pair = *i;
     *       method (pair.first, pair.second);  // use the pair
     *     }
     * @endcode
     *
     * @returns an iterator which indicates an ending condition for a loop.
     */
    Iterator End() const;

    /**
     * @brief Fusion with another Ipv6InterfaceContainer.
     * @param c container
     */
    void Add(const Ipv6InterfaceContainer& c);

    /**
     * @brief Add a couple of name/interface.
     * @param ipv6Name name of a node
     * @param interface interface index to add
     */
    void Add(std::string ipv6Name, uint32_t interface);

    /**
     * Get the std::pair of an Ptr<Ipv6> and interface stored at the location
     * specified by the index.
     *
     * @param i the index of the container entry to retrieve.
     * @return the std::pair of a Ptr<Ipv6> and an interface index
     *
     * @note The returned Ptr<Ipv6> cannot be used directly to fetch the
     *       Ipv6Interface using the returned index (the GetInterface () method
     *       is provided in class Ipv6L3Protocol, and not class Ipv6). An
     *       example usage is provided below.
     *
     * @code
     *   Ipv6InterfaceContainer c;
     *   ...
     *   std::pair<Ptr<Ipv6>, uint32_t> returnValue = c.Get (0);
     *   Ptr<Ipv6> ipv6 = returnValue.first;
     *   uint32_t index = returnValue.second;
     *   Ptr<Ipv6Interface> iface =  DynamicCast<Ipv6L3Protocol> (ipv6)->GetInterface (index);
     * @endcode
     */
    std::pair<Ptr<Ipv6>, uint32_t> Get(uint32_t i) const;

    /**
     * @brief Set the state of the stack (act as a router or as an host) for the specified index.
     * This automatically sets all the node's interfaces to the same forwarding state.
     * @param i index
     * @param state true : is a router, false : is an host
     */
    void SetForwarding(uint32_t i, bool state);

    /**
     * @brief Set the default route for all the devices (except the router itself).
     * @param router the default router index
     */
    void SetDefaultRouteInAllNodes(uint32_t router);

    /**
     * @brief Set the default route for all the devices (except the router itself).
     * Note that the route will be set to the link-local address of the node with the specified
     * address.
     * @param routerAddr the default router address
     */
    void SetDefaultRouteInAllNodes(Ipv6Address routerAddr);

    /**
     * @brief Set the default route for the specified index.
     * @param i index
     * @param router the default router
     */
    void SetDefaultRoute(uint32_t i, uint32_t router);

    /**
     * @brief Set the default route for the specified index.
     * Note that the route will be set to the link-local address of the node with the specified
     * address.
     * @param i index
     * @param routerAddr the default router address
     */
    void SetDefaultRoute(uint32_t i, Ipv6Address routerAddr);

  private:
    /**
     * @brief Container for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv6>, uint32_t>> InterfaceVector;

    /**
     * @brief List of IPv6 stack and interfaces index.
     */
    InterfaceVector m_interfaces;
};

} /* namespace ns3 */

#endif /* IPV6_INTERFACE_CONTAINER_H */
