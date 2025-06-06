/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef NODE_CONTAINER_H
#define NODE_CONTAINER_H

#include "ns3/node.h"

#include <type_traits>
#include <vector>

namespace ns3
{

/**
 * @brief keep track of a set of node pointers.
 *
 * Typically ns-3 helpers operate on more than one node at a time.  For example
 * a device helper may want to install devices on a large number of similar
 * nodes.  The helper Install methods usually take a NodeContainer as a
 * parameter.  NodeContainers hold the multiple Ptr<Node> which are used
 * to refer to the nodes.
 */
class NodeContainer
{
  public:
    /// Node container iterator
    typedef std::vector<Ptr<Node>>::const_iterator Iterator;

    /**
     * @brief Create a NodeContainer that contains a list of _all_ nodes
     * created through NodeContainer::Create() and stored in the
     * ns3::NodeList.
     *
     * Whenever a Node is created, a Ptr<Node> is added to a global list of all
     * nodes in the system.  It is sometimes useful to be able to get to all
     * nodes in one place.  This method creates a NodeContainer that is
     * initialized to contain all of the simulation nodes,
     *
     * @returns a NodeContainer which contains a list of all Nodes.
     */
    static NodeContainer GetGlobal();

    /**
     * Create an empty NodeContainer.
     */
    NodeContainer();

    /**
     * Create a NodeContainer with exactly one node which has been previously
     * instantiated.  The single Node is specified by a smart pointer.
     *
     * @param node The Ptr<Node> to add to the container.
     */
    NodeContainer(Ptr<Node> node);

    /**
     * Create a NodeContainer with exactly one node which has been previously
     * instantiated and assigned a name using the Object Name Service.  This
     * Node is then specified by its assigned name.
     *
     * @param nodeName The name of the Node Object to add to the container.
     */
    NodeContainer(std::string nodeName);

    /**
     * Create a NodeContainer with the requested number of Nodes.
     *
     * This is syntactic sugar for
     *
     * @code
     *   NodeContainer nodes;
     *   nodes.Create (size);
     *   // or nodes.Create (size, systemId);
     * @endcode
     *
     * @param [in] n The number of nodes to create.
     * @param [in] systemId The system id or rank associated with this node
     */
    explicit NodeContainer(uint32_t n, uint32_t systemId = 0);

    /**
     * Create a node container which is a concatenation of multiple input
     * NodeContainers.
     *
     * @tparam Ts \deduced Template type parameter pack for the multiple
     *         NodeContainers
     * @param nc The first NodeContainer
     * @param args The remaining NodeContainers
     *
     * @note A frequently seen idiom that uses these constructors involves the
     * implicit conversion by constructor of Ptr<Node>.  When used, Ptr<Node>
     * will be passed to this constructor instead of NodeContainer&.
     * C++ will notice the implicit conversion path that goes through the
     * NodeContainer (Ptr<Node> node) constructor above.  Using this conversion
     * one may optionally provide arguments of Ptr<Node> to these constructors.
     */
    template <typename... Ts>
    NodeContainer(const NodeContainer& nc, Ts&&... args);

    /**
     * @brief Get an iterator which refers to the first Node in the
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
     *       (*i)->method ();  // some Node method
     *     }
     * @endcode
     *
     * @returns an iterator which refers to the first Node in the container.
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
     *       (*i)->method ();  // some Node method
     *     }
     * @endcode
     *
     * @returns an iterator which indicates an ending condition for a loop.
     */
    Iterator End() const;

    /**
     * @brief Get the number of Ptr<Node> stored in this container.
     *
     * Nodes can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the direct method and is typically used to
     * define an ending condition in a for-loop that runs through the stored
     * Nodes
     *
     * @code
     *   uint32_t nNodes = container.GetN ();
     *   for (uint32_t i = 0; i < nNodes; ++i)
     *     {
     *       Ptr<Node> p = container.Get (i);
     *       i->method ();  // some Node method
     *     }
     * @endcode
     *
     * @returns the number of Ptr<Node> stored in this container.
     */
    uint32_t GetN() const;

    /**
     * @brief Get the Ptr<Node> stored in this container at a given
     * index.
     *
     * Nodes can be retrieved from the container in two ways.  First,
     * directly by an index into the container, and second, using an iterator.
     * This method is used in the direct method and is used to retrieve the
     * indexed Ptr<Node>.
     *
     * @code
     *   uint32_t nNodes = container.GetN ();
     *   for (uint32_t i = 0; i < nNodes; ++i)
     *     {
     *       Ptr<Node> p = container.Get (i);
     *       i->method ();  // some Node method
     *     }
     * @endcode
     *
     * @param i the index of the requested node pointer.
     * @returns the requested node pointer.
     */
    Ptr<Node> Get(uint32_t i) const;

    /**
     * @brief Create n nodes and append pointers to them to the end of this
     * NodeContainer.
     *
     * Nodes are at the heart of any ns-3 simulation.  One of the first tasks that
     * any simulation needs to do is to create a number of nodes.  This method
     * automates that task.
     *
     * @param n The number of Nodes to create
     */
    void Create(uint32_t n);

    /**
     * @brief Create n nodes with specified systemId for distributed simulations
     * and append pointers to them to the end of this NodeContainer.
     *
     * Nodes are at the heart of any ns-3 simulation.  One of the first tasks that
     * any simulation needs to do is to create a number of nodes.  This method
     * automates that task, and adds the ability to specify systemId for
     * distributed simulations.
     *
     * @param n The number of Nodes to create
     * @param systemId The system id or rank associated with this node
     */
    void Create(uint32_t n, uint32_t systemId);

    /**
     * @brief Append the contents of another NodeContainer to the end of
     * this container.
     *
     * @param nc The NodeContainer to append.
     */
    void Add(const NodeContainer& nc);

    /**
     * @brief Append the contents of another NodeContainer to the end of
     * this container.
     *
     * @tparam Ts \deduced Template type parameter pack for the multiple
     *         NodeContainer
     * @param nc The NodeContainer to append
     * @param args The remaining NodeContainers to append
     */
    template <typename... Ts>
    void Add(const NodeContainer& nc, Ts&&... args);

    /**
     * @brief Append a single Ptr<Node> to this container.
     *
     * @param node The Ptr<Node> to append.
     */
    void Add(Ptr<Node> node);

    /**
     * @brief Append to this container the single Ptr<Node> referred to
     * via its object name service registered name.
     *
     * @param nodeName The name of the Node Object to add to the container.
     */
    void Add(std::string nodeName);

    /**
     * @brief Return true if container contains a Node with index id
     *
     * @param id Node Id
     * @return whether the NodeContainer contains a node with index id
     */
    bool Contains(uint32_t id) const;

  private:
    std::vector<Ptr<Node>> m_nodes; //!< Nodes smart pointers
};

} // namespace ns3

///////////////////////////////////////////////////////////
// Implementation of the templates declared above
///////////////////////////////////////////////////////////

namespace ns3
{

template <typename... Ts>
NodeContainer::NodeContainer(const NodeContainer& nc, Ts&&... args)
{
    static_assert(std::conjunction_v<std::is_convertible<Ts, NodeContainer>...>,
                  "Variable types are not convertible to NodeContainer");

    Add(nc, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
NodeContainer::Add(const NodeContainer& nc, Ts&&... args)
{
    static_assert(std::conjunction_v<std::is_convertible<Ts, NodeContainer>...>,
                  "Variable types are not convertible to NodeContainer");

    Add(nc);
    Add(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* NODE_CONTAINER_H */
