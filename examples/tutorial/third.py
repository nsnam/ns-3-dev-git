#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
# //   Wifi 10.1.3.0
# //                 AP
# //  *    *    *    *
# //  |    |    |    |    10.1.1.0
# // n5   n6   n7   n0 -------------- n1   n2   n3   n4
# //                   point-to-point  |    |    |    |
# //                                   ================
# //                                     LAN 10.1.2.0


nCsma = c_int(3)
verbose = c_bool(True)
nWifi = c_int(3)
tracing = c_bool(False)

cmd = ns.CommandLine(__file__)
cmd.AddValue("nCsma", "Number of extra CSMA nodes/devices", nCsma)
cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi)
cmd.AddValue("verbose", "Tell echo applications to log if true", verbose)
cmd.AddValue("tracing", "Enable pcap tracing", tracing)

cmd.Parse(sys.argv)

# The underlying restriction of 18 is due to the grid position
# allocator's configuration; the grid layout will exceed the
# bounding box if more than 18 nodes are provided.
if nWifi.value > 18:
    print("nWifi should be 18 or less; otherwise grid layout exceeds the bounding box")
    sys.exit(1)

if verbose.value:
    ns.LogComponentEnable("UdpEchoClientApplication", ns.LOG_LEVEL_INFO)
    ns.LogComponentEnable("UdpEchoServerApplication", ns.LOG_LEVEL_INFO)

p2pNodes = ns.NodeContainer()
p2pNodes.Create(2)

pointToPoint = ns.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.StringValue("2ms"))

p2pDevices = pointToPoint.Install(p2pNodes)

csmaNodes = ns.NodeContainer()
csmaNodes.Add(p2pNodes.Get(1))
csmaNodes.Create(nCsma.value)

csma = ns.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.StringValue("100Mbps"))
csma.SetChannelAttribute("Delay", ns.TimeValue(ns.NanoSeconds(6560)))

csmaDevices = csma.Install(csmaNodes)

wifiStaNodes = ns.NodeContainer()
wifiStaNodes.Create(nWifi.value)
wifiApNode = p2pNodes.Get(0)

channel = ns.YansWifiChannelHelper.Default()
phy = ns.YansWifiPhyHelper()
phy.SetChannel(channel.Create())

mac = ns.WifiMacHelper()
ssid = ns.Ssid("ns-3-ssid")

wifi = ns.WifiHelper()

mac.SetType("ns3::StaWifiMac", "Ssid", ns.SsidValue(ssid), "ActiveProbing", ns.BooleanValue(False))
staDevices = wifi.Install(phy, mac, wifiStaNodes)

mac.SetType("ns3::ApWifiMac", "Ssid", ns.SsidValue(ssid))
apDevices = wifi.Install(phy, mac, wifiApNode)

mobility = ns.MobilityHelper()
mobility.SetPositionAllocator(
    "ns3::GridPositionAllocator",
    "MinX",
    ns.DoubleValue(0.0),
    "MinY",
    ns.DoubleValue(0.0),
    "DeltaX",
    ns.DoubleValue(5.0),
    "DeltaY",
    ns.DoubleValue(10.0),
    "GridWidth",
    ns.UintegerValue(3),
    "LayoutType",
    ns.StringValue("RowFirst"),
)

mobility.SetMobilityModel(
    "ns3::RandomWalk2dMobilityModel",
    "Bounds",
    ns.RectangleValue(ns.Rectangle(-50, 50, -50, 50)),
)
mobility.Install(wifiStaNodes)

mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel")
mobility.Install(wifiApNode)

stack = ns.InternetStackHelper()
stack.Install(csmaNodes)
stack.Install(wifiApNode)
stack.Install(wifiStaNodes)

address = ns.Ipv4AddressHelper()
address.SetBase(ns.Ipv4Address("10.1.1.0"), ns.Ipv4Mask("255.255.255.0"))
p2pInterfaces = address.Assign(p2pDevices)

address.SetBase(ns.Ipv4Address("10.1.2.0"), ns.Ipv4Mask("255.255.255.0"))
csmaInterfaces = address.Assign(csmaDevices)

address.SetBase(ns.Ipv4Address("10.1.3.0"), ns.Ipv4Mask("255.255.255.0"))
address.Assign(staDevices)
address.Assign(apDevices)

echoServer = ns.UdpEchoServerHelper(9)

serverApps = echoServer.Install(csmaNodes.Get(nCsma.value))
serverApps.Start(ns.Seconds(1.0))
serverApps.Stop(ns.Seconds(10.0))

echoClient = ns.UdpEchoClientHelper(csmaInterfaces.GetAddress(nCsma.value).ConvertTo(), 9)
echoClient.SetAttribute("MaxPackets", ns.UintegerValue(1))
echoClient.SetAttribute("Interval", ns.TimeValue(ns.Seconds(1.0)))
echoClient.SetAttribute("PacketSize", ns.UintegerValue(1024))

clientApps = echoClient.Install(wifiStaNodes.Get(nWifi.value - 1))
clientApps.Start(ns.Seconds(2.0))
clientApps.Stop(ns.Seconds(10.0))

ns.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

ns.Simulator.Stop(ns.Seconds(10.0))

if tracing.value:
    phy.SetPcapDataLinkType(phy.DLT_IEEE802_11_RADIO)
    pointToPoint.EnablePcapAll("third")
    phy.EnablePcap("third", apDevices.Get(0))
    csma.EnablePcap("third", csmaDevices.Get(0), True)

ns.Simulator.Run()
ns.Simulator.Destroy()
