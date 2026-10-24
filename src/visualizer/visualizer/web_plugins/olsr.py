#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""OLSR routing table, available when the olsr module is built."""

from ns import ns

from ..traces import type_is_registered
from . import interface_name


def _olsr(node_index):
    """Get the OLSR routing protocol of a node.

    @param node_index The node index.
    @return The olsr::RoutingProtocol pointer (null if absent).
    """
    return ns.NodeList.GetNode(node_index).GetObject[ns.olsr.RoutingProtocol]()


def build(viz, node_index):
    """Build the OLSR routing table panel of a node.

    @param viz The web visualizer.
    @param node_index The node index.
    @return The panel content dictionary.
    """
    ipv4 = ns.NodeList.GetNode(node_index).GetObject[ns.Ipv4]()
    rows = []
    for route in _olsr(node_index).__deref__().GetRoutingTableEntries():
        device = ipv4.GetNetDevice(route.interface)
        rows.append(
            [
                str(route.destAddr),
                str(route.nextAddr),
                interface_name(device if device else None, int(route.interface)),
                int(route.distance),
            ]
        )
    return {
        "sections": [
            {
                "title": "OLSR routing table",
                "columns": ["Destination", "Next hop", "Interface", "Num. Hops"],
                "rows": rows,
            }
        ]
    }


def register(viz):
    """Add the OLSR routing table node panel, if the olsr module is built.

    @param viz The web visualizer.
    """
    if not type_is_registered("ns3::olsr::RoutingProtocol"):
        return
    viz.add_node_panel(
        "olsr-routing", "OLSR routing table", build, applies=lambda viz, n: bool(_olsr(n))
    )
