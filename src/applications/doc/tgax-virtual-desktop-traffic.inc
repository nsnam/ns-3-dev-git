TGax Virtual Desktop Infrastructure Traffic Model
-------------------------------------------------

This section describes the implementation of the |ns3| TGax Virtual Desktop Infrastructure (VDI)
traffic generator model, which is based on the IEEE 802.11-14/0571r12 TGAX evaluation
methodology [TGAX_EVAL]_.
The TGax VDI traffic model is implemented in the ``src/applications/`` folder.

Virtual Desktop Infrastructure (VDI) traffic is generated from a server and, in practice,
may travel over multiple hops before arriving at an access point (AP) for transmission to
stations (STAs).  VDI traffic statistics are asymmetric (between downlink (server to client
display) and uplink (client display to server)).

The model uses an exponential distribution for packet inter-arrival times and a normal
distribution (including a bimodal distribution) for packet sizes, as specified in the
IEEE 802.11-14/0571r12 evaluation methodology.

Scope and Limitations
~~~~~~~~~~~~~~~~~~~~~

The VDI model implements the traffic generation aspects of the IEEE 802.11-14/0571r12
specification. The following describes what is covered and what is not:

**Supported Features:**

- Exponential distribution for packet inter-arrival times
- Uniform distribution for initial packet arrival (0-20ms)
- Normal distribution for packet sizes
- Bimodal normal distribution support for downlink packet sizes
- TCP protocol (default, matching specification)
- Configurable parameters for both downlink and uplink scenarios
- Trace source for monitoring packet transmission

**Known Limitations:**

- (code review comment to resolve) Bimodal distribution mode selection uses equal probability instead
  of weighted selection (70.6%/29.4%).
  future versions.
- (code review comment to resolve) Each application instance is unidirectional; bidirectional VDI simulation requires two
  separate application instances and the uplink's parameters must be manually configured.

Model Description
~~~~~~~~~~~~~~~~~

The ``TgaxVirtualDesktop`` application is a source application that generates VDI traffic
patterns according to the IEEE 802.11-14/0571r12 evaluation methodology.

**Traffic Generation Process:**

1. **Initial Packet Arrival**: The first packet is scheduled after a uniformly distributed
   delay in the range [0, 20ms], simulating random timing between client packet arrival
   and frame boundaries.

2. **Subsequent Packet Arrivals**: After the initial packet, inter-arrival times follow
   an exponential distribution with direction-specific mean values.

3. **Packet Size Generation**: Packet sizes are drawn from a normal distribution. For
   downlink traffic, a bimodal normal distribution is used to model the mix of small
   control packets and large display data packets.

**Direction-Specific Parameters:**

The specification defines different parameters for downlink and uplink traffic:

*Downlink (AP to STA - Display Data):*

.. list-table:: Downlink VDI Parameters
   :header-rows: 1

   * - Parameter
     - Distribution
     - Value
   * - Initial packet arrival
     - Uniform
     - [0, 20] ms
   * - Packet inter-arrival
     - Exponential
     - Mean = 60.2269 ms
   * - Packet size
     - Bimodal Normal
     - Mode 1: mu=41.0, sigma=3.2 (weight 53.7/76.1)
   * -
     -
     - Mode 2: mu=1478.3, sigma=11.6 (weight 22.4/76.1)

*Uplink (STA to AP - Navigation/Feedback):*

.. list-table:: Uplink VDI Parameters
   :header-rows: 1

   * - Parameter
     - Distribution
     - Value
   * - Initial packet arrival
     - Uniform
     - [0, 20] ms
   * - Packet inter-arrival
     - Exponential
     - Mean = 48.2870 ms
   * - Packet size
     - Normal
     - mu=50.598, sigma=5.0753

Configuration and Usage
***********************

The VDI traffic generator can be instantiated through the ``ApplicationHelper``.
The default configuration matches the downlink specification.

**Downlink Configuration (Default):**

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

  // Configure VDI application (downlink - default parameters)
  ApplicationHelper sourceHelper(TgaxVirtualDesktop::GetTypeId());
  sourceHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.2"), 5000)));

  auto sourceApps = sourceHelper.Install(nodes.Get(0));
  sourceApps.Start(Seconds(1.0));
  sourceApps.Stop(Seconds(11.0));

  // Create receiver
  PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
  auto sinkApps = sinkHelper.Install(nodes.Get(1));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(12.0));

**Uplink Configuration:**

::

  // Configure VDI application for uplink
  ApplicationHelper sourceHelper(TgaxVirtualDesktop::GetTypeId());
  sourceHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.1"), 5001)));

  // Set uplink inter-arrival time (mean 48.287 ms)
  auto ulInterArrival = CreateObjectWithAttributes<ExponentialRandomVariable>(
      "Mean", DoubleValue(48287000.0));  // nanoseconds
  sourceHelper.SetAttribute("InterPacketArrivals", PointerValue(ulInterArrival));

  // Set uplink packet size (normal distribution, mu=50.598, sigma=5.0753)
  sourceHelper.SetAttribute("ParametersPacketSize", StringValue("50.598 5.0753"));

  auto sourceApps = sourceHelper.Install(nodes.Get(1));  // STA node
  sourceApps.Start(Seconds(1.0));
  sourceApps.Stop(Seconds(11.0));

Attributes
~~~~~~~~~~

The VDI traffic generator uses TCP by default.

The following attributes control the behavior of the VDI traffic generator:

**Protocol Configuration:**

* ``Protocol``: The socket factory type to use. Default: ``ns3::TcpSocketFactory``
* ``Remote``: The remote address and port for the VDI destination

**Timing Parameters:**

* ``InitialPacketArrival``: A uniform random variable for the initial packet arrival time.
  Default: Uniform ranging from 0 to 20 ms
* ``InterPacketArrivals``: An exponential random variable for inter-packet arrival times.
  Default: Exponential with mean 60.2269ms (downlink specification)

**Packet Size Parameters:**

* ``ParametersPacketSize``: The mean and standard deviation for each mode of the (potentially
  multimodal) normal distribution used to generate packet sizes. Format: "mean1 std1;mean2 std2"
  for bimodal, or "mean std" for unimodal. Default: "41.0 3.2;1478.3 11.6" (downlink bimodal)

Trace Sources
~~~~~~~~~~~~~

The VDI traffic generator provides a trace source for monitoring simulation behavior:

**Tx:**

::

  typedef void (*TxTracedCallback)(Ptr<const Packet> packet);

This trace source is fired when a packet is transmitted. It provides the packet being
transmitted. This is useful for analyzing packet transmission patterns and sizes.

Examples
~~~~~~~~

The ``tgax-virtual-desktop-example.cc`` program demonstrates basic usage of the VDI traffic
generator. This example sets up a simple Wi-Fi network (AP and STA) with VDI traffic. The
example supports downlink, uplink, and bidirectional traffic configurations.

To run with downlink traffic (default):

::

  $ ./ns3 run tgax-virtual-desktop-example

To run with uplink traffic:

::

  $ ./ns3 run "tgax-virtual-desktop-example --direction=uplink"

To run with bidirectional traffic:

::

  $ ./ns3 run "tgax-virtual-desktop-example --direction=bidirectional"

Tests
~~~~~

Unit tests for the VDI traffic model are provided in
``src/applications/test/tgax-virtual-desktop-test-suite.cc``. These tests verify:

* Downlink VDI traffic with default parameters
* Uplink VDI traffic with custom parameters
* Initial packet arrival within expected bounds (0-20ms)
* Average packet inter-arrival time matches expectation
* Average packet size matches expectation
* All transmitted bytes are received

To run the VDI traffic tests:

::

  $ ./test.py -s applications-tgax-virtual-desktop

Verification
~~~~~~~~~~~~

The VDI traffic model has been checked against the IEEE 802.11-14/0571r12 specification
by comparing:

* Initial packet arrival distribution (Uniform [0, 20ms])
* Packet inter-arrival time distribution (Exponential with direction-specific means)
* Packet size distribution parameters against specification

The test suite demonstrates that the model correctly implements the exponential inter-arrival
behavior and normal packet size distributions.

