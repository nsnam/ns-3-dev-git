# Python equivalent of /examples/tcp/star.cc

# Jay Satish Shinde 16CO118
# Anshul Pinto 16CO101

import ns.core
import ns.network
import ns.applications
import ns.point_to_point
import ns.point_to_point_layout
import ns.internet

# Network topology (default)
#
#        n2 n3 n4              .
#         \ | /                .
#          \|/                 .
#     n1--- n0---n5            .
#          /|\                 .
#         / | \                .
#        n8 n7 n6              .


def main(argv):
	# Default values for simulation 
	ns.core.Config.SetDefault ("ns3::OnOffApplication::PacketSize", ns.core.UintegerValue (137))
	ns.core.Config.SetDefault ("ns3::OnOffApplication::DataRate", ns.core.StringValue ("14kb/s"))


	Command = ns.core.CommandLine()
	# Default number of nodes in the topology. Can be changed by command line
	Command.nSpokes = 8
	Command.AddValue ("nSpokes", "Number of nodes to place in the star")
	Command.Parse (sys.argv)

	
	nSpokes = int(Command.nSpokes)

	# Star topology
	print("Building Star Topology")
	pointToPoint = ns.point_to_point.PointToPointHelper ()
	pointToPoint.SetDeviceAttribute ("DataRate", ns.core.StringValue ("5Mbps"))
	pointToPoint.SetChannelAttribute ("Delay", ns.core.StringValue ("2ms"))
	star = ns.point_to_point_layout.PointToPointStarHelper (nSpokes, pointToPoint)

	# Install stack
	print("Installing Internet stack on all nodes")
	internet = ns.internet.InternetStackHelper ()
	star.InstallStack (internet)

	# Assign IP Address
	print("Assigning IP Addresses")
	star.AssignIpv4Addresses (ns.internet.Ipv4AddressHelper (ns.network.Ipv4Address ("10.1.1.0"), ns.network.Ipv4Mask ("255.255.255.0")))

	# Creating a packet sink on the star "hub" to receive packets.
	print("Create Applications")
	port = 50000
	hubLocalAddress = ns.network.Address(ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))
	packetSinkHelper = ns.applications.PacketSinkHelper ("ns3::TcpSocketFactory", hubLocalAddress)
	hubApp = packetSinkHelper.Install(star.GetHub ())
	hubApp.Start (ns.core.Seconds(1.0))
	hubApp.Stop (ns.core.Seconds(10.0))

	onOffHelper = ns.applications.OnOffHelper ("ns3::TcpSocketFactory", ns.network.Address ())
	onOffHelper.SetAttribute ("OnTime", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=1]"))
	onOffHelper.SetAttribute ("OffTime", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=0]"))

	spokeApps = ns.network.ApplicationContainer ()

	for i in range(0, star.SpokeCount ()):
		remoteAddress = ns.network.AddressValue (ns.network.InetSocketAddress (star.GetHubIpv4Address (i), port))
		onOffHelper.SetAttribute ("Remote", remoteAddress)
		spokeApps = onOffHelper.Install (star.GetSpokeNode (i))

	
	spokeApps.Start (ns.core.Seconds (1))
	spokeApps.Stop (ns.core.Seconds (10))

	print("Enable static global routing")
	ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables ()

	# Enable pcap tracing
	print("Enable pcap tracing")
	pointToPoint.EnablePcapAll ("star-py")

	# Run Simualation
	print("Run Simulation")
	ns.core.Simulator.Stop (ns.core.Seconds (12))
	ns.core.Simulator.Run ()
	ns.core.Simulator.Destroy ()
	
	print("Done")

if __name__ == '__main__':
    import sys
    main (sys.argv)




