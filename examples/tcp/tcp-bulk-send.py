'''
Python Equivalent of /examples/tcp/tcp-bulk-send.cc

16C0101 - Anshul Pinto
16CO118 - Jay Satish Shinde
'''

import ns.core
import ns.network
#import ns.packet-sink
import ns.internet
#import ns.flow-monitor
import ns.point_to_point
import ns.applications

tracing = False
maxBytes = 0

cmd = ns.core.CommandLine()

'''
Allow the user to override any of the defaults at
run-time, via command-line arguments
'''

cmd.AddValue("tracing","Flag to enable/disable tracing",tracing)
cmd.AddValue("maxBytes","Total number of bytes for application to send",maxBytes)
cmd.Parse()

'''
Explicitly create the nodes required by the topology (shown above)
'''

print("Create Nodes")

nodes = ns.network.NodeContainer()
nodes.Create(2)

print("Create channels. ")

'''
Explicitly create the point-to-point link required by the topology (shown above)
'''

pointToPoint = ns.point_to_point.PointToPointHelper ()
pointToPoint.SetDeviceAttribute ("DataRate", ns.core.StringValue ("500Kbps"))
pointToPoint.SetChannelAttribute ("Delay", ns.core.StringValue ("5ms"))

devices = pointToPoint.Install (nodes)

''' 
Install the internet stack on the nodes
'''

internet = ns.internet.InternetStackHelper ()
internet.Install (nodes)

''' 
We've got the "hardware" in place.  Now we need to add IP addresses.
'''

print( "Assign IP Addresses.")
ipv4 = ns.internet.Ipv4AddressHelper ()
ipv4.SetBase (ns.network.Ipv4Address ("10.1.1.0"), ns.network.Ipv4Mask ("255.255.255.0"))
i = ipv4.Assign (devices)

print ("Create Applications.")

''' 
 Create a BulkSendApplication and install it on node 0
''' 
port = 9  # well-known echo port number

source = ns.applications.BulkSendHelper ("ns3::TcpSocketFactory", ns.network.InetSocketAddress (i.GetAddress (1), port))

'''
Set the amount of data to send in bytes.  Zero is unlimited.
'''
source.SetAttribute ("MaxBytes", ns.core.UintegerValue (maxBytes))
sourceApps = source.Install (nodes.Get (0))
sourceApps.Start (ns.core.Seconds (0.0))
sourceApps.Stop (ns.core.Seconds (10.0))

''' 
Create a PacketSinkApplication and install it on node 1
'''

sink = ns.applications.PacketSinkHelper ("ns3::TcpSocketFactory", ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))
sinkApps = sink.Install (nodes.Get (1))
sinkApps.Start (ns.core.Seconds (0.0))
sinkApps.Stop (ns.core.Seconds (10.0))

''' 
Set up tracing if enabled
'''

if tracing == True:
	ascii = ns.network.AsciiTraceHelper ()
	pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send-py.tr"))
	pointToPoint.EnablePcapAll ("tcp-bulk-send-py", False)


''' 
Now, do the actual simulation.
'''

print ("Run Simulation.")
ns.core.Simulator.Stop (ns.core.Seconds (10.0))
ns.core.Simulator.Run ()
ns.core.Simulator.Destroy ()
print ("Done.")

sink1 = ns.applications.PacketSink (sinkApps.Get (00))
print ("Total Bytes Received:", sink1.GetTotalRx ())





