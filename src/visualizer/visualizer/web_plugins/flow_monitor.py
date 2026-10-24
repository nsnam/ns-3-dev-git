#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Optional per-flow time series: throughput, bytes, latency and jitter.

The statistics are collected by the visualizer itself (the "flow-stats"
adapter in traces.py), with the definitions of the flow-monitor module, so a
FlowMonitor of the scenario is left undisturbed. Disabled unless the scenario calls ``visualizer.enable_flow_monitor()`` or the
user enables it from the panel. Flows are filtered to those whose source or
destination is one of the selected nodes, so a selection of several nodes
shows the traffic between them.
"""

import sys

from ns import ns

from ..traces import flow_stats

# The whole simulation is kept: when a flow reaches HISTORY samples, adjacent
# pairs are merged, halving the resolution instead of dropping the start.
## Number of samples per flow that triggers merging.
HISTORY = 1200
# Simulated seconds between samples. They are taken inside the simulation and
# collected in batches after each step, so the series stay even whatever the
# speed and the browser's poll rate.
## Simulated seconds between samples.
SAMPLE_INTERVAL = 0.1
## Maximum number of flows plotted in a chart.
MAX_SERIES = 8  # categorical palette size; further flows are listed but not plotted
## Panel views.
VIEWS = ("Time series", "Histograms")
# Histograms as (title, Flow member, unit, scale from the C++ unit, bin width
# in the C++ unit); the widths are the FlowMonitor defaults used in traces.py.
## Histograms shown by the Histograms view.
HISTOGRAMS = (
    ("Delay", "delayHistogram", "ms", 1e3, 0.001),
    ("Jitter", "jitterHistogram", "ms", 1e3, 0.001),
    ("Packet size", "packetSizeHistogram", "bytes", 1, 20),
)
## Shown instead of a chart when the plotted flows received nothing.
NO_PACKETS = "No packets received"
## Names of IP protocol numbers.
PROTOCOLS = {6: "TCP", 17: "UDP"}

## Flow monitor enable request made before the visualizer starts.
_requested = {"enabled": False}


def enable():
    """Request the flow monitor to be enabled when the visualizer starts."""
    _requested["enabled"] = True


def merge_pairs(points):
    """Merge adjacent samples, keeping the interval averages exact.

    @param points List of samples, oldest first.
    @return List of about half as many samples covering the same time.
    """
    merged = []
    for first, second in zip(points[::2], points[1::2]):
        dt, packets = first["dt"] + second["dt"], first["packets"] + second["packets"]

        def mean(key, weight):
            """Average a metric of both samples.

            @param key The metric key.
            @param weight The key of the weight of each sample.
            @return The weighted average, or None if both weights are zero.
            """
            parts = [(p[key], p[weight]) for p in (first, second) if p[key] is not None]
            total = sum(w for _, w in parts)
            return sum(v * w for v, w in parts) / total if total else None

        merged.append(
            {
                "t": second["t"],
                "dt": dt,
                "packets": packets,
                "throughput": mean("throughput", "dt"),
                "tx": second["tx"],
                "rx": second["rx"],
                "latency": mean("latency", "packets"),
                "jitter": mean("jitter", "packets"),
            }
        )
    if len(points) % 2:
        merged.append(points[-1])
    return merged


class FlowMonitorSeries:
    """Sample per-flow statistics into time series."""

    ## @var stats
    ## cppyy pyviz_flows namespace collecting the statistics
    ## @var address_nodes
    ## Map from IPv4 address string to node index
    ## @var flows
    ## Map from flow id to its label, endpoints, last counters and points
    ## @var enabled
    ## Whether the statistics are being collected
    ## @var view
    ## Panel view, one of VIEWS

    def __init__(self, stats):
        """Initializer.

        @param self The object pointer.
        @param stats The cppyy pyviz_flows namespace.
        """
        self.stats = stats
        self.enabled = False
        self.view = VIEWS[0]
        self.address_nodes = {}  # IPv4 or IPv6 address string -> node id
        self.flows = {}  # flow id -> dict(label, source, target, last, points)

    def start(self, viz):
        """Start collecting flow statistics on all nodes.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        self.stats.Install(SAMPLE_INTERVAL)
        self.enabled = True
        for index in range(int(ns.NodeList.GetNNodes())):
            node = ns.NodeList.GetNode(index)
            for ip, local in ((ns.Ipv4, "GetLocal"), (ns.Ipv6, "GetAddress")):
                protocol = node.GetObject[ip]()
                if not protocol:
                    continue
                for interface in range(int(protocol.GetNInterfaces())):
                    for address in range(int(protocol.GetNAddresses(interface))):
                        entry = protocol.GetAddress(interface, address)
                        self.address_nodes[str(getattr(entry, local)())] = index

    def periodic_update(self, viz):
        """Collect the samples taken during the last step.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        if not self.enabled:
            return
        self.stats.Advance()
        for sample in self.stats.samples:
            self._add_sample(sample)
        self.stats.samples.clear()

    def _add_sample(self, sample):
        """Add one flow's counters at a sample time to its series.

        @param self The object pointer.
        @param sample The pyviz_adapters::flows::Sample.
        """
        flow_id = int(sample.flow)
        flow = self.flows.get(flow_id)
        if flow is None:
            stats = self.stats.flows[flow_id - 1]
            source, target = str(stats.source), str(stats.destination)
            flow = self.flows[flow_id] = {
                "label": "Flow %d: %s:%d -> %s:%d %s"
                % (
                    flow_id,
                    source,
                    int(stats.sourcePort),
                    target,
                    int(stats.destinationPort),
                    PROTOCOLS.get(int(stats.protocol), str(int(stats.protocol))),
                ),
                "source": self.address_nodes.get(source),
                "target": self.address_nodes.get(target),
                "last": None,
                "points": [],
            }
        current = (
            float(sample.time),
            int(sample.txBytes),
            int(sample.rxBytes),
            int(sample.rxPackets),
            float(sample.delaySum),
            float(sample.jitterSum),
            int(sample.txPackets),
            int(sample.lostPackets),
        )
        last = flow["last"]
        if last is not None and current[0] > last[0]:
            dt = current[0] - last[0]
            rx_packets = current[3] - last[3]
            flow["points"].append(
                {
                    "t": current[0],
                    "dt": dt,
                    "packets": rx_packets,
                    "throughput": (current[2] - last[2]) * 8 / dt,
                    "tx": current[1],
                    "rx": current[2],
                    # Interval averages; no received packets means no sample.
                    "latency": (current[4] - last[4]) / rx_packets * 1e3 if rx_packets else None,
                    "jitter": (current[5] - last[5]) / rx_packets * 1e3 if rx_packets else None,
                }
            )
            if len(flow["points"]) >= HISTORY:
                flow["points"] = merge_pairs(flow["points"])
        flow["last"] = current

    def _selected_flows(self, viz):
        """Get the flows from or to the selected nodes (all if none selected).

        @param self The object pointer.
        @param viz The web visualizer.
        @return List of (flow id, flow) pairs.
        """
        selected = set(viz.selected_nodes)
        return [
            (flow_id, flow)
            for flow_id, flow in sorted(self.flows.items())
            if not selected or flow["source"] in selected or flow["target"] in selected
        ]

    def build(self, viz):
        """Build the flow monitor panel.

        @param self The object pointer.
        @param viz The web visualizer.
        @return The panel content dictionary.
        """
        if not self.enabled:
            return {
                "sections": [
                    {
                        "title": "Flow monitor is off",
                        "note": "Enable it to collect per-flow statistics from now on. "
                        "Scenarios can enable it from the start with "
                        "visualizer.enable_flow_monitor().",
                    }
                ],
                "controls": [{"name": "enable", "kind": "button", "label": "Enable flow monitor"}],
            }
        flows = self._selected_flows(viz)
        # The busiest flows are plotted, in flow id order; low id flows are
        # often signaling (e.g. GTP-C) that would hide the user traffic.
        plotted = sorted(
            sorted(flows, key=lambda f: f[1]["last"][2] if f[1]["last"] else 0, reverse=True)[
                :MAX_SERIES
            ]
        )
        scope = (
            "flows to or from nodes %s" % ", ".join(map(str, viz.selected_nodes))
            if viz.selected_nodes
            else "all flows (Shift/Ctrl-click nodes to filter)"
        )
        note = "Showing %s." % scope
        if len(flows) > MAX_SERIES:
            note += (
                " Plotting the %d of %d flows with the most bytes received; all are in the table."
                % (
                    MAX_SERIES,
                    len(flows),
                )
            )

        # Charts span the whole simulation so far, from 0 s.
        span = [0, float(ns.Simulator.Now().GetSeconds())]

        def series(key):
            """Build the plotted series of one metric.

            @param key The point metric key.
            @return List of series dictionaries.
            """
            return [
                {
                    "name": flow["label"],
                    "slot": index,
                    "points": [[p["t"], p[key]] for p in flow["points"]],
                }
                for index, (_id, flow) in enumerate(plotted)
            ]

        received = sum(flow["last"][3] for _id, flow in plotted if flow["last"])

        def chart(unit, key):
            """Build the plot of one per-flow metric, or say why it is empty.

            @param unit The plot unit.
            @param key The point metric key.
            @return The plot dictionary.
            """
            plot = {"unit": unit, "range": span, "series": series(key)}
            if plotted and not received:
                plot["empty"] = NO_PACKETS
            return plot

        # Totals over the plotted flows; each flow holds its latest value
        # between its own samples.
        samples = sorted(
            (p["t"], index, p["tx"], p["rx"])
            for index, (_id, flow) in enumerate(plotted)
            for p in flow["points"]
        )
        latest = {}
        tx_total, rx_total = [], []
        for t, index, tx, rx in samples:
            latest[index] = (tx, rx)
            point_tx = sum(v[0] for v in latest.values())
            point_rx = sum(v[1] for v in latest.values())
            if tx_total and tx_total[-1][0] == t:
                tx_total[-1][1], rx_total[-1][1] = point_tx, point_rx
            else:
                tx_total.append([t, point_tx])
                rx_total.append([t, point_rx])

        rows = []
        for flow_id, flow in flows:
            last = flow["last"]
            rx_packets = last[3] if last else 0
            rows.append(
                [
                    flow["label"],
                    last[6] if last else 0,
                    rx_packets,
                    last[7] if last else 0,
                    last[1] if last else 0,
                    last[2] if last else 0,
                    round(last[4] / rx_packets * 1e3, 3) if rx_packets else "-",
                    round(last[5] / (rx_packets - 1) * 1e3, 3) if rx_packets > 1 else "-",
                ]
            )
        table = {
            "title": "Flows",
            "columns": [
                "Flow",
                "Tx pkts",
                "Rx pkts",
                "Lost",
                "Tx bytes",
                "Rx bytes",
                "Mean delay (ms)",
                "Mean jitter (ms)",
            ],
            "rows": rows,
        }
        controls = [
            {
                "name": "view",
                "kind": "radio",
                "label": "View",
                "options": list(VIEWS),
                "value": self.view,
            }
        ]
        if self.view == "Histograms":
            sections = [self._histogram(plotted, *h) for h in HISTOGRAMS]
            for section in sections:
                if plotted and not any(s["bins"] for s in section["histogram"]["series"]):
                    # Jitter needs two packets of a flow.
                    section["histogram"]["empty"] = (
                        NO_PACKETS if not received else "Fewer than two packets received per flow"
                    )
            sections[0]["note"] = note
            return {"sections": sections + [table], "controls": controls}
        return {
            "sections": [
                {
                    "title": "Throughput",
                    "note": note,
                    "plot": chart("bit/s", "throughput"),
                },
                {
                    "title": "Total bytes",
                    "plot": {
                        "unit": "bytes",
                        "range": span,
                        "series": [
                            {"name": "Transmitted", "slot": 0, "points": tx_total},
                            {"name": "Received", "slot": 1, "points": rx_total},
                        ],
                    },
                },
                {
                    "title": "Latency (interval mean)",
                    "plot": chart("ms", "latency"),
                },
                {
                    "title": "Jitter (interval mean)",
                    "plot": chart("ms", "jitter"),
                },
                table,
            ],
            "controls": controls,
        }

    def _histogram(self, flows, title, member, unit, scale, width):
        """Build a histogram section of the plotted flows.

        @param self The object pointer.
        @param flows List of (flow id, flow) pairs to show.
        @param title The section title.
        @param member The pyviz_adapters::flows::Flow histogram member.
        @param unit The displayed unit.
        @param scale Factor from the collected unit to the displayed one.
        @param width The bin width, in the collected unit.
        @return The section dictionary.
        """
        series = []
        for index, (flow_id, flow) in enumerate(flows):
            histogram = getattr(self.stats.flows[flow_id - 1], member)
            bins = [[int(item.first), int(item.second)] for item in histogram]
            series.append({"name": flow["label"], "slot": index, "bins": bins})
        return {
            "title": title,
            "histogram": {"unit": unit, "binWidth": width * scale, "series": series},
        }

    def action(self, viz, values):
        """Handle the panel controls: enable the monitor, or switch the view.

        @param self The object pointer.
        @param viz The web visualizer.
        @param values Dictionary of submitted control values.
        """
        if values.get("enable") and not self.enabled:
            self.start(viz)
        if values.get("view") in VIEWS:
            self.view = values["view"]


def register(viz):
    """Add the flow monitor panel, greyed out when the internet module is not built.

    @param viz The web visualizer.
    """
    stats = flow_stats()
    if stats is None:
        reason = "Flow monitor unavailable: the internet module is not built"
        if _requested["enabled"]:
            sys.stderr.write("PyViz: %s\n" % reason)
        viz.add_panel("flow-monitor", "Flow monitor", lambda _viz: {}, unavailable=reason)
        return
    series = FlowMonitorSeries(stats)
    if _requested["enabled"]:
        series.start(viz)
    viz.add_periodic_update(series.periodic_update)
    viz.add_panel("flow-monitor", "Flow monitor", series.build, series.action)
