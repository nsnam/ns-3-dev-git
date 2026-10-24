#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Last transmitted, received and dropped packets, with a header filter."""

from ns import ns

from . import address_text, interface_name


def _header_types():
    """Find every registered Header and Trailer subclass.

    @return Map from type name to TypeId.
    """
    header = ns.TypeId.LookupByName("ns3::Header")
    trailer = ns.TypeId.LookupByName("ns3::Trailer")
    result = {}
    for index in range(int(ns.TypeId.GetRegisteredN())):
        type_id = ns.TypeId.GetRegistered(index)
        if type_id == header or type_id == trailer:
            continue
        parent = type_id
        while True:
            if parent == header or parent == trailer:
                result[str(type_id.GetName())] = type_id
                break
            if not parent.HasParent():
                break
            parent = parent.GetParent()
    return result


class LastPackets:
    """Node panel showing the last packets with a header filter."""

    ## @var headers
    ## Map from header type name to TypeId
    ## @var options
    ## Map from node index to its selected headers and match mode

    def __init__(self):
        """Initializer.

        @param self The object pointer.
        """
        self.headers = _header_types()
        self.options = {}  # node -> {"headers": [...], "mode": "OR"|"AND"}

    def _apply(self, viz, node_index):
        """Push the node's filter selection to the C++ packet capture.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node_index The node index.
        """
        selection = self.options[node_index]
        options = ns.visualizer.PacketCaptureOptions()
        options.numLastPackets = 100
        options.mode = (
            ns.visualizer.PACKET_CAPTURE_FILTER_HEADERS_AND
            if selection["mode"] == "AND"
            else ns.visualizer.PACKET_CAPTURE_FILTER_HEADERS_OR
        )
        for name in selection["headers"]:
            options.headers.insert(self.headers[name])
        viz.sim_helper.SetPacketCaptureOptions(node_index, options)

    def _rows(self, samples, peer=None):
        """Convert packet samples into table rows.

        @param self The object pointer.
        @param samples The packet samples.
        @param peer Name of the sample's peer address field ("to" or "from"), if any.
        @return List of [time, interface, size, (peer address,) contents] rows.
        """
        rows = []
        for sample in samples:
            device = sample.device if sample.device else None
            row = [
                round(float(sample.time.GetSeconds()), 6),
                "(unknown)" if device is None else interface_name(device),
                int(sample.packet.GetSize()),
            ]
            if peer is not None:
                row.append(address_text(getattr(sample, peer)))
            row.append(str(sample.packet.ToString()))
            rows.append(row)
        return rows

    def build(self, viz, node_index):
        """Build the last packets panel of a node.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node_index The node index.
        @return The panel content dictionary.
        """
        if node_index not in self.options:
            self.options[node_index] = {"headers": sorted(self.headers), "mode": "OR"}
            self._apply(viz, node_index)
        last = viz.sim_helper.GetLastPackets(node_index)
        columns = ["Time", "Interface", "Size", "Contents"]
        selection = self.options[node_index]
        return {
            "sections": [
                {
                    "title": "Last transmitted packets",
                    "columns": columns[:3] + ["To"] + columns[3:],
                    "rows": self._rows(last.lastTransmittedPackets, "to"),
                },
                {
                    "title": "Last received packets",
                    "columns": columns[:3] + ["From"] + columns[3:],
                    "rows": self._rows(last.lastReceivedPackets, "from"),
                },
                {
                    "title": "Last dropped packets",
                    "columns": columns,
                    "rows": self._rows(last.lastDroppedPackets),
                },
            ],
            "controls": [
                {
                    "name": "mode",
                    "kind": "radio",
                    "label": "Header match",
                    "options": ["OR", "AND"],
                    "value": selection["mode"],
                },
                {
                    "name": "headers",
                    "kind": "checklist",
                    "label": "Headers",
                    "options": sorted(self.headers),
                    "value": selection["headers"],
                },
            ],
        }

    def action(self, viz, node_index, values):
        """Apply the filter controls submitted from the panel.

        @param self The object pointer.
        @param viz The web visualizer.
        @param node_index The node index.
        @param values Dictionary of submitted control values.
        """
        headers = [h for h in values.get("headers", []) if h in self.headers]
        mode = "AND" if values.get("mode") == "AND" else "OR"
        self.options[node_index] = {"headers": headers, "mode": mode}
        self._apply(viz, node_index)


def register(viz):
    """Add the last packets node panel.

    @param viz The web visualizer.
    """
    last_packets = LastPackets()
    viz.add_node_panel("last-packets", "Last packets", last_packets.build, last_packets.action)
