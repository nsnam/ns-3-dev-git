Traffic Control
---------------

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

This layer has been designed to be between NetDevices (L2) and any network
protocol (e.g. IP). It is responsible to analyze packets and to perform actions
on them: the most common is scheduling, but many others are planned / implemented.

Brief description of the old node/device/protocol interactions
**************************************************************

The main question that we would like to answer in the following paragraphs is:
how a ns-3 node can send/receive packets?

If we analyze any example out there, the ability of the node to receive/transmit
packets derives from the interaction of two helper:

* L2 Helper (something derived from NetDevice)
* L3 Helper (usually from Internet module)

L2 Helper main operations
=========================

Any good L2 Helper will do the following operations:

* Create n netdevices (n>1)
* Attach a channel between these devices
* Call Node::AddDevice ()

Obviously the last point is the most important.

Node::AddDevice (network/model/node.cc:128) assigns an interface index to the
device, calls NetDevice::SetNode, sets the receive callback of the device to
Node::NonPromiscReceiveFromDevice. Then, it schedules NetDevice::Initialize() method at
Seconds(0.0), then notify the registered DeviceAdditionListener handlers (not used BY ANYONE).

Node::NonPromiscReceiveFromDevice calls Node::ReceiveFromDevice.

Node::ReceiveFromDevice iterates through ProtocolHandlers, which are callbacks
which accept as signature:

ProtocolHandler (Ptr<NetDevice>, Ptr<const Packet>, protocol, from_addr, to_addr, packetType).

If device, protocol number and promiscuous flag corresponds, the handler is
invoked.

Who is responsible to set ProtocolHandler ? We will analyze that in the next
section.

L3 Helper
=========

We have only internet which provides network protocol (IP). That module splits
the operations between two helpers: InternetStackHelper and Ipv{4,6}AddressHelper.

InternetStackHelper::Install (internet/helper/internet-stack-helper.cc:423)
creates and aggregates protocols {ArpL3,Ipv4L3,Icmpv4}Protocol. It creates the
routing protocol, and if Ipv6 is enabled it adds {Ipv6L3,Icmpv6L4}Protocol. In
any case, it instantiates and aggregates an UdpL4Protocol object, along with a
PacketSocketFactory.
Ultimately, it creates the required objects and aggregates them to the node.

Let's assume an Ipv4 environment (things are the same for Ipv6).

Ipv4AddressHelper::Assign (src/internet/helper/ipv4-address-helper.cc:131)
registers the handlers. The process is a bit long. The method is called with
a list of NetDevice. For each of them, the node and Ipv4L3Protocol pointers are
retrieved; if an Ipv4Interface is already registered for the device, on that the
address is set. Otherwise, the method Ipv4L3Protocol::AddInterface is called,
before adding the address.

IP interfaces
=============

In Ipv4L3Protocol::AddInterface (src/internet/model/ipv4-l3-protocol.cc:300)
two protocol handlers are installed: one that react to ipv4 protocol number,
and one that react to arp protocol number (Ipv4L3Protocol::Receive and
ArpL3Protocol::Receive, respectively). The interface is then created,
initialized, and returned.

Ipv4L3Protocol::Receive (src/internet/model/ipv4-l3-protocol.cc:472) iterates
through the interface. Once it finds the Ipv4Interface which has the same device
as the one passed as argument, invokes the rxTrace callback. If the interface is
down, the packet is dropped. Then, it removes the header and trim any residual
frame padding. If checksum is not OK, it drops the packet. Otherwise, forward
the packet to the raw sockets (not used). Then, it ask the routing protocol what
is the destiny of that packet. The choices are: Ipv4L3Protocol::{IpForward,
IpMulticastForward,LocalDeliver,RouteInputError}.

Introducing Traffic Control Layer
*********************************

The layer is responsible to take actions to schedule or filter the traffic for
both INPUT and OUTPUT directions (called also RX and TX). To interact
with the IP layer and the NetDevice layer, we edited in the following manner
the IPv{4,6}Interface and IPv{4,6}L3Protocol.

Transmitting packets
====================

The IPv{4,6} interfaces uses the aggregated object TrafficControlLayer to send
down packets, instead of calling NetDevice::Send() directly. After the analysis
and the process of the packet, when the backpressure mechanism allows it,
TrafficControlLayer will call the Send() method on the right NetDevice.

Receiving packets
=================

The callback chain that (in the past) involved IPv{4,6}L3Protocol and NetDevices,
through ReceiveCallback, is extended to involve TrafficControlLayer. When an
IPv{4,6}Interface is added in the IPv{4,6}L3Protocol, the callback chain is
configured to have the following packet exchange:

NetDevice --> Node --> TrafficControlLayer --> IPv{4,6}L3Protocol

Queue disciplines
--------------------------------------------------------------
Packets received by the Traffic Control layer for transmission to a netdevice
can be passed to a Queue Discipline to perform scheduling and policing.
A netdevice can have a single (root) queue disc aggregated to it.
Aggregating a queue disc to a netdevice is not mandatory. If a netdevice does
not have a queue disc aggregated to it, the traffic control layer sends the packets
directly to the netdevice. This is the case, for instance, of the loopback netdevice.

The traffic control layer interacts with a queue disc in a simple manner: after requesting
to enqueue a packet, the traffic control layer requests the qdisc to "run", i.e., to
dequeue a set of packets, until a predefined number ("quota") of packets is dequeued
or the netdevice stops the queue disc.

Multi-queue devices (such as Wifi) are explicitly taken into account. Each
netdevice has a vector of as many NetDeviceQueue objects as the number of
transmission queues.A NetDeviceQueue object stores information related to a
single transmission queue, such as the status (i.e., whether the queue has been
stopped or not) and (yet to come) data used by techniques such as Byte Queue Limits.
Additionally, the NetDeviceQueue class offers public methods to enquire about the
status of a transmission queue and to start, stop or wake a transmission queue.
To wake a transmission queue means requesting the associated queue disc (see
below for details) to run.

"Classic" queue discs (such as pfifo_fast, red, codel and many others) are not aware
of the number of transmission queues used by the netdevice. More recently,
multi-queue aware queue discs (such mq and mq-prio) have been mainlined into the
Linux kernel. Multi-queue aware queue discs create as many queues internally as
the number of transmission queues used by the netdevice. Additionally, they
enqueue packets into their internal queues based on a packet field (implemented
as an |ns3| tag) that can be set by calling a netdevice function. Therefore,
each packet is enqueued in the "same" queue both inside the queue disc and inside
the netdevice. Classic queue discs (such as fq-codel) are classful, i.e., they
manage internal queues whose packets are scheduled by means of other kinds of
queue discs. However, there is no close relationship between the internal queues
of the queue disc and the transmission queues of the netdevice.

When a root queue disc, be it a classic classful one or a multi-queue aware one,
receives a packet to enqueue, it selects an internal queue disc and requests it
to enqueue a packet. When a root queue disc is requested to dequeue a packet, it
selects an internal queue disc and requests it to dequeue a packet. A multi-queue
aware queue disc is requested to select an internal queue disc whose corresponding
device transmission queue is not stopped.

There are a couple of operations that need to be performed when setting up a queue
disc, which vary depending on whether we are setting up a root queue disc or a
child queue disc.

Setting up a root queue disc
*****************************

.. _fig-root-queue-disc:

.. figure:: figures/root-queue-disc.*

    Setup of a root queue disc


:ref:`fig-root-queue-disc` shows how to configure a root queue disc. Basically:

* A root queue disc needs to be aggregated to a netdevice object
* The m_devQueue member of the root queue disc must point to one of the device transmission queues (e.g., the first one)
* A wake callback must be set on all the device transmissio queues to point to the Run method of the root queue disc

Setting up child queue discs
*******************************

.. _fig-child-queue-discs:

.. figure:: figures/child-queue-discs.*

    Setup of child queue discs


:ref:`fig-child-queue-discs` shows how to configure queue discs that are children
of a multi-queue aware (root) queue disc. Basically:

* Each child queue disc needs to be aggregated to a NetDeviceQueue object
* The m_devQueue member of a child queue disc must point to the corresponding device transmission queue
* A wake callback must be set on each device transmissio queue to point to the Run method of the corresponding child queue disc

In case of queue discs that are children of a classic classful (root) queue disc,
none of the above operations need to be performed, as there is not a match between
the child queue discs and the device transmission queues.
