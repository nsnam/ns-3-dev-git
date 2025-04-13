Dynamic Source Routing (DSR)
============================

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ============= Chapter
   ------------- Section (#.#)
   ~~~~~~~~~~~~~ Subsection (#.#.#)

Dynamic Source Routing (DSR) protocol is a reactive routing protocol designed specifically
for use in multi-hop wireless ad hoc networks of mobile nodes. The current model implementation is based on the RFC 4728 [2_], with some extensions
and modifications. The initial implementation was developed by `the ResiliNets research group <https://resilinets.org/>`_ at the University of Kansas with additions to the link cache mechanism done by Song Luan <lsuper@mail.ustc.edu.cn>.

The source code for this module lives in the directory ``src/dsr``.

.. _fig-dsrRreq:

.. figure:: figures/dsrRreq.*

    DSR route discovery

.. _fig-dsrRrep:

.. figure:: figures/dsrRrep.*

    DSR route reply using route caching

DSR creates routes towards a destination by adding the traversed route into the packet header during the discovery phase.
Different packets might have different routes even if they have the same source and the same destination.
The recorded information during the route discovery phase is used by intermediate nodes to determine whom the packet should be forward.
Hence, the term dynamic source routing.

Scope and Limitations
---------------------

* The model is not fully compliant with RFC 4728 [2_]. As an example, DSR fixed size header has been extended and it is four octets longer then the RFC specification. As a consequence, the DSR headers can not be correctly decoded by Wireshark.
* At the moment, the model does not handle ICMP packets, and assumes that traffic is either TCP or UDP.
* No flow state available.
* No First Hop External (F), Last Hop External (L) flags.
* There are no ways of handling unknown DSR options.
* DSR option ``flow state not supported`` is not handled.
* DSR option ``unsupported option`` is not handled.
* Route Reply header is not aligned to the DSR RFC descriptions.


Protocol operational details
----------------------------

DSR operates on a on-demand behavior. Therefore, our DSR model buffers all packets while a route request packet (RREQ) is disseminated.
The packet buffer is implemented in ``dsr-rsendbuff.cc``.

The packet queue implements garbage collection of old packets and a queue size limit.
When the packet is sent out from the send buffer, it will be queued in maintenance buffer for next hop acknowledgment.

The maintenance buffer then buffers the already sent out packets and waits for the notification of packet delivery.
Protocol operation strongly depends on broken link detection mechanism.
Three heuristics detection mechanisms are implementented as recommended by the RFC as follows:

1. A link layer feedback is used when possible, which is also the fastest mechanism of these three to detect link errors. A link is considered to be broken if frame transmission results in a transmission failure for all retries. This mechanism is meant for active links and works much faster than in its absence. DSR is able to detect the link layer transmission failure and notify that as broken. Recalculation of routes will be triggered when needed. If user does not want to use link layer acknowledgment, it can be tuned by setting the ``LinkAcknowledgment`` attribute to false in ``dsr-routing.cc``.

2. Passive acknowledgment should be used whenever possible. The node turns on "promiscuous" receive mode, in which it can receive packets not destined for itself, and when the node assures the delivery of that data packet to its destination, it cancels the passive acknowledgment timer.

3. A network layer acknowledment scheme is used to notify the receipt of a packet. Route request packet are not be acknowledged or retransmitted.

Route Cache implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~

The Route Cache implementation support garbage collection of old entries and state machine, as defined in the standard.
It is implemented as a STL map container. The destination IP address is used as a key in the map.

DSR operates with direct access to IP header, and operates between network and transport layer.
When packet is sent out from transport layer, it passes itself to DSR and DSR header is appended.

Two caching mechanisms exists in the implementation: Path cache and Link cache.

- **Path Cache:** As the name implies, this cache saves the whole path and ensures loop-free paths.
  The paths are sorted based on the hop count, and whenever one path becomes unusable a different path is chosen. Some of its characteristics are as follows:

  * The cache has automatic expire policy.
  * The cache saves multiple route entries for a certain destination and sort the entries based on hop counts.
  * The ``MaxEntriesEachDst`` can be tuned to change the maximum entries saved for a single destination.
  * When adding multiple routes for one destination, the route is compared based on hop-count and expire time, the one with less hop count or relatively new route is favored.

- **Link Cache:** This is an improvement over the patch cache in the sense that it uses different subpaths and make use ot the Dijkstra algorithm.

Modifications
~~~~~~~~~~~~~

* The DsrFsHeader has added 3 fields: message type, source id, destination id, and these changes only for post-processing.

  1. Message type is used to identify the data packet from control packet.
  2. Source id is used to identify the real source of the data packet since the packet is delivered hop-by-hop and the Ipv4Header is not carrying the real source and destination ip address as needed.
  3. Destination id is for same reason of above.

* DSR works as a shim header between transport and network protocol, it needs its own forwarding mechanism, we are changing the packet transmission to hop-by-hop delivery, so we added two fields in dsr fixed header to notify packet delivery.

Other considerations
~~~~~~~~~~~~~~~~~~~~

The following should be kept in mind when running DSR as routing protocol:

* NodeTraversalTime is the time it takes to traverse two neighboring nodes and should be chosen to fit the transmission range
* PassiveAckTimeout is the time a packet in maintenance buffer wait for passive acknowledgment, normally set as two times of NodeTraversalTime
* RouteCacheTimeout should be set smaller value when the nodes' velocity become higher. The default value is 300s.

Usage
-----

In |ns3|, DSR is used on top of WiFi nodes. The simplest way to use it requires the use of its helper
as explained in the following subsections.

Helpers
~~~~~~~

To have a node run DSR, the easiest way would be to use the DsrHelper
and DsrMainHelpers in your simulation script. For instance:

.. sourcecode:: cpp

   NodeContainer adhocNodes;
   nodes.Create(5); // Add 5 nodes to the node container
   DsrHelper dsr;
   DsrMainHelper dsrMain;
   dsrMain.Install(dsr, adhocNodes); // Install DSR on each node


Attributes
~~~~~~~~~~

The complete list of available attributes with descriptions for this model can be found on `this link <https://www.nsnam.org/doxygen/de/deb/classns3_1_1dsr_1_1_dsr_routing.html#details>`_.

Traces
~~~~~~

The complete list of available trace sources with descriptions for this model can be found on `this link <https://www.nsnam.org/doxygen/de/deb/classns3_1_1dsr_1_1_dsr_routing.html#details>`_.


Examples and Tests
------------------

The example can be found in ``src/dsr/examples/``:

* ``dsr.cc``: Use DSR as routing protocol within a traditional MANETs environment.

DSR is also built in the routing comparison case in ``examples/routing/``:

* ``manet-routing-compare.cc``: A comparison case with built in MANET routing protocols and can generate its own results.

Validation
----------

This model has been tested as follows:

* Unit tests have been written to verify the internals of DSR. This can be found in ``src/dsr/test/dsr-test-suite.cc``. These tests verify whether the methods inside DSR module which deal with packet buffer, headers work correctly.
* Simulation cases similar to [3_] have been tested and have comparable results.
* Manet-routing-compare.cc has been used to compare DSR with three of other routing protocols.
* A paper was presented on these results at the Workshop on ns-3 in 2011 [4_].

References
----------

[`1 <https://web.archive.org/web/20150430233910/http://www.monarch.cs.rice.edu/monarch-papers/dsr-chapter00.pdf>`_] Johnson, David B., David A. Maltz, and Josh Broch. "DSR: The dynamic source routing protocol for multi-hop wireless ad hoc networks." Ad hoc networking 5.1 (2001): 139-172.

[`2 <https://datatracker.ietf.org/doc/html/rfc4728>`_] RFC 4728

[`3 <https://dl.acm.org/doi/10.1145/288235.288256>`_] Josh Broch, David A. Maltz, David B. Johnson, Yih-Chun Hu, and Jorjeta Jetcheva. 1998. A performance comparison of multi-hop wireless ad hoc network routing protocols. In Proceedings of the 4th annual ACM/IEEE international conference on Mobile computing and networking (MobiCom '98). Association for Computing Machinery, New York, NY, USA, 85–97.

[`4 <https://dl.acm.org/doi/10.5555/2263019.2263077>`_] Yufei Cheng, Egemen K. Çetinkaya, and James P. G. Sterbenz. 2012. Dynamic source routing (DSR) protocol implementation in ns-3. In Proceedings of the 5th International ICST Conference on Simulation Tools and Techniques (SIMUTOOLS '12). ICST (Institute for Computer Sciences, Social-Informatics and Telecommunications Engineering), Brussels, BEL, 367–374.







