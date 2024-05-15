#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation
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

# Network topology
#
#        n0     n1
#        |      |
#       ----------
#       | Switch |
#       ----------
#        |      |
#        n2     n3
#
#
# - CBR/UDP flows from n0 to n1 and from n3 to n0
# - DropTail queues
# - Tracing of queues and packet receptions to file "csma-bridge.tr"

## \file
#  \ingroup bridge
#  Bridge example connecting two broadcast domains.

## Import ns-3
try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )


def main(argv):
    #
    # Allow the user to override any of the defaults and the above Bind() at
    # run-time, via command-line arguments
    #
    cmd = ns.CommandLine()
    cmd.Parse(argv)

    #
    # Explicitly create the nodes required by the topology(shown above).
    #
    # print "Create nodes."
    terminals = ns.NodeContainer()
    terminals.Create(4)

    csmaSwitch = ns.NodeContainer()
    csmaSwitch.Create(1)

    # print "Build Topology"
    csma = ns.CsmaHelper()
    csma.SetChannelAttribute("DataRate", ns.DataRateValue(ns.DataRate(5000000)))
    csma.SetChannelAttribute("Delay", ns.TimeValue(ns.MilliSeconds(2)))

    # Create the csma links, from each terminal to the switch

    terminalDevices = ns.NetDeviceContainer()
    switchDevices = ns.NetDeviceContainer()

    for i in range(4):
        link = csma.Install(ns.NodeContainer(ns.NodeContainer(terminals.Get(i)), csmaSwitch))
        terminalDevices.Add(link.Get(0))
        switchDevices.Add(link.Get(1))

    # Create the bridge netdevice, which will do the packet switching
    switchNode = csmaSwitch.Get(0)
    bridgeDevice = ns.BridgeNetDevice()
    switchNode.AddDevice(bridgeDevice)

    for portIter in range(switchDevices.GetN()):
        bridgeDevice.AddBridgePort(switchDevices.Get(portIter))

    # Add internet stack to the terminals
    internet = ns.InternetStackHelper()
    internet.Install(terminals)

    # We've got the "hardware" in place.  Now we need to add IP addresses.
    #
    # print "Assign IP Addresses."
    ipv4 = ns.Ipv4AddressHelper()
    ipv4.SetBase(ns.Ipv4Address("10.1.1.0"), ns.Ipv4Mask("255.255.255.0"))
    ipv4.Assign(terminalDevices)

    #
    # Create an OnOff application to send UDP datagrams from node zero to node 1.
    #
    # print "Create Applications."
    port = 9  # Discard port(RFC 863)

    inet_sock_address = ns.InetSocketAddress(ns.Ipv4Address("10.1.1.2"), port)
    onoff = ns.OnOffHelper("ns3::UdpSocketFactory", inet_sock_address.ConvertTo())
    onoff.SetConstantRate(ns.DataRate("500kb/s"))

    app = onoff.Install(ns.NodeContainer(terminals.Get(0)))
    # Start the application
    app.Start(ns.Seconds(1.0))
    app.Stop(ns.Seconds(10.0))

    # Create an optional packet sink to receive these packets
    inet_address = ns.InetSocketAddress(ns.Ipv4Address.GetAny(), port)
    sink = ns.PacketSinkHelper("ns3::UdpSocketFactory", inet_address.ConvertTo())
    app = sink.Install(ns.NodeContainer(terminals.Get(1)))
    app.Start(ns.Seconds(0.0))

    #
    # Create a similar flow from n3 to n0, starting at time 1.1 seconds
    #
    inet_address = ns.InetSocketAddress(ns.Ipv4Address("10.1.1.1"), port)
    onoff.SetAttribute("Remote", ns.AddressValue(inet_address.ConvertTo()))
    app = onoff.Install(ns.NodeContainer(terminals.Get(3)))
    app.Start(ns.Seconds(1.1))
    app.Stop(ns.Seconds(10.0))

    app = sink.Install(ns.NodeContainer(terminals.Get(0)))
    app.Start(ns.Seconds(0.0))

    #
    # Configure tracing of all enqueue, dequeue, and NetDevice receive events.
    # Trace output will be sent to the file "csma-bridge.tr"
    #
    # print "Configure Tracing."
    # ascii = ns.AsciiTraceHelper();
    # csma.EnableAsciiAll(ascii.CreateFileStream ("csma-bridge.tr"));

    #
    # Also configure some tcpdump traces; each interface will be traced.
    # The output files will be named:
    #     csma-bridge.pcap-<nodeId>-<interfaceId>
    # and can be read by the "tcpdump -r" command(use "-tt" option to
    # display timestamps correctly)
    #
    csma.EnablePcapAll("csma-bridge", False)

    #
    # Now, do the actual simulation.
    #
    # print "Run Simulation."
    ns.Simulator.Run()
    ns.Simulator.Destroy()
    # print "Done."


if __name__ == "__main__":
    import sys

    main(sys.argv)
