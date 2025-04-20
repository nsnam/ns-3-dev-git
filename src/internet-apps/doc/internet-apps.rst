Internet Applications Module Documentation
------------------------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

The goal of this module is to hold all the Internet-specific applications,
and most notably some very specific applications (e.g., ping) or daemons (e.g., radvd).  Other non-Internet-specific applications such as packet generators
are contained in other modules.

The source code for the new module lives in the directory ``src/internet-apps``.

Each application has its own goals, limitations and scope, which are briefly explained
in the following.

All the applications are extensively used in the top-level ``examples``
directories. The users are encouraged to check the scripts therein to have a
clear overview of the various options and usage tricks.


Ping
****

The ``Ping`` application supports both IPv4 and IPv6 and replaces earlier
|ns3| implementations called ``v4Ping`` and ``Ping6`` that were
address family dependent.  ``Ping`` was introduced in the ns-3.38 release cycle.

Model Description
=================

This application behaves similarly to the Unix ``ping`` application, although
with fewer options supported.  ``Ping`` sends ICMP Echo Request messages to
a remote address, and collects statistics and reports on the ICMP Echo
Reply responses that are received.  The application can be used to send
ICMP echo requests to unicast, broadcast, and multicast IPv4 and IPv6
addresses.  The application can produce a verbose output similar to the real
application, and can also export statistics and results via trace sources.
The following can be controlled via attributes of this class:

* Destination address
* Local address (sender address)
* Packet size (default 56 bytes)
* Packet interval  (default 1 second)
* Timeout value (default 1 second)
* The count, or maximum number of packets to send
* Verbose mode

In practice, the real-world ``ping`` application behavior varies slightly
depending on the operating system (Linux, macOS, Windows, etc.).  Most
implementations also support a very large number of options.  The |ns3|
model is intended to handle the most common use cases of testing for
reachability.

Design
######

The aim of |ns3| ``Ping`` application is to mimic the built-in application
found in most operating systems.  In practice, ``ping`` is usually used to
check reachability of a destination, but additional options have been
added over time and the tool can be used in different ways to gather
statistics about reachability and round trip times (RTT).  Since |ns3| is
mainly used for performance studies and not for operational forensics,
some options of real ``ping`` implementations may not be useful for simulations.
However, the |ns3| application can deliver output and RTT samples similar
to how the real application operates.

``Ping`` is usually installed on a source node and does not require any
|ns3| application installation on the destination node.  ``Ping`` is an
``Application`` that can be started and stopped using the base class
Application APIs.

Behavior
########

The behavior of real ``ping`` applications varies across operating systems.  For
example, on Linux, the first ICMP sequence number sent is one, while on
macOS, the first sequence number is zero.  The behavior when pinging
non-existent hosts also can differ (Linux is quiet while macOS is verbose).
Windows and other operating systems like Cisco routers also can behave
slightly differently.

This implementation tries to generally follow the Linux behavior, except
that it will print out a verbose 'request timed out' message when an
echo request is sent and no reply arrives in a timely manner.  The timeout
value (time that ping waits for a response to return) defaults to one
second, but once there are RTT samples available, the timeout is set
to twice the observed RTT.  In contrast to Linux (but aligned with macOS),
the first sequence number sent is zero.

Scope and Limitations
#####################

``ping`` implementations have a lot of command-line options.  The |ns3|
implementation only supports a few of the most commonly-used options;
patches to add additional options would be welcome.

At the present time, fragmentation (sending an ICMP Echo Request larger
than the path MTU) is not handled correctly during Echo Response reassembly.

Usage
=====

Users may create and install ``Ping`` applications on nodes on a one-by-one
basis using ``CreateObject`` or by using the ``PingHelper``.  For
``CreateObject``, the following can be used::

  Ptr<Node> n = ...;
  Ptr<Ping> ping = CreateObject<Ping> ();
  // Configure ping as needed...
  n->AddApplication (ping);

Users should be aware of how this application stops.  For most |ns3|
applications, ``StopApplication()`` should be called before the simulation
is stopped.  If the ``Count`` attribute of this application is set to
a positive integer, the application will stop (and a report will be printed)
either when ``Count`` responses have been received or when ``StopApplication()``
is called, whichever comes first.  If ``Count`` is zero, meaning infinite
pings, then ``StopApplication()`` should be used to eventually stop the
application and generate the report.  If ``StopApplication()`` is called
while a packet (echo request) is in-flight, the response cannot be
received and the packet will be treated as lost in the report-- real
ping applications work this way as well.  To avoid this, it is recommended
to call ``StopApplication()`` at a time when an Echo Request or Echo Response
packet is not expected to be in flight.

Helpers
#######

The ``PingHelper`` supports the typical ``Install`` usage pattern in |ns3|.
The following sample code is from the program ``examples/tcp/tcp-validation.cc``.

.. sourcecode:: cpp

    PingHelper pingHelper(Ipv4Address("192.168.1.2"));
    pingHelper.SetAttribute("Interval", TimeValue(pingInterval));
    pingHelper.SetAttribute("Size", UintegerValue(pingSize));
    pingHelper.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::SILENT));
    ApplicationContainer pingContainer = pingHelper.Install(pingServer);
    Ptr<Ping> ping = pingContainer.Get(0)->GetObject<Ping>();
    ping->TraceConnectWithoutContext("Rtt", MakeBoundCallback(&TracePingRtt, &pingOfStream));
    pingContainer.Start(Seconds(1));
    pingContainer.Stop(stopTime - Seconds(1));

The first statement sets the remote address (destination) for all application
instances created with this helper.  The second and third statements perform
further configuration.  The fourth statement configures the verbosity to
be totally silent.  The fifth statement is a typical ``Install()``
method that returns an ApplicationContainer (in this case, of size 1).
The sixth and seventh statements fetch the application instance created and
configure a trace sink (``TracePingRtt``) for the ``Rtt`` trace source.
The eighth and ninth statements configure the start and stop time,
respectively.

The helper is most useful when there are many similarly configured
applications to install on a collection of nodes (a NodeContainer).
When there is only one Ping application to configure in a program,
or when the configuration between different instances is different,
it may be more straightforward to directly create the Ping applications
without the PingHelper.

Attributes
##########

The following attributes can be configured:

* ``Destination``: The IPv4 or IPv6 address of the machine we want to ping
* ``VerboseMode``: Configure verbose, quiet, or silent output
* ``Interval``: Time interval between sending each packet
* ``Size``: The number of data bytes to be sent, before ICMP and IP headers are added
* ``Count``: The maximum number of packets the application will send
* ``InterfaceAddress``: Local address of the sender
* ``Timeout``: Time to wait for response if no RTT samples are available

Output
######

If ``VerboseMode`` mode is set to ``VERBOSE``, ping will output the results of
ICMP Echo Reply responses to ``std::cout`` output stream.  If the mode is
set to ``QUIET``, only the initial statement and summary are printed.  If the
mode is set to ``SILENT``, no output will be printed to ``std::cout``.  These
behavioral differences can be seen with the ``ping-example.cc`` as follows:

.. sourcecode:: bash

  $ ./ns3 run --no-build 'ping-example --ns3::Ping::VerboseMode=Verbose'
  $ ./ns3 run --no-build 'ping-example --ns3::Ping::VerboseMode=Quiet'
  $ ./ns3 run --no-build 'ping-example --ns3::Ping::VerboseMode=Silent'

Additional output can be gathered by using the four trace sources provided
by Ping:

* ``Tx``: This trace executes when a new packet is sent, and returns the sequence number and full packet (including ICMP header).
* ``Rtt``:  Each time an ICMP echo reply is received, this trace is called and reports the sequence number and RTT.
* ``Drop``:  If an ICMP error is returned instead of an echo reply, the sequence number and reason for reported drop are returned.
* ``Report``: When ping completes and exits, it prints output statistics to the terminal.  These values are copied to a ``struct PingReport`` and returned in this trace source.

Example
#######

A basic ``ping-example.cc`` program is provided to highlight the following
usage.  The topology has three nodes interconnected by two point-to-point links.
Each link has 5 ms one-way delay, for a round-trip propagation delay
of 20 ms.  The transmission rate on each link is 100 Mbps.  The routing
between links is enabled by ns-3's NixVector routing.

By default, this program will send 5 pings from node A to node C.
When using the default IPv6, the output will look like this:

.. sourcecode::txt

  PING 2001:1:0:1:200:ff:fe00:4 - 56 bytes of data; 104 bytes including ICMP and IPv6 headers.
  64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=0 ttl=63 time=20.033 ms
  64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=1 ttl=63 time=20.033 ms
  64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=2 ttl=63 time=20.033 ms
  64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=3 ttl=63 time=20.033 ms
  64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=4 ttl=63 time=20.033 ms
  --- 2001:1:0:1:200:ff:fe00:4 ping statistics ---
  5 packets transmitted, 5 received, 0% packet loss, time 4020ms
  rtt min/avg/max/mdev = 20/20/20/0 ms

The example program will also produce four pcap traces (one for each
NetDevice in the scenario) that can be viewed using tcpdump or Wireshark.

Other program options include options to change the destination and
source addresses, number of packets (count), packet size, interval,
and whether to enable logging (if logging is enabled in the build).
These program options will override any corresponding attribute settings.

Finally, the program has some code that can be enabled to selectively
force packet drops to check such behavior.

Validation
==========

The following test cases have been added for regression testing:

#. Unlimited pings, no losses, StopApplication () with no packets in flight
#. Unlimited pings, no losses, StopApplication () with one packet in flight
#. Test for operation of count attribute and exit time after all pings are received, for IPv4"
#. Test the operation of interval attribute, for IPv4
#. Test for behavior of pinging an unreachable host when the network does not send an ICMP unreachable message
#. Test pinging to IPv4 broadcast address and IPv6 all nodes multicast address
#. Test behavior of first reply lost in a count-limited configuration
#. Test behavior of second reply lost in a count-limited configuration
#. Test behavior of last reply lost in a count-limited configuration.

Radvd
*****

This app mimics a "RADVD" daemon. I.e., the daemon responsible for IPv6 routers
advertisements. All the IPv6 routers should have a RADVD daemon installed.

The configuration of the Radvd application mimics the one of the radvd Linux program.

DHCPv4
******

The |ns3| implementation of Dynamic Host Configuration Protocol (DHCP)
follows the specifications of :rfc:`2131` and :rfc:`2132`.

The source code for DHCP is located in ``src/internet-apps/model`` and consists of the
following 6 files:

* dhcp-server.h,
* dhcp-server.cc,
* dhcp-client.h,
* dhcp-client.cc,
* dhcp-header.h and
* dhcp-header.cc

Helpers
=======

The following two files have been added to ``src/internet-apps/helper`` for DHCP:

* dhcp-helper.h and
* dhcp-helper.cc

Tests
=====
The tests for DHCP can be found at ``src/internet-apps/test/dhcp-test.cc``

Examples
========
The examples for DHCP can be found at ``src/internet-apps/examples/dhcp-example.cc``


Scope and Limitations
=====================

The server should be provided with a network address, mask and a range of address
for the pool. One client application can be installed on only one netdevice in a
node, and can configure address for only that netdevice.

The following five basic DHCP messages are supported:

* DHCP DISCOVER
* DHCP OFFER
* DHCP REQUEST
* DHCP ACK
* DHCP NACK

Also, the following eight options of BootP are supported:

* 1 (Mask)
* 50 (Requested Address)
* 51 (Address Lease Time)
* 53 (DHCP message type)
* 54 (DHCP server identifier)
* 58 (Address renew time)
* 59 (Address rebind time)
* 255 (end)

The client identifier option (61) can be implemented in near future.

In the current implementation, a DHCP client can obtain IPv4 address dynamically
from the DHCP server, and can renew it within a lease time period.

Multiple DHCP servers can be configured, but the implementation does not support
the use of a DHCP Relay yet.

V4TraceRoute
************

Documentation is missing for this application.

DHCPv6
******

The |ns3| implementation of Dynamic Host Configuration Protocol for IPv6 (DHCPv6)
follows the specifications of :rfc:`8415`.

Model Description
=================

The aim of the DHCPv6 application is to provide a method for dynamically assigning IPv6 addresses to nodes.
This application functions similarly to the ``dhcpd`` daemon in Linux, although it supports fewer options.
The DHCPv6 client installed on a node sends Solicit messages to a server configured on the link,
which then responds with an IPv6 address offer.

The source code for DHCPv6 is located in ``src/internet-apps/model``.

There are two major operational models for DHCPv6: stateful and stateless.
Stateless DHCP is used when the client only needs to request configuration parameters from the server.

Typically, the DHCPv6 server is used to assign both addresses and other options like DNS information.
The server maintains information about the Identity Associations (IAs), which are collections of leases assigned to a client.

There are three types of IAs: IA_NA (non-temporary addresses), IA_TA (temporary addresses), and IA_PD (prefix delegation).
Temporary addresses were introduced in RFC 4941 to address privacy concerns. In DHCPv6, the IA_TA (Identity Association for Temporary Addresses)
is used for addresses that are only used for a short time, and their lifetimes are generally not extended. Non-temporary addresses, on the other hand,
are used for relatively longer durations and are usually renewed by the clients. Note that non-temporary addresses are not permanent leases - the term is
merely used to differentiate between the IA_TA and IA_NA types.

Currently, the application uses only IA_NA options to lease addresses to clients.
The state of the lease changes over time, which is why this is known as stateful DHCPv6.

The DHCPv6 client application is installed on each NetDevice (interface) of the client node.
Therefore, if there are two NetDevices installed on a single node, two client applications must be installed.
The DHCPv6 server application is installed on a single server node.
However, the user can specify multiple interfaces for the server to listen on.

Notes:

* Stateless DHCPv6 has not been implemented, as it is used to request DNS configuration information that is not currently used in |ns3|.
* All client and server nodes must be on the same link. The application does not support DHCPv6 relays and hence cannot work when the client and server are in entirely different subnets.

Stateful DHCPv6
###############

The server listens on port 547 for any incoming messages on the link.
Upon receiving a **Solicit** message from a client, the server sends an **Advertise** message with the available address pool.
The server is designed to advertise one available address from each address pool that is configured on it.
When the client sends a **Request** message, the server updates lease bindings with the new expiry time, and sends a **Reply** message.

The client attempts to accept all addresses present in the server's **Reply**.
However, before using the offered address, the client checks whether any other node on the link is using the same address (Duplicate Address Detection).
If the address is already in use, the client sends a **Decline** message to the server, which updates the lease bindings accordingly.

At the end of the preferred lifetime, the client sends a **Renew** message to the server.
The server should update the lease bindings with the new expiry time, and send a **Reply** with the corresponding status code.
In case the client does not receive a **Reply** from the server, it sends a **Rebind** message at the end of the valid lifetime.
The server is expected to update the lease bindings and send a **Reply** message back to the client.

The order of the message exchange between the client and the server is as follows:

.. figure:: figures/dhcpv6-message-exchange.*

The formats of the messages between a client and server are as follows:

.. figure:: figures/stateful-dhcpv6-format.*

Identifiers
###########

Each client and server has a unique identifier (DHCP Unique Identifier), which is used to identify the client and server in the message exchange.
There are four types of DUIDs that can be used in DHCPv6:

1. DUID-LLT (Link-layer Address plus Time): The DUID-LLT consists of a 32-bit timestamp and a variable-length link layer address. Not supported in this implementation.
2. Vendor-assigned unique ID based on Enterprise Number: This consists of the vendor's enterprise number and an identifier whose source is defined by the vendor. Not supported in this implementation.
3. **DUID-LL (Link-layer address)**: DUID-LL consists of a variable-length link-layer address as the identifier. Supported in this implementation.
4. UUID (Universally unique identifier): This should be an identifier that is persistent across system restarts and reconfigurations. Not supported in this implementation.

Usage
=====

The main way for |ns3| users to use the DHCPv6 application is through the helper
API and the publicly visible attributes of the client and server applications.

To use the DHCPv6 application, the user must install the client application on the node by passing a ``Ptr<Node>`` to the ``InstallDhcp6Client`` helper.

The server application is installed on the node of interest.
Users select the interfaces on which the server should listen for incoming messages and add them to a ``NetDeviceContainer``. This is then passed to the ``InstallDhcp6Server`` helper method.

Helpers
#######

The ``Dhcp6Helper`` supports the typical ``Install`` usage pattern in |ns3|. It
can be used to easily install DHCPv6 server and client applications on a set of
interfaces.

.. sourcecode:: cpp

  NetDeviceContainer serverNetDevices;
  serverNetDevices.Add(netdevice);
  ApplicationContainer dhcpServerApp = dhcp6Helper.InstallDhcp6Server(serverNetDevices);

  ApplicationContainer dhcpClientApp = dhcp6Helper.InstallDhcp6Client(clientNode);

Address pools can be configured on the DHCPv6 server using the following API:

.. sourcecode:: cpp

  Ptr<Dhcp6Server> server = DynamicCast<Dhcp6Server>(dhcpServerApp.Get(0));
  server->AddSubnet(Ipv6Address("2001:db8::"), Ipv6Prefix(64), Ipv6Address("2001:db8::1"), Ipv6Address("2001:db8::ff"));

In the line above, the ``AddSubnet()`` method has the following parameters:

1. ``Ipv6Address("2001:db8::")`` - The address pool that is managed by the server.
2. ``Ipv6Prefix(64)`` - The prefix of the address pool
3. ``Ipv6Address("2001:db8::1")`` - The minimum address that can be assigned to a client.
4. ``Ipv6Address("2001:db8::ff")`` - The maximum address that can be assigned to a client.

Essentially, parameters 1 and 2 define the subnet(s) managed by the server, while parameters
3 and 4 define the range of addresses that can be assigned.
If this method is not called, the server will not have any subnet configured and will not be able to assign addresses to clients.
While it does not throw an error, a user who does not configure any subnets on the server will not see any addresses leased to the client.

Attributes
##########

The following attributes can be configured on the client:

* ``Transactions``: A random variable used to set the transaction numbers.
* ``SolicitJitter``: The jitter in milliseconds that a node waits before sending a Solicit to the server.
* ``IaidValue``: The identifier of a new Identity Association that is created by a client.

The following values can be initially set on the client interface before stateful DHCPv6 begins. However, they are overridden with values received from the server during the message exchange:
* ``RenewTime``: The time after which client should renew its lease.
* ``RebindTime``: The time after which client should rebind its leased addresses.
* ``PreferredLifetime``: The preferred lifetime of the leased address.
* ``ValidLifetime``: Time after which client should release the address.

The following attributes can be configured on the server:
* ``RenewTime``: The time after which client should renew its lease.
* ``RebindTime``: The time after which client should rebind its leased addresses.
* ``PreferredLifetime``: The preferred lifetime of the leased address.
* ``ValidLifetime``: The time after which client should release the address.

Example
#######

The following example has been written in ``src/internet-apps/examples/``:

* ``dhcp6-example.cc``: Demonstrates the working of stateful DHCPv6 with a single server and 2 client nodes.

Test
====

The following example has been written in ``src/internet-apps/test/``:

* ``dhcp6-test.cc``: Tests the working of DHCPv6 with a single server and 2 client nodes that have 1 CSMA interface and 1 Wifi interface each.

Scope and Limitations
=====================
* Limited options have been included in the DHCPv6 implementation, namely:

  * Server Identifier
  * Client Identifier
  * Identity Association for Non-temporary Addresses (IA_NA)
  * Option Request (for Solicit MAX_RT)
  * Status Code

* The implementation does not support the use of a DHCP Relay agent. Hence, all the server and client nodes should be on the same link.
* The application does not yet support prefix delegation. Each client currently receives only one IPv6 address.
* If a DUID-LLT (Link-Layer Address plus Time) is used as the client identifier, there is a possibility that the timestamps of two clients may be the same. This could lead to a conflict in the lease bindings on the server if the link-layer addresses are not unique.
* If a node (such as a Wifi device) leaves the network and rejoins at a later time, the Solicit messages are not automatically restarted.
* The client application does not retransmit lost messages. Its use in wireless scenarios might lead to inconsistencies.

Future Work
===========
* The Rapid Commit option may be implemented to allow a Solicit / Reply message exchange between the client and server.
* Addition of DHCPv6 relays to allow the client and server to be in different subnets.
* Implementation of stateless DHCPv6 to allow the client to request only configuration information from the server.
* Implement the ability to configure DHCPv6 through a configuration file, similar to how it is done in Linux systems.
* Allow users to manually set the DUIDs for the client(s) and server(s).
* Implement support for host reservations, allowing specific IPv6 addresses to be assigned to particular clients based on their DUIDs or other identifying information.
* Include the ``Preference Option`` in the Advertise message sent by the server. This option allows the client to identify and choose a single server based on the preference value, instead of obtaining leases from each server that responds to the Solicit message

References
==========
* :rfc:`8415` - Dynamic Host Configuration Protocol for IPv6 (DHCPv6)
* Infoblox Blog <https://blogs.infoblox.com/ipv6-coe/slaac-to-basics-part-2-of-2-configuring-slaac/ > to understand how SLAAC and DHCPv6 operate at the same time.
