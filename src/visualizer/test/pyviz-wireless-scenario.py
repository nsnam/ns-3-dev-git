#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Wireless scenario used by pyviz-web-test.py; not meant to be run by hand.

Builds each technology only when its module is part of the build, and prints
nothing else, so the test can tell from the topology what is present:
  - Ad hoc Wi-Fi: nodes "wifi-a" -> "wifi-b", raw frames over packet sockets.
  - LR-WPAN: nodes "lrwpan-a" -> "lrwpan-b" in one PAN, raw frames over
    packet sockets.
  - LTE with EPC: UDP echo between node "remote" and UE node "ue", through
    eNB node "enb" (and the PGW).
"""

import sys

from ns import ns


def registered(type_name):
    return bool(ns.TypeId.LookupByNameFailSafe(type_name, ns.TypeId()))


def place(nodes, positions):
    mobility = ns.MobilityHelper()
    allocator = ns.CreateObject[ns.ListPositionAllocator]()
    for x, y in positions:
        allocator.Add(ns.Vector(x, y, 0))
    mobility.SetPositionAllocator(allocator)
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel")
    mobility.Install(nodes)


def packet_socket_flow(sender, sender_device, receiver_address, interval_ms, size):
    """Send raw frames from sender_device to receiver_address with a PacketSocketClient."""
    address = ns.PacketSocketAddress()
    address.SetSingleDevice(sender_device.GetIfIndex())
    address.SetPhysicalAddress(receiver_address)
    address.SetProtocol(1)
    client = ns.CreateObject[ns.PacketSocketClient]()
    client.SetRemote(address)
    client.SetAttribute("Interval", ns.TimeValue(ns.MilliSeconds(interval_ms)))
    client.SetAttribute("PacketSize", ns.UintegerValue(size))
    client.SetAttribute("MaxPackets", ns.UintegerValue(0))
    client.SetStartTime(ns.Seconds(0.5))
    sender.AddApplication(client)
    return client


def build_wifi(keep):
    nodes = ns.NodeContainer()
    nodes.Create(2)
    ns.Names.Add("wifi-a", nodes.Get(0))
    ns.Names.Add("wifi-b", nodes.Get(1))
    place(nodes, [(0, 0), (20, 0)])
    channel = ns.YansWifiChannelHelper.Default()
    phy = ns.YansWifiPhyHelper()
    phy.SetChannel(channel.Create())
    mac = ns.WifiMacHelper()
    mac.SetType("ns3::AdhocWifiMac")
    devices = ns.WifiHelper().Install(phy, mac, nodes)
    ns.PacketSocketHelper().Install(nodes)
    keep.append(
        packet_socket_flow(nodes.Get(0), devices.Get(0), devices.Get(1).GetAddress(), 50, 200)
    )
    keep.append(devices)


def build_lrwpan(keep):
    nodes = ns.NodeContainer()
    nodes.Create(2)
    ns.Names.Add("lrwpan-a", nodes.Get(0))
    ns.Names.Add("lrwpan-b", nodes.Get(1))
    place(nodes, [(0, 40), (10, 40)])
    helper = ns.LrWpanHelper()
    devices = helper.Install(nodes)
    helper.CreateAssociatedPan(devices, 10)
    ns.PacketSocketHelper().Install(nodes)
    receiver = ns.DynamicCast[ns.lrwpan.LrWpanNetDevice](devices.Get(1))
    short_address = receiver.GetMac().GetShortAddress()
    keep.append(
        packet_socket_flow(nodes.Get(0), devices.Get(0), short_address.ConvertTo(), 100, 50)
    )
    keep.append((helper, devices))


def build_lte(keep):
    lte = ns.CreateObject[ns.LteHelper]()
    epc = ns.CreateObject[ns.PointToPointEpcHelper]()
    lte.SetEpcHelper(epc)
    pgw = epc.GetPgwNode()
    ns.Names.Add("pgw", pgw)

    remote_nodes = ns.NodeContainer()
    remote_nodes.Create(1)
    remote = remote_nodes.Get(0)
    ns.Names.Add("remote", remote)
    internet = ns.InternetStackHelper()
    internet.Install(remote_nodes)
    p2p = ns.PointToPointHelper()
    p2p.SetDeviceAttribute("DataRate", ns.StringValue("100Gb/s"))
    internet_devices = p2p.Install(pgw, remote)
    addresses = ns.Ipv4AddressHelper()
    addresses.SetBase(ns.Ipv4Address("1.0.0.0"), ns.Ipv4Mask("255.0.0.0"))
    internet_interfaces = addresses.Assign(internet_devices)
    routing = ns.Ipv4StaticRoutingHelper()
    remote_routes = routing.GetStaticRouting(remote.GetObject[ns.Ipv4]())
    remote_routes.AddNetworkRouteTo(ns.Ipv4Address("7.0.0.0"), ns.Ipv4Mask("255.0.0.0"), 1)

    enb_nodes = ns.NodeContainer()
    enb_nodes.Create(1)
    ue_nodes = ns.NodeContainer()
    ue_nodes.Create(1)
    ns.Names.Add("enb", enb_nodes.Get(0))
    ns.Names.Add("ue", ue_nodes.Get(0))
    place(enb_nodes, [(80, 0)])
    place(ue_nodes, [(90, 10)])
    core_nodes = ns.NodeContainer()
    core_nodes.Add(pgw)
    core_nodes.Add(remote)
    place(core_nodes, [(80, -40), (40, -40)])
    enb_devices = lte.InstallEnbDevice(enb_nodes)
    ue_devices = lte.InstallUeDevice(ue_nodes)
    internet.Install(ue_nodes)
    ue_interfaces = epc.AssignUeIpv4Address(ue_devices)
    ue_routes = routing.GetStaticRouting(ue_nodes.Get(0).GetObject[ns.Ipv4]())
    ue_routes.SetDefaultRoute(epc.GetUeDefaultGatewayAddress(), 1)
    lte.Attach(ue_devices, enb_devices.Get(0))

    server = ns.UdpEchoServerHelper(9)
    server_apps = server.Install(ue_nodes.Get(0))
    server_apps.Start(ns.Seconds(0.1))
    client = ns.UdpEchoClientHelper(ue_interfaces.GetAddress(0).ConvertTo(), 9)
    client.SetAttribute("MaxPackets", ns.UintegerValue(100000))
    client.SetAttribute("Interval", ns.TimeValue(ns.MilliSeconds(50)))
    client.SetAttribute("PacketSize", ns.UintegerValue(300))
    client_apps = client.Install(remote)
    client_apps.Start(ns.Seconds(0.5))
    keep.append((lte, epc, internet_interfaces, server_apps, client_apps, enb_devices, ue_devices))


def main(argv):
    # Accepts ns-3 arguments, e.g. --SimulatorImplementationType used by --vis.
    ns.CommandLine(__file__).Parse(argv)
    keep = []
    if registered("ns3::WifiNetDevice"):
        build_wifi(keep)
    if registered("ns3::lrwpan::LrWpanNetDevice"):
        build_lrwpan(keep)
    if registered("ns3::LteEnbNetDevice"):
        build_lte(keep)
    ns.Simulator.Stop(ns.Seconds(6))
    import visualizer

    visualizer.start()


if __name__ == "__main__":
    main(sys.argv)
