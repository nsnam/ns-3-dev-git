#
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
# Author: Blake Hurd <naimorai@gmail.com>
# Modified by: Josh Pelkey <joshpelkey@gmail.com>
#              Gabriel Ferreira <gabrielcarvfer@gmail.com>
#

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )

ns.LogComponentEnable("OpenFlowInterface", ns.LOG_LEVEL_ALL)
ns.LogComponentEnable("OpenFlowSwitchNetDevice", ns.LOG_LEVEL_ALL)

terminals = ns.NodeContainer()
terminals.Create(4)

csmaSwitch = ns.NodeContainer()
csmaSwitch.Create(1)

csma = ns.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.DataRateValue(5000000))
csma.SetChannelAttribute("Delay", ns.TimeValue(ns.MilliSeconds(2)))

terminalDevices = ns.NetDeviceContainer()
switchDevices = ns.NetDeviceContainer()
for i in range(4):
    container = ns.NodeContainer()
    container.Add(terminals.Get(i))
    container.Add(csmaSwitch)
    link = csma.Install(container)
    terminalDevices.Add(link.Get(0))
    switchDevices.Add(link.Get(1))

switchNode = csmaSwitch.Get(0)
swtch = ns.OpenFlowSwitchHelper()
controller = ns.ofi.DropController()
# controller = ns.CreateObject[ns.ofi.LearningController]()
swtch.Install(switchNode, switchDevices, controller)
# controller->SetAttribute("ExpirationTime", TimeValue(timeout))


internet = ns.InternetStackHelper()
internet.Install(terminals)

ipv4 = ns.Ipv4AddressHelper()
ipv4.SetBase("10.1.1.0", "255.255.255.0")
ipv4.Assign(terminalDevices)

port = 9

onoff = ns.OnOffHelper(
    "ns3::UdpSocketFactory", ns.InetSocketAddress(ns.Ipv4Address("10.1.1.2"), port).ConvertTo()
)
onoff.SetConstantRate(ns.DataRate("500kb/s"))

app = onoff.Install(terminals.Get(0))

app.Start(ns.Seconds(1.0))
app.Stop(ns.Seconds(10.0))

sink = ns.PacketSinkHelper(
    "ns3::UdpSocketFactory", ns.InetSocketAddress(ns.Ipv4Address.GetAny(), port).ConvertTo()
)
app = sink.Install(terminals.Get(1))
app.Start(ns.Seconds(0.0))

onoff.SetAttribute(
    "Remote", ns.AddressValue(ns.InetSocketAddress(ns.Ipv4Address("10.1.1.1"), port).ConvertTo())
)
app = onoff.Install(terminals.Get(3))
app.Start(ns.Seconds(1.1))
app.Stop(ns.Seconds(10.0))

app = sink.Install(terminals.Get(0))
app.Start(ns.Seconds(0.0))

ns.Simulator.Run()
ns.Simulator.Destroy()
