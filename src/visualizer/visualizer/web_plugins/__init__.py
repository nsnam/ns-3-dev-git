#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Built-in PyViz plugins.

Each module exposes ``register(viz)``, which adds panels, overlays, periodic
updates or tooltip information through the ``web.Visualizer`` API.
"""

import importlib
import sys
import traceback

## Names of the built-in plugin modules, loaded in this order.
PLUGINS = [
    "node_info",
    "interface_statistics",
    "ipv4_routing_table",
    "olsr",
    "show_last_packets",
    "wifi_links",
    "cellular_links",
    "flow_monitor",
    "energy",
]


def register_all(viz):
    """Register every built-in plugin, reporting the ones that fail to load.

    @param viz The web visualizer.
    """
    for name in PLUGINS:
        try:
            importlib.import_module("." + name, __name__).register(viz)
        except Exception:
            sys.stderr.write("PyViz: plugin %s failed to load\n" % name)
            traceback.print_exc()


def address_text(address):
    """Format a device address, as a MAC address when it is one.

    ConvertFrom() asserts on another address type (LTE and NR devices use
    Mac64Address), aborting the simulation, so the type is checked first.

    @param address The ns3::Address.
    @return The address text, or "" for an empty address.
    """
    from ns import ns

    if address.IsInvalid():
        return ""
    for mac in (ns.Mac48Address, ns.Mac64Address, ns.Mac16Address, ns.Mac8Address):
        if mac.IsMatchingType(address):
            return str(mac.ConvertFrom(address))
    return str(address)


def interface_name(device, index=None):
    """Name a device by its Names entry, or by its interface index.

    @param device The NetDevice, or None for the loopback interface.
    @param index Interface index to use instead of the device IfIndex.
    @return The interface name.
    """
    from ns import ns

    if device is None:
        return "lo"
    name = str(ns.Names.FindName(device))
    if name:
        return name
    return "(interface %i)" % (device.GetIfIndex() if index is None else index)
