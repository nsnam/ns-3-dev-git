#
# SPDX-License-Identifier: GPL-2.0-only
#
# Ported to Python by Mohit P. Tahiliani
#

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )
import sys
from ctypes import c_bool, c_int

# // Default Network Topology
# //
# //       10.1.1.0
# // n0 -------------- n1   n2   n3   n4
# //    point-to-point  |    |    |    |
# //                    ================
# //                      LAN 10.1.2.0


nCsma = c_int(3)
verbose = c_bool(True)
cmd = ns.CommandLine(__file__)
cmd.AddValue("nCsma", "Number of extra CSMA nodes/devices", nCsma)
cmd.AddValue("verbose", "Tell echo applications to log if true", verbose)
cmd.Parse(sys.argv)

if verbose.value:
    ns.LogComponentEnable("UdpEchoClientApplication", ns.LOG_LEVEL_INFO)
    ns.LogComponentEnable("UdpEchoServerApplication", ns.LOG_LEVEL_INFO)
nCsma.value = 1 if nCsma.value == 0 else nCsma.value

p2pNodes = ns.NodeContainer()
p2pNodes.Create(2)

csmaNodes = ns.NodeContainer()
csmaNodes.Add(p2pNodes.Get(1))
csmaNodes.Create(nCsma.value)

pointToPoint = ns.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.StringValue("2ms"))

p2pDevices = pointToPoint.Install(p2pNodes)

csma = ns.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.StringValue("100Mbps"))
csma.SetChannelAttribute("Delay", ns.TimeValue(ns.NanoSeconds(6560)))

csmaDevices = csma.Install(csmaNodes)

stack = ns.InternetStackHelper()
stack.Install(p2pNodes.Get(0))
stack.Install(csmaNodes)

address = ns.Ipv4AddressHelper()
address.SetBase(ns.Ipv4Address("10.1.1.0"), ns.Ipv4Mask("255.255.255.0"))
p2pInterfaces = address.Assign(p2pDevices)

address.SetBase(ns.Ipv4Address("10.1.2.0"), ns.Ipv4Mask("255.255.255.0"))
csmaInterfaces = address.Assign(csmaDevices)

echoServer = ns.UdpEchoServerHelper(9)

serverApps = echoServer.Install(csmaNodes.Get(nCsma.value))
serverApps.Start(ns.Seconds(1))
serverApps.Stop(ns.Seconds(10))

echoClient = ns.UdpEchoClientHelper(csmaInterfaces.GetAddress(nCsma.value).ConvertTo(), 9)
echoClient.SetAttribute("MaxPackets", ns.UintegerValue(1))
echoClient.SetAttribute("Interval", ns.TimeValue(ns.Seconds(1)))
echoClient.SetAttribute("PacketSize", ns.UintegerValue(1024))

clientApps = echoClient.Install(p2pNodes.Get(0))
clientApps.Start(ns.Seconds(2))
clientApps.Stop(ns.Seconds(10))

ns.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

pointToPoint.EnablePcapAll("second")
csma.EnablePcap("second", csmaDevices.Get(1), True)

ns.Simulator.Run()
ns.Simulator.Destroy()
