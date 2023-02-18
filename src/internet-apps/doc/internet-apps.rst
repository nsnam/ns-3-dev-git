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
