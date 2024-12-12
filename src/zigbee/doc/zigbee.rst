.. include:: replace.txt
.. highlight:: cpp


Zigbee
======

This chapter describes the implementation of |ns3| models for Zigbee
protocol stack (Zigbee Pro) as specified by
the Connectivity Standards Alliance (CSA), Zigbee Specification Revision 22 1.0 (2017).

The model described in the present document, represents the uncertified simulated models of Zigbee (TM)
technology to be used in |ns3| for research and educational purposes. The objective is to provide
|ns3| with a non-IP dependent routing alternative to be used with the  |ns3| ``lr-wpan`` (IEEE 802.15.4-2011) module.
This module is presented here with no guarantees of any kind and it has no association whatsoever with the
CSA or Zigbee (TM) products.


The source code for the new module lives in the directory ``src/zigbee``.

.. _fig-ZigbeeStackArch:

.. figure:: figures/zigbeeStackArch.*

    Zigbee Stack Architecture in ns-3

The Zigbee stack implementation is designed to operate on top of an existing |ns3| NetDevice that incorporates at least one object derived from ``LrWpanMacBase`` (which is compliant with IEEE 802.15.4-2011 MAC standards).
This device should have been previously aggregated, such as in the case of ``LrWpanNetDevice``.

While other technologies like 6loWPAN can interact with the underlying MAC layer through general-purpose NetDevice interfaces, Zigbee has specific requirements that necessitate certain features from a lr-wpan MAC.
Consequently, the |ns3| Zigbee implementation directly accesses the aggregated ``LrWpanMacBase`` and interfaces with it accordingly.

The current scope of the project includes **only the NWK layer in the Zigbee stack**. However, the project can be later extended
to support higher layers like the Application Sublayer (APS) and the Zigbee Cluster Library (ZCL).


Scope and Limitations
---------------------

- MacInterfaceTable is not supported (Multiple interface support).
- Security handling is not supported.
- Source routing and Tree routing are not implemented.
- Zigbee Stack profile 0x01 (used for tree routing and distributed address assignment) is not supported.
- A few NIB attributes setting and NWK constants are not supported by the |ns3| attribute system.
- Traces are not implemented yet.
- Data broadcast do not support retries or passive acknowledgment.
- Data broadcast to low power routers is not supported as the underlying lr-wpan netdevice has no concept of energy consumption.
- The following Zigbee layers are not supported yet:
   - Zigbee Application Support Sub-Layer (APS)
   - Zigbee Cluster Library (ZCL)
   - Zigbee Device Object (ZDO)
   - Application Framework (AF)

The network layer (NWK)
-----------------------

Similar to the implementation of IEEE 802.15.4 (lr-wpan) layers, communication between layers is done via "primitives".
These primitives are implemented in |ns3| with a combination of functions and callbacks.

The following is a list of NWK primitives supported:

- NLME-NETWORK-DISCOVERY (Request, Confirm)
- NLME-ROUTE-DISCOVERY (Request, Confirm)
- NLME-NETWORK-FORMATION (Request, Confirm)
- NLME-JOIN (Request, Confirm, Indication)
- NLME-DIRECT-JOIN (Request, Confirm)
- NLME-START-ROUTER (Request, Confirm)
- NLDE-DATA (Request, Confirm, Indication)

For details on how to use these primitives, please consult the zigbee pro specification.

Typically, users do not have to interact with the zigbee network layer (NWK) directly. Most commonly, the NWK is used by the Application Support Sub-Layer (APS) instead.
In the current implementation of zigbee in |ns3| we only support the NWK, for this reason, users wishing to use the routing, network joining and scanning capabilities of zigbee
must interact directly with the NWK.

The following is a brief explanation of how users can interact with the NWK and what are the supported capabilities.

Network joining
~~~~~~~~~~~~~~~

In a Zigbee network, before devices attempt to exchange data with one another, they must first be organized and related to one another based on their specific roles.
There are three primary roles in a Zigbee network:

- **Coordinator (ZC):** This is the initiating device in the network, and there can only be one coordinator at a time.
- **Router (ZR):** This device is capable of relaying data and commands on behalf of other devices.
- **End Device (ZE):** These are devices with limited capabilities (for example, they cannot route messages) that serve as endpoints within the network.

Devices can join a Zigbee network as either routers or end devices.
They can do this through one of two methods: by using the MAC association process or by joining the network directly.


**Mac Association Join**



**Direct Join (a.k.a. Orphaning process)**

Direct Join is employed when the number of nodes in the network and their relationships are known in advance.
As the name suggests, this method involves registering devices directly with a coordinator or router.
Subsequently, these devices confirm their registration with the coordinator to receive a network address.

This process is simpler than the association method of joining a network, but it is less flexible since it requires manual intervention.
Below is a summary of the direct join process:

1. If a coordinator device or router is already operational, devices that can communicate with this coordinator must be manually registered using the primitive `NLME-DIRECT-JOIN.request`.
2. After this registration, the devices registered in step 1 will send a `NLME-JOIN.request` with the rejoin network parameter set to `DIRECT_OR_REJOIN`. This action triggers a MAC orphaning request message, which is used to confirm the device's existence with the coordinator.
3. The coordinator or router will respond to the orphaning message by providing an assigned short address.
4. The device accepts this short address and successfully joins the network.

Note: The process described above outlines the steps for joining the network using a direct join method.
However, devices that are required to act as routers must also issue an additional `NLME-START-ROUTER.request` primitive after joining the network in order to begin functioning as routers.

In |ns3|, a direct join primitive is used as follows::

    CapabilityInformation capaInfo;
    capaInfo.SetDeviceType(zigbee::MacDeviceType::ROUTER); // The joining device capability is defined here
    capaInfo.SetAllocateAddrOn(true); // If false, an extended address mode will be used instead
    // zstack is an instance of a ZigbeeStack object installed in node that is the
    // coordinator or router where the joining device is being registered
    NlmeDirectJoinRequestParams directParams;
    directParams.m_capabilityInfo = capaInfo.GetCapability();
    directParams.m_deviceAddr = Mac64Address("00:00:00:00:00:00:00:01"); // The device IEEE address (Ext address)
    zstack->GetNwk()->NlmeDirectJoinRequest(directParams);

The device joining the network must issue a primitive similar to this one::

    // zstack is an instance of a ZigbeeStack object installed in the node joining the network
    // The orphaning message will be sent to every channel and interface specified.
    // The rejoin network parameter must be DIRECT_OR_REJOIN
    NlmeJoinRequestParams joinParams;
    joinParams.m_rejoinNetwork = zigbee::JoiningMethod::DIRECT_OR_REJOIN;
    joinParams.m_scanChannelList.channelPageCount = 1;
    joinParams.m_scanChannelList.channelsField[0] = zigbee::ALL_CHANNELS;
    joinParams.m_capabilityInfo = capaInfo.GetCapability();
    joinParams.m_extendedPanId = Mac64Address("00:00:00:00:00:00:CA:FE").ConvertToInt();
    zstack->GetNwk()->NlmeJoinRequest(joinParams);


See zigbee/examples for detailed examples using network joining.

Routing
~~~~~~~

Despite the common belief that the routing protocol contained in Zigbee is AODV, Zigbee only borrows a few basic ideas from this protocol.
In fact, Zigbee supports not one but 4 different methods of routing: Mesh routing, Many-To-One routing, Source routing, and Tree routing.
From these routing protocols, the current |ns3| implementation only supports the first two.

**Many-To-One Routing**

In Zigbee networks, it's common for multiple nodes to need to communicate with a single node, often referred to as a concentrator.
If all nodes perform a Mesh route discovery for this single point in the network, it can lead to an overwhelming number of route request messages, which may degrade network performance.
Under these circumstances, Many-to-One routing is typically more efficient than Mesh routing.

In Many-to-One routing, one route discovery operation is used to establish a route from all devices to the single concentrator (or collector) node.
The general procedure for Many-to-One routing is as follows:

1. The concentrator node broadcast a Many-to-One route request (RREQ) with the concentrator node address set as the target.
2. Devices receiving this broadcast store a reverse route into their routing table pointing to the path towards the concentrator.
3. If we are receiving the RREQ for the first time or the accumulated pathcost is better to the route stored in the routing table, the RREQ is re-broadcasted.

.. _fig-manyToOne:

.. figure:: figures/manyToOne.*

    Zigbee Many-To-One routing

There can be multiple concentrator nodes in the network and it is possible to run Many-To-One routing along Mesh routing.
In |ns3|, Many-To-One routing is achieved by using the ``NLME-ROUTE-DISCOVERY.request`` primitive::

   // zstack is an instance of a ZigbeeStack object installed in node that will
   // become the concentrator node.
   // The destination address mode must be set to NO_ADDRESS to automatically
   // trigger a Many-To-One route discovery.

   NlmeRouteDiscoveryRequestParams routeDiscParams;
   routeDiscParams.m_dstAddrMode = NO_ADDRESS;
   zstack->GetNwk()->NlmeRouteDiscoveryRequest(routeDiscParams);

Important: The process described above assumes that devices have already joined the network.
A route discovery request issued before a device is part of the network (join process) will result in failure.

**Mesh Routing**

Mesh routing in Zigbee is often attributed to the mechanisms used by the AODV routing protocol (`RFC 3561 <https://datatracker.ietf.org/doc/html/rfc3561>`_).
Although Zigbee mesh routing and the AODV protocol share some similarities, there are significant differences between them that directly influence performance.

Firstly, AODV was designed for IP-based networks, whereas Zigbee operates without the concept of IP addresses, thus eliminating their associated overhead.
Additionally, unlike AODV, Zigbee employs several distinct tables (such as the RREQ table, routing table, neighbor table, and discovery table) to manage route discovery and storage.
The use of these tables allows Zigbee to implement various storage and update policies based on the type of information they hold.
For instance, entries for neighboring nodes are typically updated frequently and have a short lifespan.
In contrast, entries in routing tables are usually long-lived or do not expire but may be replaced by new entries when space is limited.
The RREQ and discovery tables, on the other hand, are used exclusively during the route discovery process to optimize this process and facilitate the early detection of errors and loops.

In addition to these differences, Zigbee incorporates detailed policies, such as packet retransmissions, which come with defined default values to enhance consistency across various Zigbee implementations.
For more information on the implementation details and policies that govern Zigbee, please refer to the Zigbee specification.

The primary goal of mesh routing is to establish the shortest path to a specific destination within the network.
While hop count is the most commonly used metric for calculating path cost, Zigbee can also utilize the link quality indicator (LQI) to determine the shortest route to the destination (the default option).
The general procedure for mesh routing is as follows:

1. The source node broadcasts a route request (RREQ) to its neighboring nodes, specifying the desired destination.
2. Upon receiving the RREQ, the device calculates the link cost and adds it to the path cost already present in the RREQ.
3. The device then searches for a valid route to the destination, first in its neighbor table, followed by its discovery table and routing table.
4. If the destination is found, the device issues a route reply (RREP). If not, the RREQ is re-broadcasted only if it is the first time that a RREQ for that destination is sent from this location, or if a better path cost has been obtained.
5. When the RREQ reaches the destination or a neighboring node that knows the destination, an RREP is sent back to the source, using the information contained in the discovery table.
6. If there is no existing entry in the routing table, a new entry is created with the details of the most optimal path.

.. _fig-mesh:

.. figure:: figures/mesh.*

    Zigbee Mesh routing

Routing table entries are created only when an RREP is received, and only for the destination specified in the route discovery request.
A separate route discovery request must be issued for each destination that needs to be recorded in the source device.

In |ns3|, Mesh routing is achieved by using the ``NLME-ROUTE-DISCOVERY.request`` primitive::

   // zstack is an instance of a ZigbeeStack object installed in node that is the
   // source of the route discovery request.
   // Only a destination address parameter is necessary to be specified.
   // Parameters destination address mode = UCST_BCST and radius = 0 are the default values and
   // do not need be explicitly written.
   // Note: As specified by Zigbee, when a radius is set to 0, the radius used is equal to
   // nwkMaxDepth(5) * 2.
   NlmeRouteDiscoveryRequestParams routeDiscParams;
   routeDiscParams.m_dstAddr = Mac16Address("BE:EF");
   routeDiscParams.m_dstAddrMode = UCST_BCST;
   routeDiscParams.m_radius = 0;
   zstack->GetNwk()->NlmeRouteDiscoveryRequest(routeDiscParams);

Alternatively, a Mesh route discovery can be performed along a data transmission request by using the
``NLDE-DATA.request`` primitive with the ``ENABLE_ROUTE_DISCOVERY`` option::

    // zstack is an instance of a ZigbeeStack object installed in node that is the
    // source of the data transmission request.
    // The data transmission can be sent with either the ENABLE_ROUTE_DISCOVERY option (default) or the
    // SUPPRESS_ROUTE_DISCOVERY option.
    NldeDataRequestParams dataReqParams;
    Ptr<Packet> p = Create<Packet>(5);
    dataReqParams.m_dstAddrMode = UCST_BCST;
    dataReqParams.m_dstAddr = Mac16Address("BE:EF");
    dataReqParams.m_discoverRoute = ENABLE_ROUTE_DISCOVERY;
    zstack->GetNwk()->NldeDataRequest(dataReqParams, p);

Important: The process described above assumes that devices have already joined the network.
A route discovery request issued before a device is part of the network (join process) will result in failure.

Usage
-----

Helpers
~~~~~~~

The model includes a ``ZigbeeHelper`` used to quickly configure and install the NWK layer
on top of an existing Lr-wpan MAC layer. In essence, ``ZigbeeHelper`` creates a  ``ZigbeeNwk`` object
wraps it in a ``ZigbeeStack`` and connects this stack to another existing
``LrwpanNetDevice``. All the necessary callback hooks are taken care of to ensure communication between
the Zigbee NWK layer and the Lr-wpan MAC layer is possible.

Attributes
~~~~~~~~~~

It is possible to adjust the behavior of the Zigbee module by modifying the values of the available attributes.
Some of these attributes are constants and NIB (Network Information Base) attributes as defined in the Zigbee network layer (NWK) specification.

For easy reference, the attributes are named the same as those found in the specification.
Constants are values that remain unchanged during the execution of the protocol but can be specified in the configuration files of actual Zigbee devices.
In ns-3, both Zigbee constants and NIB attributes are represented as simple attributes.

Below is a list of the currently supported attributes:

- ``NwkcCoordinatorCapable``: [Constant] Indicates whether the device is capable of becoming a Zigbee coordinator.
- ``NwkcProtocolVersion``: [Constant] The version of the Zigbee NWK protocol in the device (placeholder).
- ``NwkcRouteDiscoveryTime``: [Constant] The duration until a route discovery expires.
- ``NwkcInitialRREQRetries``: [Constant] The number of times the first broadcast transmission of a RREQ command frame is retried.
- ``NwkcRREQRetries``: [Constant] The number of times the broadcast transmission of a RREQ command frame is retried on relay by intermediate router or coordinator.
- ``NwkcRREQRetryInterval``: [Constant] The duration between retries of a broadcast RREQ command frame.
- ``NwkcMinRREQJitter``: [Constant] The minimum jitter for broadcast retransmission of a RREQ (msec).
- ``NwkcMaxRREQJitter``: [Constant] The duration between retries of a broadcast RREQ (msec).
- ``MaxPendingTxQueueSize``: The maximum size of the table storing pending packets awaiting to be transmitted after discovering a route to the destination.



Traces
~~~~~~

The following trace sources have been implemented:

- ``RreqRetriesExhausted``: Trace source indicating when a node has reached the maximum allowed number of RREQ retries during a oute discovery request.

Examples and Tests
------------------

The prent capabilities of the Zigbee module implements only the NWK layer. Generally, Zigbee users are not required (or in some situations even allowed) to access the NWK layer API directly.
This is because the NWK API is essentially created to provide services to higher Zigbee layers instead (typically the Zigbee APS layer). Nevertheless, direct usage is possible and described by the Zigbee specification.
The following examples were created to highlight how a Zigbee NWK layer should be used to achieve network joining, discovery, routing, and data transmission capabilities.

All the examples listed here shows scenarios in which a quasi-layer implementation is used to handle incoming or outcoming events from the NWK layer.

* ``zigbee-direct-join.cc``:  An example showing the NWK layer join process of devices using the orphaning procedure (Direct join).
* ``zigbee-association-join.cc``:  An example showing the NWK layer join process of 3 devices in a zigbee network (MAC association).
* ``zigbee-nwk-routing.cc``: Shows a simple topology of 5 router devices sequentially joining a network. Data transmission and/or route discovery also shown in this example
* ``zigbee-nwk-routing2.cc``: Shows a complex grid topology of 50 router devices sequentially joining a network. Data transmission and/or route discovery also shown in this example.

The following unit test have been developed to ensure the correct behavior of the module:

* ``zigbee-rreq-test``: Test some situations in which RREQ messages should be retried during a route discovery process.


Validation
----------

No formal validation has been done.


References
----------

[`1 <https://csa-iot.org/developer-resource/specifications-download-request/>`_] Zigbee Pro Specification 2017 (R22 1.0)

[`2 <https://dsr-iot.com/downloads/open-source-zigbee/>`_] DSR Z-boss stack 1.0

[`3 <https://www.digi.com/resources/documentation/Digidocs/90002002/Default.htm#Containers/cont_xbee_pro_zigbee_networks.htm?TocPath=Zigbee%2520networks%257C_____0>`_] XBee/XBee-PRO® S2C Zigbee® RF Module User Guide

[4] Farahani, Shahin. ZigBee wireless networks and transceivers. Newnes, 2011.

[5] Gislason, Drew. Zigbee wireless networking. Newnes, 2008.
