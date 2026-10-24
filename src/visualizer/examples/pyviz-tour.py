#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Show what PyViz displays: every way of losing a packet, one per row of nodes,
and, next to them, a Wi-Fi network with moving, battery powered stations, and a
Wi-Fi mesh.

  - Full queue: a 1.1 Mbit/s flow enters a 1 Mbit/s link.
  - Channel: the receiver's error model corrupts 10% of the packets.
  - TTL: the sender's packets have a TTL of 1, which the router cannot forward.
  - No route: the router has no route to the destination.
  - Not received: two Wi-Fi nodes are tuned to different channels.
  - Wi-Fi: stations bounce inside a box, like a DVD player's logo, and browse an
    HTTP server through their access point and a router.
  - Mesh (when the mesh module is built): the ends of a chain exchange traffic
    through the nodes between them.
"""

import sys

from ns import ns

import visualizer

## Area the Wi-Fi stations stay in, as (x min, x max, y min, y max) in metres.
BOX = (150.0, 220.0, 0.0, 70.0)


def p2p_link(first, second, rate, network):
    """Connect two nodes with a point-to-point link and number its interfaces.

    @param first First node.
    @param second Second node.
    @param rate Link data rate.
    @param network IPv4 network, such as "10.1.1.0".
    @return (devices, interfaces).
    """
    helper = ns.PointToPointHelper()
    helper.SetDeviceAttribute("DataRate", ns.StringValue(rate))
    helper.SetChannelAttribute("Delay", ns.StringValue("2ms"))
    helper.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", ns.StringValue("5p"))
    devices = helper.Install(first, second)
    address = ns.Ipv4AddressHelper()
    address.SetBase(ns.Ipv4Address(network), ns.Ipv4Mask("255.255.255.0"))
    return devices, address.Assign(devices)


def udp_flow(node, destination, rate):
    """Send constant bit rate UDP traffic of 1000 byte packets.

    @param node Sending node.
    @param destination Destination Ipv4Address.
    @param rate Data rate.
    """
    helper = ns.OnOffHelper(
        "ns3::UdpSocketFactory", ns.InetSocketAddress(destination, 9).ConvertTo()
    )
    helper.SetAttribute("OnTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=1]"))
    helper.SetAttribute("OffTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=0]"))
    helper.SetAttribute("DataRate", ns.StringValue(rate))
    helper.SetAttribute("PacketSize", ns.UintegerValue(1000))
    helper.Install(node).Start(ns.Seconds(0.1))


def bounce_in_box(node, x, y, direction, speed):
    """Move a node in straight lines, reflected by the walls of BOX.

    A random walk whose direction never expires only changes course at a wall.

    @param node Node.
    @param x Initial x coordinate.
    @param y Initial y coordinate.
    @param direction Initial direction in radians.
    @param speed Speed in m/s.
    """
    model = ns.CreateObject[ns.RandomWalk2dMobilityModel]()
    model.SetAttribute("Bounds", ns.RectangleValue(ns.Rectangle(*BOX)))
    model.SetAttribute("Mode", ns.StringValue("Time"))
    model.SetAttribute("Time", ns.TimeValue(ns.Seconds(1e6)))
    model.SetAttribute(
        "Direction", ns.StringValue("ns3::ConstantRandomVariable[Constant=%g]" % direction)
    )
    model.SetAttribute("Speed", ns.StringValue("ns3::ConstantRandomVariable[Constant=%g]" % speed))
    model.SetPosition(ns.Vector(x, y, 0))
    node.AggregateObject(model)


def wifi_network(router, labels):
    """Build stations browsing a server through an access point and a router.

    @param router Router node, which has an Internet stack and a fixed position.
    @param labels Node index to label dictionary, filled here.
    @return (station node indices, objects to keep alive).
    """
    ap, server, stations = ns.NodeContainer(), ns.NodeContainer(), ns.NodeContainer()
    ap.Create(1)
    server.Create(1)
    stations.Create(4)

    mobility = ns.MobilityHelper()
    mobility.Install(ap)
    mobility.Install(server)
    centre_x, centre_y = (BOX[0] + BOX[1]) / 2, (BOX[2] + BOX[3]) / 2
    ap.Get(0).GetObject[ns.MobilityModel]().SetPosition(ns.Vector(centre_x, centre_y, 0))
    server.Get(0).GetObject[ns.MobilityModel]().SetPosition(ns.Vector(BOX[0] - 40, BOX[3] + 25, 0))
    for index, (x, y, direction, speed) in enumerate(
        ((-25, -20, 0.7, 18), (20, -25, 2.2, 14), (25, 20, 4.0, 22), (-20, 25, 5.4, 16))
    ):
        bounce_in_box(stations.Get(index), centre_x + x, centre_y + y, direction, speed)

    stack = ns.InternetStackHelper()
    stack.Install(ap)
    stack.Install(server)
    stack.Install(stations)
    p2p_link(ap.Get(0), router, "100Mbps", "10.6.1.0")
    server_interfaces = p2p_link(router, server.Get(0), "100Mbps", "10.6.2.0")[1]

    wifi = ns.WifiHelper()
    wifi.SetStandard(ns.WIFI_STANDARD_80211n)
    phy = ns.YansWifiPhyHelper()
    phy.SetChannel(ns.YansWifiChannelHelper.Default().Create())
    mac = ns.WifiMacHelper()
    ssid = ns.Ssid("pyviz")
    mac.SetType("ns3::ApWifiMac", "Ssid", ns.SsidValue(ssid))
    ap_devices = wifi.Install(phy, mac, ap)
    mac.SetType("ns3::StaWifiMac", "Ssid", ns.SsidValue(ssid))
    station_devices = wifi.Install(phy, mac, stations)
    address = ns.Ipv4AddressHelper()
    address.SetBase(ns.Ipv4Address("10.6.3.0"), ns.Ipv4Mask("255.255.255.0"))
    address.Assign(ap_devices)
    address.Assign(station_devices)

    battery = ns.BasicEnergySourceHelper()
    battery.Set("BasicEnergySourceInitialEnergyJ", ns.DoubleValue(100))
    energy_sources = battery.Install(stations)
    radios = ns.WifiRadioEnergyModelHelper().Install(station_devices, energy_sources)

    remote = ns.InetSocketAddress(server_interfaces.GetAddress(1), 80).ConvertTo()
    ns.ThreeGppHttpServerHelper(remote).Install(server)
    ns.ThreeGppHttpClientHelper(remote).Install(stations).Start(ns.Seconds(0.5))

    labels[int(ap.Get(0).GetId())] = "access point"
    labels[int(server.Get(0).GetId())] = "HTTP server"
    indices = [int(stations.Get(i).GetId()) for i in range(4)]
    for number, index in enumerate(indices):
        labels[index] = "HTTP client %d" % number
    return indices, (energy_sources, radios)


def mesh_network(labels):
    """Build a chain of Wi-Fi mesh nodes whose ends exchange traffic.

    @param labels Node index to label dictionary, filled here.
    """
    nodes = ns.NodeContainer()
    nodes.Create(4)
    mobility = ns.MobilityHelper()
    mobility.Install(nodes)
    for index in range(4):
        position = ns.Vector(BOX[0] + index * 40.0, BOX[3] + 60, 0)
        nodes.Get(index).GetObject[ns.MobilityModel]().SetPosition(position)
        labels[int(nodes.Get(index).GetId())] = "mesh %d" % index
    mesh = ns.MeshHelper.Default()
    mesh.SetStackInstaller("ns3::Dot11sStack")
    phy = ns.YansWifiPhyHelper()
    phy.SetChannel(ns.YansWifiChannelHelper.Default().Create())
    devices = mesh.Install(phy, nodes)
    # Global routing cannot handle mesh devices; the mesh forwards by itself.
    stack = ns.InternetStackHelper()
    stack.SetRoutingHelper(ns.Ipv4StaticRoutingHelper())
    stack.Install(nodes)
    address = ns.Ipv4AddressHelper()
    address.SetBase(ns.Ipv4Address("10.7.1.0"), ns.Ipv4Mask("255.255.255.0"))
    interfaces = address.Assign(devices)
    any_address = ns.InetSocketAddress(ns.Ipv4Address.GetAny(), 9).ConvertTo()
    ns.PacketSinkHelper("ns3::UdpSocketFactory", any_address).Install(nodes.Get(3))
    udp_flow(nodes.Get(0), interfaces.GetAddress(3), "100kbps")


def main(argv):
    """Build the scenario and run it in the visualizer.

    @param argv Command line arguments.
    """
    cmd = ns.CommandLine(__file__)
    cmd.Parse(argv)

    # visualizer.start() would be too late: installing a mesh creates packets.
    ns.Packet.EnablePrinting()

    rows = ["queue", "channel", "ttl", "no route", "not received"]
    nodes = ns.NodeContainer()
    nodes.Create(3 * 4 + 2)
    mobility = ns.MobilityHelper()
    mobility.Install(nodes)
    labels = {}

    def place(index, column, row, label):
        model = nodes.Get(index).GetObject[ns.MobilityModel]()
        model.SetPosition(ns.Vector(column * 40.0, row * 30.0, 0))
        labels[index] = label

    for row, name in enumerate(rows[:4]):
        for column, role in enumerate(("source", "router", "sink")):
            place(row * 3 + column, column, row, "%s %s" % (name, role))
    # Closer to each other than to the row above: the "not received" stub
    # points to the nearest node.
    place(12, 0, 5, "channel 36")
    place(13, 1, 5, "channel 149")

    stack = ns.InternetStackHelper()
    stack.Install(nodes)

    routed = {}
    for row, rate in enumerate(("1Mbps", "5Mbps", "5Mbps", "5Mbps")):
        first = row * 3
        routed[row] = (
            p2p_link(nodes.Get(first), nodes.Get(first + 1), "10Mbps", "10.%d.1.0" % (row + 1)),
            p2p_link(nodes.Get(first + 1), nodes.Get(first + 2), rate, "10.%d.2.0" % (row + 1)),
        )

    # The default queue disc is so large that it would take long to overflow.
    router_egress = routed[0][1][0].Get(0)
    traffic_control = ns.TrafficControlHelper()
    traffic_control.Uninstall(router_egress)
    traffic_control.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", ns.StringValue("10p"))
    traffic_control.Install(router_egress)

    sink = lambda row: routed[row][1][1].GetAddress(1)
    udp_flow(nodes.Get(0), sink(0), "1.1Mbps")
    # Without a receiving application the sinks would answer with ICMP errors.
    any_address = ns.InetSocketAddress(ns.Ipv4Address.GetAny(), 9).ConvertTo()
    packet_sink = ns.PacketSinkHelper("ns3::UdpSocketFactory", any_address)
    packet_sink.Install(nodes.Get(2))
    packet_sink.Install(nodes.Get(5))

    error_model = ns.CreateObject[ns.RateErrorModel]()
    error_model.SetAttribute("ErrorRate", ns.DoubleValue(0.1))
    error_model.SetAttribute("ErrorUnit", ns.StringValue("ERROR_UNIT_PACKET"))
    routed[1][1][0].Get(1).SetAttribute("ReceiveErrorModel", ns.PointerValue(error_model))
    udp_flow(nodes.Get(3), sink(1), "200kbps")

    nodes.Get(6).GetObject[ns.Ipv4L3Protocol]().SetAttribute("DefaultTtl", ns.UintegerValue(1))
    udp_flow(nodes.Get(6), sink(2), "40kbps")

    static_routing = ns.Ipv4StaticRoutingHelper()
    routes = static_routing.GetStaticRouting(nodes.Get(9).GetObject[ns.Ipv4]())
    routes.AddNetworkRouteTo(
        ns.Ipv4Address("10.99.0.0"),
        ns.Ipv4Mask("255.255.0.0"),
        routed[3][0][1].GetAddress(1),
        1,
    )
    udp_flow(nodes.Get(9), ns.Ipv4Address("10.99.0.1"), "40kbps")

    wifi = ns.WifiHelper()
    wifi.SetStandard(ns.WIFI_STANDARD_80211a)
    mac = ns.WifiMacHelper()
    mac.SetType("ns3::AdhocWifiMac")
    channel = ns.YansWifiChannelHelper.Default().Create()
    wifi_devices = []
    for index, settings in ((12, "{36, 20, BAND_5GHZ, 0}"), (13, "{149, 20, BAND_5GHZ, 0}")):
        phy = ns.YansWifiPhyHelper()
        phy.SetChannel(channel)
        phy.Set("ChannelSettings", ns.StringValue(settings))
        wifi_devices.append(wifi.Install(phy, mac, nodes.Get(index)))
    address = ns.Ipv4AddressHelper()
    address.SetBase(ns.Ipv4Address("10.5.1.0"), ns.Ipv4Mask("255.255.255.0"))
    address.Assign(wifi_devices[0])
    address.Assign(wifi_devices[1])
    udp_flow(nodes.Get(12), ns.Ipv4Address("10.5.1.255"), "200kbps")

    router = ns.NodeContainer()
    router.Create(1)
    mobility.Install(router)
    position = ns.Vector((BOX[0] + BOX[1]) / 2, BOX[3] + 25, 0)
    router.Get(0).GetObject[ns.MobilityModel]().SetPosition(position)
    stack.Install(router)
    labels[int(router.Get(0).GetId())] = "router"
    stations, keep_alive = wifi_network(router.Get(0), labels)
    if ns.TypeId.LookupByNameFailSafe("ns3::MeshPointDevice", ns.TypeId()):
        mesh_network(labels)
    ns.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

    def hook(viz):
        for index, label in labels.items():
            viz.set_node_label(index, label)
        # A selected node's mobility bounds are drawn, which shows the box.
        for index in stations:
            viz.control({"action": "select", "node": index, "add": True})

    visualizer.add_initialization_hook(hook)
    visualizer.set_bounds(-130, -15, 270, 185)  # the legend covers the bottom left
    visualizer.set_speed(0.1)
    visualizer.skip_to(2.0)  # the queue takes this long to fill
    ns.Simulator.Stop(ns.Seconds(1000))
    visualizer.start()


if __name__ == "__main__":
    main(sys.argv)
