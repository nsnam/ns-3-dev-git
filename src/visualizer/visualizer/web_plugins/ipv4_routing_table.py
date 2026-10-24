#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""IPv4 static and global routing tables."""

import ctypes

from ns import ns

from . import interface_name


def _protocols(routing):
    """List the static and global routing protocols of a routing protocol.

    @param routing The node Ipv4RoutingProtocol.
    @return List of (protocol, type string, priority) tuples.
    """
    type_name = str(routing.GetInstanceTypeId().GetName())
    if type_name == "ns3::Ipv4StaticRouting":
        return [(ns.DynamicCast[ns.Ipv4StaticRouting](routing), "static", 0)]
    if type_name != "ns3::Ipv4ListRouting":
        return []
    list_routing = ns.DynamicCast[ns.Ipv4ListRouting](routing)
    result = []
    for index in range(int(list_routing.GetNRoutingProtocols())):
        priority = ctypes.c_int16(0)
        protocol = list_routing.GetRoutingProtocol(index, priority)
        name = str(protocol.GetInstanceTypeId().GetName())
        if name == "ns3::Ipv4StaticRouting":
            result.append(
                (ns.DynamicCast[ns.Ipv4StaticRouting](protocol), "static", priority.value)
            )
        elif name == "ns3::Ipv4GlobalRouting":
            result.append(
                (ns.DynamicCast[ns.Ipv4GlobalRouting](protocol), "global", priority.value)
            )
    return result


def build(viz, node_index):
    """Build the IPv4 routing table panel of a node.

    @param viz The web visualizer.
    @param node_index The node index.
    @return The panel content dictionary.
    """
    ipv4 = ns.NodeList.GetNode(node_index).GetObject[ns.Ipv4]()
    rows = []
    routing = ipv4.GetRoutingProtocol() if ipv4 else None
    for protocol, type_string, priority in (_protocols(routing) if routing else []):
        for index in range(int(protocol.GetNRoutes())):
            route = protocol.GetRoute(index)
            interface = int(route.GetInterface())
            device = ipv4.GetNetDevice(interface)
            rows.append(
                [
                    str(route.GetDest()),
                    str(route.GetGateway()),
                    interface_name(device if device else None, interface),
                    type_string,
                    priority,
                ]
            )
    return {
        "sections": [
            {
                "title": "IPv4 routing table",
                "columns": ["Destination", "Next hop", "Interface", "Type", "Prio"],
                "rows": rows,
            }
        ]
    }


def register(viz):
    """Add the IPv4 routing table node panel.

    @param viz The web visualizer.
    """
    viz.add_node_panel(
        "ipv4-routing",
        "IPv4 routing table",
        build,
        applies=lambda viz, n: bool(ns.NodeList.GetNode(n).GetObject[ns.Ipv4]()),
    )
