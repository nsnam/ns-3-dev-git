.. include:: replace.txt
.. _Mobility:

Mobility
========

.. heading hierarchy:
   ============= Chapter
   ------------- Section (#.#)
   ~~~~~~~~~~~~~ Subsection (#.#.#)
   ^^^^^^^^^^^^^ Paragraph (no number)

The mobility framework in |ns3| provides support for modeling movement and position in network simulations. This system enables simulation of mobile networks, wireless protocols, and location-dependent phenomena.

The |ns3| mobility system consists of three main components:

- **Mobility Models**: Track and report on the current position, velocity, and movement patterns of network nodes
- **Position Allocators**: Determine initial node placement
- **Helper Classes**: Simplify configuration and provide parsers for external mobility trace formats

Most mobility-related source code is located in the ``src/mobility`` directory. For building-related mobility models, see ``src/buildings``.

The design includes mobility models, position allocators, and helper
functions.

In |ns3|, `MobilityModel`` objects track the evolution of position
with respect to a (cartesian) coordinate system.  The mobility model
is typically aggregated to an ``ns3::Node`` object and queried using
``GetObject<MobilityModel> ()``. The base class ``ns3::MobilityModel``
is subclassed for different motion behaviors.

The initial position of objects is typically set with a PositionAllocator.
These types of objects will lay out the position on a notional canvas.
Once the simulation starts, the position allocator may no longer be
used, or it may be used to pick future mobility "waypoints" for such
mobility models.

Most users interact with the mobility system using mobility helper
classes.  The MobilityHelper combines a mobility model and position
allocator, and can be used with a node container to install a similar
mobility capability on a set of nodes.

We first describe the coordinate system and issues
surrounding multiple coordinate systems.

Scope and Limitations
---------------------

**Current Support**:

- Cartesian, geocentric cartesian, and geographic coordinates are presently supported
- Twelve (12) open-field mobility models and one (1) buildings aware mobility model
- Integration with external mobility trace generators

**Current Limitations**:

- Z-axis movement support varies by model
- Limited built-in support for obstacle-aware movement
- No native support for indoor mobility patterns



Coordinate Systems
------------------

|ns3| supports multiple coordinate systems to accommodate different simulation scenarios and real-world applications.

**Cartesian Coordinates (Default)**:

- **Format**: (x, y, z) coordinates in 3D space
- **Use case**: Most suitable for simulation scenarios spanning a few kilometers
- **Implementation**: ``ns3::Vector`` class represents both positions and velocities

**Topocentric Coordinates**:

- **Format**: Cartesian coordinates with a specific Earth surface reference point
- **Use case**: Converting between local and global coordinate systems
- **Implementation**: Extends Cartesian with geographic origin reference (``GeographicPositions::TopocentricToGeographicCoordinates`` in ``src/mobility/model/geographic-position.cc``)

**Geocentric Cartesian Coordinates**:

- **Format**: Earth-centered Cartesian system with origin at Earth's center of mass
- **Use case**: Satellite communications and global-scale simulations
- **Implementation**: ``GeocentricConstantPositionMobilityModel``
- **Reference**: Follows 3GPP TR 38.811, Section 6.3 [2]_

**Geographic Coordinates**:

- **Format**: Latitude, longitude, and altitude relative to Earth's surface
- **Use case**: GPS-based applications and real-world location mapping
- **Implementation**: ``GeographicPosition`` class with conversion utilities

At the moment, the geocentric Cartesian coordinates are adopted by the
GeocentricConstantPositionMobilityModel only.
This class implements the Get/SetPosition methods, which leverage the
GeographicPosition class to offer conversions to and from Cartesian coordinates.
Additionally, users can set the position of a node by its geographical coordinates
via the methods Get/SetGeographicPosition.



Available Mobility Models
-------------------------


Core Classes
~~~~~~~~~~~~

**MobilityModel Base Class**

The ``ns3::MobilityModel`` class is the foundation of the mobility system:

.. sourcecode:: cpp

   // Key methods
   Vector GetPosition() const;           // Current position
   Vector GetVelocity() const;          // Current velocity
   double GetDistanceFrom(Ptr<const MobilityModel> other) const;

   // Attributes
   Vector Position;                     // Current position
   Vector Velocity;                     // Current velocity

   // Trace sources
   TracedCallback<Ptr<const MobilityModel>> CourseChange;

**Key Features**:

- **Position Tracking**
- **Velocity Tracking**
- **Distance Calculation**: Calculates distance to another mobility model object
- **Course Change Notifications**: Trace source for movement events
- **Node Aggregation**: Typically aggregated to ``ns3::Node`` objects

Child Classes
~~~~~~~~~~~~~

**Static Models:**

- **ConstantPositionMobilityModel**: Nodes remain at fixed positions throughout the simulation. The position is set once during initialization and only changes if explicitly set by the user during runtime. This model is ideal for stationary infrastructure nodes, base stations, or scenarios where mobility is not required. It has near zero computational overhead during simulation runtime.

- **GeocentricConstantPositionMobilityModel**: Similar to ConstantPositionMobilityModel but with geographic coordinate support. Positions can be specified using latitude, longitude, and altitude coordinates, which are internally converted to the simulation's Cartesian coordinate system. Useful for real-world GPS-based scenarios and satellite communication simulations.

**Linear Motion Models:**

- **ConstantVelocityMobilityModel**: Nodes move in a straight line with constant speed. The velocity vector (speed and direction) is set once and remains unchanged unless explicitly modified by external events. This model is commonly used with |ns2| mobility traces where setdest commands update the velocity.

- **ConstantAccelerationMobilityModel**: Implements uniformly accelerated motion following kinematic equations. The node starts with an initial velocity and acceleration vector, with position updated according to:

.. math::

  \vec{p}(t) = \vec{p_0} + \vec{v_0} \cdot t + \frac{1}{2} \vec{a} \cdot t^2

This model is useful for scenarios involving vehicle acceleration/deceleration or projectile motion.

**Random Motion Models:**

- **RandomWalk2dMobilityModel**: Implements a two-dimensional random walk where nodes change direction and speed at regular time intervals. At each step, a new direction is randomly selected from a uniform distribution [0, 2pi], and speed is drawn from a configurable random variable. Movement is constrained within specified rectangular bounds, with reflection or wrapping behavior at boundaries. The model includes configurable parameters for step time, speed distribution, and boundary behavior.

- **RandomWalk2dOutdoorMobilityModel**: Enhanced version of RandomWalk2dMobilityModel that incorporates building awareness. Nodes avoid moving through building obstacles by checking for intersections with building polygons before committing to movement. When a building collision is detected, the node selects an alternative direction. This model requires integration with building models and is particularly useful for urban mobility scenarios.

- **RandomDirection2dMobilityModel**: Nodes move in straight lines for random durations, then pause and select new random directions. Unlike RandomWalk2d, movement occurs in longer straight-line segments rather than frequent small steps. The model alternates between movement and pause periods, with both durations drawn from configurable random variables. Direction changes occur uniformly over [0, 2pi], and speed is constant during each movement phase.

- **RandomWaypointMobilityModel**: Classic random waypoint model where nodes move from their current position to randomly selected destination waypoints. Upon reaching a waypoint, the node pauses for a random duration, then selects a new random destination. Movement between waypoints follows straight-line paths at constant speeds drawn from a configurable distribution. This model exhibits well-known characteristics including non-uniform spatial distribution and speed decay over time.

- **SteadyStateRandomWaypointMobilityModel**: Addresses the initial transient behavior of the standard RandomWaypointMobilityModel by starting nodes with positions and velocities drawn from the model's steady-state distribution. This eliminates the artificial clustering and speed artifacts present during the initial simulation phase of the standard random waypoint model, providing more realistic results from simulation start.

**Advanced Models:**

- **GaussMarkovMobilityModel**: Implements a Gaussian-Markov stochastic process where velocity components are correlated over time. The model balances between random movement and momentum conservation using a tunable randomness parameter alpha in the range [0,1]. When alpha=0, movement is completely random; when alpha=1, movement maintains constant velocity. The velocity update equation is:

.. math::

  v_n = alpha \cdot v_{n-1} + (1-alpha) \cdot \bar{v} + \sqrt{1-alpha^2} \cdot v_{random}

This model produces more realistic mobility patterns with temporal correlation, suitable for human pedestrian movement.

- **WaypointMobilityModel**: Follows user-defined sequences of waypoints with precise timing control. Each waypoint specifies a position and arrival time, allowing deterministic or scripted mobility patterns. The model supports complex trajectories, synchronized movement scenarios, and replay of real-world mobility traces. Waypoints can be added dynamically during simulation, enabling adaptive mobility patterns based on simulation events.

- **HierarchicalMobilityModel**: Supports group mobility scenarios using parent-child relationships. Child nodes move relative to a parent mobility model, with the final position being the vector sum of parent and child positions. The parent model defines the group's overall movement pattern, while child models define individual member movements relative to the group. This architecture enables scenarios like vehicular convoys, pedestrian groups, or mobile sensor clusters where individual nodes maintain local mobility within a moving group context.

**Position Allocators**:

- **ListPositionAllocator**: Uses a predefined list of positions
- **GridPositionAllocator**: Arranges nodes in regular grid patterns
- **RandomRectanglePositionAllocator**: Uniform distribution within rectangular areas
- **RandomBoxPositionAllocator**: Uniform distribution within 3D box regions
- **RandomDiscPositionAllocator**: Uniform distribution within circular areas
- **UniformDiscPositionAllocator**: Even distribution on disc circumference

A position allocator is not always required, as some mobility models generate initial positions during initialization. Among the built-in ns-3 models, **SteadyStateRandomWaypointMobilityModel** is the only one with this capability.

Helper Classes
--------------

A special mobility helper is provided that is mainly aimed at supporting
the installation of mobility to a Node container (when using containers
at the helper API level).  The MobilityHelper class encapsulates
a MobilityModel factory object and a PositionAllocator used for
initial node layout.

Group mobility is also configurable via a GroupMobilityHelper object.
Group mobility reuses the HierarchicalMobilityModel allowing one to
define a reference (parent) mobility model and child (member) mobility
models, with the position being the vector sum of the two mobility
model positions (i.e., the child position is defined as an offset to
the parent position).  In the GroupMobilityHelper, the parent mobility
model is not associated with any node, and is used as the parent mobility
model for all (distinct) child mobility models.  The reference point group
mobility model [1]_ is the basis for this |ns3| model.


MobilityHelper
~~~~~~~~~~~~~~

The ``MobilityHelper`` class simplifies mobility configuration:

.. sourcecode:: cpp

   // Basic configuration
   MobilityHelper mobility;
   mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue(0.0),
                                 "MinY", DoubleValue(0.0),
                                 "DeltaX", DoubleValue(5.0),
                                 "DeltaY", DoubleValue(10.0));

   mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));

   // Installation
   mobility.Install(nodeContainer);

GroupMobilityHelper
~~~~~~~~~~~~~~~~~~~

Supports group mobility scenarios using the reference point group mobility model [1]_:

.. sourcecode:: cpp

   GroupMobilityHelper groupMobility;
   groupMobility.SetReferencePointMobilityModel("ns3::RandomWaypointMobilityModel");
   groupMobility.SetMemberMobilityModel("ns3::RandomWalk2dMobilityModel");
   groupMobility.Install(groupNodes);

Ns2MobilityHelper
~~~~~~~~~~~~~~~~~

Parses |ns2| format mobility traces for compatibility with existing tools:

.. sourcecode:: cpp

   Ns2MobilityHelper ns2mobility("mobility-trace.ns_movements");
   ns2mobility.Install();

The |ns2| mobility format is a widely used mobility trace format. Valid trace files use the following |ns2| statements:

.. sourcecode:: bash

   $node set X_ x1
   $node set Y_ y1
   $node set Z_ z1
   $ns at $time $node setdest x2 y2 speed

Supported ns-2 Commands
~~~~~~~~~~~~~~~~~~~~~~~

- ``$node set X_ x1``: Set initial X position
- ``$node set Y_ y1``: Set initial Y position
- ``$node set Z_ z1``: Set initial Z position
- ``$ns at $time $node setdest x2 y2 speed``: Move to destination at specified time

Note that in |ns3|, movement along the Z dimension is not supported by all mobility models.


Usage
-----

Most |ns3| program authors typically interact with the mobility system only at configuration time. However, various |ns3| objects interact with mobility objects repeatedly during runtime, such as a propagation model trying to determine the path loss between two mobile nodes.

Basic Configuration
~~~~~~~~~~~~~~~~~~~

Random Walk Setup
^^^^^^^^^^^^^^^^^

.. sourcecode:: cpp

  // Create nodes
  NodeContainer nodes;
  nodes.Create(10);

  // Configure mobility
  MobilityHelper mobility;

  // Grid initial placement
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(10.0),
                                "DeltaY", DoubleValue(10.0),
                                "GridWidth", UintegerValue(5),
                                "LayoutType", StringValue("RowFirst"));

  // Random walk mobility
  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                           "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)),
                           "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                           "Direction", StringValue("ns3::UniformRandomVariable[Min=0|Max=6.28]"));

  mobility.Install(nodes);

Group Mobility Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. sourcecode:: cpp

  // Reference point group mobility
  GroupMobilityHelper groupMobility;

  // Parent mobility (reference point)
  groupMobility.SetReferencePointMobilityModel("ns3::WaypointMobilityModel");

  // Child mobility (group members)
  groupMobility.SetMemberMobilityModel("ns3::RandomWalk2dMobilityModel",
                                       "Bounds", RectangleValue(Rectangle(-10, 10, -10, 10)));

  // Set reference point waypoints
  Ptr<WaypointMobilityModel> reference = groupMobility.GetReferencePointMobilityModel();
  reference->AddWaypoint(Waypoint(Seconds(0), Vector(0, 0, 0)));
  reference->AddWaypoint(Waypoint(Seconds(100), Vector(100, 0, 0)));
  reference->AddWaypoint(Waypoint(Seconds(200), Vector(100, 100, 0)));

  groupMobility.Install(groupNodes);

Geographic Positioning
^^^^^^^^^^^^^^^^^^^^^^

.. sourcecode:: cpp

  // Use geocentric coordinates
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::GeocentricConstantPositionMobilityModel");

  // Set geographic position
  Ptr<GeocentricConstantPositionMobilityModel> model =
      node->GetObject<GeocentricConstantPositionMobilityModel>();
  model->SetGeographicPosition(Vector(latitude, longitude, altitude));

Advanced Usage
~~~~~~~~~~~~~~

Random Number Stream Assignment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To avoid the possibility that configuration or parameter changes in a program affect its mobility behavior, it is necessary to assign fixed stream numbers to any random variables associated with mobility models. It is strongly advised to read the section on how to perform independent replications (see "Random Variables", the chapter 3.1 of ns-3 Manual).

.. sourcecode:: cpp

  // Assign specific random streams
  int64_t streamIndex = 100;
  MobilityHelper mobility;
  // ... configure mobility ...
  mobility.Install(nodes);

  // Assign streams after installation
  int64_t streamsUsed = mobility.AssignStreams(nodes, streamIndex);

Class ``MobilityModel`` and class ``PositionAllocator`` both have public API to assign streams to underlying random variables:

.. sourcecode:: cpp

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams(int64_t stream);

External Tool Integration
^^^^^^^^^^^^^^^^^^^^^^^^^

**BonnMotion**

BonnMotion is a Java software which creates and analyses mobility scenarios [3]_ . It is developed within the Communication Systems group at the Institute of Computer Science 4 of the University of Bonn, Germany, where it serves as a tool for the investigation of mobile ad hoc network characteristics. BonnMotion is being jointly developed by the Communication Systems group at the University of Bonn, Germany, the Toilers group at the Colorado School of Mines, Golden, CO, USA, and the Distributed Systems group at the University of Osnabrück, Germany.

- **Installation**: `Installation instructions <https://www.nsnam.org/wiki/HOWTO_use_ns-3_with_BonnMotion_mobility_generator_and_analysis_tool>`_ for using BonnMotion with |ns3|
- **Documentation**: `Documentation <https://sys.cs.uos.de/bonnmotion/doc/README.pdf>`_ available
- **Output Format**: |ns2| compatible traces

**SUMO (Simulation of Urban Mobility)**

Literature on the subject of urban wireless network often use SUMO to model the mobility in a given scenario, either based on a fictionnal setup or on real urban environment extracted through tools like OpenStreetMap.

- **Purpose**: Realistic vehicular mobility simulation
- **Website**: `SUMO <https://sourceforge.net/apps/mediawiki/sumo/index.php?title=Main_Page>`_
- **Integration**: Exports |ns2| format traces
- **Use Case**: Urban traffic scenarios

**TraNS**

- **Purpose**: Traffic and network simulation environment
- **Website**: `TraNS <http://trans.epfl.ch/>`_
- **Features**: Combines SUMO with network simulation
- **Output**: Compatible mobility traces

**|ns2| setdest Utility**

The |ns2| `setdest <http://www.winlab.rutgers.edu/~zhibinwu/html/ns2_wireless_scene.htm>`_ utility can generate basic mobility patterns.

Tracing and Visualization
~~~~~~~~~~~~~~~~~~~~~~~~~

Tracing provides access to mobility models whenever the mobility model declares a course change (a change in position or velocity).  Mobility models can also be polled at any time to find the current position of the model.

**Course Change Tracing**

An example of this tracing method is the Waypoint Mobility Model. When a node selects a new waypoint, the ``NotifyCourseChange()`` function of the ``MobilityModel`` class (parent to the ``WaypointMobilityModel`` class) is called, triggering the course change trace. This allow the user to log when an object has reached certain points.

.. sourcecode:: cpp

  // Enable course change logging
  void CourseChangeCallback(Ptr<const MobilityModel> model) {
      Vector pos = model->GetPosition();
      Vector vel = model->GetVelocity();
      std::cout << "Node moved to: " << pos << " with velocity: " << vel << std::endl;
  }

  // Connect trace source
  Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
                  MakeCallback(&CourseChangeCallback));

**Mobility Model Polling**

If client code needs access to a mobility model's position or other state information outside of course change events, it may directly query the mobility model at any time.  Mobility models are often aggregated (using ns-3 Object aggregation) to ns-3 nodes, so the mobility model pointer can usually be easily obtained from a node pointer using ``GetObject()``.

.. sourcecode:: cpp

  // Periodic position printing
  void PrintPositions(NodeContainer nodes) {
      for (auto i = nodes.Begin(); i != nodes.End(); ++i) {
          Ptr<MobilityModel> mobility = (*i)->GetObject<MobilityModel>();
          Vector pos = mobility->GetPosition();
          std::cout << "Node " << (*i)->GetId() << ": " << pos << std::endl;
      }
      Simulator::Schedule(Seconds(1.0), &LogPositions, nodes);
  }

Common Use Cases
~~~~~~~~~~~~~~~~

**Wireless Network Evaluation**

- **Scenario**: Mobile ad-hoc networks (MANETs)
- **Models**: RandomWaypoint, RandomWalk2d
- **Considerations**: Realistic speed distributions, pause times

**Vehicular Networks**

- **Scenario**: Vehicle-to-vehicle (V2V) communication
- **Tools**: SUMO integration
- **Models**: Trace-based mobility from traffic simulators

**Sensor Networks**

- **Scenario**: Environmental monitoring
- **Models**: ConstantPosition with occasional RandomWalk
- **Considerations**: Energy-efficient movement patterns

**Satellite Communications**

- **Scenario**: Low Earth Orbit (LEO) constellations
- **Models**: GeocentricConstantPosition or custom orbital models
- **Coordinates**: Geocentric Cartesian system

Examples and Tests
------------------

The following example programs demonstrate mobility usage:

- ``main-random-topology.cc`` - Random initial placement
- ``main-random-walk.cc`` - Random walk mobility
- ``main-grid-topology.cc`` - Grid-based positioning
- ``ns2-mobility-trace.cc`` - |ns2| trace file parsing
- ``reference-point-group-mobility-example.cc`` - Group mobility demonstration

ns2-mobility-trace Example
~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``ns2-mobility-trace.cc`` program is an example of loading an |ns2| trace file that specifies the movements of two nodes over 100 seconds of simulation time. It is paired with the file ``default.ns_movements``.

The program behaves as follows:

- A Ns2MobilityHelper object is created, with the specified trace file.
- A log file is created, using the log file name argument.
- A node container is created with the number of nodes specified in the command line. For this particular trace file, specify the value 2 for this argument.
- The Install() method of Ns2MobilityHelper to set mobility to nodes. At this moment, the file is read line by line, and the movement is scheduled in the simulator.
- A callback is configured, so each time a node changes its course a log message is printed.

Example usage:

.. sourcecode:: bash

  $ ./ns3 run "ns2-mobility-trace \
  --traceFile=src/mobility/examples/default.ns_movements \
  --nodeNum=2 \
  --duration=100.0 \
  --logFile=ns2-mob.log"

Sample log file output:

.. sourcecode:: text

  +0.0ns POS: x=150, y=93.986, z=0; VEL:0, y=50.4038, z=0
  +0.0ns POS: x=195.418, y=150, z=0; VEL:50.1186, y=0, z=0
  +104727357.0ns POS: x=200.667, y=150, z=0; VEL:50.1239, y=0, z=0
  +204480076.0ns POS: x=205.667, y=150, z=0; VEL:0, y=0, z=0

bonnmotion-ns2-example
~~~~~~~~~~~~~~~~~~~~~~

The ``bonnmotion-ns2-example.cc`` program, which models the movement of
a single mobile node for 1000 seconds of simulation time, has a few
associated files:

- ``bonnmotion.ns_movements`` is the |ns2|-formatted mobility trace
- ``bonnmotion.params`` is a BonnMotion-generated file with some metadata about the mobility trace
- ``bonnmotion.ns_params`` is another BonnMotion-generated file with ns-2-related metadata.

Neither of the latter two files is used by |ns3|, although they are generated
as part of the BonnMotion process to output ns-2-compatible traces.

The program ``bonnmotion-ns2-example.cc`` will output the following to stdout:

.. sourcecode:: text

  At 0.00 node 0: Position(329.82, 66.06, 0.00);   Speed(0.53, -0.22, 0.00)
  At 100.00 node 0: Position(378.38, 45.59, 0.00);   Speed(0.00, 0.00, 0.00)
  At 200.00 node 0: Position(304.52, 123.66, 0.00);   Speed(-0.92, 0.97, 0.00)
  At 300.00 node 0: Position(274.16, 131.67, 0.00);   Speed(-0.53, -0.46, 0.00)
  At 400.00 node 0: Position(202.11, 123.60, 0.00);   Speed(-0.98, 0.35, 0.00)
  At 500.00 node 0: Position(104.60, 158.95, 0.00);   Speed(-0.98, 0.35, 0.00)
  At 600.00 node 0: Position(31.92, 183.87, 0.00);   Speed(0.76, -0.51, 0.00)
  At 700.00 node 0: Position(107.99, 132.43, 0.00);   Speed(0.76, -0.51, 0.00)
  At 800.00 node 0: Position(184.06, 80.98, 0.00);   Speed(0.76, -0.51, 0.00)
  At 900.00 node 0: Position(250.08, 41.76, 0.00);   Speed(0.60, -0.05, 0.00)

The motion of the mobile node is sampled every 100 seconds, and its position
and speed are printed out.  This output may be compared to the output of
a similar |ns2| program (found in the |ns2| ``tcl/ex/`` directory of |ns2|)
running from the same mobility trace.

The next file is generated from |ns2| (users will have to download and
install |ns2| and run this Tcl program to see this output).
The output of the |ns2| ``bonnmotion-example.tcl`` program is shown below
for comparison (file ``bonnmotion-example.tr``):

.. sourcecode:: text

  M 0.00000 0 (329.82, 66.06, 0.00), (378.38, 45.59), 0.57
  M 100.00000 0 (378.38, 45.59, 0.00), (378.38, 45.59), 0.57
  M 119.37150 0 (378.38, 45.59, 0.00), (286.69, 142.52), 1.33
  M 200.00000 0 (304.52, 123.66, 0.00), (286.69, 142.52), 1.33
  M 276.35353 0 (286.69, 142.52, 0.00), (246.32, 107.57), 0.70
  M 300.00000 0 (274.16, 131.67, 0.00), (246.32, 107.57), 0.70
  M 354.65589 0 (246.32, 107.57, 0.00), (27.38, 186.94), 1.04
  M 400.00000 0 (202.11, 123.60, 0.00), (27.38, 186.94), 1.04
  M 500.00000 0 (104.60, 158.95, 0.00), (27.38, 186.94), 1.04
  M 594.03719 0 (27.38, 186.94, 0.00), (241.02, 42.45), 0.92
  M 600.00000 0 (31.92, 183.87, 0.00), (241.02, 42.45), 0.92
  M 700.00000 0 (107.99, 132.43, 0.00), (241.02, 42.45), 0.92
  M 800.00000 0 (184.06, 80.98, 0.00), (241.02, 42.45), 0.92
  M 884.77399 0 (241.02, 42.45, 0.00), (309.59, 37.22), 0.60
  M 900.00000 0 (250.08, 41.76, 0.00), (309.59, 37.22), 0.60

The output formatting is slightly different, and the course change
times are additionally plotted, but it can be seen that the position
vectors are the same between the two traces at intervals of 100 seconds.

The mobility computations performed on the |ns2| trace file are slightly
different in |ns2| and |ns3|, and floating-point arithmetic is used,
so there is a chance that the position in |ns2| may be slightly
different than the respective position when using the trace file
in |ns3|.

reference-point-group-mobility-example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The reference point group mobility model [1]_ is demonstrated in the example program ``reference-point-group-mobility-example.cc``. This example runs a short simulation that illustrates a parent WaypointMobilityModel traversing a rectangular course within a bounding box, and three member nodes independently execute a two-dimensional random walk around the parent position, within a small bounding box.

The example illustrates configuration using the GroupMobilityHelper and manual configuration without a helper; the configuration option is selectable by command-line argument.

The example outputs two mobility trace files, a course change trace and a time-series trace of node position. The latter trace file can be parsed by a Bash script (``reference-point-group-mobility-animate.sh``) to create PNG images at one-second intervals, which can then be combined using an image processing program such as ImageMagick to form a basic animated gif of the mobility.

Tests
~~~~~

Specific validation results and test cases are documented in the |ns3| test suite under ``src/mobility/test/``.

Validation
----------

No formal validation has been done.

References
----------

.. [1] T. Camp, J. Boleng, V. Davies. "A survey of mobility models for ad hoc network research",
   in Wireless Communications and Mobile Computing, 2002: vol. 2, pp. 2483-2502.

.. [2] 3GPP. 2018. TR 38.811, Study on New Radio (NR) to support non-terrestrial networks, V15.4.0. (2020-09).

.. [3] BonnMotion documentation, https://sys.cs.uos.de/bonnmotion/doc/README.pdf
