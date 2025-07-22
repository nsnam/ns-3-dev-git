.. include:: replace.txt
.. highlight:: cpp

Global Routing
==============

This chapter defines the implementation of the ``GlobalRouting`` API present in the internet module. Global routing is not a dynamic routing protocol that
sends packets over the simulated network.  Instead, it is a simulation
process that walks the simulation topology (typically just before the
simulation starts), runs a shortest path algorithm on the discovered
topology, and populates each node's routing tables accordingly. No actual
protocol overhead (on the simulated links) is incurred with this approach.

The quagga (`<https://www.nongnu.org/quagga/>`_) OSPF implementation was used as the
basis for the routing computation logic. The Link State information exported by each global router in
this API conforms to the OSPFv2 specs (:rfc:`2328`) as used by the quagga implementation.

Presently, global centralized IPv[4,6] unicast routing over both point-to-point and
shared (CSMA) links is supported.

The following is a high level overview of the classes that are used to implement the Global Routing API:

* **User Interface**: ``Ipv4GlobalRoutingHelper`` and ``GlobalRouteManager``
* **Route Computation**: ``SPFVertex``, ``GlobalRouteManagerLSDB`` and  ``GlobalRouteManagerImpl``
* **Link State Advertisements**: ``GlobalRoutingLSA``, ``GlobalRoutingLinkRecord`` and ``GlobalRouter``
* **RouteLookup and Forwarding**: ``GlobalRouting``

Scope and Limitations
---------------------
The API has a few constraints:

* **Wired only:** It is not intended for use in wireless networks. For wireless, OLSR dynamic routing is recommended, as described below.
* **Unicast only:** It does not support multicast.
* **Scalability:** Some users of this API in large topologies (e.g., 1000 nodes) have observed that the current implementation
  is not very scalable.The global centralized routing is planned to be modified in the future to reduce computations and improve runtime performance.

The following are the known limitations of the GlobalRouting implementation:

#. When calculating routes for networks using GlobalRouting, if multiple exit interfaces are present from the root node to a network,
   packets may travel an extra hop to reach their destination within that network.
   To clarify, the following are the generalized conditions:

  #. It is assumed that there is a network (e.g., 10.1.2.0/24) that has 2 (or more) routers (2 egress points, or more).
  #. It is assumed that a node has 2 (or more) equal-cost paths toward the aforementioned network.
  #. It is assumed that multiple IP addresses with that network prefix are present on one or more interfaces.

This will result in packets reaching the internal interfaces of the "other" router(s) via one unnecessary hop,
passing through the designated router, and discarding the (shorter) path that directly leads to the destination.

 .. _Global-Routing-Limitation:

 .. figure:: figures/global-routing-limitation.*

#. Although adding multiple IP addresses on the same interface and Equal Cost Multi Path are both supported individually,
the combination of both is not fully supported and may lead to unexpected results and,
in some specific cases, routing loops.
This limitation is tracked in issue #1242 of the issue tracker.

The Following are the known limitations of the GlobalRouting implementation for Ipv6 ONLY:

#. The GlobalRouting implementation for Ipv6 does not support non-Onlink Ipv6Addresses. The API will assert if it encounters such an address.
#. The GlobalRouting implementation for Ipv6 does not support StrongEndSystem Models. Calls to GlobalRouting
   via the Ipv6GlobalRoutingHelper will automatically disable StrongEndSystem model for all nodes in the system.
   When using the GlobalRouteManager directly, make sure to disable StrongEndSystem model for all nodes in the system.

Usage
-----

To use GlobalRouting as the routing protocol, an instance of Ipv[4,6]GlobalRoutingHelper needs
to be set using InternetStackHelper::SetRoutingProtocol().  Example code of this is::

  InternetStackHelper internet;
  Ipv4GlobalRoutingHelper glbrouting;
  internet.SetRoutingHelper(glbrouting);

Further usage is typically based on the following helper code.

Helpers
~~~~~~~
Users must include the ns3/ipv4-global-routing-helper.h or ns3/ipv6-global-routing-helper.h
header file for IPv4 or IPv6, respectively.  After IP addresses are configured, the
following function call will cause all of the nodes that have an Ipv4 interface
to receive forwarding tables entered automatically by the GlobalRouteManager::

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

Likewise, if IPv6 is used::

  Ipv6GlobalRoutingHelper::PopulateRoutingTables();

.. note::
    A reminder that the wifi NetDevice will work, but does not take any
    wireless effects into account. For wireless, we recommend OLSR dynamic routing.

.. note::
    For IPv6, calls to GlobalRouting via the Ipv6GlobalRoutingHelper will enable
    forwarding for all nodes in the simulation with GlobalRouting installed.
    If more control is desired, it is recommended to use the GlobalRouteManager directly.


It is possible to call this function again in the midst of a simulation using
the following additional public function::

  Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
  Ipv6GlobalRoutingHelper::RecomputeRoutingTables();

which flushes the old tables, queries the nodes for new interface information,
and rebuilds the routes.

For instance, this scheduling call will cause the tables to be rebuilt
at time 5 seconds::

  Simulator::Schedule(Seconds(5),
                      &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);

GlobalRouteManager
~~~~~~~~~~~~~~~~~~

GlobalRouteManager class can also be used to run the Global Routing API.

Users must include ns3/global-route-manager.h header file.  After the
IPv4 topology has been built and addresses assigned, users call
ns3::GlobalRouteManager::PopulateRoutingTables (), prior to the
ns3::Simulator::Run() call.

It has the following functions:

* DeleteGlobalRoutes() - Deletes all the routes in the routing table for each node in the simulation.
* BuildGlobalRoutingDatabase() - Queries each of the globalRouter in the simulation and collects
  the Link State Advertisements exported by them into a Link State Database with key as the LinkstateID.
* InitializeRoutes() -For each node that is a global router (which is determined by the presence of an aggregated GlobalRouter interface),
  run the Dijkstra SPF calculation on the database rooted at that router, and populate the node forwarding tables.

Note: Calls to GlobalRouteManager will in turn call the Simulation Singleton Object GlobalRouteManagerImpl to run the actual routing logic.


Default Usage
~~~~~~~~~~~~~
By default, when using the |ns3| helper API and the default InternetStackHelper,
global routing capability will be added to the node, and global routing will be
inserted as a routing protocol with lower priority than the static routes (i.e.,
users can insert routes via Ipv[4,6]StaticRouting API and they will take precedence
over routes found by global routing).

The public API is very minimal. User scripts include the following::

    #include "ns3/internet-module.h"

If the default InternetStackHelper is used, then an instance of GlobalRouting and globalRouter
will be aggregated to each node.

Attributes
~~~~~~~~~~

There are two attributes that govern the behavior of global routing.

#. Ipv4GlobalRouting::RandomEcmpRouting. If set to true, packets are randomly
   routed across equal-cost multipath routes. If set to false (default), only one
   route is consistently used.

#. Ipv4GlobalRouting::RespondToInterfaceEvents. If set to true, dynamically
   recompute the global routes upon Interface notification events (up/down, or
   add/remove address). If set to false (default), routing may break unless the
   user manually calls RecomputeRoutingTables() after such events. The default is
   set to false to preserve legacy |ns3| program behavior.

The following code shows how to configure these attributes::

    Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

The helper also provides a ``PrintRoute()`` method that allows users to
print the routing path calculated between two nodes; see the example
program ``src/internet/examples/ipv4-global-routing-print-route.cc`` for a
number of example statements showing how this method can be called.


Traces
~~~~~~

Not Applicable



Implementation Details
----------------------
This section is for those readers who care about how this is implemented.
A singleton object (GlobalRouteManager) is responsible for populating the static routes on each node,
using the public Ipv[4,6] API of that node.
It queries each node in the topology for a “globalRouter” interface.
If found, it uses the API of that interface to obtain a “link state advertisement (LSA)” for the router.
Link State Advertisements are used in OSPF routing, and the formatting is followed.

It is important to note that all of these computations are done before packets are flowing in the network.
In particular, there are no overhead or control packets being exchanged when using this implementation.
Instead, this global route manager just walks the list of nodes to build the necessary information
and configure each node’s routing table.

The GlobalRouteManager populates a link state database with LSAs gathered from the entire topology.
Then, for each router in the topology, the GlobalRouteManager executes
the OSPF shortest path first (SPF) computation on the database and populates the routing tables on each node.

The quagga (https://www.nongnu.org/quagga/) OSPF implementation was used as the basis for the routing computation logic.
One benefit of following an existing OSPF SPF implementation is that
OSPF already has defined link state advertisements for all common types of network links:
- Point-to-point (serial links).
- Point-to-multipoint (Frame Relay, ad hoc wireless).
- Non-broadcast multiple access (ATM).
- Broadcast (Ethernet).

Therefore, it is believed that enabling these other link types will be more
straightforward now that the underlying OSPF SPF framework is in place.
Currently, the implementation can handle IPv[4,6] point-to-point, numbered links,
as well as shared broadcast (CSMA) links. Equal-cost multipath is also supported.
Although wireless link types are supported by the implementation,
it is noted that due to the nature of this implementation, any channel effects will not be considered,
and the routing tables will assume that every node on the same shared channel is reachable from every other node
(i.e., it will be treated like a broadcast CSMA link).

The GlobalRouteManager first walks the list of nodes and aggregates a GlobalRouter interface to each one as follows::

  typedef std::vector<Ptr<Node>>::iterator Iterator;
  for (Iterator i = NodeList::Begin(); i != NodeList::End(); i++)
  {
  Ptr<Node> node = *i;
  Ptr<GlobalRouter> globalRouter = CreateObject<GlobalRouter>(node);
  node->AggregateObject(globalRouter);
  }

This interface is later queried and used to generate a Link State Advertisement for each router,
and this link state database is fed into the OSPF shortest path computation logic.
The Ipv4 API is finally used to populate the routes themselves.

Examples and Tests
------------------

Currently no examples are available for the Global Routing API.

Two TestSuites are available for the Global Routing API in internet/test:

* ``ipv4-global-routing-test-suite``
* ``global-route-manager-impl-test-suite``

The TestSuites are located in the ns3/src/internet/test directory.
The Following is a brief description of the TestCases Contained within:

**global-route-manager-impl-test-suite**

* ``LinkRoutesTestCase`` : Checks Default Routes for stub nodes and routes for p2p links
* ``LanRoutesTestCase`` : Checks Network Routes for LAN Topologies. Checks that the Network LSA is handled by GlobalRouteManagerImpl
* ``RandomEcmpTestCase`` : Checks the RandomEcmp Attribute. i.e if equal cost multipath routes are possible build them and use them at random
* ``GlobalRoutingProtocolTestCase`` : Checks the normal operation of GlobalRouting, by looking at the output of the RouteInput() and RouteOutput() for several different paths. This mimics how normal calls to GlobalRouting are made.

**ipv4-global-routing-test-suite**

* ``Link test``: Point-to-point link between two nodes, verifying default stub-link routes.
* ``LAN test``: Two nodes on a broadcast network, verifying direct LAN subnet routes.
* ``Two link test``: Three nodes on two point-to-point links, checking default and subnet routes.
* ``Two LANs test``: Three nodes on two broadcast networks, testing inter-LAN routing.
* ``Bridge test``: Two Links connected through a bridge, verifying routing across the bridged segment.
* ``Two Bridge test``: links connected by two bridges in series, testing multi-hop bridging.


Validation
----------

No formal validation (comparison of the routing tables generated by this
implementation against the results of a real routing protocol)
has been performed.

References
----------

[`1 <https://www.nongnu.org/quagga/>`_] The quagga routing suite was used as the basis for the routing computation logic.

[2] :rfc:`2328` was used as the basis for the information exported by the global routers for a single area network configuration.


