#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Standard-library web front end for live ns-3 simulations.

All simulation logic lives in Python (this module and ``web_plugins``). The
browser only renders the JSON it receives and posts user actions back.

How it runs
-----------

``start()`` is called by the scenario after it builds its topology (or by
``VisualSimulatorImpl`` when a C++ program runs with the visualizer). It
attaches a ``Visualizer`` to the simulation, then blocks in an HTTP server
bound to 127.0.0.1 on a random port. Exit on the toolbar runs the rest of the
simulation without the visualizer and returns from ``start()``, so the code
after Simulator::Run() (writing statistics, flushing traces) runs as usual.

The simulation is advanced in steps, each running the simulator for the
wall-clock time elapsed since the previous step multiplied by ``speed``
(capped at ``MAX_STEP_WALL``), so ``speed`` is simulated seconds per
wall-clock second regardless of how often the page polls or how long a step
takes. Steps run while the server waits for the next poll of
``/api/state``, so a simulation that cannot keep up does not idle while the
page draws; each poll returns what the steps since the previous one
recorded. Pausing is just "poll without advancing".

Threading
---------

The server is single threaded and runs on the thread that called
``start()``, which is also the thread that built the topology. ns-3 is not
thread safe, and Simulator::Run() records its caller as the main thread:
Schedule() then asserts when called from any other thread, so every request
(stepping, enabling the flow monitor, setting attributes, the console) must
run on that one thread.

``Visualizer.lock`` (re-entrant) is kept so plugins or user threads that touch
ns-3 can serialize with request handling.

HTTP API
--------

GET  /                        page (index.html)
GET  /app.js, /style.css      static assets from ./web
GET  /favicon.ico, /ns-3-logo.png  ns-3 logo
GET  /api/state               advance one step, return the scene snapshot
GET  /api/types               TypeId catalogue (attributes, traces, methods)
GET  /api/objects?node=N      object tree root for node N
GET  /api/objects?id=X        one object of the tree, expanded lazily
GET  /api/panels[?node=N]     panels available for a node, or global panels
GET  /api/panel?[node=N&]id=P contents of one panel
GET  /api/tooltip?node=N      extra tooltip lines from plugins
GET  /api/icon?node=N         custom node icon set with set_node_icon()
POST /api/control             play/pause/stop/exit/restart/speed/select/move...
POST /api/panel-action        values from a panel's controls
POST /api/console             run Python in the visualizer process
POST /api/object-attribute    set an attribute on a live object
POST /api/default-attribute   set an attribute default for new objects

Every request but the page and its static assets must carry the session
token in the ``X-PyViz-Token`` header (see ``_Handler._authorized``). The
token is only handed out in the fragment of the URL that ``start()`` opens
and prints, so neither other local processes nor web pages can read it from
the server.
"""

import code
import contextlib
import html
import io
import json
import math
import mimetypes
import os
import re
import runpy
import secrets
import select
import shutil
import socket
import sys
import tempfile
import threading
import time
import types
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

from ns import ns

from .traces import connect_traces

# Upper bound on the wall-clock time one step catches up on, so a stalled
# page (hidden tab, slow request) does not cause one huge jump afterwards.
## Maximum wall-clock seconds one step catches up on
MAX_STEP_WALL = 0.5

## Simulated seconds of rate averaging per step of the Smooth slider
RATE_WINDOW_UNIT = 0.1
## Maximum samples kept per rate history, which bounds the work at slow speeds
RATE_HISTORY_LIMIT = 2000

## Simulated seconds without a reception before a transmission counts as lost
UNRECEIVED_HOLD = 0.02

# Trails keep at most this many points per node, one per TRAIL_MIN_STEP metres
# of movement, so a trail shows the recent path without growing forever.
## Maximum number of points kept per trail
TRAIL_POINTS = 400
## Minimum movement in metres between trail points
TRAIL_MIN_STEP = 0.5

# Wall-clock compute time one step may spend running the simulator. When the
# simulation cannot keep up with the requested speed, steps are shortened to
# fit this budget instead of growing (which would make every poll slower and
# the page unresponsive); the effective speed is reported in the snapshot.
## Wall-clock compute budget in seconds for one step
STEP_BUDGET = 0.02
# Compute time accumulated over steps before the simulation rate is updated.
# Steps are often far shorter than a timer can resolve; measuring only the
# long ones would underestimate the rate, and the shorter steps it then
# allows would never be measured again.
## Compute time in seconds per simulation rate sample
RATE_SAMPLE = 0.01
# While the page is busy drawing or waiting between polls, the server keeps
# running the simulation in slices, at most one per IDLE_SLICE of wall-clock
# time, so a simulation that cannot keep up is not left idle; the next poll
# returns everything those slices recorded.
## Minimum wall-clock seconds between simulation slices run between polls
IDLE_SLICE = 0.01

# Channels with more than two devices (e.g. a CSMA bus) are drawn as a small
# hub vertex linked to each member, instead of a full mesh of links.
## Draw shared channels as hub vertices
REPRESENT_CHANNELS_AS_NODES = True

# Device type name -> (is_wireless, is_virtual). Wireless and virtual devices
# do not produce wired links in the topology. Unknown types are assumed to be
# non-virtual wireless devices, so new technologies do not draw bogus links.
## Device type name -> (is_wireless, is_virtual)
NETDEVICE_TRAITS = {
    "ns3::PointToPointNetDevice": (False, False),
    "ns3::CsmaNetDevice": (False, False),
    "ns3::WifiNetDevice": (True, False),
    "ns3::BridgeNetDevice": (False, True),
    "ns3::LoopbackNetDevice": (False, True),
    "ns3::MeshPointDevice": (False, True),
    "ns3::LteUeNetDevice": (True, False),
    "ns3::LteEnbNetDevice": (True, False),
    "ns3::NrUeNetDevice": (True, False),
    "ns3::NrGnbNetDevice": (True, False),
    "ns3::lrwpan::LrWpanNetDevice": (True, False),
    "ns3::SixLowPanNetDevice": (True, False),
}

## Directory holding the page assets
_ASSETS = Path(__file__).with_name("web")
# ns-3 logo files, copied from doc/ns3_html_theme/static.
## Binary asset URL path -> content type
_BINARY_ASSETS = {"/favicon.ico": "image/x-icon", "/ns-3-logo.png": "image/png"}
## Paths served without the session token: the page and its static assets
_PUBLIC_PATHS = {"/", "/app.js", "/style.css", *_BINARY_ASSETS}

# Attribute value types that hold other objects. Their values are shown as
# children in the object tree rather than as editable text.
## Value type name of Pointer attributes
_POINTER_TYPE = "ns3::PointerValue"
## Value type name of object container attributes
_CONTAINER_TYPE = "ns3::ObjectPtrContainerValue"

# TypeId::AttributeFlag bits (see attribute.h).
## Attribute flag: readable
_ATTR_GET = 1
## Attribute flag: writable
_ATTR_SET = 2


# ---------------------------------------------------------------------------
# Small helpers
# ---------------------------------------------------------------------------


def _address(obj):
    """Return the address of the C++ object behind a Ptr<>, as hex text.

    Used as a stable identifier for objects in the browser: the same ns-3
    object reached through different paths gets the same id.

    @param obj ns-3 object pointer (Ptr<>).
    @return The object address as hex text.
    """
    # __deref__() reaches the object through the Ptr<>; ns.PeekPointer() would
    # also work, but makes Cppyy print compiler errors the first time.
    return "%x" % ns.cppyy.addressof(obj.__deref__())


def _type_name(obj):
    """Return the most derived TypeId name of an ns-3 Object.

    @param obj ns-3 Object.
    @return The TypeId name.
    """
    return str(obj.GetInstanceTypeId().GetName())


def _node_height(node):
    """Return z from the node's mobility model, or 0 without one.

    @param node ns-3 Node.
    @return The z coordinate.
    """
    mobility = node.GetObject[ns.MobilityModel]()
    return float(mobility.__deref__().GetPosition().z) if mobility else 0.0


def _node_position(node):
    """Return (x, y) from the node's mobility model, or None without one.

    @param node ns-3 Node.
    @return The (x, y) tuple or None.
    """
    mobility = node.GetObject[ns.MobilityModel]()
    if not mobility:
        return None
    # Cppyy returns Ptr<MobilityModel>; __deref__() reaches the object itself.
    position = mobility.__deref__().GetPosition()
    return float(position.x), float(position.y)


def _global_double(name):
    """Read a GlobalValue holding a double.

    @param name GlobalValue name.
    @return The value.
    """
    value = ns.DoubleValue()
    ns.GlobalValue.GetValueByName(name, value)
    return float(value.Get())


def _all_nodes():
    """Return every node of the simulation.

    @return List of ns-3 Nodes.
    """
    return [ns.NodeList.GetNode(i) for i in range(int(ns.NodeList.GetNNodes()))]


def _has_moving_model(node):
    """Tell whether the node's mobility model can move it on its own.

    @param node ns-3 Node.
    @return False without a mobility model or with ConstantPositionMobilityModel.
    """
    mobility = node.GetObject[ns.MobilityModel]()
    if not mobility:
        return False
    return str(mobility.GetInstanceTypeId().GetName()) != "ns3::ConstantPositionMobilityModel"


def _mobility_bounds(model):
    """Return (xMin, yMin, xMax, yMax) of a mobility model's "Bounds" attribute, or None.

    Bounded models (random walk and random direction use a Rectangle,
    Gauss-Markov a Box) keep their node inside this area.

    @param model Mobility model.
    @return The bounds tuple or None.
    """
    for value_type in (ns.RectangleValue, ns.BoxValue):
        value = value_type()
        if model.GetAttributeFailSafe("Bounds", value):
            bounds = value.Get()
            return float(bounds.xMin), float(bounds.yMin), float(bounds.xMax), float(bounds.yMax)
    return None


def _clamp_to_bounds(model, x, y):
    """Keep a position inside a mobility model's bounds, if it has any.

    Bounded models abort when SetPosition() is given a position outside their
    bounds, so a node dragged out of its area is placed at the nearest point
    inside it.

    @param model Mobility model.
    @param x Requested x coordinate.
    @param y Requested y coordinate.
    @return The clamped (x, y) tuple.
    """
    bounds = _mobility_bounds(model)
    if bounds is None:
        return x, y
    return min(max(x, bounds[0]), bounds[2]), min(max(y, bounds[1]), bounds[3])


def _spring_layout(vertices, edges, fixed, iterations=80):
    """Place free vertices with a Fruchterman-Reingold layout; fixed ones stay put.

    @param vertices all vertex keys (node indices and hub ids)
    @param edges list of (a, b) vertex pairs that attract each other
    @param fixed {vertex: (x, y)} for vertices with a known position (nodes
           with mobility); they repel and attract others but never move
    @param iterations number of layout iterations
    @return {vertex: (x, y)} for every vertex
    """
    positions = dict(fixed)
    free = [v for v in vertices if v not in fixed]
    if not free:
        return positions

    # Pick a working area: around the fixed vertices if there are any, so the
    # free ones are laid out at a comparable scale, otherwise one that grows
    # with the number of vertices.
    size, cx, cy = 20.0 * math.sqrt(len(free)) + 20.0, 0.0, 0.0
    if fixed:
        xs = [p[0] for p in fixed.values()]
        ys = [p[1] for p in fixed.values()]
        cx, cy = sum(xs) / len(xs), sum(ys) / len(ys)
        # A small cluster (e.g. a cell of a few meters) must not be dwarfed
        # by the free vertices around it.
        extent = max(max(xs) - min(xs), max(ys) - min(ys))
        if extent > 0:
            size = max(extent, 10.0)

    # Deterministic start on a circle, so the same topology always gets the
    # same layout (no random seed involved).
    for index, vertex in enumerate(free):
        angle = 2 * math.pi * index / len(free)
        positions[vertex] = (cx + size / 2 * math.cos(angle), cy + size / 2 * math.sin(angle))

    k = size / math.sqrt(len(vertices))  # ideal edge length
    temperature = size / 10  # maximum displacement per iteration, cools down
    for _ in range(iterations):
        moves = {v: [0.0, 0.0] for v in free}
        # Every pair of vertices repels with force k^2 / distance.
        for a in free:
            ax, ay = positions[a]
            for b in vertices:
                if a == b:
                    continue
                dx, dy = ax - positions[b][0], ay - positions[b][1]
                dist = math.hypot(dx, dy) or 0.01
                force = k * k / dist
                moves[a][0] += dx / dist * force
                moves[a][1] += dy / dist * force
        # Connected vertices attract with force distance^2 / k.
        for a, b in edges:
            dx, dy = positions[a][0] - positions[b][0], positions[a][1] - positions[b][1]
            dist = math.hypot(dx, dy) or 0.01
            force = dist * dist / k
            for vertex, sign in ((a, -1), (b, 1)):
                if vertex in moves:
                    moves[vertex][0] += sign * dx / dist * force
                    moves[vertex][1] += sign * dy / dist * force
        # Move each free vertex along its net force, capped by the temperature.
        for vertex, (mx, my) in moves.items():
            length = math.hypot(mx, my) or 0.01
            step = min(length, temperature)
            x, y = positions[vertex]
            positions[vertex] = (x + mx / length * step, y + my / length * step)
        temperature *= 0.95
    return positions


class _ModuleView(types.ModuleType):
    """A module whose attributes are those of a globals dictionary, live."""

    def __init__(self, scope):
        """Initializer.

        @param self The object pointer.
        @param scope The globals dictionary of the module.
        """
        super().__init__(scope.get("__name__", "__main__"))
        object.__setattr__(self, "_scope", scope)

    def __getattr__(self, name):
        """Look an attribute up in the globals.

        @param self The object pointer.
        @param name The attribute name.
        @return The global value.
        """
        try:
            return object.__getattribute__(self, "_scope")[name]
        except KeyError:
            raise AttributeError(name) from None

    def __setattr__(self, name, value):
        """Set a global.

        @param self The object pointer.
        @param name The attribute name.
        @param value The new value.
        """
        object.__getattribute__(self, "_scope")[name] = value

    def __dir__(self):
        """List the globals.

        @param self The object pointer.
        @return The global names.
        """
        return sorted(object.__getattribute__(self, "_scope"))


class _ScenarioReady(Exception):
    """Raised in place of Simulator::Run() while Restart reruns the scenario."""


def _stop_scenario_at_run(*_args, **_kwargs):
    """Replace Simulator::Run() during a restart, unwinding the scenario.

    @param _args Ignored positional arguments.
    @param _kwargs Ignored keyword arguments.
    """
    raise _ScenarioReady()


# ---------------------------------------------------------------------------
# Visualizer state
# ---------------------------------------------------------------------------


class Visualizer:
    """State of the web visualizer; also the object handed to plugins and hooks.

    Plugin API:
      - add_node_panel(panel_id, title, build, action=None, applies=None)
      - add_panel(panel_id, title, build, action=None) -> a panel that is not
        tied to one node; build(viz) can use viz.selected_nodes
      - add_overlay(callback) -> callback(viz) returns a list of
        {"source", "target", "class", "title"} links drawn over the topology
      - add_periodic_update(callback) -> called after each simulation step
      - add_step_guard(callback) -> called on every poll; returning True holds
        the simulation without advancing it
      - add_tooltip_info(callback) -> callback(viz, node_index) returns lines
      - add_node_info(callback) -> callback(viz, node_index) returns a dict
        merged into that node's entry in each snapshot (e.g. "battery")
      - set_node_color / set_node_label / set_node_size / set_node_icon

    Registrations made by plugins are cleared on every attach(), because a
    restart rebuilds the simulation and plugins register again.
    """

    ## @var lock
    ## Lock serializing access to the simulation
    ## @var token
    ## Per-process secret required on POST requests
    ## @var sim_helper
    ## C++ PyViz helper owning trace handlers and samples
    ## @var running
    ## Whether the simulation is advancing
    ## @var stopped
    ## Whether the simulation was stopped from the toolbar
    ## @var finished
    ## Whether no events remain or Simulator::Stop() was called
    ## @var exit_requested
    ## Whether Exit was clicked, so start() returns to the scenario
    ## @var held
    ## Whether a step guard currently holds the simulation
    ## @var _last_step_wall
    ## Wall-clock time of the previous step
    ## @var _sim_rate
    ## Moving average of simulated seconds per compute second
    ## @var _rate_sample
    ## Simulated and compute seconds accumulated for the next rate sample
    ## @var effective_speed
    ## Moving average of achieved simulation speed
    ## @var pause_messages
    ## Messages from PyViz::Pause() in simulation code
    ## @var speed
    ## Requested simulated seconds per wall-clock second
    ## @var smoothing
    ## Number of samples averaged for link rates
    ## @var show_transmissions
    ## Transmission display mode: all, selected or none
    ## @var bounds
    ## Initial view [x1, y1, x2, y2], or None
    ## @var show_trails
    ## Whether node trails are recorded and drawn
    ## @var _trails
    ## Node index -> list of recent [x, y] positions
    ## @var selected_node
    ## Primary selected node index, or None
    ## @var selected_nodes
    ## Selected node indices in click order
    ## @var objects
    ## Objects shown in the object tree, by id
    ## @var console_namespace
    ## Globals of the browser Python console
    ## @var _appearance
    ## Node index -> appearance overrides (color, label, size, icon)
    ## @var _seen_flows
    ## Packet flow keys already sent to the page
    ## @var _last_transmissions
    ## Recent (duration, transmissions) samples
    ## @var _last_drops
    ## Recent (duration, drops) samples
    ## @var _pending
    ## Duration, transmissions and drops recorded since the last poll
    ## @var _links
    ## Wired links as (vertex, vertex) pairs
    ## @var _hubs
    ## Hub id -> set of member node indices
    ## @var _layout
    ## Vertex -> (x, y) for nodes without mobility and hubs
    ## @var _panels
    ## Per-node panels by id
    ## @var _global_panels
    ## Simulation-wide panels by id
    ## @var _overlays
    ## Overlay callbacks
    ## @var _periodic
    ## Periodic update callbacks
    ## @var _step_guards
    ## Step guard callbacks
    ## @var _tooltip_info
    ## Tooltip line callbacks
    ## @var _node_info
    ## Node snapshot info callbacks
    ## @var _init_hooks
    ## Scenario initialization hooks as (hook, args)
    ## @var _user_plugins
    ## Plugins registered by the scenario
    ## @var _scenario_frames
    ## Unwound scenario reruns kept alive by restart()
    ## @var restarting
    ## Whether the scenario is being rerun by restart()

    def __init__(self):
        """Initialize the visualizer state.

        @param self The object pointer.
        """
        self.lock = threading.RLock()
        # Random per-process secret required on POST requests.
        # A process re-executed by restart() keeps the token, so the open
        # page can keep sending requests.
        self.token = os.environ.pop(_TOKEN_VARIABLE, None) or secrets.token_urlsafe(32)
        # C++ helper (ns3::visualizer::PyViz) that owns the trace handlers and
        # the transmission, drop and packet samples.
        self.sim_helper = None

        # Simulation control state. "stopped" differs from "paused" only in
        # how the page labels it; "finished" means no events remain or the
        # scenario called Simulator::Stop(), so stepping is pointless. The
        # simulation starts paused, so panels can be set up before it runs.
        self.running = False
        self.stopped = False
        self.finished = False
        self.exit_requested = False
        self.held = False  # a step guard (see add_step_guard) blocks stepping
        self._last_step_wall = None  # wall-clock time of the previous step
        # Moving averages: simulated seconds per second of computation, and
        # simulated seconds per wall-clock second actually achieved.
        self._sim_rate = None
        self._rate_sample = [0.0, 0.0]
        self.effective_speed = None
        self.pause_messages = []  # from PyViz::Pause() in simulation code

        # View options changed from the toolbar.
        # Slowest setting by default, so what happens is visible at once. A
        # re-executed program keeps the speed the page is set to.
        inherited_speed = os.environ.pop(_SPEED_VARIABLE, None)
        self.speed = float(inherited_speed or 0.01)
        # Restarts keep the page's speed instead of the scenario's set_speed().
        self._keep_speed = inherited_speed is not None
        self._skip_target = None  # simulated seconds, from skip_to()
        self.smoothing = 5  # number of samples averaged for link rates
        self.show_transmissions = "all"  # "all", "selected" or "none"
        self.bounds = None  # initial view [x1, y1, x2, y2], from set_bounds()
        # Trails: recent positions of moving nodes, recorded while enabled.
        self.show_trails = False
        self._trails = {}  # node index -> [[x, y], ...]

        # Selection: selected_nodes keeps click order; selected_node is the
        # primary one, whose object tree is shown.
        self.selected_node = None
        self.selected_nodes = []

        # Objects shown in the object tree, by id (see _register()).
        self.objects = {}
        # Globals of the browser console; "node"/"nodes" follow the selection.
        self.console_namespace = {"ns": ns, "viz": self}

        self._appearance = {}  # node index -> {"color", "label", "size", "icon"}

        # Rolling sample history; each entry holds one step's samples.
        self._seen_flows = set()
        self._last_transmissions = []
        self._last_drops = []
        self._last_node_tx = []
        self._unreceived_since = {}  # node index -> simulated seconds
        self._drop_events = []
        self._pending = [0.0, [], [], []]

        # Topology computed once per attach() by _scan_topology().
        self._links = []
        self._can_move = False  # whether a mobility model can move a node
        self._mesh_nodes = set()  # indices of the nodes with mesh devices
        self._hubs = {}  # hub id -> set of member node indices
        self._layout = {}  # vertex -> (x, y) for nodes without mobility and hubs

        # Plugin registrations (see the class docstring).
        self._panels = {}
        self._global_panels = {}
        self._overlays = []
        self._periodic = []
        self._step_guards = []
        self._tooltip_info = []
        self._node_info = []

        # Registered by the scenario through the module-level functions. They
        # survive attach() but are cleared on restart, since the scenario
        # registers them again when it reruns.
        self._init_hooks = []
        self._user_plugins = []

        # Unwound scenario reruns kept alive by restart().
        self._scenario_frames = []
        self.restarting = False
        # Whether restart() asked for the process to be executed again.
        self.reexec_pending = False

    # -- Plugin and scenario API ---------------------------------------------

    def add_node_panel(self, panel_id, title, build, action=None, applies=None):
        """Register a per-node panel.

        @param self The object pointer.
        @param build callable(viz, node_index) returning {"sections": [...],
               "controls": [...]}; a section is either {"title", "columns",
               "rows"}, {"title", "items": [[name, value], ...]} or
               {"title", "plot": {"unit", "series": [...]}}.
        @param action optional callable(viz, node_index, values) for controls.
        @param applies optional callable(viz, node_index) -> bool; the panel is
               offered only for nodes where it returns True.
        @param panel_id Unique panel identifier.
        @param title Panel title.
        """
        self._panels[panel_id] = (title, build, action, applies)

    def add_panel(self, panel_id, title, build, action=None, unavailable=None):
        """Register a simulation-wide panel; build(viz) and action(viz, values).

        @param self The object pointer.
        @param panel_id Unique panel identifier.
        @param title Panel title.
        @param build callable(viz) returning the panel contents.
        @param action optional callable(viz, values) for controls.
        @param unavailable optional reason the panel cannot be used; the page
               lists the panel greyed out, with the reason as its tooltip.
        """
        self._global_panels[panel_id] = (title, build, action, unavailable)

    def add_overlay(self, callback):
        """Draw extra links returned by callback(viz) on every snapshot.

        @param self The object pointer.
        @param callback callable(viz) returning a list of links.
        """
        self._overlays.append(callback)

    def add_step_guard(self, callback):
        """Hold the simulation while callback(viz) returns True; runs every poll.

        @param self The object pointer.
        @param callback callable(viz) returning True to hold.
        """
        self._step_guards.append(callback)

    def add_periodic_update(self, callback):
        """Call callback(viz) after each simulation step, e.g. to sample statistics.

        @param self The object pointer.
        @param callback callable(viz).
        """
        self._periodic.append(callback)

    def add_node_info(self, callback):
        """Merge the dict returned by callback(viz, node_index) into each node's snapshot entry.

        @param self The object pointer.
        @param callback callable(viz, node_index) returning a dict.
        """
        self._node_info.append(callback)

    def add_tooltip_info(self, callback):
        """Add [name, value] lines returned by callback(viz, node_index) to node summaries.

        @param self The object pointer.
        @param callback callable(viz, node_index) returning [name, value] lines.
        """
        self._tooltip_info.append(callback)

    def set_node_color(self, node_index, color):
        """Fill the node with a CSS color.

        @param self The object pointer.
        @param node_index Node index.
        @param color CSS color.
        """
        self._appearance.setdefault(int(node_index), {})["color"] = str(color)

    def set_node_label(self, node_index, label):
        """Replace the "Node N" label under the node.

        @param self The object pointer.
        @param node_index Node index.
        @param label Label text.
        """
        self._appearance.setdefault(int(node_index), {})["label"] = str(label)

    def set_node_size(self, node_index, size):
        """Scale the node (1.0 is the default size).

        @param self The object pointer.
        @param node_index Node index.
        @param size Scale factor.
        """
        self._appearance.setdefault(int(node_index), {})["size"] = float(size)

    def set_node_icon(self, node_index, path):
        """Draw the node with an image file (SVG, PNG...) instead of a circle.

        @param self The object pointer.
        @param node_index Node index.
        @param path Image file path.
        """
        self._appearance.setdefault(int(node_index), {})["icon"] = str(Path(path).resolve())

    def set_bounds(self, x1, y1, x2, y2):
        """Initial visible area, in simulation coordinates.

        @param self The object pointer.
        @param x1 Left x coordinate.
        @param y1 Top y coordinate.
        @param x2 Right x coordinate.
        @param y2 Bottom y coordinate.
        """
        self.bounds = [float(x1), float(y1), float(x2), float(y2)]

    def tooltip_lines(self, node_index):
        """Collect the plugin-provided summary lines for a node.

        @param self The object pointer.
        @param node_index Node index.
        @return The list of [name, value] lines.
        """
        lines = []
        for callback in self._tooltip_info:
            lines.extend(callback(self, node_index) or [])
        return lines

    # -- Lifecycle -----------------------------------------------------------

    def attach(self):
        """Hook the visualizer into the simulation the scenario just built.

        Must run after the topology exists: trace sources are connected with
        Config paths, which only match objects that already exist.
        """
        from . import web_plugins

        # Drop everything tied to a previous simulation (after a restart).
        self._panels.clear()
        self._global_panels.clear()
        self._overlays.clear()
        self._periodic.clear()
        self._step_guards.clear()
        self._tooltip_info.clear()
        self._node_info.clear()
        self._trails.clear()
        self._last_transmissions.clear()
        self._last_drops.clear()
        self._last_node_tx.clear()
        self._unreceived_since.clear()
        self._pending = [0.0, [], [], []]
        self._skip_target = None
        self._seen_flows = set()
        self.objects.clear()
        self._last_step_wall = None
        self._sim_rate = None
        self._rate_sample = [0.0, 0.0]
        self.effective_speed = None

        self.sim_helper = ns.visualizer.PyViz()
        connect_traces(self.sim_helper)
        self._apply_transmissions_mode()
        self._scan_topology()

        # Built-in plugins first, then the scenario's plugins and hooks, so a
        # scenario can override or extend what the built-ins set up.
        web_plugins.register_all(self)
        for plugin in self._user_plugins:
            plugin(self)
        for hook, args in self._init_hooks:
            hook(self, *args)
        # The command line (--PyVizSpeed, --PyVizStartTime) wins over the scenario.
        speed = _global_double("PyVizSpeed")
        if speed > 0 and not self._keep_speed:
            self.set_speed(speed)
        self._skip_target = _global_double("PyVizStartTime") or self._skip_target
        if self._skip_target is not None:
            self.skip_to(self._skip_target)

    def set_speed(self, speed):
        """Set the simulated seconds per wall-clock second, within the slider's range.

        @param self The object pointer.
        @param speed Requested speed.
        """
        self.speed = min(100.0, max(0.01, float(speed)))
        self.effective_speed = None

    def skip_to(self, seconds):
        """Run the simulation up to a time without animating it.

        @param self The object pointer.
        @param seconds Simulated time; earlier times do nothing.
        """
        with self.lock:
            delay = float(seconds) - float(ns.Simulator.Now().GetSeconds())
            if delay <= 0 or self.finished:
                return
            print("PyViz: skipping to %g s" % float(seconds), flush=True)
            reached = self.sim_helper.SimulatorRunUntil(ns.Simulator.Now() + ns.Seconds(delay))
            messages = [str(m) for m in self.sim_helper.GetPauseMessages()]
            if messages:
                self.pause_messages = messages
                self.running = False
            elif not reached:
                self.finished = True
            # What happened during the skip must not be animated afterwards.
            self._flows()
            self._last_step_wall = None

    def restart(self):
        """Destroy the simulation and rebuild it by rerunning the scenario file."""
        main_file = getattr(sys.modules.get("__main__"), "__file__", None)
        if not main_file or not Path(main_file).is_file():
            # A C++ program cannot be rerun in this process; it is executed
            # again once the reply has been sent (see _Handler.do_POST()).
            if not Path("/proc/self/exe").exists():
                raise RuntimeError(
                    "Cannot restart: the simulation was not started from a Python scenario file"
                )
            self.reexec_pending = True
            return
        ns.Simulator.Destroy()
        # Names outlive Simulator::Destroy(); the rerun scenario would abort
        # trying to add the same names again.
        ns.Names.Clear()
        self.sim_helper = None
        # The scenario re-registers these when it runs again.
        self._appearance.clear()
        self._init_hooks.clear()
        self._user_plugins.clear()
        self.bounds = None
        self.selected_node = None
        self.selected_nodes = []
        self.running, self.stopped, self.finished = False, False, False
        self.pause_messages = []
        self._keep_speed = True
        # Rerun the scenario only up to the point where it would run the
        # simulation: visualizer.start(), or Simulator::Run() under
        # VisualSimulatorImpl (followed by the scenario's own Destroy()). Both
        # raise _ScenarioReady during a restart, which unwinds the scenario
        # there; this server keeps stepping the topology it built.
        original_run = ns.Simulator.Run
        ns.Simulator.Run = _stop_scenario_at_run
        self.restarting = True
        try:
            runpy.run_path(str(Path(main_file).resolve()), run_name="__main__")
        except _ScenarioReady as ready:
            # Keep the unwound scenario frames (and so their locals) alive, as
            # they are on the first run, where the scenario never returns from
            # Simulator::Run(). Objects created from Python, such as
            # ns.BridgeNetDevice() in csma-bridge.py, are owned by their
            # Python wrapper: freeing it would delete an object the topology
            # still points to. They are never released, since releasing them
            # after a later Simulator::Destroy() could free them twice.
            self._scenario_frames.append(ready)
            self._expose_rerun_module(ready, main_file)
        finally:
            ns.Simulator.Run = original_run
            self.restarting = False
        self.attach()

    def _expose_rerun_module(self, ready, main_file):
        """Make the console's __main__ the scenario module of the last run.

        runpy.run_path() runs the scenario in a temporary __main__ module and
        puts the original one back in sys.modules when it unwinds, so without
        this the console would keep showing the globals of the first run. The
        console gets a view of the rerun's globals, not a copy, so later
        changes to them (e.g. by a plugin or an initialization hook) show too.

        @param self The object pointer.
        @param ready The _ScenarioReady exception that unwound the scenario.
        @param main_file Path of the scenario script.
        """
        path = Path(main_file).resolve()
        tb = ready.__traceback__
        while tb is not None:
            scope = tb.tb_frame.f_globals
            file = scope.get("__file__")
            if scope.get("__name__") == "__main__" and file and Path(file).resolve() == path:
                self.console_namespace["__main__"] = _ModuleView(scope)
                return
            tb = tb.tb_next

    # -- Topology ------------------------------------------------------------

    def _scan_topology(self):
        """Find wired links and shared channels, and lay out nodes without mobility.

        Only wired, non-virtual devices produce links; wireless connectivity
        is not drawn (plugins such as wifi_links add overlays for it).
        """
        links = set()
        hubs = {}
        self._can_move = any(_has_moving_model(node) for node in _all_nodes())
        # A mesh device's receive trace only fires at the final destination, so
        # forwarding hops would look like transmissions nobody received.
        self._mesh_nodes = {
            int(node.GetId())
            for node in _all_nodes()
            for device in range(int(node.GetNDevices()))
            if _type_name(node.GetDevice(device)) == "ns3::MeshPointDevice"
        }
        count = int(ns.NodeList.GetNNodes())
        for index in range(count):
            node = ns.NodeList.GetNode(index)
            for device_index in range(int(node.GetNDevices())):
                device = node.GetDevice(device_index)
                wireless, virtual = NETDEVICE_TRAITS.get(_type_name(device), (True, False))
                if wireless or virtual:
                    continue
                channel = device.GetChannel()
                if not channel:
                    continue
                peers = [
                    int(channel.GetDevice(i).GetNode().GetId())
                    for i in range(int(channel.GetNDevices()))
                ]
                if len(peers) > 2 and REPRESENT_CHANNELS_AS_NODES:
                    # Shared medium: link the node to a hub for the channel.
                    hub = "channel:" + _address(channel)
                    hubs.setdefault(hub, set()).add(index)
                    links.add((index, hub))
                else:
                    # Point-to-point: one undirected link per node pair. Both
                    # ends find it, so store it with the lower index first.
                    for peer in peers:
                        if peer != index:
                            links.add((min(index, peer), max(index, peer)))

        # Nodes with mobility keep their real positions. A hub whose members
        # all have positions sits at their centroid; other hubs are laid out.
        fixed = {}
        for index in range(count):
            position = _node_position(ns.NodeList.GetNode(index))
            if position is not None:
                fixed[index] = position
        for hub, members in hubs.items():
            if all(m in fixed for m in members):
                fixed[hub] = (
                    sum(fixed[m][0] for m in members) / len(members),
                    sum(fixed[m][1] for m in members) / len(members),
                )
        vertices = list(range(count)) + list(hubs)
        self._layout = _spring_layout(vertices, list(links), fixed)
        # Links mix int and str vertices, so sort by text for a stable order.
        self._links = sorted(links, key=str)
        self._hubs = hubs

    def move_node(self, node_index, x, y):
        """Move a node dragged in the browser.

        Nodes with mobility are moved in the simulation itself; others only
        change their place in the layout.

        @param self The object pointer.
        @param node_index Node index.
        @param x New x coordinate.
        @param y New y coordinate.
        """
        node = ns.NodeList.GetNode(node_index)
        mobility = node.GetObject[ns.MobilityModel]()
        if mobility:
            model = mobility.__deref__()
            x, y = _clamp_to_bounds(model, float(x), float(y))
            model.SetPosition(ns.Vector(x, y, model.GetPosition().z))
        else:
            self._layout[node_index] = (float(x), float(y))

    def _apply_transmissions_mode(self):
        """Tell PyViz which nodes to sample transmissions and packets for."""
        if self.sim_helper is None:
            return
        if self.show_transmissions == "all":
            nodes = list(range(int(ns.NodeList.GetNNodes())))
        elif self.show_transmissions == "selected":
            nodes = list(self.selected_nodes)
        else:
            nodes = []
        self.sim_helper.SetNodesOfInterest(nodes)

    # -- Simulation stepping -------------------------------------------------

    def step(self):
        """Advance the simulation by one sample period and return a snapshot.

        Called for every /api/state poll. Paused, held or finished
        simulations return a snapshot without advancing. The snapshot covers
        this advance and the slices run since the previous poll.

        @param self The object pointer.
        @return The snapshot.
        """
        with self.lock:
            # Slices run since the previous poll already cover the time up to
            # now; running another one would only delay this reply.
            if self._pending[0] == 0:
                self.advance()
            else:
                self.held = any([guard(self) for guard in self._step_guards])
            duration, transmissions, drops, node_tx = self._pending
            self._drop_events = []
            if duration > 0:
                self._pending = [0.0, [], [], []]
                # One sample per link and per node, as rates are averaged
                # over the durations of the entries where each one appears.
                links = {}
                for source, target, size in transmissions:
                    links[(source, target)] = links.get((source, target), 0) + size
                nodes = {}
                events = {}
                for node, kind, size, packets in drops:
                    nodes[(node, kind)] = nodes.get((node, kind), 0) + size
                    event = events.setdefault((node, kind), [0, 0])
                    event[0] += size
                    event[1] += packets
                self._drop_events = [
                    {"node": key[0], "kind": key[1], "bytes": size, "packets": packets}
                    for key, (size, packets) in events.items()
                ]
                self._last_transmissions.append(
                    (duration, [(s, t, size) for (s, t), size in links.items()])
                )
                self._last_drops.append((duration, list(nodes.items())))
                senders = {}
                for node, size in node_tx:
                    senders[node] = senders.get(node, 0) + size
                self._last_node_tx.append((duration, list(senders.items())))
                for history in (self._last_transmissions, self._last_drops, self._last_node_tx):
                    self._trim_history(history)
                for callback in self._periodic:
                    callback(self)
            return self._state()

    def advance(self):
        """Run the simulation for the wall-clock time since the last advance.

        Called by step() on each poll, and between polls by start(), which
        keeps a simulation that cannot keep up from idling while the page
        draws. Samples accumulate until the next poll collects them.

        @param self The object pointer.
        """
        with self.lock:
            # Ask every guard (not any() short-circuit) so each one can poll
            # its own state on every request.
            self.held = any([guard(self) for guard in self._step_guards])
            now_wall = time.monotonic()
            elapsed_wall = 0.0 if self._last_step_wall is None else now_wall - self._last_step_wall
            self._last_step_wall = now_wall
            elapsed_wall = min(elapsed_wall, MAX_STEP_WALL)
            if (
                self.running
                and not self.held
                and self.sim_helper is not None
                and not self.finished
                and elapsed_wall > 0
            ):
                start = float(ns.Simulator.Now().GetSeconds())
                advance = elapsed_wall * self.speed
                if self._sim_rate is not None:
                    advance = min(advance, self._sim_rate * STEP_BUDGET)
                target = ns.Simulator.Now() + ns.Seconds(advance)
                compute_start = time.perf_counter()
                # False when the run ended before the target: no events left,
                # or the scenario called Simulator::Stop().
                reached = self.sim_helper.SimulatorRunUntil(target)
                compute = time.perf_counter() - compute_start
                advanced = float(ns.Simulator.Now().GetSeconds()) - start
                self._rate_sample[0] += advanced
                self._rate_sample[1] += compute
                if self._rate_sample[1] >= RATE_SAMPLE:
                    rate = self._rate_sample[0] / self._rate_sample[1]
                    self._rate_sample = [0.0, 0.0]
                    self._sim_rate = (
                        rate if self._sim_rate is None else 0.7 * self._sim_rate + 0.3 * rate
                    )
                achieved = advanced / elapsed_wall
                self.effective_speed = (
                    achieved
                    if self.effective_speed is None
                    else 0.8 * self.effective_speed + 0.2 * achieved
                )
                messages = [str(m) for m in self.sim_helper.GetPauseMessages()]
                if messages:
                    # Simulation code called PyViz::Pause(): pause and show why.
                    self.pause_messages = messages
                    self.running = False
                elif not reached:
                    self.finished = True
                self._collect_samples(float(ns.Simulator.Now().GetSeconds()) - start)

    def wants_slice(self):
        """Return the wall-clock seconds until the next slice between polls.

        @param self The object pointer.
        @return Seconds to wait, or None when the simulation is not advancing.
        """
        if (
            not self.running
            or self.held
            or self.finished
            or self.sim_helper is None
            or self._last_step_wall is None
        ):
            return None
        return max(0.0, self._last_step_wall + IDLE_SLICE - time.monotonic())

    def _collect_samples(self, duration):
        """Add this run's transmission and drop samples to those of the next poll.

        PyViz clears its samples at the start of each SimulatorRunUntil(), so
        they must be read after every run. Each poll's samples form one
        history entry, and only the last ``smoothing`` entries are kept;
        rates are averaged over them.

        @param self The object pointer.
        @param duration simulated seconds covered by this step; steps vary in
               length, so rates are computed from the recorded durations
        """
        transmissions = []
        for sample in self.sim_helper.GetTransmissionSamples():
            if sample.transmitter and sample.receiver:
                transmissions.append(
                    (
                        int(sample.transmitter.GetId()),
                        int(sample.receiver.GetId()),
                        int(sample.bytes),
                    )
                )
        drops = [
            (int(d.transmitter.GetId()), str(d.kind), int(d.bytes), int(d.packets))
            for d in self.sim_helper.GetPacketDropSamples()
            if d.transmitter
        ]
        self._pending[0] += duration
        self._pending[1].extend(transmissions)
        self._pending[2].extend(drops)
        self._pending[3].extend(
            (int(s.transmitter.GetId()), int(s.bytes))
            for s in self.sim_helper.GetNodeTxSamples()
            if s.transmitter
        )

    def _transmissions(self):
        """Average transmission rate per (source, target) over the history.

        Each PyViz sample is the bytes sent on one link during one step, so
        total bytes / total duration of the steps where the link was active
        gives the mean rate while active. The arrow width grows with the
        logarithm of the rate, so slow and fast links both stay visible.
        """
        totals = {}
        for _, sample_set in self._last_transmissions:
            for source, target, size in sample_set:
                totals[(source, target)] = totals.get((source, target), 0) + size
        span = self._rate_span(self._last_transmissions)
        return [
            {
                "source": s,
                "target": t,
                "kbps": total * 8 / 1e3 / span,
                "width": max(0.5, math.log(max(total / span, 1.0)) / 2),
            }
            for (s, t), total in totals.items()
        ]

    def _trim_history(self, history):
        """Keep the entries covering the averaging window, in simulated time.

        @param self The object pointer.
        @param history List of (duration, samples), oldest first.
        """
        window = self.smoothing * RATE_WINDOW_UNIT
        covered = sum(duration for duration, _ in history)
        drop = 0
        while drop < len(history) - 1 and covered - history[drop][0] >= window:
            covered -= history[drop][0]
            drop += 1
        del history[: max(drop, len(history) - RATE_HISTORY_LIMIT)]

    def _rate_span(self, history):
        """Return the simulated seconds a history's bytes are averaged over.

        This is the whole window, idle time included: dividing by the time a
        link was active would make sparse traffic look fast. A window that is
        not full yet still counts whole, so rates rise to their value instead
        of starting too high.

        @param self The object pointer.
        @param history List of (duration, samples).
        @return Seconds, greater than zero.
        """
        covered = sum(duration for duration, _ in history)
        if len(history) >= RATE_HISTORY_LIMIT:
            return covered
        return max(covered, self.smoothing * RATE_WINDOW_UNIT)

    def _drops(self):
        """Average drop rate per node over the history, as for transmissions."""
        return [
            {"node": rate["node"][0], "kind": rate["node"][1], "kbps": rate["kbps"]}
            for rate in self._node_rates(self._last_drops)
        ]

    def _unreceived(self, transmissions):
        """Return the nodes transmitting without any reception in the history.

        @param self The object pointer.
        @param transmissions The links shown, from _transmissions().
        @return The node rates, as for _drops().
        """
        sources = {t["source"] for t in transmissions} | self._mesh_nodes
        now = float(ns.Simulator.Now().GetSeconds())
        rates = [r for r in self._node_rates(self._last_node_tx) if r["node"] not in sources]
        # A packet still in flight has no reception yet either.
        since = self._unreceived_since
        self._unreceived_since = {r["node"]: since.get(r["node"], now) for r in rates}
        return [r for r in rates if now - self._unreceived_since[r["node"]] >= UNRECEIVED_HOLD]

    def _node_rates(self, history):
        """Average a per-node byte history into rates over the whole window.

        @param self The object pointer.
        @param history List of (duration, [(key, bytes), ...]).
        @return List of {"node", "kbps"}.
        """
        totals = {}
        for _, sample_set in history:
            for node, size in sample_set:
                totals[node] = totals.get(node, 0) + size
        span = self._rate_span(history)
        return [{"node": n, "kbps": total * 8 / 1e3 / span} for n, total in totals.items()]

    def _flows(self):
        """Return packets that completed a hop since the last snapshot.

        PyViz keeps a rolling list of recent Tx-to-Rx intervals and returns
        all of it every time, so already-sent entries are filtered out here.
        The page animates each one along its link for its observed delay.
        """
        flows = []
        for flow in self.sim_helper.GetPacketFlowSamples():
            key = (
                int(flow.packetUid),
                float(flow.startTx.GetSeconds()),
                float(flow.startRx.GetSeconds()),
                int(flow.transmitter.GetId()),
                int(flow.receiver.GetId()),
            )
            if key in self._seen_flows:
                continue
            self._seen_flows.add(key)
            flows.append(
                {
                    "packet": key[0],
                    "source": key[3],
                    "target": key[4],
                    "bytes": int(flow.bytes),
                    "startTx": key[1],
                    "startRx": key[2],
                    "delay": max(0.0, key[2] - key[1]),
                }
            )
        return flows

    def _state(self):
        """Build the JSON snapshot rendered by the page."""
        nodes = []
        for index in range(int(ns.NodeList.GetNNodes())):
            node = ns.NodeList.GetNode(index)
            # Positions are read on every snapshot so mobility is animated;
            # nodes without mobility use the layout computed at attach().
            position = _node_position(node)
            x, y = position if position is not None else self._layout.get(index, (0.0, 0.0))
            appearance = self._appearance.get(index, {})
            if self.show_trails and position is not None:
                self._record_trail(index, x, y)
            entry = {
                "id": index,
                "x": x,
                "y": y,
                "z": _node_height(node),
                "name": str(ns.Names.FindName(node)),
                "mobility": position is not None,
                "color": appearance.get("color"),
                "label": appearance.get("label"),
                "size": appearance.get("size", 1.0),
                "icon": "icon" in appearance,  # the image is served by /api/icon
            }
            for callback in self._node_info:
                entry.update(callback(self, index) or {})
            nodes.append(entry)
        hubs = [
            {"id": hub, "x": self._layout[hub][0], "y": self._layout[hub][1]} for hub in self._hubs
        ]
        overlays = []
        for callback in self._overlays:
            overlays.extend(callback(self) or [])
        # The helper is briefly missing during a restart.
        helper = self.sim_helper is not None
        transmissions = self._transmissions() if helper else []
        positions = {n["id"]: (n["x"], n["y"]) for n in nodes}
        return {
            "time": float(ns.Simulator.Now().GetSeconds()),
            "nodes": nodes,
            "hubs": hubs,
            "links": [{"source": a, "target": b} for a, b in self._links],
            "transmissions": transmissions,
            "unreceived": self._unreceived(transmissions) if helper else [],
            "drops": self._drops() if helper else [],
            # Packets dropped since the previous snapshot, animated by the page.
            "dropEvents": self._drop_events,
            "flows": self._flows() if helper else [],
            "overlays": overlays,
            "running": self.running,
            "held": self.held,
            "stopped": self.stopped,
            "finished": self.finished,
            "pauseMessages": self.pause_messages,
            "speed": self.speed,
            "effectiveSpeed": self.effective_speed,
            "smoothing": self.smoothing,
            "showTransmissions": self.show_transmissions,
            "selected": self.selected_node,
            "selectedNodes": self.selected_nodes,
            "bounds": self.bounds,
            "showTrails": self.show_trails,
            "canMove": self._can_move,
            # Mobility areas of the selected nodes, drawn when they are selected.
            "selectedBounds": self._selected_bounds(),
            # The page draws up to the node's position, so one point is enough
            # once the node has moved; slow speeds take long to add a second.
            "trails": {
                str(i): points
                for i, points in self._trails.items()
                if len(points) > 1 or positions.get(i, tuple(points[-1])) != tuple(points[-1])
            },
        }

    def _selected_bounds(self):
        """Collect the mobility bounds of the selected nodes.

        @param self The object pointer.
        @return {node index text: (xMin, yMin, xMax, yMax)}
        """
        result = {}
        for index in self.selected_nodes:
            mobility = ns.NodeList.GetNode(index).GetObject[ns.MobilityModel]()
            bounds = _mobility_bounds(mobility.__deref__()) if mobility else None
            if bounds is not None:
                result[str(index)] = bounds
        return result

    def _record_trail(self, node_index, x, y):
        """Append a position to a node's trail when it moved far enough.

        @param self The object pointer.
        @param node_index Node index.
        @param x Current x coordinate.
        @param y Current y coordinate.
        """
        trail = self._trails.setdefault(node_index, [])
        if not trail or math.hypot(x - trail[-1][0], y - trail[-1][1]) >= TRAIL_MIN_STEP:
            trail.append([x, y])
            del trail[: max(0, len(trail) - TRAIL_POINTS)]

    # -- User actions --------------------------------------------------------

    def control(self, request):
        """Apply a toolbar or topology action posted by the page.

        @param self The object pointer.
        @param request {"action": name, ...action-specific fields}
        @return {"ok": True}
        """
        action = str(request["action"])
        with self.lock:
            if action == "play":
                self.running, self.stopped = True, False
                # Do not catch up on the time spent paused.
                self._last_step_wall = None
                self.pause_messages = []
            elif action == "pause":
                self.running = False
            elif action == "stop":
                self.running, self.stopped = False, True
            elif action == "exit":
                # start() returns once this reply is sent.
                self.running, self.exit_requested = False, True
            elif action == "restart":
                self.restart()
            elif action == "speed":
                self.set_speed(request["value"])
            elif action == "smoothing":
                self.smoothing = min(50, max(1, int(request["value"])))
            elif action == "transmissions":
                if request["value"] not in ("all", "selected", "none"):
                    raise ValueError("Unknown transmissions mode")
                self.show_transmissions = request["value"]
                self._apply_transmissions_mode()
            elif action == "select":
                # "add" toggles the node in the selection (Shift/Ctrl-click);
                # otherwise the selection becomes just this node.
                node = request.get("node")
                node = None if node is None else int(node)
                if request.get("add") and node is not None:
                    if node in self.selected_nodes:
                        self.selected_nodes.remove(node)
                    else:
                        self.selected_nodes.append(node)
                else:
                    self.selected_nodes = [] if node is None else [node]
                # The primary node is the one just clicked, or the most recent
                # remaining one if the click deselected it.
                self.selected_node = (
                    node
                    if node in self.selected_nodes
                    else (self.selected_nodes[-1] if self.selected_nodes else None)
                )
                self.console_namespace["node"] = (
                    None if self.selected_node is None else ns.NodeList.GetNode(self.selected_node)
                )
                self.console_namespace["nodes"] = [
                    ns.NodeList.GetNode(n) for n in self.selected_nodes
                ]
                # "Selected node" transmission mode follows the selection.
                self._apply_transmissions_mode()
            elif action == "trails":
                self.show_trails = bool(request["value"])
                self._trails.clear()
            elif action == "move":
                self.move_node(int(request["node"]), request["x"], request["y"])
            else:
                raise ValueError("Unknown simulation action")
        return {"ok": True}

    def node_panels(self, node_index):
        """List the panels that apply to a node (for its menu and buttons).

        @param self The object pointer.
        @param node_index Node index.
        @return {"panels": [{"id", "title"}, ...]}
        """
        with self.lock:
            return {
                "panels": [
                    {"id": panel_id, "title": title}
                    for panel_id, (title, _build, _action, applies) in self._panels.items()
                    if applies is None or applies(self, node_index)
                ]
            }

    def node_panel(self, node_index, panel_id):
        """Build the current contents of a per-node panel.

        @param self The object pointer.
        @param node_index Node index.
        @param panel_id Panel identifier.
        @return The panel contents.
        """
        with self.lock:
            title, build, _action, _applies = self._panels[panel_id]
            panel = build(self, node_index) or {}
            # The page expects both keys, even when a plugin omits them.
            panel.setdefault("sections", [])
            panel.setdefault("controls", [])
            panel["title"] = "%s - node %d" % (title, node_index)
            return panel

    def global_panels(self):
        """List the simulation-wide panels (for the toolbar menu)."""
        with self.lock:
            return {
                "panels": [
                    {"id": i, "title": t, "unavailable": u}
                    for i, (t, _b, _a, u) in self._global_panels.items()
                ]
            }

    def global_panel(self, panel_id):
        """Build the current contents of a simulation-wide panel.

        @param self The object pointer.
        @param panel_id Panel identifier.
        @return The panel contents.
        """
        with self.lock:
            title, build, _action, unavailable = self._global_panels[panel_id]
            if unavailable:
                panel = {"sections": [{"title": unavailable, "rows": []}]}
            else:
                panel = build(self) or {}
            panel.setdefault("sections", [])
            panel.setdefault("controls", [])
            panel["title"] = title
            return panel

    def panel_action(self, node_index, panel_id, values):
        """Pass control values to a panel's action callback.

        @param self The object pointer.
        @param node_index node of a per-node panel, or None for a global panel
        @param values {control name: value} as posted by the page
        @param panel_id Panel identifier.
        @return {"ok": True}
        """
        with self.lock:
            if node_index is None:
                _title, _build, action, unavailable = self._global_panels[panel_id]
                if unavailable:
                    raise ValueError(unavailable)
                if action is None:
                    raise ValueError("Panel has no actions")
                action(self, values)
                return {"ok": True}
            action = self._panels[panel_id][2]
            if action is None:
                raise ValueError("Panel has no actions")
            action(self, node_index, values)
        return {"ok": True}

    def run_console(self, source):
        """Run Python typed in the browser console and return its output.

        Expressions print their repr like the interactive interpreter;
        statements are executed. Globals persist between calls. This runs
        arbitrary code in the visualizer process, which is why POSTs need
        the session token.

        @param self The object pointer.
        @param source Python source text.
        @return {"ok": True, "output": text}
        """
        output = io.StringIO()
        with self.lock, contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            self.console_namespace.setdefault("__main__", sys.modules.get("__main__"))
            interpreter = code.InteractiveInterpreter(self.console_namespace)
            try:
                compiled = compile(source, "<console>", "eval")
            except SyntaxError:
                compiled = None
            if compiled is None:
                # Not an expression: run it as statements, outside the except
                # block so errors are not chained to the SyntaxError above.
                # runsource() prints syntax errors and tracebacks itself.
                interpreter.runsource(source, "<console>", "exec")
            else:
                try:
                    result = eval(compiled, self.console_namespace)
                    if result is not None:
                        print(repr(result))
                except Exception:
                    interpreter.showtraceback()
        return {"ok": True, "output": output.getvalue()}

    def node_icon(self, node_index):
        """Return the icon file set for a node, or None.

        @param self The object pointer.
        @param node_index Node index.
        @return The icon Path or None.
        """
        path = self._appearance.get(node_index, {}).get("icon")
        return Path(path) if path else None


# Environment variables handing the port and token to a re-executed process.
## Listening socket descriptor the re-executed process serves on
_SOCKET_VARIABLE = "PYVIZ_REEXEC_SOCKET"
## Page token kept by the re-executed process
_TOKEN_VARIABLE = "PYVIZ_REEXEC_TOKEN"
## Simulation speed kept by the re-executed process
_SPEED_VARIABLE = "PYVIZ_REEXEC_SPEED"

# One visualizer per process: the HTTP handler and the module-level API
# functions below all act on it.
## The process-wide Visualizer instance
_viz = Visualizer()
## The HTTP server, once started
_server = None
## Directory of the page that opened the browser, removed on exit
_launch_directory = None


def _reexec():
    """Restart a C++ program by executing it again with the same arguments.

    The new process serves on the same port with the same token and speed, so
    the open page reconnects to it by itself. It inherits the listening socket
    instead of binding the port again: the page keeps polling, with the token,
    while the program starts, and a port left free meanwhile could be bound
    by another local user to collect the token.
    """
    with open("/proc/self/cmdline", "rb") as cmdline:
        argv = [os.fsdecode(a) for a in cmdline.read().split(b"\0")[:-1]]
    _server.socket.set_inheritable(True)
    os.environ[_SOCKET_VARIABLE] = str(_server.socket.fileno())
    # Other processes of the same user can read the environment (e.g.
    # /proc/<pid>/environ), as they can this process's memory; the new
    # process removes the token from its environment when it starts.
    os.environ[_TOKEN_VARIABLE] = _viz.token
    os.environ[_SPEED_VARIABLE] = str(_viz.speed)
    if _launch_directory is not None:
        shutil.rmtree(_launch_directory, ignore_errors=True)
    sys.stdout.flush()
    sys.stderr.flush()
    os.execv("/proc/self/exe", argv)


# ---------------------------------------------------------------------------
# Runtime type information (Types tab)
# ---------------------------------------------------------------------------


def _type_catalogue():
    """Read the live TypeId registry, including attributes and trace signatures.

    Types whose C++ class Cppyy cannot resolve (e.g. templates with unusual
    names) are skipped rather than failing the whole catalogue.
    """
    result = []
    for index in range(int(ns.TypeId.GetRegisteredN())):
        try:
            type_id = ns.TypeId.GetRegistered(index)
            entry = {
                "name": str(type_id.GetName()),
                "group": str(type_id.GetGroupName()),
                "attributes": [],
                "traces": [],
                "methods": [],
            }
            # Walk the C++ namespaces in Cppyy to find the class, e.g.
            # "ns3::lrwpan::LrWpanMac" -> cppyy.gbl.ns3.lrwpan.LrWpanMac.
            scope = ns.cppyy.gbl
            for part in entry["name"].replace("::", ".").split("."):
                scope = getattr(scope, part)
            entry["methods"] = sorted(
                name
                for name in dir(scope)
                if not name.startswith("_") and callable(getattr(scope, name, None))
            )
            for attr_index in range(int(type_id.GetAttributeN())):
                attr = type_id.GetAttribute(attr_index)
                entry["attributes"].append(
                    {
                        "name": str(attr.name),
                        "help": str(attr.help),
                        "type": str(attr.checker.GetValueTypeName()),
                        "flags": int(attr.flags),
                        "initial": _serialize_attribute_value(attr.initialValue, attr.checker),
                    }
                )
            for trace_index in range(int(type_id.GetTraceSourceN())):
                trace = type_id.GetTraceSource(trace_index)
                entry["traces"].append(
                    {
                        "name": str(trace.name),
                        "help": str(trace.help),
                        "callback": str(trace.callback),
                    }
                )
            result.append(entry)
        except Exception:
            continue
    return {"types": result}


def _types_in_use():
    """Return the TypeId names of the objects reachable from nodes and channels.

    @return {"types": sorted names}.
    """
    with _viz.lock:
        seen, names = set(), set()
        pending = [_register(node) for node in _all_nodes()]
        while pending:
            object_id = pending.pop()
            if object_id in seen:
                continue
            seen.add(object_id)
            description = _describe(object_id)
            names.add(description.get("type"))
            pending.extend(child["id"] for child in description.get("children", []))
    return {"types": sorted(name for name in names if name)}


def _serialize_attribute_value(value, checker):
    """Return the text form of a Ptr<AttributeValue>, or "" if unavailable.

    @param value Ptr<AttributeValue>.
    @param checker Attribute checker.
    @return The serialized text.
    """
    if not value:
        return ""
    try:
        return str(value.__deref__().SerializeToString(checker))
    except Exception:
        return ""


## Pattern of valid TimeValue text
_TIME_TEXT = re.compile(
    r"^\s*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?\s*(y|d|h|min|s|ms|us|ns|ps|fs)?\s*$"
)


def _value_text_is_valid(checker, text):
    """Reject text that some ns-3 value types abort on instead of rejecting.

    UintegerValue, IntegerValue, DoubleValue and TimeValue terminate the
    whole process (NS_ABORT) on malformed text in DeserializeFromString(), so
    their text is checked here first; other types report failure normally.

    @param checker Attribute checker.
    @param text Value text.
    @return True if the text is safe to deserialize.
    """
    value_type = str(checker.GetValueTypeName())
    try:
        if value_type == "ns3::UintegerValue":
            return int(text.strip()) >= 0
        if value_type == "ns3::IntegerValue":
            int(text.strip())
        elif value_type == "ns3::DoubleValue":
            float(text.strip())
        elif value_type == "ns3::TimeValue":
            return bool(_TIME_TEXT.match(text))
    except ValueError:
        return False
    return True


def _set_default_attribute(type_name, attribute_name, raw_value):
    """Apply an attribute default after ns-3 validates its textual value.

    Affects only objects created afterwards, like Config::SetDefault().

    @param type_name TypeId name.
    @param attribute_name Attribute name.
    @param raw_value Value text.
    @return True on success.
    """
    with _viz.lock:
        type_id = ns.TypeId.LookupByName(type_name)
        for index in range(int(type_id.GetAttributeN())):
            info = type_id.GetAttribute(index)
            if str(info.name) != attribute_name:
                continue
            # checker.Create() makes an empty value of the attribute's type,
            # which DeserializeFromString() fills and validates.
            value = info.checker.Create()
            value_ref = value.__deref__()
            if not _value_text_is_valid(
                info.checker, raw_value
            ) or not value_ref.DeserializeFromString(raw_value, info.checker):
                return False
            ns.Config.SetDefault(str(type_id.GetAttributeFullName(index)), value_ref)
            return True
    return False


# ---------------------------------------------------------------------------
# Object tree (Node tab)
#
# The tree is expanded lazily: the browser first asks for a node, then for
# each child it opens. Every object sent to the browser is kept in
# _viz.objects under its address, so later requests (expanding, editing an
# attribute) can find it again. Selecting another node clears the registry.
# ---------------------------------------------------------------------------


def _register(obj):
    """Register an object under a stable id derived from its address.

    @param obj ns-3 Object.
    @return The object id.
    """
    # The aggregate iterator yields Ptr<const Object>; attributes can only be
    # set on a non-const pointer.
    if "const" in type(obj).__cpp_name__:
        obj = ns.ConstCast[ns.Object](obj)
    object_id = _address(obj)
    _viz.objects[object_id] = obj
    return object_id


def _child_ref(obj, label):
    """Describe a child without its contents; the browser fetches those on expand.

    @param obj ns-3 Object.
    @param label Label shown in the tree.
    @return The child reference dict.
    """
    return {"id": _register(obj), "label": label, "type": _type_name(obj)}


def _object_details(obj):
    """Return identifying values that are not exposed as attributes.

    Objects that lack one of these getters are simply skipped for it.

    @param obj ns-3 Object.
    @return A list of {"name", "value"} dicts.
    """
    details = []
    for method, label in (
        ("GetId", "Id"),
        ("GetIfIndex", "Interface index"),
        ("GetAddress", "Address"),
        ("GetMtu", "MTU"),
    ):
        try:
            details.append({"name": label, "value": str(getattr(obj, method)())})
        except Exception:
            continue
    if _type_name(obj) == "ns3::Ipv4L3Protocol":
        ipv4 = obj.GetObject[ns.Ipv4]()
        for interface in range(int(ipv4.GetNInterfaces())):
            for index in range(int(ipv4.GetNAddresses(interface))):
                address = ipv4.GetAddress(interface, index)
                details.append(
                    {
                        "name": "Interface %d address" % interface,
                        "value": "%s/%s"
                        % (address.GetLocal(), address.GetMask().GetPrefixLength()),
                    }
                )
    return details


def _describe(object_id):
    """Describe one object with its attributes and the objects it references.

    Children are the aggregated objects, Pointer attribute targets, the
    entries of object containers, and a device's channel, so the browser can
    expand the tree lazily.

    @param object_id Registered object id.
    @return The object description dict.
    """
    obj = _viz.objects[object_id]
    type_id = obj.GetInstanceTypeId()
    attributes = []
    children = []
    for index in range(int(type_id.GetAttributeN())):
        info = type_id.GetAttribute(index)
        if not (int(info.flags) & _ATTR_GET):
            continue
        # Reading an obsolete attribute aborts the simulation, and a deprecated
        # one prints a warning; their replacements are listed anyway.
        if info.supportLevel != ns.TypeId.SupportLevel.SUPPORTED:
            continue
        name = str(info.name)
        value_type = str(info.checker.GetValueTypeName())
        # Attributes that hold objects become children instead of values.
        try:
            if value_type == _POINTER_TYPE:
                pointer = ns.PointerValue()
                if obj.GetAttributeFailSafe(name, pointer) and pointer.GetObject():
                    children.append(_child_ref(pointer.GetObject(), name))
                continue
            if value_type == _CONTAINER_TYPE:
                container = ns.ObjectPtrContainerValue()
                if obj.GetAttributeFailSafe(name, container):
                    for item in range(int(container.GetN())):
                        entry = container.Get(item)
                        if entry:
                            children.append(_child_ref(entry, "%s[%d]" % (name, item)))
                continue
        except Exception:
            continue
        # Plain attributes: read into an empty value of the right type and
        # send its text form, which is also what editing posts back.
        value = info.checker.Create()
        if not obj.GetAttributeFailSafe(name, value.__deref__()):
            continue
        attributes.append(
            {
                "name": name,
                "type": value_type,
                "help": str(info.help),
                "value": _serialize_attribute_value(value, info.checker),
                "writable": bool(int(info.flags) & _ATTR_SET),
            }
        )
    # NetDevice::GetChannel() is not an attribute, so add the channel by hand.
    try:
        channel = obj.GetChannel()
        if channel:
            children.append(_child_ref(channel, "Channel"))
    except Exception:
        pass
    # Aggregated objects (IP stack, mobility, ...). The iterator also yields
    # the object itself, which is skipped by comparing ids.
    iterator = obj.GetAggregateIterator()
    while iterator.HasNext():
        aggregate = iterator.Next()
        if _register(aggregate) != object_id:
            children.append(_child_ref(aggregate, "Aggregated"))
    return {
        "id": object_id,
        "type": str(type_id.GetName()),
        "details": _object_details(obj),
        "attributes": attributes,
        "children": children,
    }


def _describe_registered(object_id):
    """Expand a child previously sent to the browser.

    @param object_id Registered object id.
    @return The object description or an error dict.
    """
    with _viz.lock:
        if object_id not in _viz.objects:
            # The registry was cleared by selecting another node (or a restart).
            return {"error": "Unknown object; reselect the node"}
        return _describe(object_id)


def _objects_for_node(node_index):
    """Describe a node as the root of its object tree.

    @param node_index Node index.
    @return The node description or an error dict.
    """
    with _viz.lock:
        if node_index < 0 or node_index >= int(ns.NodeList.GetNNodes()):
            return {"error": "Unknown node"}
        _viz.objects.clear()
        return _describe(_register(ns.NodeList.GetNode(node_index)))


def _set_object_attribute(object_id, attribute_name, raw_value):
    """Set an attribute on a live object; returns False if ns-3 rejects it.

    @param object_id Registered object id.
    @param attribute_name Attribute name.
    @param raw_value Value text.
    @return True on success.
    """
    with _viz.lock:
        obj = _viz.objects.get(object_id)
        if obj is None:
            return False
        type_id = obj.GetInstanceTypeId()
        for index in range(int(type_id.GetAttributeN())):
            info = type_id.GetAttribute(index)
            if str(info.name) != attribute_name or not (int(info.flags) & _ATTR_SET):
                continue
            value = info.checker.Create()
            value_ref = value.__deref__()
            if not _value_text_is_valid(
                info.checker, raw_value
            ) or not value_ref.DeserializeFromString(raw_value, info.checker):
                return False
            return bool(obj.SetAttributeFailSafe(attribute_name, value_ref))
    return False


# ---------------------------------------------------------------------------
# HTTP
# ---------------------------------------------------------------------------


class _Handler(BaseHTTPRequestHandler):
    """Routes requests to the Visualizer; see the module docstring for the API.

    Errors raised while handling a request are returned as
    {"ok": false, "error": text} with status 400, so the page can show them.
    """

    ## Server name sent in the Server header
    server_version = "ns3-PyViz/1.0"
    ## Seconds a connection may stay idle; the server handles one request at
    ## a time, so a connection that never sends would otherwise hang it
    timeout = 5

    def _send(self, status, payload, content_type="application/json", cache="no-store"):
        """Send a response; non-bytes payloads are encoded as JSON.

        @param self The object pointer.
        @param status HTTP status code.
        @param payload Response body (bytes or JSON-serializable).
        @param content_type Content-Type header value.
        @param cache Cache-Control header value.
        """
        if not isinstance(payload, bytes):
            payload = json.dumps(payload).encode()
        try:
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Cache-Control", cache)
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        except (BrokenPipeError, ConnectionResetError):
            # The page went away mid-response (reload, closed tab); nothing
            # to deliver, and the simulation state is unaffected.
            ## Whether to close the connection after this response
            self.close_connection = True

    def _authorized(self, path):
        """Check a request may proceed, replying 403 if it may not.

        The page and its static assets hold no secrets and are public, since
        a browser loads them before the page can attach the token. Everything
        else, including the API GETs (which advance the simulation and expose
        its objects), needs the token. Other web pages cannot send the custom
        header without a CORS preflight, which this server never grants. The
        Host check stops DNS rebinding from making a hostile page same-origin.

        @param self The object pointer.
        @param path The request path.
        @return True if the request may proceed.
        """
        port = self.server.server_address[1]
        if self.headers.get("Host") not in ("127.0.0.1:%d" % port, "localhost:%d" % port):
            self._send(403, {"ok": False, "error": "Unexpected Host header"})
            return False
        if path in _PUBLIC_PATHS:
            return True
        # Compared as bytes: compare_digest() rejects non-ASCII text.
        sent = self.headers.get("X-PyViz-Token", "").encode("latin-1", "replace")
        if not secrets.compare_digest(sent, _viz.token.encode()):
            self._send(403, {"ok": False, "error": "Missing or invalid token"})
            return False
        return True

    def do_GET(self):
        """Serve page assets and the read-only API.

        @param self The object pointer.
        """
        url = urlsplit(self.path)
        query = {key: values[0] for key, values in parse_qs(url.query).items()}
        if not self._authorized(url.path):
            return
        try:
            if url.path == "/api/state":
                self._send(200, _viz.step())
            elif url.path == "/api/types":
                self._send(200, _type_catalogue())
            elif url.path == "/api/types-in-use":
                self._send(200, _types_in_use())
            elif url.path == "/api/objects":
                if "id" in query:
                    self._send(200, _describe_registered(query["id"]))
                else:
                    self._send(200, _objects_for_node(int(query["node"])))
            elif url.path == "/api/panels":
                if "node" in query:
                    self._send(200, _viz.node_panels(int(query["node"])))
                else:
                    self._send(200, _viz.global_panels())
            elif url.path == "/api/panel":
                if "node" in query:
                    self._send(200, _viz.node_panel(int(query["node"]), query["id"]))
                else:
                    self._send(200, _viz.global_panel(query["id"]))
            elif url.path == "/api/tooltip":
                with _viz.lock:
                    self._send(200, {"lines": _viz.tooltip_lines(int(query["node"]))})
            elif url.path == "/api/icon":
                path = _viz.node_icon(int(query["node"]))
                if path is None or not path.is_file():
                    self.send_error(404)
                    return
                content_type = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
                self._send(200, path.read_bytes(), content_type, "max-age=3600")
            elif url.path in _BINARY_ASSETS:
                # Fixed names only, as for the text assets below.
                self._send(
                    200,
                    (_ASSETS / url.path[1:]).read_bytes(),
                    _BINARY_ASSETS[url.path],
                    "max-age=3600",
                )
            elif url.path in ("/", "/app.js", "/style.css"):
                # Only these fixed names are served, so the URL can never
                # reach other files on disk.
                name = "index.html" if url.path == "/" else url.path[1:]
                text = (_ASSETS / name).read_text()
                content_type = {"html": "text/html", "js": "text/javascript", "css": "text/css"}[
                    name.rsplit(".", 1)[1]
                ]
                self._send(200, text.encode(), content_type + "; charset=utf-8", "no-cache")
            else:
                self.send_error(404)
        except Exception as error:
            self._send(400, {"ok": False, "error": str(error)})

    def do_POST(self):
        """Handle token-protected API actions.

        @param self The object pointer.
        """
        if not self._authorized(urlsplit(self.path).path):
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length > 65536:
                raise ValueError("Request is too large")
            request = json.loads(self.rfile.read(length))
            endpoint = urlsplit(self.path).path
            if endpoint == "/api/control":
                result = _viz.control(request)
            elif endpoint == "/api/panel-action":
                node = request.get("node")
                result = _viz.panel_action(
                    None if node is None else int(node),
                    str(request["id"]),
                    request.get("values", {}),
                )
            elif endpoint == "/api/console":
                result = _viz.run_console(str(request["code"]))
            elif endpoint == "/api/object-attribute":
                result = {
                    "ok": _set_object_attribute(
                        str(request["object"]), str(request["attribute"]), str(request["value"])
                    )
                }
            elif endpoint == "/api/default-attribute":
                result = {
                    "ok": _set_default_attribute(
                        str(request["type"]), str(request["attribute"]), str(request["value"])
                    )
                }
            else:
                self.send_error(404)
                return
            self._send(200 if result.get("ok") else 400, result)
        except Exception as error:
            self._send(400, {"ok": False, "error": str(error)})
        if _viz.reexec_pending:
            _reexec()

    def log_message(self, _format, *_args):
        """Suppress per-request logging.

        @param self The object pointer.
        @param _format Ignored format string.
        @param _args Ignored format arguments.
        """
        # The page polls about 30 times per second; logging each request would
        # bury the simulation's own output.
        pass


# ---------------------------------------------------------------------------
# Public entry points (re-exported by the package __init__)
# ---------------------------------------------------------------------------


def add_initialization_hook(hook, *args):
    """Call hook(viz, *args) once the visualizer is attached to the simulation.

    @param hook callable(viz, *args).
    @param args Extra arguments passed to the hook.
    """
    _viz._init_hooks.append((hook, args))


def register_plugin(plugin):
    """Call plugin(viz) on start, typically to add panels or overlays.

    @param plugin callable(viz).
    """
    _viz._user_plugins.append(plugin)


def enable_flow_monitor():
    """Plot per-flow statistics in the browser from the start."""
    from .web_plugins import flow_monitor

    flow_monitor.enable()


def set_speed(speed):
    """Initial playback speed; a restart keeps the speed set in the page.

    @param speed Simulated seconds per wall-clock second (0.01 to 100).
    """

    def hook(viz):
        if not viz._keep_speed:
            viz.set_speed(speed)

    add_initialization_hook(hook)


def skip_to(seconds):
    """Run up to this simulated time, unanimated, before the page shows it.

    @param seconds Simulated time.
    """

    def hook(viz):
        viz._skip_target = seconds

    add_initialization_hook(hook)


def set_bounds(x1, y1, x2, y2):
    """Initial visible area, in simulation coordinates.

    @param x1 Left x coordinate.
    @param y1 Top y coordinate.
    @param x2 Right x coordinate.
    @param y2 Bottom y coordinate.
    """
    add_initialization_hook(lambda viz: viz.set_bounds(x1, y1, x2, y2))


def _download_directory():
    """Return the user's download directory, or None if there is none.

    Browsers packaged as snaps or flatpaks have a private /tmp but can read
    the download directory, so the page opening the visualizer is put there.

    @return The directory Path, or None.
    """
    home = Path.home()
    try:
        config = Path(os.environ.get("XDG_CONFIG_HOME") or home / ".config") / "user-dirs.dirs"
        match = re.search(r'^XDG_DOWNLOAD_DIR="(.*)"', config.read_text(), re.M)
        directory = Path(match.group(1).replace("$HOME", str(home))) if match else None
    except OSError:
        directory = None
    for candidate in (directory, home / "Downloads"):
        if candidate is not None and candidate.is_dir() and candidate != home:
            return candidate
    return None


def _open_browser(url):
    """Open the page in the default browser, discarding the browser's output.

    The browser is started with the URL on its command line, which other
    local users can read (e.g. with ps), so it is given a redirect page in a
    directory only this user can read instead of the URL with the token. The
    directory is created in the download directory, which sandboxed (snap,
    flatpak) browsers can read, or else in the temporary directory.

    The browser process inherits this process's terminal, so its own log
    lines (sandbox and zygote messages, "Opening in existing browser
    session") would otherwise mix with the simulation's output.

    @param url Page URL, with the token.
    @return The directory holding the redirect page, to remove on exit.
    """
    # mkdtemp() makes a directory readable by this user only.
    try:
        directory = tempfile.mkdtemp(prefix=".pyviz-", dir=_download_directory())
    except OSError:
        directory = tempfile.mkdtemp(prefix="pyviz-")
    page = Path(directory) / "open.html"
    fd = os.open(page, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as redirect:
        redirect.write(
            '<!doctype html><meta charset="utf-8">'
            '<meta http-equiv="refresh" content="0;url=%s">'
            "<title>ns-3 PyViz</title>"
            '<script>location.replace(%s)</script><a href="%s">Open PyViz</a>'
            % (html.escape(url), json.dumps(url), html.escape(url))
        )
    saved = [os.dup(1), os.dup(2)]
    devnull = os.open(os.devnull, os.O_WRONLY)
    try:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(devnull, 1)
        os.dup2(devnull, 2)
        webbrowser.open(page.as_uri())
    except Exception:
        pass
    finally:
        os.dup2(saved[0], 1)
        os.dup2(saved[1], 2)
        for fd in (*saved, devnull):
            os.close(fd)
    return directory


def start():
    """Serve the live topology view and run the current simulation.

    Blocks until the process is interrupted. When Restart reruns the
    scenario, the scenario's call ends the rerun (see Visualizer.restart()).
    """
    global _server, _launch_directory
    if _server is not None:
        if _viz.restarting:
            # Unwind the rerun scenario instead of returning, so its frames
            # (and objects owned by them) stay alive; see Visualizer.restart().
            raise _ScenarioReady()
        return _server
    # Packet metadata is needed for packet contents and the header filter.
    # It must be enabled before packets are created, hence here.
    print("PyViz: preparing the visualizer; this can take several seconds", flush=True)
    ns.Packet.EnablePrinting()
    _viz.attach()
    # Port 0 lets the OS pick a free port; binding to 127.0.0.1 keeps the
    # server (and its Python console) off the network.
    # Single threaded on purpose: every request must run on this thread (see
    # "Threading" in the module docstring).
    inherited = os.environ.pop(_SOCKET_VARIABLE, None)
    if inherited is None:
        _server = HTTPServer(("127.0.0.1", 0), _Handler)
    else:
        # The listening socket of the process restart() executed again.
        _server = HTTPServer(("127.0.0.1", 0), _Handler, bind_and_activate=False)
        _server.socket = socket.socket(fileno=int(inherited))
        _server.socket.set_inheritable(False)
        _server.server_address = _server.socket.getsockname()
    # The token travels in the fragment, which browsers never send to the
    # server; the page moves it to session storage and drops it from the URL.
    url = "http://%s:%d/#token=%s" % (*_server.server_address, _viz.token)
    if inherited is None:
        _launch_directory = _open_browser(url)
    print("PyViz is available at %s" % url, flush=True)
    try:
        while not _viz.exit_requested:
            # Run the simulation in slices while no request is waiting.
            wait = _viz.wants_slice()
            if wait is None or select.select([_server], [], [], wait)[0]:
                _server.handle_request()
            else:
                _viz.advance()
    finally:
        _server.server_close()
        if _launch_directory is not None:
            shutil.rmtree(_launch_directory, ignore_errors=True)
    with _viz.lock:
        if not _viz.finished and _viz.sim_helper is not None:
            print("PyViz: running the rest of the simulation", flush=True)
            _viz.sim_helper.SimulatorRun()
    return _server
