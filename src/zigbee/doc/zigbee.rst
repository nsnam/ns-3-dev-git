Zigbee
======

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ============= Chapter
   ------------- Section (#.#)
   ~~~~~~~~~~~~~ Subsection (#.#.#)

This chapter describes the implementation of |ns3| models for Zigbee
Pro stack (also known as Zigbee 3.x) as specified by
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

The Zigbee stack implementation is designed to operate on top of an existing |ns3| ``NetDevice`` that aggregates an object derived from ``LrWpanMacBase`` (which is compliant with IEEE 802.15.4-2011 MAC standards).
This is the case of ``NetDevices`` such as the ``LrWpanNetDevice``.

While other technologies like 6loWPAN can interact with the underlying MAC layer through general-purpose ``NetDevice`` interfaces, Zigbee has specific requirements that necessitate certain features from a lr-wpan MAC.
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
- Address duplication detection is not supported.
- Beacon mode is not througly tested.
- The following Zigbee layers are not supported yet:
   - Zigbee Application Support Sub-Layer (APS)
   - Zigbee Cluster Library (ZCL)
   - Zigbee Device Object (ZDO)
   - Application Framework (AF)

To see a list of |ns3| Zigbee undergoing development efforts check issue `#1165 <https://gitlab.com/nsnam/ns-3-dev/-/issues/1165>`_

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

For details on how to use these primitives, please consult the Zigbee Pro specification.

Typically, users do not have to interact with the Zigbee network layer (NWK) directly. Most commonly, the NWK is used by the Application Support Sub-Layer (APS) instead.
In the current implementation of Zigbee in |ns3| we only support the NWK, for this reason, users wishing to use the routing, network joining and scanning capabilities of Zigbee
must interact directly with the NWK.

The following is a brief explanation of how users can interact with the NWK and what are the supported capabilities.

Network joining
~~~~~~~~~~~~~~~

In a Zigbee network, before devices attempt to exchange data with one another, they must first be organized and related to one another based on their specific roles.
There are three primary roles in a Zigbee network:

- **Coordinator (ZC):** This is the initiating device in the network, and there can only be one coordinator at a time.
- **Router (ZR):** This device is capable of relaying data and commands on behalf of other devices.
- **End Device (ZE):** These are devices with limited capabilities (for example, they cannot route messages) that serve as endpoints within the network.

Devices must first join a Zigbee network before they can upgrade their roles to either routers or remain as end devices.
There are two ways for devices to join a Zigbee network: they can either go through the MAC association process or directly join the network.

**MAC Association Join**

This method is the default approach used by Zigbee. As the name suggests, it utilizes the underlying MAC layer association mechanism to connect to the Zigbee network.
For a comprehensive explanation of the association mechanism occurring in the MAC, please refer to the lr-wpan model documentation.

When employing the association mechanism, devices communicate with each other to join an existing network via the network coordinator or a designated router within the network.
As a result of this joining process, devices are assigned a short address (16-bit address) that they use for routing and data exchange operations. Below is a summary of the MAC association join process:

1. At least one coordinator must be operational and capable of receiving join requests (i.e. A device successfully completed a `NLME-NETWORK-FORMATION.request` primitive).
2. Devices must issue a `NLME-NETWORK-DISCOVERY.request` to look for candidate coordinators/routers to join. The parameters channel numbers (represented by a bitmap for channels 11-26) and scan duration (ranging from 0 to 14) must also be specified to define the channel in which the device will search for a coordinator or router and the duration of that search.
3. If a coordinator (or a capable router) is found on the designated channel and within communication range, it will respond to the device's request with a beacon that contains both a PAN descriptor and a beacon payload describing the capabilities of the coordinator or router.
4. Upon receiving these beacons, the device will select "the most ideal coordinator or router candidate" to join.
5. After selecting the candidate, devices must issue a `NLME-JOIN.request` primitive with the network parameter set to `ASSOCIATION`. This will initiate the join process.
6. If the association is successful, the device will receive a confirmation from the coordinator or router, along with a short address that the device will use for its operations within the Zigbee network.
7. Short addresses are assigned by the coordinator randomly, which means there could be instances of address duplication. Although the Zigbee specification includes a mechanism for detecting address duplication, this feature is not currently supported in this implementation.
8. If the association request fails, the device will receive a confirmation with a status indicating failure, rather than `SUCCESSFUL`, and the short address FF:FF will be received (indicating that the device is not associated).

Note: The process described above outlines the steps for joining the network using a MAC association join.
However, devices that are required to act as routers must also issue an additional `NLME-START-ROUTER.request` primitive after joining the network in order to begin functioning as routers.

In |ns3|, Zigbee NWK, coordinators or routers can be found using the following primitive::

    // zstack is an instance of a ZigbeeStack object installed in the node looking
    // for coordinator or routers
    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1; // only one page structure is supported
    netDiscParams.m_scanChannelList.channelsField[0] = 0x7800; // Bitmap representing channels 11-14
    netDiscParams.m_scanDuration = 14; // (Ranging 0-14) See lr-wpan documentation for time equivalency
    zstack->GetNwk()->NlmeNetworkDiscoveryRequest(netDiscParams);

In |ns3| a Zigbee NWK join request (using MAC association) is used as follows::

        // zstack is an instance of a ZigbeeStack object installed in the node sending
        // the join request. The extendedPANId value is typically obtained from the network descriptor
        // received from a previous network discovery request step.
        zigbee::CapabilityInformation capaInfo; // define the capabilities and
        capaInfo.SetDeviceType(ROUTER);         // requirements of the current device
        capaInfo.SetAllocateAddrOn(true);

        NlmeJoinRequestParams joinParams;
        joinParams.m_rejoinNetwork = zigbee::JoiningMethod::ASSOCIATION; // Must be set to ASSOCIATION
        joinParams.m_capabilityInfo = capaInfo.GetCapability(); // Set the capabilities (bitmap)
        joinParams.m_extendedPanId = 0xCAFE0000BEEF0000; // The 64 bits representing the extended PAN ID
        zstack->GetNwk()->NlmeJoinRequest(joinParams);

See zigbee/examples for detailed examples using network joining.

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

Similar to the MAC association mechanism, devices that want to function as routers must issue an additional `NLME-START-ROUTER.request` once the joining process is completed.

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
    joinParams.m_scanChannelList.channelsField[0] = zigbee::ALL_CHANNELS; // Bitmap representing Ch. 11~26
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

.. note::
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

.. note::
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
The following demonstrates a simple scenario in which a ``LrwpanNetDevice`` devices are set and a ``ZigbeeHelper``
is used to establish a Zigbee stack on top of these devices::

    NodeContainer nodes;
    nodes.Create(2);

    // Create the Lr-wpan (IEEE 802.15.4) Netdevices and install them in each node
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

    // Important: NetDevices MAC extended addresses (EUI-64) must always be set
    // either manually or using other methods. Short address assignation is not
    // required as this is later done by the Zigbee join mechanisms.
    Ptr<LrWpanNetDevice> dev0 = lrwpanDevices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = lrwpanDevices.Get(1)->GetObject<LrWpanNetDevice>();
    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
    dev1->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:01");

    // Install a zigbee stack on a set of devices using the helper
    ZigbeeHelper zigbee;
    ZigbeeStackContainer zigbeeStackContainer = zigbee.Install(lrwpanDevices);

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

- ``RreqRetriesExhausted``: Trace source indicating when a node has reached the maximum allowed number of RREQ retries during a route discovery request.

Examples and Tests
------------------

The prent capabilities of the Zigbee module implements only the NWK layer. Generally, Zigbee users are not required (or in some situations even allowed) to access the NWK layer API directly.
This is because the NWK API is essentially created to provide services to higher Zigbee layers instead (typically the Zigbee APS layer). Nevertheless, direct usage is possible and described by the Zigbee specification.
The following examples were created to highlight how a Zigbee NWK layer should be used to achieve network joining, discovery, routing, and data transmission capabilities.

All the examples listed here shows scenarios in which a quasi-layer implementation is used to handle incoming or outcoming events from the NWK layer.

* ``zigbee-direct-join.cc``:  An example showing the NWK layer join process of devices using the orphaning procedure (Direct join).
* ``zigbee-association-join.cc``:  An example showing the NWK layer join process of 3 devices in a zigbee network (MAC association).
* ``zigbee-nwk-routing.cc``: Shows a simple topology of 5 router devices sequentially joining a network. Data transmission and route discovery (MESH routing) are also shown in this example
* ``zigbee-nwk-routing-grid.cc``: Shows a complex grid topology of 50 router devices sequentially joining a network. Route discovery (MANY-TO-ONE routing) is also shown in this example.

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
