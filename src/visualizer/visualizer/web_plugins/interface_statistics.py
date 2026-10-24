#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Per-interface packet and bit counters and rates."""

from ns import ns

from . import interface_name

## Number of statistics samples kept per node.
NODE_STATISTICS_MEMORY = 10


class StatisticsCollector:
    """Collect per-interface counters and show them in a node panel."""

    ## @var node_statistics
    ## Map from node index to its recent statistics samples

    def __init__(self):
        """Initializer.

        @param self The object pointer.
        """
        # node id -> list of (simulation time, [(txP, txB, rxP, rxB) per interface])
        self.node_statistics = {}

    def periodic_update(self, viz):
        """Sample the interface counters of every node.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        for stats in viz.sim_helper.GetNodesStatistics():
            history = self.node_statistics.setdefault(int(stats.nodeId), [])
            history.append(
                (
                    float(ns.Simulator.Now().GetSeconds()),
                    [
                        (
                            int(s.transmittedPackets),
                            int(s.transmittedBytes),
                            int(s.receivedPackets),
                            int(s.receivedBytes),
                        )
                        for s in stats.statistics
                    ],
                )
            )
            del history[: max(0, len(history) - NODE_STATISTICS_MEMORY)]

    def build(self, viz, node_index):
        """Build the interface statistics panel of a node.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node_index The node index.
        @return The panel content dictionary.
        """
        columns = [
            "Interface",
            "Tx Packets",
            "Tx Bytes",
            "Tx pkt/1s",
            "Tx bit/1s",
            "Rx Packets",
            "Rx Bytes",
            "Rx pkt/1s",
            "Rx bit/1s",
        ]
        history = self.node_statistics.get(node_index, [])
        if len(history) < 2:
            return {
                "sections": [
                    {
                        "title": "Interface statistics",
                        "columns": columns,
                        "rows": [],
                        "note": "Collecting samples...",
                    }
                ]
            }
        node = ns.NodeList.GetNode(node_index)
        # Rates use whatever history is available so the table fills
        # before the full window is collected.
        span = history[-1][0] - history[0][0]
        if span <= 0:
            return {
                "sections": [
                    {
                        "title": "Interface statistics",
                        "columns": columns,
                        "rows": [],
                        "note": "Collecting samples...",
                    }
                ]
            }
        first, last = history[0][1], history[-1][1]
        rows = []
        for iface, (tx_p, tx_b, rx_p, rx_b) in enumerate(last):
            tx_p0, tx_b0, rx_p0, rx_b0 = first[iface] if iface < len(first) else (0, 0, 0, 0)
            device = node.GetDevice(iface) if iface < int(node.GetNDevices()) else None
            rows.append(
                [
                    interface_name(device, iface),
                    tx_p,
                    tx_b,
                    round((tx_p - tx_p0) / span, 2),
                    round((tx_b - tx_b0) * 8 / span, 2),
                    rx_p,
                    rx_b,
                    round((rx_p - rx_p0) / span, 2),
                    round((rx_b - rx_b0) * 8 / span, 2),
                ]
            )
        return {"sections": [{"title": "Interface statistics", "columns": columns, "rows": rows}]}


def register(viz):
    """Install the statistics collector and its node panel.

    @param viz The web visualizer.
    """
    collector = StatisticsCollector()
    viz.add_periodic_update(collector.periodic_update)
    viz.add_node_panel("interface-statistics", "Interface statistics", collector.build)
