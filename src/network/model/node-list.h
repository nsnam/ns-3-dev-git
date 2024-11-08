/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Mathieu Lacage <mathieu.lacage@sophia.inria.fr>,
 */
#ifndef NODE_LIST_H
#define NODE_LIST_H

#include "ns3/ptr.h"

#include <vector>

namespace ns3
{

class Node;
class CallbackBase;

/**
 * @ingroup network
 *
 * @brief the list of simulation nodes.
 *
 * Every Node created is automatically added to this list.
 */
class NodeList
{
  public:
    /// Node container iterator
    typedef std::vector<Ptr<Node>>::const_iterator Iterator;

    /**
     * @param node node to add
     * @returns index of node in list.
     *
     * This method is called automatically from Node::Node so
     * the user has little reason to call it himself.
     */
    static uint32_t Add(Ptr<Node> node);
    /**
     * @returns a C++ iterator located at the beginning of this
     *          list.
     */
    static Iterator Begin();
    /**
     * @returns a C++ iterator located at the end of this
     *          list.
     */
    static Iterator End();
    /**
     * @param n index of requested node.
     * @returns the Node associated to index n.
     */
    static Ptr<Node> GetNode(uint32_t n);
    /**
     * @returns the number of nodes currently in the list.
     */
    static uint32_t GetNNodes();
};

} // namespace ns3

#endif /* NODE_LIST_H */
