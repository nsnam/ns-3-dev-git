#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Dashed links from Wi-Fi stations to the access point they are associated with."""

from ns import ns

from ..traces import type_is_registered


class WifiLinkMonitor:
    """Track Wi-Fi station to access point associations and draw them as links."""

    ## @var access_points
    ## Map from AP BSSID string to AP node index
    ## @var stations
    ## List of (WifiNetDevice, node index) pairs for stations
    ## @var links
    ## Overlay link descriptions from the latest update

    def __init__(self):
        """Initializer.

        @param self The object pointer.
        """
        self.access_points = {}  # bssid -> node id
        self.stations = []  # (device, node id)
        self.links = []

    def scan(self):
        """Find the Wi-Fi stations and access points of all nodes.

        @param self The object pointer.
        """
        for index in range(int(ns.NodeList.GetNNodes())):
            node = ns.NodeList.GetNode(index)
            for device_index in range(int(node.GetNDevices())):
                device = node.GetDevice(device_index)
                if str(device.GetInstanceTypeId().GetName()) != "ns3::WifiNetDevice":
                    continue
                device = ns.DynamicCast[ns.WifiNetDevice](device)
                mac_type = str(device.GetMac().GetInstanceTypeId().GetName())
                if mac_type == "ns3::StaWifiMac":
                    self.stations.append((device, index))
                elif mac_type == "ns3::ApWifiMac":
                    self.access_points[str(ns.Mac48Address.ConvertFrom(device.GetAddress()))] = (
                        index
                    )

    def periodic_update(self, viz):
        """Rebuild the links of associated stations.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        links = []
        for device, station in self.stations:
            if not device.IsLinkUp():
                continue
            mac = ns.DynamicCast[ns.StaWifiMac](device.GetMac())
            bssid = str(mac.GetBssid(0))
            access_point = self.access_points.get(bssid)
            if access_point is None:
                continue
            a = ns.NodeList.GetNode(station).GetObject[ns.MobilityModel]()
            b = ns.NodeList.GetNode(access_point).GetObject[ns.MobilityModel]()
            distance = (" distance=%.2f m." % a.__deref__().GetDistanceFrom(b)) if a and b else ""
            links.append(
                {
                    "source": station,
                    "target": access_point,
                    "class": "wifi-link",
                    "title": "WiFi link between STA Node %d and AP Node %d;%s\nSSID: %s\nBSSID: %s"
                    % (station, access_point, distance, mac.GetSsid(), bssid),
                }
            )
        self.links = links

    def overlay(self, viz):
        """Return the current Wi-Fi links overlay.

        @param self The object pointer.
        @param viz The web visualizer.
        @return The list of link descriptions.
        """
        return self.links


def register(viz):
    """Install the Wi-Fi link monitor, if the wifi module is built.

    @param viz The web visualizer.
    """
    if not type_is_registered("ns3::WifiNetDevice"):
        return
    monitor = WifiLinkMonitor()
    monitor.scan()
    viz.add_periodic_update(monitor.periodic_update)
    viz.add_overlay(monitor.overlay)
