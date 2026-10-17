RTA TIG Mobile Gaming Traffic Model
-----------------------------------

This section describes the implementation of the |ns3| Real-Time Applications (RTA) Task
Interest Group (TIG) mobile gaming traffic generator model, which is based on the
IEEE 802.11-18/2009r6 RTA TIG Report [RTA-TIG]_.

The RTA TIG mobile gaming traffic model is implemented in the ``src/applications/`` folder.

Real-time mobile gaming is characterized by small packets (30-500 bytes) transmitted
frequently (every 30-60ms) with strict latency requirements. The traffic is asymmetric
and bidirectional, with different characteristics for uplink (player commands) and
downlink (game state updates).

The model uses the Largest Extreme Value (LEV) distribution for packet inter-arrival
times and packet sizes during the gaming stage, with uniform distributions for initial
and ending packets.

Scope and Limitations
~~~~~~~~~~~~~~~~~~~~~

The mobile gaming model implements the traffic generation aspects of the IEEE 802.11-18/2009r6
specification (Section 4.1.4).

**Supported Features:**

- Two synchronization mechanisms: Status Sync and Frame Lockstep Sync
- Three-stage traffic model: Initial, Gaming, and Ending
- Largest Extreme Value distribution for packet arrivals and sizes
- Uniform distribution for initial and end packet sizes
- UDP protocol (default, matching specification)
- Configurable parameters for all four scenarios (DL/UL x Status-Sync/Lockstep)
- Trace source with stage information

**Known Limitations:**

- Each application instance is unidirectional; bidirectional gaming simulation requires
  two separate application instances
- No preset configuration enum; users must manually configure parameters for non-default
  scenarios

Model Description
~~~~~~~~~~~~~~~~~

The ``RtaTigMobileGaming`` application is a source application that generates real-time
mobile gaming traffic patterns according to the IEEE 802.11-18/2009r6 RTA TIG Report.

**Traffic Stages:**

The model implements a three-stage traffic pattern:

1. **INITIAL Stage**: A single packet is sent with size drawn from a uniform distribution.
   This represents the initial game setup/synchronization packet.

2. **GAMING Stage**: Continuous packet transmission during active gameplay. Packet sizes
   and inter-arrival times are drawn from Largest Extreme Value distributions.

3. **ENDING Stage**: A single packet is sent when the application stops, with size drawn
   from a uniform distribution. This represents the game termination packet.

**Synchronization Mechanisms:**

The specification defines two traffic models for different game synchronization approaches:

*Status Sync Model:*

Used by games that synchronize game state between clients and server. Characterized by
smaller, more frequent packets.

.. list-table:: Status Sync Traffic Parameters
   :header-rows: 1

   * - Component
     - Distribution
     - DL (a, b)
     - UL (a, b)
   * - Initial packet size
     - Uniform
     - [0, 20] bytes
     - [0, 20] bytes
   * - Packet arrival time
     - Largest Extreme Value
     - a=13ms, b=3.7ms
     - a=15ms, b=5.7ms
   * - Packet size
     - Largest Extreme Value
     - a=50, b=11 bytes
     - a=38, b=3.7 bytes
   * - End packet size
     - Uniform
     - [500, 600] bytes
     - [400, 550] bytes

*Frame Lockstep Sync Model:*

Used by games that require frame-by-frame synchronization. Characterized by larger
packets with slightly longer intervals.

.. list-table:: Frame Lockstep Sync Traffic Parameters
   :header-rows: 1

   * - Component
     - Distribution
     - DL (a, b)
     - UL (a, b)
   * - Initial packet size
     - Uniform
     - [0, 80] bytes
     - [0, 80] bytes
   * - Packet arrival time
     - Largest Extreme Value
     - a=28ms, b=4.2ms
     - a=22ms, b=3.4ms
   * - Packet size
     - Largest Extreme Value
     - a=210, b=35 bytes
     - a=92, b=38 bytes
   * - End packet size
     - Uniform
     - [1400, 1500] bytes
     - [500, 600] bytes

In the Largest Extreme Value distribution, **a** is the location parameter and **b** is
the scale parameter.

Configuration and Usage
***********************

The mobile gaming traffic generator can be instantiated through the ``ApplicationHelper``.
The default configuration matches the Status-Sync Downlink parameters.

**Default Configuration (Status-Sync DL):**

::

  // Create nodes
  NodeContainer nodes;
  nodes.Create(2);

  // Install Internet stack and assign IP addresses
  InternetStackHelper internet;
  internet.Install(nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.255.255.0");
  ipv4.Assign(devices);

  // Configure gaming application (defaults to status-sync DL)
  ApplicationHelper sourceHelper(RtaTigMobileGaming::GetTypeId());
  sourceHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.2"), 5000)));

  auto sourceApps = sourceHelper.Install(nodes.Get(0));
  sourceApps.Start(Seconds(1.0));
  sourceApps.Stop(Seconds(11.0));

  // Create receiver
  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
  auto sinkApps = sinkHelper.Install(nodes.Get(1));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(12.0));

**Lockstep DL Configuration:**

Note:  The below would be better configured via a mode attribute such as LOCKSTEP_DL.

::

  ApplicationHelper sourceHelper(RtaTigMobileGaming::GetTypeId());
  sourceHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.2"), 5000)));

  // Configure lockstep DL parameters
  auto ips = CreateObjectWithAttributes<UniformRandomVariable>(
      "Min", DoubleValue(0), "Max", DoubleValue(80));
  sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

  auto eps = CreateObjectWithAttributes<UniformRandomVariable>(
      "Min", DoubleValue(1400), "Max", DoubleValue(1500));
  sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

  auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
      "Location", DoubleValue(28000),   // 28ms in microseconds
      "Scale", DoubleValue(4200));      // 4.2ms in microseconds
  sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));

  auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
      "Location", DoubleValue(210),
      "Scale", DoubleValue(35));
  sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

Attributes
~~~~~~~~~~

The mobile gaming traffic generator uses UDP by default, as specified for in-game data
in the IEEE specification.

The following attributes control the behavior of the traffic generator:

**Protocol Configuration:**

* ``Protocol``: The socket factory type to use. Default: ``ns3::UdpSocketFactory``
* ``Remote``: The remote address and port for the gaming traffic destination

**Initial and End Packet Parameters:**

* ``InitialPacketSize``: A uniform random variable for initial packet size in bytes.
  Default: Uniform[0, 20]
* ``EndPacketSize``: A uniform random variable for end packet size in bytes.
  Default: Uniform[500, 600]

**Gaming Stage Parameters:**

* ``PacketArrivalLev``: A Largest Extreme Value random variable for packet inter-arrival
  times in microseconds. Default: LEV(Location=13000, Scale=3700) (13ms, 3.7ms)
* ``PacketSizeLev``: A Largest Extreme Value random variable for gaming packet sizes
  in bytes. Default: LEV(Location=50, Scale=11)

Trace Sources
~~~~~~~~~~~~~

The mobile gaming traffic generator provides a trace source for monitoring simulation
behavior:

**TxWithStage:**

::

  typedef void (*TxTracedCallback)(Ptr<const Packet> packet, TrafficModelStage stage);

This trace source is fired when a packet is transmitted. It provides the packet and
the current traffic model stage (INITIAL, GAMING, or ENDING). This is useful for
analyzing traffic patterns and verifying stage transitions.

Examples
~~~~~~~~

The ``rta-tig-mobile-gaming-example.cc`` program demonstrates usage of the mobile gaming
traffic generator. This example sets up a simple Wi-Fi network (AP and STA) with gaming
traffic. The example supports all four traffic model presets.

To run with Status-Sync Downlink (default):

::

  $ ./ns3 run rta-tig-mobile-gaming-example

To run with Status-Sync Uplink:

::

  $ ./ns3 run "rta-tig-mobile-gaming-example --model=status-sync-ul"

To run with Frame Lockstep Downlink:

::

  $ ./ns3 run "rta-tig-mobile-gaming-example --model=lockstep-dl"

To run with Frame Lockstep Uplink:

::

  $ ./ns3 run "rta-tig-mobile-gaming-example --model=lockstep-ul"

Tests
~~~~~

Unit tests for the mobile gaming traffic model are provided in
``src/applications/test/rta-tig-mobile-gaming-test-suite.cc``. These tests verify:

* Correct stage transitions (INITIAL -> GAMING -> ENDING)
* Initial packet size within expected bounds
* End packet size within expected bounds
* Average gaming packet size matches LEV distribution mean
* Average packet inter-arrival time matches LEV distribution mean
* All transmitted bytes are received

To run the mobile gaming traffic tests:

::

  $ ./test.py -s applications-rta-tig-mobile-gaming

Verification
~~~~~~~~~~~~

The mobile gaming traffic model has been checked against the IEEE 802.11-18/2009r6
specification by comparing:

* Default parameter values against Table 4-3 (Status Sync) in the specification
* Largest Extreme Value distribution behavior for packet sizes and arrivals
* Stage transition logic

The test suite demonstrates that the model correctly implements the three-stage traffic
pattern and produces traffic characteristics consistent with real-time mobile gaming
applications.

