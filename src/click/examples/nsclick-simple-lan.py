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
recvapp.Start(ns.Seconds(5.0))
recvapp.Stop(ns.Seconds(10.0))

onOffHelper = ns.OnOffHelper("ns3::TcpSocketFactory", ns.Address())
onOffHelper.SetAttribute("OnTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=1]"))
onOffHelper.SetAttribute("OffTime", ns.StringValue("ns3::ConstantRandomVariable[Constant=0]"))

appcont = ns.ApplicationContainer()

remoteAddress = ns.InetSocketAddress(ns.Ipv4Address("172.16.1.2"), 50000).ConvertTo()
onOffHelper.SetAttribute("Remote", ns.AddressValue(remoteAddress))
appcont.Add(onOffHelper.Install(csmaNodes.Get(0)))

appcont.Start(ns.Seconds(5.0))
appcont.Stop(ns.Seconds(10.0))

# For tracing
csma.EnablePcap("nsclick-simple-lan", csmaDevices, False)

ns.Simulator.Stop(ns.Seconds(20.0))
ns.Simulator.Run()

ns.Simulator.Destroy()
