.. include:: replace.txt
.. highlight:: cpp


6LoWPAN: IPv6 over Low-Power Wireless Personal Area Networks
=================================================================

This describes the implementation of |ns3| model for the
compression of IPv6 packets over low-power wireless networks,
as originally specified by :rfc:`4944` ("Transmission of IPv6 Packets over IEEE 802.15.4 Networks")
and :rfc:`6282` ("Compression Format for IPv6 Datagrams over IEEE 802.15.4-Based Networks").

The 6LoWPAN optimized neighbour discovery (6LoWPAN-ND) is also implemented in this module, as specified by :rfc:`8505` ("Registration Extensions for IPv6 over Low-Power Wireless Personal Area Network (6LoWPAN) Neighbor Discovery") and :rfc:`6775` ("Neighbor Discovery Optimization for IPv6 over Low-Power Wireless Personal Area Networks (6LoWPANs)").

6LoWPAN was originally proposed for IEEE 802.15.4 (Lr-WPAN), but it has been also proposed for other constrained resource networks, such as Sub-1 GHz low-power RF, Bluetooth LE, etc. To reflect this, the ns-3 module design allows for 6LoWPAN to be used on any network, even non-constrained ones like CSMA or WiFi. The module strictly follows :rfc:`4944` and :rfc:`6282`, with the exception of header compression 2 (HC2) encoding which is not supported, as it has been superseded by IP Header Compression (IPHC) and Next Header Compression (NHC) types (:rfc:`6282`).

**SixLowPanNetDevice**

The whole module is developed as a transparent NetDevice, which can act as a
proxy between IPv6 and any NetDevice (the module has been successfully tested
with ``PointToPointNetDevice``, ``CsmaNetDevice`` and ``LrWpanNetDevice``). For this reason, the module implements a virtual NetDevice, and all the calls are passed
without modifications to the underlying NetDevice. The only important difference is in
the ``GetMtu`` behaviour, which will always return *at least* 1280 bytes, as mandated by the minimum IPv6 MTU:

* If the underlying NetDevice has an MTU smaller than 1280 octets (common for IEEE 802.15.4), 6LoWPAN silently raises the reported MTU to 1280 and handles fragmentation and reassembly internally at the 6LoWPAN level.
* If the underlying NetDevice has an MTU of 1280 octets or larger, the MTU is left unchanged and 6LoWPAN fragmentation is applied only when necessary (uncommon, but foreseen by the standard).

In both cases this is completely transparent to the upper layers.

The source code for the sixlowpan module lives in the directory ``src/sixlowpan``.

Scope and Limitations
---------------------

The following is a list of known limitations of the |ns3| 6LowPAN implementation:

* 6lowPAN requires a preset MAC address before the start of the simulation. In other words, it cannot dynamically extract the underlying MAC addresses once the simulation has started.
* When used along side IEEE 802.15.4, only short addresses (16-bit) are supported.
* HC2 is not included but it is deprecated in the RFCs.

The following is a list of limitations for 6LowPAN-ND:

* Lack of support for multi-hop DAD exchanges
* Limited NA (EARO) status errors supported from :rfc:`6775` (Currently only supports "Duplicate Address" error)
* Currently supports only single-hop, mesh-under routing topologies between 6LBR and 6LN
* Missing Transaction ID validation, which is part of the :rfc:`8505` specification

Compression
-----------

IPHC stateful (context-based) and HC1 compressions are supported. The IPv6/MAC addressing schemes defined in :rfc:`6282` and :rfc:`4944` are different.
One adds the PanId in the pseudo-MAC address (4944) and the other doesn't (6282).

The expected use cases (confirmed by the RFC editor) is to *never* have a mixed environment
where part of the nodes are using HC1 and part IPHC because this would lead to confusion on
what the IPv6 address of a node is. Due to this, the nodes configured to use IPHC will drop the packets compressed with HC1
and vice-versa. The drop is logged in the drop trace as ``DROP_DISALLOWED_COMPRESSION``.

When using the IPHC stateful compression, nodes need to be aware of the context. To manually set the context,
it is possible to use the  ``SixLowPanHelper::AddContext`` function. Please be aware that installing different contexts for different nodes will lead to decompression failures.

Routing Handling
----------------

6lowPAN provides two different approaches for routing IPv6 packets within a 6lowPAN network:

1. Mesh-under routing
2. Route over routing

**Mesh-under routing**

A mesh-under routing approach indicates that a routing system is implemented below IP and 6lowPAN will make the packet routing decisions based on layer 2 addresses.
The 6lowPAN |ns3| module provides a very simple mesh-under routing, implemented as a flooding

At node level, each packet is re-broadcasted if its BC0 sequence number is not in the cache of the
recently seen packets. The cache length (by default 10) can be changed through the ``MeshCacheLength`` attribute. This functionality can be activated through the ``UseMeshUnder`` attribute and fine-tuned using
the ``MeshUnderRadius`` and ``MeshUnderJitter`` attributes.

.. note::
    Flooding in a PAN generates a lot of overhead, which is often not wanted. Moreover, when using the mesh-under facility, ALL the packets are sent without acknowledgment because, at lower level, they are sent to a broadcast address.

The current mesh-under flooding routing mechanism could be improved by providing the following:

* Adaptive hop-limit calculation,
* Adaptive forwarding jitter,
* Use of direct (non mesh) transmission for packets directed to 1-hop neighbors.

**Route-over routing**

The routing decisions are made at the network layer (over IP) and use IPv6 addresses for routing like traditional IP networks.
The usage of route-over routing requires more processing at each hop as the IPV6 and other headers needs to be processed.
However, it is more flexible than the mesh-under approach as you can use one or more routing mechanism to reach the destination.

Examples of routing protocols used with 6lowPAN route-over include protocols such as the Routing protocol for Low-Power and Lossy Networks (RPL) and
the Ad-hoc On-Demand Distance Vector for IPV6 (AODVv6).

6lowPAN Optimized Neighbor Discovery (6lowPAN-ND)
-------------------------------------------------

IPv6 Neighbor Discovery (ND) as defined in :rfc`4861` assumes always on, multicast friendly links, which are not possible for low-power and lossy networks (LLNs) such as those using IEEE 802.15.4.

:rfc:`6775` and :rfc:`8505` define 6LoWPAN Neighbor Discovery (6LoWPAN-ND) to address these limitations. Some key optimizations include:

- Host-initiated interactions (sleep-aware RAs): routers need no longer send periodic or unsolicited RAs, instead relying on hosts to initiate interactions for address registration.
- Use of unicast communication for address resolution and neighbor discovery.
- Registration of addresses with a 6LoWPAN Router (6LR) or 6LoWPAN Border Router (6LBR) to reduce broadcast traffic and energy usage.
- 6LoWPAN Context Option (6CO) distributes header-compression contexts to hosts using 6LoWPAN, enabling efficient compression of IPv6 headers.

In ns-3, 6LoWPAN-ND subclasses Icmpv6L4Protocol, taking over the functions of conventional Ipv6 Neighbour Discovery as defined in :rfc:`4861`.

It does this by introducing a registration mechanism for 6LoWPAN nodes (6LNs) to register their addresses with a 6LoWPAN router (6LR) or border router (6LBR).
This allows 6LNs to use the 6LR/LBR as a proxy for address resolution and neighbor discovery, reducing the need for broadcast messages.

Every node that implements 6LoWPAN-ND will typically have a protocol stack that resembles the following:

.. list-table::
   :align: center
   :class: bordered-table
   :header-rows: 0

   * - **L4 SixLowPanNdProtocol (Subclasses Icmpv6L4Protocol)**
   * - **L3 Ipv6L3Protocol**
   * - **L2.5 SixLowPanNetDevice (shim)**
   * - **L2 LrWpanNetDevice**

6LoWPAN-ND Network Topologies:
------------------------------

**Mesh-Under Routing**

The current implementation of 6LoWPAN-ND in |ns3| supports mesh-under routing with single-hop star topologies, where one or more 6LoWPAN Nodes (6LNs) register their IPv6 addresses directly with a single 6LoWPAN Router (6LR), which may also act as the 6LoWPAN Border Router (6LBR).

In this setup:

- All address registration (using NS(EARO)) is performed over the same local link.
- Router Advertisements (RAs) and Neighbor Solicitations (NS) are exchanged directly between 6LNs and the 6LR/LBR.
- The registration cache is maintained at the 6LBR.

.. _fig-sixlowpanndtopology:

.. figure:: figures/sixlowpanndtopology.*
    :width: 200

    Example topology of a 6LoWPAN-ND network using mesh-under routing

**Route-Over Topology**

The multi-hop route-over topology is defined in :rfc:`8505`, and is currently in active development in ns-3. In this topology, 6LNs register with a single 6LR, and the 6LR can forward packets to the 6LBR, allowing for multi-hop DAD and global address registration across the LLN.

In this setup:

- All link-local address registration (using NS(EARO)) is performed over the same local link.
- Router Advertisements (RAs) and Neighbor Solicitations (NS) are exchanged directly between 6LNs and the 6LR/LBR, or between 6LRs and the 6LBR.
- The global address registration cache is maintained at the 6LBR.

.. _fig-sixlowpanndrouteovertopology:

.. figure:: figures/sixlowpanndrouteovertopology.*
    :width: 200

    Example topology of a 6LoWPAN-ND network using route-over topology

6LoWPAN-ND Node Roles
----------------------

6LoWPAN-ND defines the following node roles:

- **6LBR (SixLowPanBorderRouter)**: Acts as the gateway between the 6LoWPAN network and the wider Internet. It maintains the Globally Unique Address registration cache for all nodes in the LLN, and Link-local addresses for nodes that are within 1 hop to the 6LBR.
- **6LR (SixLowPanRouter)**: Forwards packets between 6LNs and the 6LBR. It may also perform address registration for Link-local addresses for nodes within its network.
- **6LN (SixLowPanNodeOnly)**: A device that participates in the 6LoWPAN network, capable of sending and receiving IPv6 packets.

An additional node role, SixLowPanNode, is used for nodes that start out as 6LNs, but can be upgraded to 6LR later in the simulation. It is used in Route-Over topologies, where a 6LN can become a 6LR to forward packets for other 6LNs as part of multi-hop DAD.

Usage
-----

As the name implies, 6LoWPAN can handle only IPv6 packets. Any other protocol will be discarded.

6LoWPAN can be used alongside other L3 protocols in networks supporting an EtherType (e.g.,
Ethernet, WiFi, etc.). If the network does not have an EtherType in the frame header
(like in the case of 802.15.4), then the network must be uniform, as is all the devices
connected by the same same channel must use 6LoWPAN.

The reason is simple: if the L2 frame doesn't have a "EtherType" field, then there is no
demultiplexing at MAC layer and the protocol carried by L2 frames must be known
in advance.

.. note::
    By default, 6LoWPAN will use an EtherType equal to 0xA0ED, as mandated by :rfc:`7973`. If the device does not support EtherTypes (e.g., 802.15.4), this value is discarded.

IPHC stateful (context-based) compression is supported. Contexts map a short numeric ID to an IPv6 prefix, allowing addresses that share that prefix to be compressed more aggressively.

**Context configuration is only required when using IPHC stateful (context-based) compression.**
When no contexts are configured, nodes automatically fall back to stateless IPHC compression and no context setup is needed. If contexts are configured, all nodes in the network must share an identical set of contexts, or decompression will fail.

When stateful compression is used, there are two ways to distribute contexts to nodes:

**Automatic Context Distribution (Recommended)**

If 6LoWPAN-ND is enabled, the 6LBR automatically advertises contexts to 6LNs via the 6LoWPAN Context Option (6CO) carried in Router Advertisements. No manual context configuration is required on the 6LNs::

    // Configure 6LBR to advertise prefix and context automatically
    SixLowPanHelper sixlowpan;
    sixlowpan.InstallSixLowPanNdBorderRouter(borderRouterDevice, "2001::");
    sixlowpan.SetAdvertisedPrefix(borderRouterDevice, Ipv6Prefix("2001::", 64));
    sixlowpan.AddAdvertisedContext(borderRouterDevice, Ipv6Prefix("2001::", 64));

    // 6LNs learn prefix and contexts automatically from Router Advertisements
    sixlowpan.InstallSixLowPanNdNode(nodeDevice);

**Manual Context Configuration**

If 6LoWPAN-ND is not used (i.e., standard IPv6 Neighbor Discovery is used instead), contexts can be configured manually and identically on every node in the network::

    // Manual context setup - must be identical on all nodes
    sixlowpan.AddContext(allDevices, 0, Ipv6Prefix("2001::", 64), Time(Minutes(60)));

Note that when using manual context configuration, all nodes in the network must have
identical context configurations to avoid decompression failures

Helpers
~~~~~~~

The 6lowPAN helpers are patterned after other device helpers.

The main helper is ``SixLowPanHelper``, which provides methods to install 6LoWPAN on a NetDevice and configure the attributes of the 6LoWPAN protocol. The typical use case is over ``LrWpanNetDevice``, as shown below (taken from ``src/sixlowpan/examples/example-sixlowpan-nd-basic.cc``)::

        NodeContainer nodes;
        nodes.Create(1 + numLn); // node 0 = 6LBR, nodes 1-n = 6LNs

        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);

        LrWpanHelper lrWpanHelper;
        NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
        lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        SixLowPanHelper sixlowpan;
        NetDeviceContainer sixlowpanNetDevices = sixlowpan.Install(lrwpanDevices);

.. note::
    6LoWPAN can be used on any ``NetDevice``, not just ``LrWpanNetDevice``. For an example using ``CsmaNetDevice``, see ``src/sixlowpan/examples/example-sixlowpan.cc``.

Users have two options for neighbor discovery in their 6LoWPAN networks:

1. **Use standard IPv6 Neighbor Discovery**: If 6LoWPAN-ND is not installed, the simulation will use the standard IPv6 Neighbor Discovery protocol (:rfc:`4861`) with manual IPv6 address assignment. This approach works but lacks the optimizations for low-power networks.

2. **Use 6LoWPAN-ND (Recommended for realistic scenarios)**: Installing 6LoWPAN-ND provides optimized neighbor discovery with automatic address configuration, energy-efficient unicast communication, and context distribution for header compression.

To install 6LoWPAN-ND, two helper methods are used:

- ``SixLowPanHelper::InstallSixLowPanNdBorderRouter``: called on the **single** net device that will act as the 6LoWPAN Border Router (6LBR). Only one 6LBR is supported per network.
- ``SixLowPanHelper::InstallSixLowPanNdNode``: called on each net device that will act as a 6LoWPAN Node (6LN). This must **not** be called on the 6LBR device.

The helper also provides methods to set the advertised prefix and other attributes related to 6LoWPAN-ND.::

        // Configure 6LoWPAN ND
        // Node 0 = 6LBR, Node 1 = 6LN
        sixlowpan.InstallSixLowPanNdBorderRouter(sixlowpanNetDevices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(sixlowpanNetDevices.Get(0), Ipv6Prefix("2001::", 64));

        sixlowpan.InstallSixLowPanNdNode(sixlowpanNetDevices.Get(1));

Attributes
~~~~~~~~~~

The 6lowPAN provide some attributes:

* ``CompressionType``: (enum, default ``IPHC``), selects the header compression scheme: ``HC1`` (RFC4944) or ``IPHC`` (RFC6282).
* ``OmitUdpChecksum``: (boolean, default true), Omit the UDP checksum in IPHC compression.
* ``FragmentReassemblyListSize``: (integer, default 0), indicating the number of packets that can be reassembled at the same time. If the limit is reached, the oldest packet is discarded. Zero means infinite.
* ``FragmentExpirationTimeout``: (Time, default 60 seconds), being the timeout to wait for further fragments before discarding a partial packet.
* ``CompressionThreshold``: (unsigned 32 bits integer, default 0), minimum compressed payload size.
* ``UseMeshUnder``: (boolean, default false), it enables mesh-under flood routing.
* ``MeshUnderRadius``: (unsigned 8 bits integer, default 10), the maximum number of hops that a packet will be forwarded.
* ``MeshCacheLength``: (unsigned 16 bits integer, default 10), the length of the cache for each source.
* ``MeshUnderJitter``: (ns3::UniformRandomVariable[Min=0.0|Max=10.0]), the jitter in ms a node uses to forward mesh-under packets - used to prevent collisions.

The CompressionThreshold attribute is similar to Contiki's SICSLOWPAN_CONF_MIN_MAC_PAYLOAD
option. If a compressed packet size is less than the threshold, the uncompressed version is
used (plus one byte for the correct dispatch header).
This option is useful when a MAC requires a minimum frame size (e.g., ContikiMAC) and the
compression would violate the requirement.

The following is a list of attributes connected to the 6lowPAN-ND implementation:

* ``AddressRegistrationJitter``: The amount of jitter (in milliseconds) applied before sending an Address Registration. This jitter, sampled uniformly between 0 and the maximum value, helps avoid packet collisions among nodes registering simultaneously.
* ``RegistrationLifetime``: Specifies the lifetime of a registered address in the neighbor cache of a router. The value is in units of 60 seconds (i.e., a value of 65535 represents the maximum of ~45 days).
* ``AdvanceTime``: Time (in seconds) before expiration that the protocol proactively maintains or refreshes Router Advertisement and registration state. Useful for avoiding expiry during active operation.
* ``DefaultRouterLifetime``: Lifetime assigned to a default router entry. After this period, the router will no longer be considered valid unless refreshed. Default is 60 minutes.
* ``DefaultPrefixInformationPreferredLifetime``: Preferred lifetime for prefix information sent in Router Advertisements. Affects address autoconfiguration preferences. Default is 10 minutes.
* ``DefaultPrefixInformationValidLifetime``: Valid lifetime for prefixes advertised. Beyond this period, the prefix is no longer valid for use in address formation. Default is 10 minutes.
* ``DefaultContextValidLifetime``: Lifetime of 6LoWPAN Context Information Options (CIOs). Determines how long compression context information remains valid. Default is 10 minutes.
* ``DefaultAbroValidLifetime``: Lifetime of Authoritative Border Router Option (ABRO) information. Default is 10 minutes. This is relevant in multihop 6LoWPAN ND deployments with multiple border routers.
* ``MaxRtrSolicitationInterval``: The maximum interval between sending Router Solicitations when retrying with backoff. This controls how long a node waits between RS attempts. Default is 60 seconds.

Traces
~~~~~~

The supported 6LowPAN trace sources are:

* ``Tx``:  Exposing packet (including 6LoWPAN header), SixLoWPanNetDevice Ptr, interface index.
* ``Rx``:  Exposing packet (including 6LoWPAN header), SixLoWPanNetDevice Ptr, interface index.
* ``Drop``: Exposing DropReason, packet (including 6LoWPAN header), SixLoWPanNetDevice Ptr, interface index.

The Tx and Rx traces are called as soon as a packet is received or sent. The Drop trace is
invoked when a packet (or a fragment) is discarded.

The supported 6LowPAN-ND trace sources are:

* ``AddressRegistrationResult``:  Exposing the IPv6 address associated with the registration, and a boolean indicating whether it was successful or not.
* ``MulticastRS``:  Exposing the IPv6 address associated with the multicast Router Solicitation that was sent.

The AddressRegistrationResult trace is called when a 6LoWPAN node successfully registers its address with a 6LR or 6LBR, or when the registration fails, which is a core function of the 6LoWPAN-ND protocol.
The MulticastRS trace is called when a multicast Router Solicitation is sent by a 6LoWPAN node. This is useful for monitoring the behaviour of nodes on startup as they attempt to discover routers in the network.

Examples and Tests
-------------------

Example can be found in ``src/sixlowpan/examples/`` and test under ``src/sixlowpan/test/``.

The following shows 6lowPAN usage under |ns3|:

* ``example-sixlowpan.cc``:  A simple example showing end-to-end data transfer, with a CSMA network forced to carry 6LoWPAN compressed packets.

The following examples have been created to show the 6lowPAN-ND usage:

* ``example-sixlowpan-nd-basic.cc``: An example showing how to instantiate the 6LBR and 6LN in 6lowpan-nd with the helper methods in a typical 6lowpan network, enabling features like automatic address configuration.

The following tests have been created to verify the correct behavior of 6lowPAN-ND:

* ``sixlowpan-nd-packet-test.cc``: Contains unit tests that verify helper methods for parsing and validating 6LoWPAN-ND packets, including NS(EARO) and NA(EARO) formats.
* ``sixlowpan-nd-reg-test.cc``: Verifies basic address registration flows between 1-20 6LNs and a 6LBR, including successful and failed registration scenarios, as well as multicast RS binary backoff behaviour.
* ``sixlowpan-nd-rovr-test.cc``: Runs a scenario where an address registration for an already registered address is made, under a different ROVR, to test that the 6LBR behaviour, and NA (EARO) response is correct.


Validation
----------

The 6LoWPAN and 6LoWPAN-ND models have been verified against Wireshark, checking that packets are correctly interpreted and verified.

**6LoWPAN Header Compression Validation**

The 6LoWPAN header compression implementation has been verified by comparing the compressed packets generated by ns-3 with reference implementations and ensuring proper decompression by standard tools.

**6LoWPAN-ND Protocol Validation**

The 6LoWPAN-ND implementation has been verified through analysis of PCAP traces generated in the examples and tests. The validation process includes:

* **Packet Format Verification**: PCAP traces from ``example-sixlowpan-nd-basic.cc`` are analyzed using Wireshark to verify that:

  - Router Solicitation (RS) messages contain proper 6LoWPAN-ND options
  - Router Advertisement (RA) messages include correct 6LoWPAN Context Options (6CO) and Authoritative Border Router Options (ABRO)
  - Neighbor Solicitation with Extended Address Registration Option (NS-EARO) messages are properly formatted
  - Neighbor Advertisement with Extended Address Registration Option (NA-EARO) responses contain correct status codes

* **Protocol Flow Validation**: The timing and sequence of 6LoWPAN-ND message exchanges are verified to ensure compliance with RFC 6775 and RFC 8505 specifications:

  - Initial Router Solicitation upon node startup
  - Address registration sequences using NS(EARO)/NA(EARO) exchanges
  - Context distribution via 6CO options in Router Advertisements

References
----------

[`1 <https://books.google.it/books?id=3Nm7ZCxscMQC>`_] Z. Shelby and C. Bormann, 6LoWPAN: The Wireless Embedded Internet. Wiley, 2011.

[`2 <https://datatracker.ietf.org/doc/html/rfc8505>`_] RFC 8505: Registration Extensions for 6LoWPAN Neighbor Discovery, E. Thubert, November 2018.

[`3 <https://datatracker.ietf.org/doc/html/rfc6775>`_] RFC 6775: Neighbor Discovery Optimization for IPv6 over Low-Power Wireless Personal Area Networks (6LoWPANs), Z. Shelby, S. Chakrabarti, E. Nordmark, C. Bormann, November 2012.
