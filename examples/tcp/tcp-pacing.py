# Python equivalent of /examples/tcp/tcp-pacing.cc

# Jay Satish Shinde 16CO118
# Anshul Pinto 16CO101


import string
import ns.core
import ns.point_to_point
import ns.internet
import ns.applications
import ns.network

#Not required
#include <fstream>
#include "ns3/packet-sink.h"

#include "ns3/flow-monitor-module.h"
import ns.flow_monitor

def main(argv):

	cmd = ns.core.CommandLine ()
	cmd.tracing = "False"
	cmd.maxBytes = 0
	cmd.TCPFlows = 1
	cmd.isPacingEnabled = "True"
	cmd.isSack = "False"
	cmd.maxPackets = 0
	cmd.pacingRate = "4Gbps"
	cmd.AddValue("tracing", "Flag to enable/disable tracing")
	cmd.AddValue("maxBytes", "Total number of bytes for application to send")
	cmd.AddValue("TCPFlows", "Number of application flows between sender and receiver")
	cmd.AddValue("maxPackets", "Total number of packets for application to send")
	cmd.AddValue("isPacingEnabled", "Flag to enable/disable pacing in TCP")
	cmd.AddValue("isSack", "Flag to enable/disable sack in TCP")
	cmd.AddValue("pacingRate", "Max Pacing Rate in bps")
	cmd.Parse (sys.argv)


	tracing = cmd.tracing
	maxBytes = int(cmd.maxBytes)
	TCPFlows = int(cmd.TCPFlows)
	isPacingEnabled = cmd.isPacingEnabled
	isSack = cmd.isSack
	maxPackets = int(cmd.maxPackets)
	pacingRate = cmd.pacingRate 

	if (maxPackets!= 0):
		maxBytes = 500 * maxPackets

	ns.core.Config.SetDefault ("ns3::TcpSocketState::MaxPacingRate", ns.core.StringValue (pacingRate))
	ns.core.Config.SetDefault ("ns3::TcpSocketState::EnablePacing", ns.core.BooleanValue (isPacingEnabled))
	ns.core.Config.SetDefault ("ns3::TcpSocketBase::Sack", ns.core.BooleanValue (isSack))
	
	print("Create nodes")
	nodes = ns.network.NodeContainer ()
	nodes.Create (2)

	print("Create Channels")
	pointToPoint = ns.point_to_point.PointToPointHelper ()
	pointToPoint.SetDeviceAttribute ("DataRate", ns.core.StringValue ("40Gbps"))
	pointToPoint.SetChannelAttribute ("Delay", ns.core.StringValue ("0.01ms"))

	devices = pointToPoint.Install (nodes)

	stack = ns.internet.InternetStackHelper ()
	stack.Install (nodes)

	print("Assign IP Addresses")
	address = ns.internet.Ipv4AddressHelper ()
	address.SetBase (ns.network.Ipv4Address ("10.1.1.0"), ns.network.Ipv4Mask ("255.255.255.0"))
	i = address.Assign (devices)

	print("Create applications")

	sourceApps = ns.applications.ApplicationContainer()
	sinkApps = ns.applications.ApplicationContainer()

	for iterator in range(0, TCPFlows):
		port = 10000 + iterator
		source = ns.applications.BulkSendHelper ("ns3::TcpSocketFactory", ns.network.InetSocketAddress (i.GetAddress (1), port))
		source.SetAttribute ("MaxBytes", ns.core.UintegerValue (maxBytes))
		sourceApps.Add(source.Install (nodes.Get (0)))
		sink = ns.applications.PacketSinkHelper ("ns3::TcpSocketFactory", ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))
		sinkApps.Add(sink.Install(nodes.Get (1)))

	sinkApps.Start (ns.core.Seconds (0.0))
	sinkApps.Stop (ns.core.Seconds (5.0))
	sourceApps.Start (ns.core.Seconds (1.0))
	sourceApps.Stop (ns.core.Seconds (5.0))

	if tracing == "True":
		ascii = ns.network.AsciiTraceHelper ()
		pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp--pacing.tr"))
		pointToPoint.EnablePcapAll ("tcp-pacing", False)

 	
 	flowmon = ns.flow_monitor.FlowMonitorHelper()

 	# Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
 	# monitor = FlowMonitor()
 	monitor = flowmon.InstallAll()

 	print "Run Simulation."
	ns.core.Simulator.Stop (ns.core.Seconds (5.0))
	ns.core.Simulator.Run ()

	# 	monitor->CheckForLostPackets ();
	monitor.CheckForLostPackets()
	#   Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	#   FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
	classifier = flowmon.GetClassifier()
	stats = monitor.GetFlowStats()
	for i in stats:
		t = classifier.FindFlow(i.first) 
		if t.sourceAddress == "10.1.1.2":
			continue
		try:
			print("Flow ",i.first,"(",t.sourceAddress,"->",t.destinationAddress,")") 
			print("Tx Packets: ",i.second.txPackets)
			print("Tx Bytes: ", i.second.txBytes)
			print("TxOffered: ", i.second.txBytes * 8.0 /9 .0 / 1000 / 1000, " Mbps")
			print("Rx Packets: ", i.second.rxPackets)
			print("Rx Bytes: ", i.second.rxBytes)
			print("Throughput: ", i.second.rxBytes * 8.0 / 9.0 / 1000 / 1000, " Mbps")
		except:
			print("Flow ",i[0],"(",t.sourceAddress,"->",t.destinationAddress,")") 
			print("Tx Packets: ",i[1].txPackets)
			print("Tx Bytes: ", i[1].txBytes)
			print("TxOffered: ", i[1].txBytes * 8.0 /9 .0 / 1000 / 1000, " Mbps")
			print("Rx Packets: ", i[1].rxPackets)
			print("Rx Bytes: ", i[1].rxBytes)
			print("Throughput: ", i[1].rxBytes * 8.0 / 9.0 / 1000 / 1000, " Mbps")


	ns.core.Simulator.Destroy ()
	print "Done."



