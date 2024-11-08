#
# SPDX-License-Identifier: GPL-2.0-only
#
# Authors: Lalith Suresh <suresh.lalith@gmail.com>
# Modified by: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#

import os.path

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )

ns.LogComponentEnable("Ipv4ClickRouting", ns.LOG_LEVEL_ALL)
ns.LogComponentEnable("Ipv4L3ClickProtocol", ns.LOG_LEVEL_ALL)

clickConfigFolder = os.path.dirname(__file__)

csmaNodes = ns.NodeContainer()
csmaNodes.Create(2)

# Setup CSMA channel between the nodes
csma = ns.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.DataRateValue(ns.DataRate(5000000)))
csma.SetChannelAttribute("Delay", ns.TimeValue(ns.MilliSeconds(2)))
csmaDevices = csma.Install(csmaNodes)

# Install normal internet stack on node B
internet = ns.InternetStackHelper()
internet.Install(csmaNodes.Get(1))

# Install Click on node A
clickinternet = ns.ClickInternetStackHelper()
clickinternet.SetClickFile(
    csmaNodes.Get(0), clickConfigFolder + "/nsclick-lan-single-interface.click"
)
clickinternet.SetRoutingTableElement(csmaNodes.Get(0), "rt")
clickinternet.Install(csmaNodes.Get(0))

# Configure IP addresses for the nodes
ipv4 = ns.Ipv4AddressHelper()
ipv4.SetBase("172.16.1.0", "255.255.255.0")
ipv4.Assign(csmaDevices)

# Configure traffic application and sockets
LocalAddress = ns.InetSocketAddress(ns.Ipv4Address.GetAny(), 50000).ConvertTo()
packetSinkHelper = ns.PacketSinkHelper("ns3::TcpSocketFactory", LocalAddress)
recvapp = packetSinkHelper.Install(csmaNodes.Get(1))
recvapp.Start(ns.Seconds(5))
recvapp.Stop(ns.Seconds(10))

onOffHelper = ns.OnOffHelper("ns3::TcpSocketFactory", ns.Address())
onOffHelper.SetAttribute("OnTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=1]"))
onOffHelper.SetAttribute("OffTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=0]"))

appcont = ns.ApplicationContainer()

remoteAddress = ns.InetSocketAddress(ns.Ipv4Address("172.16.1.2"), 50000).ConvertTo()
onOffHelper.SetAttribute("Remote", ns.AddressValue(remoteAddress))
appcont.Add(onOffHelper.Install(csmaNodes.Get(0)))

appcont.Start(ns.Seconds(5))
appcont.Stop(ns.Seconds(10))

# For tracing
csma.EnablePcap("nsclick-simple-lan", csmaDevices, False)

ns.Simulator.Stop(ns.Seconds(20))
ns.Simulator.Run()

ns.Simulator.Destroy()
