# /*
#  * SPDX-License-Identifier: GPL-2.0-only
#  */

#
#  This ns-3 example demonstrates the use of helper functions to ease
#  the construction of simulation scenarios.
#
#  The simulation topology consists of a mixed wired and wireless
#  scenario in which a hierarchical mobility model is used.
#
#  The simulation layout consists of N backbone routers interconnected
#  by an ad hoc wifi network.
#  Each backbone router also has a local 802.11 network and is connected
#  to a local LAN.  An additional set of(K-1) nodes are connected to
#  this backbone.  Finally, a local LAN is connected to each router
#  on the backbone, with L-1 additional hosts.
#
#  The nodes are populated with TCP/IP stacks, and OLSR unicast routing
#  on the backbone.  An example UDP transfer is shown.  The simulator
#  be configured to output tcpdumps or traces from different nodes.
#
#
#           +--------------------------------------------------------+
#           |                                                        |
#           |              802.11 ad hoc, ns-2 mobility              |
#           |                                                        |
#           +--------------------------------------------------------+
#                    |       o o o(N backbone routers)       |
#                +--------+                               +--------+
#      wired LAN | mobile |                     wired LAN | mobile |
#     -----------| router |                    -----------| router |
#                ---------                                ---------
#                    |                                        |
#           +----------------+                       +----------------+
#           |     802.11     |                       |     802.11     |
#           |      net       |                       |       net      |
#           |   K-1 hosts    |                       |   K-1 hosts    |
#           +----------------+                       +----------------+
#

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )

# #
# #  This function will be used below as a trace sink
# #
# static void
# CourseChangeCallback(std.string path, Ptr<const MobilityModel> model)
# {
#   Vector position = model.GetPosition();
#   std.cout << "CourseChange " << path << " x=" << position.x << ", y=" << position.y << ", z=" << position.z << std.endl;
# }


def main(argv):
    #
    #  First, we initialize a few local variables that control some
    #  simulation parameters.
    #
    from ctypes import c_double, c_int

    backboneNodes = c_int(10)
    infraNodes = c_int(2)
    lanNodes = c_int(2)
    stopTime = c_double(20)
    cmd = ns.CommandLine(__file__)

    #
    #  Simulation defaults are typically set next, before command line
    #  arguments are parsed.
    #
    ns.Config.SetDefault("ns3::OnOffApplication::PacketSize", ns.StringValue("1472"))
    ns.Config.SetDefault("ns3::OnOffApplication::DataRate", ns.StringValue("100kb/s"))

    #
    #  For convenience, we add the local variables to the command line argument
    #  system so that they can be overridden with flags such as
    #  "--backboneNodes=20"
    #

    cmd.AddValue("backboneNodes", "number of backbone nodes", backboneNodes)
    cmd.AddValue("infraNodes", "number of leaf nodes", infraNodes)
    cmd.AddValue("lanNodes", "number of LAN nodes", lanNodes)
    cmd.AddValue["double"]("stopTime", "simulation stop time(seconds)", stopTime)

    #
    #  The system global variables and the local values added to the argument
    #  system can be overridden by command line arguments by using this call.
    #
    cmd.Parse(argv)

    if stopTime.value < 10:
        print("Use a simulation stop time >= 10 seconds")
        exit(1)
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
    #                                                                        #
    #  Construct the backbone                                                #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /

    #
    #  Create a container to manage the nodes of the adhoc(backbone) network.
    #  Later we'll create the rest of the nodes we'll need.
    #
    backbone = ns.NodeContainer()
    backbone.Create(backboneNodes.value)
    #
    #  Create the backbone wifi net devices and install them into the nodes in
    #  our container
    #
    wifi = ns.WifiHelper()
    mac = ns.WifiMacHelper()
    mac.SetType("ns3::AdhocWifiMac")
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager", "DataMode", ns.StringValue("OfdmRate54Mbps")
    )
    wifiPhy = ns.YansWifiPhyHelper()
    wifiPhy.SetPcapDataLinkType(wifiPhy.DLT_IEEE802_11_RADIO)
    wifiChannel = ns.YansWifiChannelHelper.Default()
    wifiPhy.SetChannel(wifiChannel.Create())
    backboneDevices = wifi.Install(wifiPhy, mac, backbone)
    #
    #  Add the IPv4 protocol stack to the nodes in our container
    #
    print("Enabling OLSR routing on all backbone nodes")
    internet = ns.InternetStackHelper()
    olsr = ns.OlsrHelper()
    internet.SetRoutingHelper(olsr)
    # has effect on the next Install ()
    internet.Install(backbone)
    # re-initialize for non-olsr routing.
    # internet.Reset()
    #
    #  Assign IPv4 addresses to the device drivers(actually to the associated
    #  IPv4 interfaces) we just created.
    #
    ipAddrs = ns.Ipv4AddressHelper()
    ipAddrs.SetBase(ns.Ipv4Address("192.168.0.0"), ns.Ipv4Mask("255.255.255.0"))
    ipAddrs.Assign(backboneDevices)

    #
    #  The ad-hoc network nodes need a mobility model so we aggregate one to
    #  each of the nodes we just finished building.
    #
    mobility = ns.MobilityHelper()
    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX",
        ns.DoubleValue(20.0),
        "MinY",
        ns.DoubleValue(20.0),
        "DeltaX",
        ns.DoubleValue(20.0),
        "DeltaY",
        ns.DoubleValue(20.0),
        "GridWidth",
        ns.UintegerValue(5),
        "LayoutType",
        ns.StringValue("RowFirst"),
    )
    mobility.SetMobilityModel(
        "ns3::RandomDirection2dMobilityModel",
        "Bounds",
        ns.RectangleValue(ns.Rectangle(-500, 500, -500, 500)),
        "Speed",
        ns.StringValue("ns3::ConstantRandomVariable[Constant=2]"),
        "Pause",
        ns.StringValue("ns3::ConstantRandomVariable[Constant=0.2]"),
    )
    mobility.Install(backbone)

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
    #                                                                        #
    #  Construct the LANs                                                    #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /

    #  Reset the address base-- all of the CSMA networks will be in
    #  the "172.16 address space
    ipAddrs.SetBase(ns.Ipv4Address("172.16.0.0"), ns.Ipv4Mask("255.255.255.0"))

    for i in range(backboneNodes.value):
        print("Configuring local area network for backbone node ", i)
        #
        #  Create a container to manage the nodes of the LAN.  We need
        #  two containers here; one with all of the new nodes, and one
        #  with all of the nodes including new and existing nodes
        #
        newLanNodes = ns.NodeContainer()
        newLanNodes.Create(lanNodes.value - 1)
        #  Now, create the container with all nodes on this link
        lan = ns.NodeContainer(ns.NodeContainer(backbone.Get(i)), newLanNodes)
        #
        #  Create the CSMA net devices and install them into the nodes in our
        #  collection.
        #
        csma = ns.CsmaHelper()
        csma.SetChannelAttribute("DataRate", ns.DataRateValue(ns.DataRate(5000000)))
        csma.SetChannelAttribute("Delay", ns.TimeValue(ns.MilliSeconds(2)))
        lanDevices = csma.Install(lan)
        #
        #  Add the IPv4 protocol stack to the new LAN nodes
        #
        internet.Install(newLanNodes)
        #
        #  Assign IPv4 addresses to the device drivers(actually to the
        #  associated IPv4 interfaces) we just created.
        #
        ipAddrs.Assign(lanDevices)
        #
        #  Assign a new network prefix for the next LAN, according to the
        #  network mask initialized above
        #
        ipAddrs.NewNetwork()
        #
        # The new LAN nodes need a mobility model so we aggregate one
        # to each of the nodes we just finished building.
        #
        mobilityLan = ns.MobilityHelper()
        positionAlloc = ns.ListPositionAllocator()
        for j in range(newLanNodes.GetN()):
            positionAlloc.Add(ns.Vector(0.0, (j * 10 + 10), 0.0))

        mobilityLan.SetPositionAllocator(positionAlloc)
        mobilityLan.PushReferenceMobilityModel(backbone.Get(i))
        mobilityLan.SetMobilityModel("ns3::ConstantPositionMobilityModel")
        mobilityLan.Install(newLanNodes)

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
    #                                                                        #
    #  Construct the mobile networks                                         #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /

    #  Reset the address base-- all of the 802.11 networks will be in
    #  the "10.0" address space
    ipAddrs.SetBase(ns.Ipv4Address("10.0.0.0"), ns.Ipv4Mask("255.255.255.0"))
    tempRef = []  # list of references to be held to prevent garbage collection
    for i in range(backboneNodes.value):
        print("Configuring wireless network for backbone node ", i)
        #
        #  Create a container to manage the nodes of the LAN.  We need
        #  two containers here; one with all of the new nodes, and one
        #  with all of the nodes including new and existing nodes
        #
        stas = ns.NodeContainer()
        stas.Create(infraNodes.value - 1)
        #  Now, create the container with all nodes on this link
        infra = ns.NodeContainer(ns.NodeContainer(backbone.Get(i)), stas)
        #
        #  Create another ad hoc network and devices
        #
        ssid = ns.Ssid("wifi-infra" + str(i))
        wifiInfra = ns.WifiHelper()
        wifiPhy.SetChannel(wifiChannel.Create())
        macInfra = ns.WifiMacHelper()
        macInfra.SetType("ns3::StaWifiMac", "Ssid", ns.SsidValue(ssid))

        # setup stas
        staDevices = wifiInfra.Install(wifiPhy, macInfra, stas)
        # setup ap.
        macInfra.SetType("ns3::ApWifiMac", "Ssid", ns.SsidValue(ssid))
        apDevices = wifiInfra.Install(wifiPhy, macInfra, backbone.Get(i))
        # Collect all of these new devices
        infraDevices = ns.NetDeviceContainer(apDevices, staDevices)

        #  Add the IPv4 protocol stack to the nodes in our container
        #
        internet.Install(stas)
        #
        #  Assign IPv4 addresses to the device drivers(actually to the associated
        #  IPv4 interfaces) we just created.
        #
        ipAddrs.Assign(infraDevices)
        #
        #  Assign a new network prefix for each mobile network, according to
        #  the network mask initialized above
        #
        ipAddrs.NewNetwork()

        # This call returns an instance that needs to be stored in the outer scope
        # not to be garbage collected when overwritten in the next iteration
        subnetAlloc = ns.ListPositionAllocator()

        # Appending the object to a list is enough to prevent the garbage collection
        tempRef.append(subnetAlloc)

        #
        #  The new wireless nodes need a mobility model so we aggregate one
        #  to each of the nodes we just finished building.
        #
        for j in range(infra.GetN()):
            subnetAlloc.Add(ns.Vector(0.0, j, 0.0))

        mobility.PushReferenceMobilityModel(backbone.Get(i))
        mobility.SetPositionAllocator(subnetAlloc)
        mobility.SetMobilityModel(
            "ns3::RandomDirection2dMobilityModel",
            "Bounds",
            ns.RectangleValue(ns.Rectangle(-10, 10, -10, 10)),
            "Speed",
            ns.StringValue("ns3::ConstantRandomVariable[Constant=3]"),
            "Pause",
            ns.StringValue("ns3::ConstantRandomVariable[Constant=0.4]"),
        )
        mobility.Install(stas)

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
    #                                                                        #
    #  Application configuration                                             #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /

    #  Create the OnOff application to send UDP datagrams of size
    #  210 bytes at a rate of 448 Kb/s, between two nodes
    print("Create Applications.")
    port = 9  #  Discard port(RFC 863)

    appSource = ns.NodeList.GetNode(backboneNodes.value)
    lastNodeIndex = (
        backboneNodes.value
        + backboneNodes.value * (lanNodes.value - 1)
        + backboneNodes.value * (infraNodes.value - 1)
        - 1
    )
    appSink = ns.NodeList.GetNode(lastNodeIndex)

    ns.cppyy.cppdef(
        """
        Ipv4Address getIpv4AddressFromNode(Ptr<Node> node){
        return node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
        }
    """
    )
    # Let's fetch the IP address of the last node, which is on Ipv4Interface 1
    remoteAddr = ns.cppyy.gbl.getIpv4AddressFromNode(appSink)
    socketAddr = ns.InetSocketAddress(remoteAddr, port)
    onoff = ns.OnOffHelper("ns3::UdpSocketFactory", socketAddr.ConvertTo())
    apps = onoff.Install(ns.NodeContainer(appSource))
    apps.Start(ns.Seconds(3))
    apps.Stop(ns.Seconds(stopTime.value - 1))

    #  Create a packet sink to receive these packets
    sink = ns.PacketSinkHelper(
        "ns3::UdpSocketFactory",
        ns.InetSocketAddress(ns.InetSocketAddress(ns.Ipv4Address.GetAny(), port)).ConvertTo(),
    )
    sinkContainer = ns.NodeContainer(appSink)
    apps = sink.Install(sinkContainer)
    apps.Start(ns.Seconds(3))

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
    #                                                                        #
    #  Tracing configuration                                                 #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /

    print("Configure Tracing.")
    csma = ns.CsmaHelper()
    #
    #  Let's set up some ns-2-like ascii traces, using another helper class
    #
    ascii = ns.AsciiTraceHelper()
    stream = ascii.CreateFileStream("mixed-wireless.tr")
    wifiPhy.EnableAsciiAll(stream)
    csma.EnableAsciiAll(stream)
    internet.EnableAsciiIpv4All(stream)

    #  Csma captures in non-promiscuous mode
    csma.EnablePcapAll("mixed-wireless", False)
    #  Let's do a pcap trace on the backbone devices
    wifiPhy.EnablePcap("mixed-wireless", backboneDevices)
    wifiPhy.EnablePcap("mixed-wireless", appSink.GetId(), 0)

    #   #ifdef ENABLE_FOR_TRACING_EXAMPLE
    #     Config.Connect("/NodeList/*/$MobilityModel/CourseChange",
    #       MakeCallback(&CourseChangeCallback))
    #   #endif

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    #                                                                        #
    #  Run simulation                                                        #
    #                                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    print("Run Simulation.")
    ns.Simulator.Stop(ns.Seconds(stopTime.value))
    ns.Simulator.Run()
    ns.Simulator.Destroy()


if __name__ == "__main__":
    import sys

    main(sys.argv)
