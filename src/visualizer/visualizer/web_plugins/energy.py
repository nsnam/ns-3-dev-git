#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Batteries of nodes with an energy source (energy module).

Adds to each node's snapshot a "battery" entry (charge fraction, remaining
energy and mAh) drawn as a battery icon, a per-node "Battery" panel (opened by
clicking the icon) with the charge over time, its drainers (device energy
models) and charge sources (energy harvesters), and a simulation-wide
"Energy" panel with the network totals.
"""

from ns import ns

from ..traces import type_is_registered

## Simulated seconds between history samples.
SAMPLE_INTERVAL = 0.1  # simulated seconds between history samples
## Number of samples kept per node; older history is thinned out.
HISTORY_LIMIT = 1200  # samples kept; older history is thinned out, not dropped
## Maximum number of nodes plotted in a chart.
MAX_SERIES = 8  # charts plot at most this many nodes (categorical palette size)


def _mah(joules, voltage):
    """Charge in mAh for an energy at a supply voltage: J / V = A*s, / 3.6 = mAh.

    @param joules Energy in joules.
    @param voltage Supply voltage in volts.
    @return Charge in mAh, or 0 if the voltage is not positive.
    """
    return joules / voltage / 3.6 if voltage > 0 else 0.0


def _sources(node_index):
    """Get the energy sources of a node.

    @param node_index The node index.
    @return List of EnergySource pointers.
    """
    node = ns.NodeList.GetNode(node_index)
    container = node.GetObject[ns.energy.EnergySourceContainer]()
    if container:
        return [container.Get(index) for index in range(int(container.GetN()))]
    # Some scenarios aggregate a single energy source directly to the node.
    source = node.GetObject[ns.energy.EnergySource]()
    return [source] if source else []


def _harvesters(node_index):
    """Get the energy harvesters of a node.

    @param node_index The node index.
    @return List of EnergyHarvester pointers.
    """
    container = ns.NodeList.GetNode(node_index).GetObject[ns.energy.EnergyHarvesterContainer]()
    if not container:
        return []
    return [container.Get(index) for index in range(int(container.GetN()))]


def _device_model_types():
    """All registered DeviceEnergyModel subclasses (EnergySource only finds models by exact type).

    @return List of DeviceEnergyModel subclass TypeIds.
    """
    base = ns.TypeId.LookupByName("ns3::energy::DeviceEnergyModel")
    types = []
    for index in range(int(ns.TypeId.GetRegisteredN())):
        type_id = ns.TypeId.GetRegistered(index)
        parent = type_id
        while parent.HasParent() and parent != base:
            parent = parent.GetParent()
        if parent == base and type_id != base:
            types.append(type_id)
    return types


def battery(node_index):
    """Remaining charge summed over the node's energy sources, or None.

    @param node_index The node index.
    @return Dictionary with charge fraction, energy and mAh, or None without sources.
    """
    sources = _sources(node_index)
    if not sources:
        return None
    remaining = initial = remaining_mah = initial_mah = 0.0
    for source in sources:
        voltage = float(source.GetSupplyVoltage())
        source_remaining = float(source.GetRemainingEnergy())
        source_initial = float(source.GetInitialEnergy())
        remaining += source_remaining
        initial += source_initial
        remaining_mah += _mah(source_remaining, voltage)
        initial_mah += _mah(source_initial, voltage)
    return {
        "fraction": max(0.0, min(1.0, remaining / initial)) if initial > 0 else 0.0,
        "remainingJ": remaining,
        "initialJ": initial,
        "remainingmAh": remaining_mah,
        "capacitymAh": initial_mah,
    }


class EnergyMonitor:
    """Sample node batteries and build the battery and energy panels."""

    ## @var model_types
    ## Registered DeviceEnergyModel subclass TypeIds
    ## @var nodes
    ## Indices of the nodes with an energy source
    ## @var history
    ## Map from node index to [time, percent, remaining J] samples
    ## @var last_sample
    ## Simulation time of the previous sample, or None
    ## @var drainers
    ## Method listing the device energy model rows of a node
    ## @var harvesters
    ## Method listing the energy harvester rows of a node
    ## @var source_rows
    ## Method listing the energy source rows of a node

    def __init__(self):
        """Initializer.

        @param self The object pointer.
        """
        self.model_types = _device_model_types()
        self.nodes = [i for i in range(int(ns.NodeList.GetNNodes())) if _sources(i)]
        self.history = {}  # node -> [[t, percent, remaining J], ...]
        self.last_sample = None

    # -- sampling --------------------------------------------------------

    def periodic_update(self, viz):
        """Sample the battery of every node at SAMPLE_INTERVAL.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        now = float(ns.Simulator.Now().GetSeconds())
        if self.last_sample is not None and now - self.last_sample < SAMPLE_INTERVAL:
            return
        self.last_sample = now
        for node in self.nodes:
            state = battery(node)
            history = self.history.setdefault(node, [])
            history.append([now, state["fraction"] * 100, state["remainingJ"]])
            if len(history) > HISTORY_LIMIT:
                # Keep the whole run visible: halve the resolution of the
                # older half instead of discarding the start.
                half = len(history) // 2
                history[:half] = history[:half:2]

    def total_history(self):
        """Network remaining energy over time, summed over nodes at shared sample times.

        @param self The object pointer.
        @return List of [time, joules] points.
        """
        series = {}
        for history in self.history.values():
            for t, _percent, joules in history:
                series[t] = series.get(t, 0.0) + joules
        return [[t, joules] for t, joules in sorted(series.items())]

    # -- drainers and charge sources --------------------------------------

    def drainers(self, node):
        """List the device energy models draining a node's sources.

        @param self The object pointer.
        @param node The node index.
        @return List of [node, source, model, current mA, consumed J] rows.
        """
        rows = []
        for source_index, source in enumerate(_sources(node)):
            for type_id in self.model_types:
                models = source.FindDeviceEnergyModels(type_id)
                for index in range(int(models.GetN())):
                    model = models.Get(index)
                    rows.append(
                        [
                            node,
                            source_index,
                            str(type_id.GetName()),
                            round(float(model.GetCurrentA()) * 1e3, 3),
                            round(float(model.GetTotalEnergyConsumption()), 3),
                        ]
                    )
        return rows

    def harvesters(self, node):
        """List the energy harvesters of a node.

        @param self The object pointer.
        @param node The node index.
        @return List of [node, harvester, power mW] rows.
        """
        rows = []
        for harvester in _harvesters(node):
            rows.append(
                [
                    node,
                    str(harvester.GetInstanceTypeId().GetName()),
                    round(float(harvester.GetPower()) * 1e3, 3),
                ]
            )
        return rows

    def source_rows(self, node):
        """List the energy sources of a node.

        @param self The object pointer.
        @param node The node index.
        @return List of energy source table rows.
        """
        rows = []
        for index, source in enumerate(_sources(node)):
            voltage = float(source.GetSupplyVoltage())
            remaining, initial = float(source.GetRemainingEnergy()), float(
                source.GetInitialEnergy()
            )
            rows.append(
                [
                    node,
                    index,
                    str(source.GetInstanceTypeId().GetName()),
                    round(voltage, 3),
                    round(initial, 3),
                    round(remaining, 3),
                    round(_mah(remaining, voltage), 3),
                    round(100 * remaining / initial, 2) if initial > 0 else 0.0,
                ]
            )
        return rows

    # -- panels ------------------------------------------------------------

    def build_node(self, viz, node):
        """Build the battery panel of a node.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node The node index.
        @return The panel content dictionary.
        """
        state = battery(node)
        history = self.history.get(node, [])
        sections = [
            {
                "title": "Battery",
                "items": [
                    ["Charge", "%.2f%%" % (state["fraction"] * 100)],
                    [
                        "Remaining",
                        "%.3f mAh (%.3f J)" % (state["remainingmAh"], state["remainingJ"]),
                    ],
                    ["Capacity", "%.3f mAh (%.3f J)" % (state["capacitymAh"], state["initialJ"])],
                ],
            },
            {
                "title": "Charge",
                "plot": {
                    "unit": "%",
                    "series": [
                        {
                            "name": "Node %d" % node,
                            "slot": 0,
                            "points": [[t, p] for t, p, _j in history],
                        }
                    ],
                },
            },
            {
                "title": "Remaining energy",
                "plot": {
                    "unit": "J",
                    "series": [
                        {
                            "name": "Node %d" % node,
                            "slot": 0,
                            "points": [[t, j] for t, _p, j in history],
                        }
                    ],
                },
            },
            {"title": "Sources", "columns": SOURCE_COLUMNS, "rows": self.source_rows(node)},
            {
                "title": "Drainers (device energy models)",
                "columns": DRAINER_COLUMNS,
                "rows": self.drainers(node),
            },
        ]
        harvesters = self.harvesters(node)
        if harvesters:
            sections.append(
                {
                    "title": "Charge sources (harvesters)",
                    "columns": HARVESTER_COLUMNS,
                    "rows": harvesters,
                }
            )
        return {"sections": sections}

    def build_global(self, viz):
        """Build the network-wide energy panel.

        @param self The object pointer.
        @param viz The web visualizer.
        @return The panel content dictionary.
        """
        if not self.nodes:
            return {"sections": [{"title": "Energy", "note": "No node has an energy source."}]}
        states = {node: battery(node) for node in self.nodes}
        initial = sum(s["initialJ"] for s in states.values())
        remaining = sum(s["remainingJ"] for s in states.values())
        plotted = self.nodes[:MAX_SERIES]
        note = None
        if len(self.nodes) > MAX_SERIES:
            note = "Plotting the first %d of %d nodes; all are in the tables." % (
                MAX_SERIES,
                len(self.nodes),
            )
        rows = lambda method: [row for node in self.nodes for row in method(node)]
        return {
            "sections": [
                {
                    "title": "Network",
                    "items": [
                        ["Nodes with an energy source", str(len(self.nodes))],
                        [
                            "Energy at the start",
                            "%.3f J (%.3f mAh)"
                            % (initial, sum(s["capacitymAh"] for s in states.values())),
                        ],
                        [
                            "Energy now",
                            "%.3f J (%.3f mAh)"
                            % (remaining, sum(s["remainingmAh"] for s in states.values())),
                        ],
                        [
                            "Consumed",
                            "%.3f J (%.2f%%)"
                            % (
                                initial - remaining,
                                100 * (initial - remaining) / initial if initial else 0,
                            ),
                        ],
                    ],
                },
                {
                    "title": "Charge per node",
                    "note": note,
                    "plot": {
                        "unit": "%",
                        "series": [
                            {
                                "name": "Node %d" % node,
                                "slot": slot,
                                "points": [[t, p] for t, p, _j in self.history.get(node, [])],
                            }
                            for slot, node in enumerate(plotted)
                        ],
                    },
                },
                {
                    "title": "Total remaining energy",
                    "plot": {
                        "unit": "J",
                        "series": [{"name": "Total", "slot": 0, "points": self.total_history()}],
                    },
                },
                {"title": "Sources", "columns": SOURCE_COLUMNS, "rows": rows(self.source_rows)},
                {
                    "title": "Drainers (device energy models)",
                    "columns": DRAINER_COLUMNS,
                    "rows": rows(self.drainers),
                },
                {
                    "title": "Charge sources (harvesters)",
                    "columns": HARVESTER_COLUMNS,
                    "rows": rows(self.harvesters),
                },
            ]
        }

    # -- node snapshot and summary ------------------------------------------

    def node_info(self, viz, node):
        """Add the battery state to a node snapshot.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node The node index.
        @return Dictionary with a battery entry, or empty.
        """
        state = battery(node) if node in self.nodes else None
        return {"battery": state} if state else {}

    def tooltip_info(self, viz, node):
        """Describe the battery of a node in its tooltip.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node The node index.
        @return List of [label, value] lines.
        """
        if node not in self.nodes:
            return []
        state = battery(node)
        return [
            ["Battery", "%.1f%%" % (state["fraction"] * 100)],
            [
                "Remaining",
                "%.2f mAh (%.2f J) of %.2f mAh"
                % (state["remainingmAh"], state["remainingJ"], state["capacitymAh"]),
            ],
        ]


## Column titles of the energy source tables.
SOURCE_COLUMNS = [
    "Node",
    "Source",
    "Type",
    "Voltage (V)",
    "Initial (J)",
    "Remaining (J)",
    "Remaining (mAh)",
    "Charge (%)",
]
## Column titles of the device energy model tables.
DRAINER_COLUMNS = ["Node", "Source", "Model", "Current (mA)", "Consumed (J)"]
## Column titles of the energy harvester tables.
HARVESTER_COLUMNS = ["Node", "Harvester", "Power (mW)"]


def register(viz):
    """Install the energy monitor, if the energy module is built.

    @param viz The web visualizer.
    """
    if not type_is_registered("ns3::energy::EnergySourceContainer"):
        return
    monitor = EnergyMonitor()
    viz.add_periodic_update(monitor.periodic_update)
    viz.add_node_info(monitor.node_info)
    viz.add_tooltip_info(monitor.tooltip_info)
    viz.add_node_panel(
        "battery", "Battery", monitor.build_node, applies=lambda viz, node: node in monitor.nodes
    )
    viz.add_panel("energy", "Energy", monitor.build_global)
