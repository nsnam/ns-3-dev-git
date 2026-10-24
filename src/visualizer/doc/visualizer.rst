.. include:: replace.txt
.. highlight:: python

.. heading hierarchy:
   ============= Chapter
   ------------- Section (#.#)
   ~~~~~~~~~~~~~ Subsection (#.#.#)


PyViz visualizer
================

PyViz is a live visualizer for |ns3| simulations, originally created by
Gustavo Carneiro [`1`_] and rewritten for web browsers by Gabriel Ferreira.
It shows the topology, the packets moving through it and the state of every
object while the simulation runs, without trace files. It runs in a web
browser and only needs the |ns3| Python bindings; the browser page is served
by the Python process that runs the simulation, from ``127.0.0.1``, with no
external resources, so it works offline.

The source code lives in ``src/visualizer``. The visualizer has three layers:


* ``model/pyviz.{h,cc}`` (C++): the ``ns3::visualizer::PyViz`` helper. It
  steps the simulator, pairs each packet transmission with its receptions,
  and keeps transmission, drop, packet and interface statistics samples. It
  only links against the ``core`` and ``network`` modules: its trace
  handlers are generic (a device sent a packet, a device received a packet,
  a node dropped a packet).
* ``visualizer/*.py`` (Python): everything else. ``traces.py`` connects the
  generic handlers to the ``MacTx`` and ``MacRx`` trace sources (signature
  ``ns3::Packet::TracedCallback``) found by introspecting the TypeId of each
  device, or of the objects its Pointer attributes refer to (e.g. ``Mac`` of
  Wi-Fi and LR-WPAN devices), so devices that follow this convention need no
  visualizer code. It also connects a few other trace sources, skipping
  modules that are not built, and compiles small C++ adapters at run time
  (with Cppyy) for trace sources with other signatures (IPv4 drops, NR device
  transmissions, the LTE eNB and NR gNB applications). ``web.py`` paces the simulation, computes the topology,
  layout and rates, serves the page and implements every user action.
  ``web_plugins/`` holds the panels and overlays.
* ``visualizer/web`` (HTML, CSS, JavaScript): presentation only. The page
  renders the JSON snapshots it receives and posts user actions back.

The Python process runs the simulator in short steps, each covering the
wall-clock time elapsed since the previous step multiplied by the chosen
speed, so the speed is in simulated seconds per wall-clock second. The page
polls about 30 times per second and receives what the steps since its
previous poll recorded; steps keep running while the page draws. One step
spends at most about 20 ms running the simulator: when a simulation cannot
keep up with the requested speed it runs slower, the page stays responsive,
and the status line shows the speed actually achieved.

Scope and Limitations
---------------------

* A packet is paired with a device reception to be drawn, so a transmission
  nobody receives (for example, on a frequency no receiver is tuned to) has no
  arrow. Once a node has transmitted for 20 ms of simulated time with no
  reception observed, a dashed stub ending in a cross leaves it, towards its wired
  peer or nearest neighbour, labelled with the rate. The direction is only a hint:
  the visualizer does not know the intended receiver.
* Channel corruption is reported for point-to-point and CSMA devices with a receive
  error model. A wireless packet that is lost shows up only as a stub, and only for
  a sender with no reception at all in the smoothing window. TTL expiry is reported for IPv4.
* The visualizer runs Python-driven simulations (or C++ programs through
  ``VisualSimulatorImpl``, which starts it from embedded Python), so the
  Python bindings must be enabled.
* Wired links are drawn from point-to-point and CSMA channels; channels with
  more than two devices are drawn as a hub. Wireless devices produce no
  links; their traffic appears as transmission arrows, Wi-Fi stations
  are joined to their access point by a dashed line, and LTE and NR UEs to
  their serving cell. Nodes without a
  mobility model are placed by an automatic layout.
* Transmissions are displayed for every device with ``MacTx`` and ``MacRx``
  trace sources (Wi-Fi, LR-WPAN, CSMA, point-to-point,
  ``VirtualNetDevice``, ``FdNetDevice``, ...), and for
  LTE and NR (with EPC) devices, which have dedicated adapters. Other devices
  can be added in ``traces.py``. UAN devices are not supported.
* Rates are averaged over a window of simulated time, 0.1 s per step of the
  **Smooth** slider, idle time included. A window that is not full yet counts as
  full, so rates rise to their value at first.
* Mesh devices report receptions only at the final destination, so only the last
  hop of a mesh path has an arrow, and their forwarding is not reported as lost.
* Scenarios that install Wi-Fi mesh devices must call
  ``ns.Packet.EnablePrinting()`` first, because the installation creates packets
  and the visualizer can only enable packet metadata before any packet exists.
* Packet arrows are slowed down so they can be followed: each hop is shown
  for at least 0.7 s, starting at its real offset in time. At high speeds
  only a sample of the packets is drawn.
* The flow monitor panel follows the definitions of the ``FlowMonitor``
  defaults, which it does not read from a FlowMonitor the scenario
  configures: histogram bin widths of 1 ms (delay and jitter) and 20 bytes,
  and packets are lost when not received within 10 s of last being sent or
  forwarded (``MaxPerHopDelay``).
* Objects created with a Python constructor (for example
  ``ns.CsmaNetDevice()`` instead of ``ns.CreateObject[ns.CsmaNetDevice]()``)
  report their base TypeId in the object tree.

.. _sec-pyviz-tour:

A tour of the view
------------------

``src/visualizer/examples/pyviz-tour.py`` puts most of what the visualizer draws in
one scenario. Run it and press **Play**:

.. sourcecode:: bash

  $ ./ns3 run src/visualizer/examples/pyviz-tour.py

.. _fig-pyviz-tour:

.. figure:: figures/pyviz-tour.*

   The tour: one cause of loss per row on the left, moving Wi-Fi stations in their
   box on the right, and a mesh below them.

The legend in the corner of the topology summarizes the symbols, and its title
collapses it. They mean:

Wired connections
  Solid grey lines are point-to-point links. A white dot joins the devices of a
  shared channel, such as a CSMA bus. In the tour, each row on the left is a wired
  chain, and the access point, router and HTTP server are wired too.

Wireless connections
  Wireless devices have no fixed link. Dashed red lines join Wi-Fi stations to
  their access point, and dashed blue lines join LTE and NR UEs to their serving
  cell. These lines show the association, not traffic; ad hoc nodes have none.

Traffic rates
  A green arrow is traffic that a device received. There is one per direction, on the
  right-hand side of the link, thicker for higher rates and, when the link is long
  enough, labelled with the rate. Rates are measured at the device, so they include
  headers, and are averaged over the time set with **Smooth**. Zoom in to make room
  for labels on short links.

Packets in flight
  Yellow arrows move along a link from the sender to the receiver. Each hop is slowed
  down to last at least 0.7 s, so it can be followed, and at high speeds only a sample
  of the packets is drawn. A packet is drawn once it has been received, so these
  arrows are a replay of the hop, not its exact position at the current time.

Packet drops
  An envelope flies off the top of the node that discarded a packet, and its color
  gives the reason: orange for a full queue (first row of the tour), red for
  corruption in the channel, discarded by the receiver (second row), blue for an
  expired TTL (third row) and grey for anything else, such as no route (fourth row).
  It is labelled with the packet size. When a node drops too many to read, it shows
  the rate of each reason instead, such as ``queue full 128.66 kbit/s``.

Transmissions nobody received
  A dashed red stub ending in a cross leaves a node that transmits while none of its
  packets are received, with a moving packet that fades out and the rate. In the tour,
  the two nodes at the bottom are tuned to different Wi-Fi channels. The stub points
  to the node's wired peer or nearest neighbour, which is only a hint.

Movement and trails
  Nodes move as their mobility model says. In the tour, the HTTP clients bounce off
  the walls of their box. Selecting a node whose model has bounds draws them as a
  dashed rectangle (the tour selects its clients at the start). **Trails** draws the
  recent path of moving nodes, and is disabled when no mobility model can move a node.

Node appearance
  Nodes with a mobility model are red and others green; the selected nodes have a
  yellow border. **Size** scales all nodes, and scenarios can set the size, color,
  label and icon of each node (see `Customizing the view`_). Nodes with an energy
  source show a battery with its charge.

Visualizer General GUI options
------------------------------

.. _fig-pyviz-start:

.. figure:: figures/pyviz-start.*

   The page right after start. The simulation is paused, and starts at the
   slowest speed (0.01x) so the first events can be seen.

The toolbar is a single row:

* **Pause** / **Play**: one button that pauses and resumes the simulation.
  The simulation starts paused, also after a restart, so panels such as the
  flow monitor can be enabled before any traffic is sent.
* **Restart**: destroys the simulation and reruns the scenario script from
  the beginning. A C++ program is executed again instead (Linux only); it
  serves on the same port, so the page reconnects to it.
* **Exit**: runs the rest of the simulation without the visualizer and
  returns from ``visualizer.start()`` (or ``Simulator::Run()``), so the code
  that follows it, such as writing statistics or flushing traces, runs as
  usual. The page stops updating.
* **Speed**: from 0.01x to 100x simulated seconds per wall-clock second. The
  page starts at 0.01x. When the simulation cannot keep up, the status line
  shows the speed actually reached, for example ``running at 1.32x``.
* **Smooth**: how much simulated time rates are averaged over, 0.1 s per step.
* **Traffic**: draw the transmissions of all nodes, of the selected nodes
  only, or none.
* **Size** (node size), **Trails**, **Fit** (fit the view to the scenario)
  and **Screenshot** (save the topology as SVG).
* **Panels...**: windows that are not tied to one node, such as the flow
  monitor and the energy overview.
* The status line: simulation state, simulated time and number of nodes.

On narrow windows the control labels give way to their tooltips, so the
toolbar stays on one row.

.. _fig-pyviz-overview:

.. figure:: figures/pyviz-overview.*

   Running at 1x. Green arrows are transmissions, labelled with their rate
   and drawn on the right-hand side of the link for each direction. Yellow
   arrows are packets. Envelopes fly off the top of the node that discarded a
   packet, labelled with their size: orange for a full queue, red for corruption in
   the channel, blue for an expired TTL and grey for other reasons. When there are
   too many to read, the node shows the smoothed rate of each cause instead. The
   legend in the corner of the topology explains every symbol. Nodes with an energy source show a battery.

In the topology:

* Scroll to zoom, drag the background to pan, drag a node to move it (nodes
  with a mobility model are moved in the simulation too, kept inside the
  model's ``Bounds`` when it has them). While dragging, the position under
  the pointer is shown as Cartesian coordinates ``(x, y, z)`` in metres; z
  is kept.
* Selecting a node whose mobility model has ``Bounds`` (random walk, random
  direction, Gauss-Markov, ...) draws that area as a dashed rectangle.
* Hover a node for a summary: name, mobility model, position, devices,
  addresses and battery.
* Nodes with mobility are drawn in red, the others in green. A white dot is
  a shared channel (CSMA bus). Dashed red lines join Wi-Fi stations to their
  access point, and dashed blue lines join LTE and NR UEs to their serving
  eNB or gNB.
* Nodes with an energy source (energy module) show a battery: its fill and
  colour follow the remaining charge, the percentage is written next to it,
  and hovering it shows the remaining and total charge in mAh and J.
  Clicking it opens the node's **Battery** panel.
* **Trails** draws the recent path of every moving node, brighter towards
  its current position.

.. _fig-pyviz-mobility-bounds:

.. figure:: figures/pyviz-mobility-bounds.*

   A selected Wi-Fi station and the bounds of its random walk (dashed). The
   other station walks in the same area.

.. _fig-pyviz-drag-coordinates:

.. figure:: figures/pyviz-drag-coordinates.*

   Dragging the station: the label shows the position under the pointer.
   Here the pointer is outside the station's bounds (x starts at 110 m), so
   when released the station is placed at the nearest point inside them.

.. _fig-pyviz-trails:

.. figure:: figures/pyviz-trails-battery.*

   Trails of the two Wi-Fi stations, zoomed in. Their batteries are at 28%.

Inspecting objects
~~~~~~~~~~~~~~~~~~

Click a node to select it. Shift-click or Ctrl-click adds nodes to the
selection or removes them; panels such as the flow monitor use the whole
selection.

The **Node** tab shows the selected node as a tree of objects: its devices
and their channels and queues, its applications, and the objects aggregated
to it (IP stack, mobility model, routing, ...). Expanding an entry loads
it. Each object lists its identifying values (IDs, addresses, MTU, IP
addresses) and its attributes; writable attributes can be changed with
**Apply**, and ns-3 validates the new value.

.. _fig-pyviz-object-tree:

.. figure:: figures/pyviz-object-tree.*

   The object tree of the Wi-Fi access point, with its device expanded.

Panels
~~~~~~

Right-click a node for its panels, or use the buttons in the **Node** tab:

* **Node summary**: devices, MAC and IP addresses, mobility and battery.
* **Interface statistics**: packet and byte counters and rates per device.
* **IPv4 routing table**: static and global routes.
* **OLSR routing table**: when the node runs OLSR.
* **Last packets**: the last packets sent, received and dropped, with their
  decoded headers. Sent packets show the address of the device that
  received them (the broadcast address if several did), and received
  packets the address of the device that sent them. The checklist selects which headers to capture, combined
  with OR or AND.
* **Follow node**: keeps the node centred while it moves.

.. _fig-pyviz-node-menu:

.. figure:: figures/pyviz-node-menu.*

   The node menu.

Panels open as windows over the topology. Drag them by the title bar, click
one to bring it to the front, and use **Minimize** to send it to the taskbar
in the top left corner of the topology (where panels, which open on the
right, do not cover it); clicking its button restores it. Minimized windows
stop refreshing.

.. _fig-pyviz-panels:

.. figure:: figures/pyviz-panels.*

   Two panel windows side by side, a third one minimized to the taskbar in
   the top left corner.

Theme
~~~~~

The page follows the light or dark theme chosen in the system or the browser.
The button at the right end of the toolbar switches theme, and the browser
remembers that choice.

Types and console
~~~~~~~~~~~~~~~~~

The **Types** tab lists the TypeIds of the objects in the simulation with their
attributes, trace sources and the methods Cppyy exposes; unchecking **Only
types used by this simulation** lists every registered TypeId. Attributes that can be set at
construction time can be given a new default for objects created afterwards
(like ``Config::SetDefault()``), so this has no effect on objects that already
exist.

.. _fig-pyviz-types:

.. figure:: figures/pyviz-types.*

   The Types tab.

The **Console** tab is optional and meant for scripting: it runs Python in the
simulation process, to read or change anything the other tabs do not expose.
``ns``, ``viz`` (the visualizer), ``node`` (the primary selected node) and
``nodes`` (all selected nodes) are defined. Shift+Enter runs the input, and the
tab has examples that can be clicked. For example, this reads the simulation
time and colors the selected node::

  ns.Simulator.Now().GetSeconds()
  viz.set_node_color(node.GetId(), "#ff00ff")

.. _fig-pyviz-console:

.. figure:: figures/pyviz-console.*

   The console, with the selected node available as ``node``.

The sidebar's **Minimize** button gives the topology the whole width; the
sidebar then waits in the taskbar like a minimized window.

.. _fig-pyviz-sidebar-hidden:

.. figure:: figures/pyviz-sidebar-hidden.*

   The sidebar minimized to the taskbar.

Energy GUI options
------------------

The **Battery** panel of a node (click its battery, or use the node menu)
plots its charge and remaining energy over time and lists its energy
sources, its drainers (the device energy models attached to them, with their
current draw and the energy they consumed) and its charge sources (energy
harvesters, with their power). History is sampled at most every 0.1 s of
simulated time; at high speeds, once per simulation step.

.. _fig-pyviz-battery-panel:

.. figure:: figures/pyviz-battery-panel.*

   The Battery panel of a Wi-Fi station. The radio's energy model switched
   the radio off at 28% here, so the charge stays flat afterwards.

**Panels... > Energy** summarizes the network: the energy at the start, now,
and consumed, the charge of each node over time, the total remaining energy
over time, and every source, drainer and harvester.

.. _fig-pyviz-energy-panel:

.. figure:: figures/pyviz-energy-panel.*

   The Energy panel.

Flow Monitor
------------

The flow monitor panel plots, per flow, the throughput, the latency and the
jitter (means over each 0.1 s of simulated time), and the total bytes sent
and received. It is optional: enable it from **Panels... > Flow monitor >
Enable flow monitor**, or from the start in the scenario::

  visualizer.enable_flow_monitor()

The visualizer collects these statistics itself, for IPv4 and IPv6 TCP and
UDP flows, with the definitions of the :ref:`flow-monitor` module, from the
time the panel is enabled. It does not
use a FlowMonitor, so a FlowMonitor the scenario installs, in Python or C++,
keeps reporting its own statistics undisturbed.

The **View** control switches between these time series and histograms of
the delay, the jitter and the received packet size of each flow, with the
bin widths of the ``FlowMonitor`` defaults (1 ms, 1 ms and 20 bytes). The
time series span the simulation from 0 s to its current time; samples are
taken every 0.1 s of simulated time, whatever the speed.

With nodes selected, only the flows to or from them are shown, so selecting
the two ends of a connection shows just that traffic. Up to eight flows are
plotted, those with the most bytes received; all of them are listed in the
table.

.. _fig-pyviz-flow-monitor:

.. figure:: figures/pyviz-flow-monitor.*

   The flows between two selected nodes.

Hover a chart to read the values at that time. Drag across a chart to zoom
into that time range; **Reset zoom** or a double-click zooms back out.
Histograms zoom the same way into a range of values; wide ranges are drawn
with merged bins, and zooming in shows the original bins again.

.. _fig-pyviz-flow-monitor-zoom:

.. figure:: figures/pyviz-flow-monitor-zoom.*

   A zoomed throughput chart.

Usage
-----

This section describes how to build the visualizer, how to run a
simulation with it, and how scenarios can customize the view or extend it
with plugins.

Building
~~~~~~~~

.. note::
   The visualizer needs the |ns3| Python bindings, even for C++ programs. Set
   them up before anything else: the bindings need Cppyy, normally installed in
   a Python virtual environment from which |ns3| is then configured and run (see
   the Python section of the installation guide for supported versions).

.. sourcecode:: bash

  $ python3 -m venv ns3-venv
  $ source ns3-venv/bin/activate
  $ pip install cppyy==3.5.0
  $ ./ns3 configure --enable-python-bindings
  $ ./ns3 build

The visualizer module is built whenever the bindings are enabled. It needs
no other package: GTK, GooCanvas, PyGObject, GraphViz and IPython, which
earlier versions required, are no longer used.

Running
~~~~~~~

In a Python scenario, build the topology and applications as usual and end
with ``visualizer.start()`` instead of ``ns.Simulator.Run()``::

  from ns import ns
  import visualizer

  # ... create nodes, devices, stacks, applications ...
  ns.Simulator.Stop(ns.Seconds(10))
  visualizer.start()

Then run the script, for example with ``./ns3 run scratch/my-scenario.py``.
It prints the address of the page (``PyViz is available at
http://127.0.0.1:.../#token=...``) and opens it in the default browser. The
simulation advances while the page is open and stops at
``Simulator::Stop()``. Loading the bindings and preparing the visualizer can
take several seconds before the page appears, and longer for large scenarios.

The server only listens on 127.0.0.1, and every request except those for the
page and its static assets must carry a random token generated for each run.
The token appears only in that printed URL, so only whoever holds the URL can
drive the simulation or use its Python console. To open the page in another
browser, copy the whole URL, including the ``#token=`` part.

C++ programs, and Python programs that call ``ns.Simulator.Run()``, can be
visualized by selecting ``VisualSimulatorImpl``, which starts the visualizer
when the simulation runs:

.. sourcecode:: bash

  $ ./ns3 run 'csma-bridge.py --SimulatorImplementationType=ns3::VisualSimulatorImpl'
  $ ./ns3 run wifi-simple-adhoc --visualize

Customizing the view
~~~~~~~~~~~~~~~~~~~~

Scenarios can register code that runs when the visualizer starts::

  import visualizer

  def hook(viz):
      viz.set_node_color(0, "#ff00ff")
      viz.set_node_label(1, "router")
      viz.set_node_size(2, 2.5)
      viz.set_node_icon(3, "tower.svg")

  visualizer.add_initialization_hook(hook)
  visualizer.set_bounds(-10, -40, 160, 40)  # initial view, in metres
  visualizer.set_speed(0.5)  # initial playback speed
  visualizer.skip_to(2.0)  # start showing at 2 s of simulated time

``set_speed()`` lets a scenario suggest the speed that suits it, and ``skip_to()``
runs past an uninteresting start without animating it (before the page opens, so
a long skip delays it). Both also apply after **Restart**, except that the speed
set in the page is kept.

The same can be set without editing the program, for C++ programs too, with the
``PyVizSpeed`` and ``PyVizStartTime`` global values, given in simulated seconds per
wall-clock second and simulated seconds. They take precedence over the scenario, and
need the program to parse its command line with ``CommandLine``:

.. sourcecode:: bash

  $ ./ns3 run "wifi-simple-adhoc-grid --PyVizSpeed=1 --PyVizStartTime=5" --visualize

Plugins
~~~~~~~

A plugin is a function that receives the visualizer object; built-in ones
are in ``visualizer/web_plugins``. Plugins return plain data, which the page
renders::

  def my_plugin(viz):
      viz.add_node_panel(
          "queue", "Queue length",
          lambda viz, node: {"sections": [{"title": "Queue", "items": [["length", "3"]]}]},
          applies=lambda viz, node: node == 0,
      )

  visualizer.register_plugin(my_plugin)

The visualizer object provides:

* ``add_node_panel(id, title, build, action=None, applies=None)`` and
  ``add_panel(id, title, build, action=None)``: windows built from sections
  (``{"title", "items": [[name, value], ...]}``, ``{"title", "columns",
  "rows"}`` tables and ``{"title", "plot": {"unit", "series"}}`` charts) and
  controls (``checklist``, ``radio``, ``select`` and ``button``), whose
  values are passed to ``action``.
* ``add_overlay(callback)``: extra links drawn over the topology.
* ``add_node_info(callback)``: values merged into each node's snapshot, as
  the battery indicator does.
* ``add_tooltip_info(callback)``: lines added to the node summary.
* ``add_periodic_update(callback)``: called after each simulation step.
* ``add_step_guard(callback)``: holds the simulation while it returns True.
* ``selected_nodes``, ``set_node_color()`` and the other appearance setters.

Examples and Tests
------------------

``src/visualizer/test/pyviz-web-test.py`` tests every feature: the HTTP
API, simulation control and pacing, topology, transmissions of each
wireless technology, drops, the object tree and attributes, all panels,
the flow monitor, restart, ``VisualSimulatorImpl``, and, with
Selenium and Chrome or Chromium, the page itself:

.. sourcecode:: bash

  $ pip install selenium
  $ ./ns3 run "src/visualizer/test/pyviz-web-test.py -v"

The browser tests are skipped when Selenium or a browser is missing, unless
``--require-selenium`` is given. ``--chrome-binary``, ``--chromedriver`` and
``--webdriver-url`` select the browser and driver (``--help-options`` lists
them); other arguments, such as test names, go to ``unittest``. The weekly
``weekly-pyviz`` CI job runs the whole suite.

The tour (see :ref:`sec-pyviz-tour`) shows most of what the visualizer can draw.

Scenarios that show the visualizer well have few nodes, traffic from the start
and, ideally, mobility. Good first ones are ``wifi-simple-adhoc-grid`` (run with
``--visualize``), ``src/bridge/examples/csma-bridge.py`` and the test scenario below.

The test scenario, ``src/visualizer/test/pyviz-test-scenario.py``, also runs
on its own and is a compact tour of the visualizer (wired and wireless links,
a shared channel, drops, batteries and mobility):

.. sourcecode:: bash

  $ ./ns3 run "src/visualizer/test/pyviz-test-scenario.py --stopTime=60 --flowMonitor"

Validation
----------

No formal validation has been done. The test suite described in the
previous section checks that every feature works.

References
----------

.. _`1`:

[1] G. Carneiro, "Transparent metropolitan vehicular network: design and fast
prototyping methodology", PhD thesis, Faculdade de Engenharia da Universidade
do Porto, 2012.
