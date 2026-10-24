#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Node summary: name, mobility, devices, MAC and IP addresses."""

from ns import ns

from . import address_text


def _device_lines(node):
    """Describe the devices of a node and their addresses.

    @param node The node.
    @return List of [label, value] lines.
    """
    ipv4 = node.GetObject[ns.Ipv4]()
    ipv6 = node.GetObject[ns.Ipv6]()
    lines = []
    for index in range(int(node.GetNDevices())):
        device = node.GetDevice(index)
        type_name = str(device.GetInstanceTypeId().GetName())
        lines.append(["NetDevice %d" % index, type_name])
        name = str(ns.Names.FindName(device))
        if name:
            lines.append(["  Name", name])
        if type_name == "ns3::lrwpan::LrWpanNetDevice":
            lines.append(["  MAC short address", str(device.GetMac().GetShortAddress())])
            lines.append(["  MAC extended address", str(device.GetMac().GetExtendedAddress())])
        else:
            address = device.GetAddress()
            suffix = " (Generated)" if type_name == "ns3::SixLowPanNetDevice" else ""
            text = address_text(address)
            if text != str(address):
                lines.append(["  MAC address", text + suffix])
            else:
                lines.append(["  Address", text])
        if ipv4:
            interface = int(ipv4.GetInterfaceForDevice(device))
            if interface != -1:
                for i in range(int(ipv4.GetNAddresses(interface))):
                    address = ipv4.GetAddress(interface, i)
                    lines.append(
                        [
                            "  IPv4 address",
                            "%s/%s (Subnet Mask: %s)"
                            % (
                                address.GetLocal(),
                                address.GetMask().GetPrefixLength(),
                                address.GetMask(),
                            ),
                        ]
                    )
        if ipv6:
            interface = int(ipv6.GetInterfaceForDevice(device))
            if interface != -1:
                for i in range(int(ipv6.GetNAddresses(interface))):
                    address = ipv6.GetAddress(interface, i)
                    lines.append(
                        ["  IPv6 address", "%s%s" % (address.GetAddress(), address.GetPrefix())]
                    )
    return lines


def build(viz, node_index):
    """Build the node summary panel.

    @param viz The web visualizer.
    @param node_index The node index.
    @return The panel content dictionary.
    """
    node = ns.NodeList.GetNode(node_index)
    items = []
    name = str(ns.Names.FindName(node))
    if name:
        items.append(["Name", name])
    items.extend(viz.tooltip_lines(node_index))
    mobility = node.GetObject[ns.MobilityModel]()
    if mobility:
        position = mobility.__deref__().GetPosition()
        items.append(["Mobility model", str(mobility.__deref__().GetInstanceTypeId().GetName())])
        items.append(["Position", "(%.2f, %.2f, %.2f)" % (position.x, position.y, position.z)])
    items.extend(_device_lines(node))
    return {"sections": [{"title": "Node %d" % node_index, "items": items}]}


def register(viz):
    """Add the node summary panel.

    @param viz The web visualizer.
    """
    viz.add_node_panel("node-info", "Node summary", build)
