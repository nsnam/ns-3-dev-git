#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Dashed links from LTE and NR UEs to the cell they are attached to."""

from ns import ns

from ..traces import type_is_registered

## Technologies as (label, UE device type, base station device type) tuples.
_TECHNOLOGIES = [
    ("LTE", "ns3::LteUeNetDevice", "ns3::LteEnbNetDevice"),
    ("NR", "ns3::NrUeNetDevice", "ns3::NrGnbNetDevice"),
]


class CellularLinkMonitor:
    """Track UE to serving cell attachments and draw them as links."""

    ## @var cells
    ## Map from (technology, cell ID) to base station node index
    ## @var ues
    ## List of (technology, UE RRC, node index) tuples
    ## @var links
    ## Overlay link descriptions from the latest update

    def __init__(self):
        """Initializer.

        @param self The object pointer.
        """
        self.cells = {}
        self.ues = []
        self.links = []

    def scan(self):
        """Find the UEs and base stations of all nodes.

        @param self The object pointer.
        """
        for label, ue_type, bs_type in _TECHNOLOGIES:
            if not type_is_registered(ue_type):
                continue
            ue_class = getattr(ns, ue_type.split("::")[1])
            bs_class = getattr(ns, bs_type.split("::")[1])
            for index in range(int(ns.NodeList.GetNNodes())):
                node = ns.NodeList.GetNode(index)
                for device_index in range(int(node.GetNDevices())):
                    device = node.GetDevice(device_index)
                    type_name = str(device.GetInstanceTypeId().GetName())
                    if type_name == ue_type:
                        rrc = ns.DynamicCast[ue_class](device).GetRrc()
                        self.ues.append((label, rrc, index))
                    elif type_name == bs_type:
                        bs = ns.DynamicCast[bs_class](device)
                        # With carrier aggregation, a base station serves
                        # several cells.
                        if label == "LTE":
                            cell_ids = list(bs.GetCellIds())
                        else:
                            cell_ids = [bs.GetCellId()]
                        for cell_id in cell_ids:
                            self.cells[(label, int(cell_id))] = index

    def periodic_update(self, viz):
        """Rebuild the links of attached UEs.

        @param self The object pointer.
        @param viz The web visualizer.
        """
        links = []
        for label, rrc, ue in self.ues:
            cell_id = int(rrc.GetCellId())
            bs = self.cells.get((label, cell_id))
            if bs is None:
                continue
            a = ns.NodeList.GetNode(ue).GetObject[ns.MobilityModel]()
            b = ns.NodeList.GetNode(bs).GetObject[ns.MobilityModel]()
            distance = (" distance=%.2f m." % a.__deref__().GetDistanceFrom(b)) if a and b else ""
            links.append(
                {
                    "source": ue,
                    "target": bs,
                    "class": "cellular-link",
                    "title": "%s link between UE Node %d and %s Node %d;%s\nCell ID: %d\nIMSI: %d"
                    % (
                        label,
                        ue,
                        "eNB" if label == "LTE" else "gNB",
                        bs,
                        distance,
                        cell_id,
                        int(rrc.GetImsi()),
                    ),
                }
            )
        self.links = links

    def overlay(self, viz):
        """Return the current cellular links overlay.

        @param self The object pointer.
        @param viz The web visualizer.
        @return The list of link descriptions.
        """
        return self.links


def register(viz):
    """Install the cellular link monitor, if the lte or nr module is built.

    @param viz The web visualizer.
    """
    if not any(type_is_registered(ue_type) for _, ue_type, _ in _TECHNOLOGIES):
        return
    monitor = CellularLinkMonitor()
    monitor.scan()
    viz.add_periodic_update(monitor.periodic_update)
    viz.add_overlay(monitor.overlay)
