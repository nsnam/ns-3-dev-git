VoIP Traffic Model
------------------

This section describes the implementation of the |ns3| VoIP traffic generator model, which
is based on the IEEE 802.11-14/0571r12 TGAX evaluation methodology [TGAX_EVAL]_. The model
generates realistic voice-over-IP (VoIP) traffic patterns suitable for evaluating IEEE 802.11
networks and other wireless systems.

The VoIP traffic model is implemented in the ``src/applications/`` folder.

The VoIP traffic generator models the behavior of a real VoIP source, producing traffic
that alternates between periods of active talking and silence. This model is useful for
simulating realistic VoIP scenarios in network simulations, particularly for evaluating
the performance of wireless access technologies such as IEEE 802.11 (Wi-Fi).

The model implements a two-state Markov chain to represent voice activity, where transitions
occur between active (talking) and inactive (silence) states. During active states, the source
generates packets at regular intervals (20 ms) with one payload size, and during silence states,
packets are generated at longer intervals (160 ms) with a smaller payload size. Both states
can last for exponentially distributed durations, giving the model stochastic properties that
match real voice traffic characteristics.

The VoIP model can be used with various socket types including IPv4/UDP, IPv6/UDP, and
PacketSocket, making it flexible for different network configurations and simulation scenarios.

Scope and Limitations
~~~~~~~~~~~~~~~~~~~~~

The VoIP model implements the core traffic generation aspects of the IEEE 802.11-14/0571r12
specification. The following describes what is covered and what is not:

**Supported Features:**

- Two-state Markov model for voice activity (active/inactive states)
- Configurable state transition probabilities
- Exponential distribution for state durations
- Variable packet payload sizes for active and inactive states
- Laplacian distribution for downlink packet delay jitter
- Integration with UDP, TCP, and PacketSocket protocols
- Comprehensive trace sources for monitoring packet transmission and state changes

**Known Limitations:**

- Protocol header overhead (3 bytes for IPv4, 5 bytes for IPv6-- both values assuming IP header compression (not implemented in ns-3) is used) must be added to payload sizes
- Jitter is applied uniformly to all packets (asymmetric uplink/downlink jitter not differentiated)
- TCP transmission does not validate buffer availability; packets may be dropped if the TX buffer is full
- The model does not enforce the implicit 12.2 kbps bit rate constraint in the IEEE document; users must configure appropriate parameters
- Directional traffic (unidirectional source only, no bidirectional conversation modeling)

Model Description
~~~~~~~~~~~~~~~~~

The ``TgaxVoipTraffic`` application is a source application that generates VoIP traffic patterns
according to the IEEE 802.11-14/0571r12 evaluation methodology. The application uses a simple
but effective two-state Markov model to generate realistic voice activity patterns.

**State Machine:**

The model maintains two states:

* **ACTIVE_TALKING**: The source is actively transmitting voice data. In this state, packets
  are generated at the voice encoder interval (default 20 ms) with a payload size configured
  for active voice packets (default 33 bytes).

* **INACTIVE_SILENCE**: The source is in a silent period. In this state, packets are generated
  at the silence encoder interval (default 160 ms) with a smaller payload size (default 7 bytes).
  Silence packets typically carry comfort noise or silence indication in real VoIP systems.

**State Transitions:**

The transition from one state to another follows a two-state Markov chain with configurable
transition probabilities:

* **VoiceToSilenceProbability** (default 0.016): Probability per state update interval to transition
  from ACTIVE_TALKING to INACTIVE_SILENCE.

* **SilenceToVoiceProbability** (default 0.016): Probability per state update interval to transition
  from INACTIVE_SILENCE to ACTIVE_TALKING.

With default parameters (both probabilities at 0.016), the model produces a voice activity factor
of approximately 50%, meaning the source is in the active state about half the time.

**State Duration:**

Once a state is entered, the source remains in that state for an exponentially distributed duration:

* **MeanActiveStateDuration** (default 1250 ms): Mean duration of active talking periods.
* **MeanInactiveStateDuration** (default 1250 ms): Mean duration of silence periods.

These exponential distributions produce realistic variability in conversation patterns, where some
active periods are very short and others are quite long.

**Packet Generation and Jitter:**

Packets are generated at regular intervals determined by the current state (20 ms for active,
160 ms for silence). Each generated packet receives a random delay jitter before transmission,
drawn from a Laplacian distribution:

* **MeanDelayJitter** (default 0 us): Mean of the Laplacian distribution.
* **ScaleDelayJitter** (default 5110 us): Scale parameter of the Laplacian distribution.
* **BoundDelayJitter** (default 80 ms): Bound on the jitter magnitude to ensure stability.

The jitter model is applied only in the downlink direction, as specified in the IEEE 802.11-14/0571r12
methodology.

**Payload Sizes and Protocol Headers:**

The model specifies default payload sizes based on the IEEE specification:

* **ActivePacketPayloadSize** (default 33 bytes): Size of packets during active talking.
* **SilencePacketPayloadSize** (default 7 bytes): Size of packets during silence periods.

When using UDP over IPv4 or IPv6, users should account for compressed protocol headers:
- IPv4 with compression: add 3 bytes to each payload size
- IPv6 with compression: add 5 bytes to each payload size

These header sizes are NOT automatically included; the user must adjust the payload size
attributes accordingly if protocol header compression is desired in the simulation.

Configuration and Usage
***********************

The VoIP traffic generator is instantiated as an application through the ``ApplicationHelper``.
Here is a basic example using IPv4/UDP:

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

  // Configure VoIP application
  ApplicationHelper sourceHelper(TgaxVoipTraffic::GetTypeId());
  sourceHelper.SetAttribute("Protocol", StringValue("ns3::UdpSocketFactory"));
  sourceHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.2"), 5000)));
  sourceHelper.SetAttribute("ActivePacketPayloadSize", UintegerValue(33 + 3)); // +3 for IPv4 header
  sourceHelper.SetAttribute("SilencePacketPayloadSize", UintegerValue(7 + 3));

  auto sourceApps = sourceHelper.Install(nodes.Get(0));
  sourceApps.Start(Seconds(1.0));
  sourceApps.Stop(Seconds(11.0));

  // Create receiver
  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
  auto sinkApps = sinkHelper.Install(nodes.Get(1));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(12.0));

Attributes
~~~~~~~~~~

The VoIP traffic generator supports multiple socket types through the ``Protocol`` attribute:

* **ns3::UdpSocketFactory**: UDP socket (recommended for most scenarios)
* **ns3::TcpSocketFactory**: TCP socket (may experience packet drops if TX buffer is full)
* **ns3::PacketSocketFactory**: Raw packet socket (useful for link-layer only simulations)

For each socket type, the ``Remote`` address must be configured appropriately:

* For UDP/TCP: Use ``InetSocketAddress`` for IPv4 or ``Inet6SocketAddress`` for IPv6
* For PacketSocket: Use ``PacketSocketAddress`` with physical layer addressing

The following additional attributes control the behavior of the VoIP traffic generator:

**Protocol Configuration:**

* ``Protocol``: The socket factory type to use. Default: ``ns3::PacketSocketFactory``
* ``Remote``: The remote address and port for the VoIP destination

**Packet Sizes:**

* ``ActivePacketPayloadSize``: Payload size (in bytes) for packets during active talking states. Default: 33
* ``SilencePacketPayloadSize``: Payload size (in bytes) for packets during silence states. Default: 7

**State Machine Parameters:**

* ``MeanActiveStateDuration``: Mean duration (Time) of active talking states. Default: 1250 ms
* ``MeanInactiveStateDuration``: Mean duration (Time) of silence states. Default: 1250 ms
* ``VoiceToSilenceProbability``: Transition probability from active to silence state per state update. Default: 0.016
* ``SilenceToVoiceProbability``: Transition probability from silence to active state per state update. Default: 0.016

**Encoder Parameters:**

* ``VoiceEncoderInterval``: Time interval between generation of active voice packets. Default: 20 ms
* ``SilenceEncoderInterval``: Time interval between generation of silence packets. Default: 160 ms

**Jitter Model:**

* ``MeanDelayJitter``: Mean of the Laplacian distribution for packet delay jitter. Default: 0 us
* ``ScaleDelayJitter``: Scale parameter of the Laplacian distribution for jitter. Default: 5110 us
* ``BoundDelayJitter``: Upper bound on the magnitude of applied jitter. Default: 80 ms

Trace Sources
~~~~~~~~~~~~~

The VoIP traffic generator provides two main trace sources for monitoring simulation behavior:

**TxWithJitter:**

::

  typedef void (*TxTracedCallback)(Ptr<const Packet> packet, Time jitter);

This trace source is fired when a packet is transmitted. It provides both the packet being
transmitted and the jitter value that was applied to that packet. This is useful for analyzing
packet transmission patterns and the distribution of applied jitter.

**StateUpdate:**

::

  typedef void (*StateUpdatedCallback)(VoiceActivityState state, Time duration);

This trace source is fired when the voice activity state changes (from active to silence or vice versa).
It provides the new state and the expected duration of that state. This is useful for analyzing voice
activity patterns and verifying that the Markov model is producing the expected statistics.

Examples
~~~~~~~~

The ``voip-example.cc`` program demonstrates basic usage of the VoIP traffic generator.
This example sets up a simple two-node Wi-Fi network (AP and STA) with VoIP traffic flowing
from the STA to the AP. The example supports three socket type configurations: IPv4/UDP,
IPv6/UDP, and PacketSocket.

Tests
~~~~~

Unit tests for the VoIP traffic model are provided in ``src/applications/test/tgax-voip-traffic-test-suite.cc``.
These tests verify:

* Correct packet sizes for active and silence states
* Proper encoder frame intervals (20 ms for active, 160 ms for silence)
* State duration statistics matching exponential distribution expectations
* State transition correctness
* Proper application of jitter to packets
* Voice activity factor statistics matching theoretical expectations

To run the VoIP traffic tests:

::

  $ ./test.py -s tgax-voip-traffic

Verification
~~~~~~~~~~~~

The VoIP traffic model has been checked against the IEEE 802.11-14/0571r12 specification
by comparing:

* Default parameter values against those specified in the standard
* Observed state durations against exponential distribution properties
* Voice activity factor (ratio of active to total time) against theoretical expectations
* Packet sizes and intervals against specification

The test suite demonstrates that the model correctly implements the two-state Markov behavior,
produces appropriate state durations, and generates traffic patterns consistent with real VoIP systems.

