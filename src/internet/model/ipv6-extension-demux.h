/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#ifndef IPV6_EXTENSION_DEMUX_H
#define IPV6_EXTENSION_DEMUX_H

#include "ns3/object.h"
#include "ns3/ptr.h"

#include <list>

namespace ns3
{

class Ipv6Extension;
class Node;

/**
 * @ingroup ipv6HeaderExt
 *
 * @brief Demultiplexes IPv6 extensions.
 */
class Ipv6ExtensionDemux : public Object
{
  public:
    /**
     * @brief The interface ID.
     * @return type ID
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor.
     */
    Ipv6ExtensionDemux();

    /**
     * @brief Destructor.
     */
    ~Ipv6ExtensionDemux() override;

    /**
     * @brief Set the node.
     * @param node the node to set
     */
    void SetNode(Ptr<Node> node);

    /**
     * @brief Insert a new IPv6 Extension.
     * @param extension the extension to insert
     */
    void Insert(Ptr<Ipv6Extension> extension);

    /**
     * @brief Get the extension corresponding to extensionNumber.
     * @param extensionNumber extension number of the extension to retrieve
     * @return a matching IPv6 extension
     */
    Ptr<Ipv6Extension> GetExtension(uint8_t extensionNumber);

    /**
     * @brief Remove an extension from this demux.
     * @param extension pointer on the extension to remove
     */
    void Remove(Ptr<Ipv6Extension> extension);

  protected:
    /**
     * @brief Dispose object.
     */
    void DoDispose() override;

  private:
    /**
     * @brief Container of the IPv6 Extensions.
     */
    typedef std::list<Ptr<Ipv6Extension>> Ipv6ExtensionList_t;

    /**
     * @brief List of IPv6 Extensions supported.
     */
    Ipv6ExtensionList_t m_extensions;

    /**
     * @brief The node.
     */
    Ptr<Node> m_node;
};

} /* namespace ns3 */

#endif /* IPV6_EXTENSION_DEMUX_H */
