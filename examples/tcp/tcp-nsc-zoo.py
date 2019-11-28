'''
Python Equivalent of /examples/tcp/tcp-nsc-zoo.cc

16C0101 - Anshul Pinto
16CO118 - Jay Satish Shinde


 Network topology

       n0    n1   n2   n3
       |     |    |    |
       =================
              LAN

 - Pcap traces are saved as tcp-nsc-zoo-$n-0.pcap, where $n represents the node number
 - TCP flows from n0 to n1, n2, n3, from n1 to n0, n2, n3, etc.
  Usage (e.g.): ./waf --run 'tcp-nsc-zoo --nodes=5'
'''

import ns.core
import ns.applications
import ns.network 
import ns.csma
import ns.internet
import sys

'''
Simulates a diverse network with various stacks supported by NSC
'''

csma = ns.csma.CsmaHelper()

cmd = ns.core.CommandLine ()
cmd.MaxNodes = 4
cmd.runtime = 3
cmd.AddValue ("nodes", "Number of nodes in the network (must be > 1)")
cmd.AddValue ("runtime", "How long the applications should send data (default 3 seconds)")
cmd.Parse (sys.argv)

ns.core.Config.SetDefault("ns3::OnOffApplication::PacketSize",ns.core.UintegerValue(2048))
ns.core.Config.SetDefault("ns3::OnOffApplication::DataRate",ns.core.StringValue ("8kbps"))

if cmd.MaxNodes < 2 :
	print("Error: --nodes: must be >= 2 \n " )
	sys.exit(0)


csma.SetChannelAttribute ("DataRate", ns.network.DataRateValue(ns.network.DataRate(100000000)))
csma.SetChannelAttribute ("Delay", ns.core.TimeValue (ns.core.MicroSeconds (200)))

nodes = ns.network.NodeContainer()
nodes.Create(cmd.MaxNodes)

ethInterfaces = ns.network.NetDeviceContainer(csma.Install(nodes))

internet = ns.internet.InternetStackHelper ()
internet.SetTcp ("ns3::NscTcpL4Protocol","Library",ns.core.StringValue("liblinux2.6.26.so"))

'''
 this switches nodes 0 and 1 to NSCs Linux 2.6.26 stack.
'''
internet.Install (nodes.Get (0));
internet.Install (nodes.Get (1));

'''
this disables TCP SACK, wscale and timestamps on node 1 (the attributes represent sysctl-values)
'''

ns.core.Config.Set("/NodeList/1/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack",ns.core.StringValue("0"))
ns.core.Config.Set("/NodeList/1/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps",ns.core.StringValue("0"))
ns.core.Config.Set("/NodeList/1/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling",ns.core.StringValue("0"))

if MaxNodes > 2 :
	internet.Install(nodes.Get(2))

if MaxNodes > 3 :
	'''
	the next statement doesn't change anything for the nodes 0, 1, and 2; since they
	already have a stack assigned
	'''
	internet.SetTcp("ns3::NscTcpL4Protocol","Library",ns.core.StringValue("liblinux2.6.26.so"))
	'''
	this switches node 3 to NSCs Linux 2.6.26 stack.
	'''
	internet.Install(nodes.Get(3))
	'''
	and then agains disables sack/timestamps/wscale on node 3
	'''
	ns.Core.Config.Set("/NodeList/3/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", ns.Core.StringValue ("0"))
	ns.Core.Config.Set("/NodeList/3/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", ns.Core.StringValue ("0"))
	ns.Core.Config.Set("/NodeList/3/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", ns.Core.StringValue ("0"))

'''
the freebsd stack is not yet built by default, so its commented out for now.
nternetStack.SetNscStack ("libfreebsd5.so")
'''
for i in range(4,cmd.MaxNodes):
	internet.Install(nodes.Get(i))

ipv4 = ns.internet.Ipv4AddressHelper ()
ipv4.SetBase (ns.network.Ipv4Address ("10.0.0.0"), ns.network.Ipv4Mask ("255.255.255.0"))
ipv4Interfaces = ipv4.Assign (ethInterfaces)

ns.Internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

servPort = 8080

sink = ns.applications.PacketSinkHelper ("ns3::TcpSocketFactory", ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))

'''
start a sink client on all nodes
'''

sinkApp = sink.Install (nodes)
sinkApp.Start(ns.core.Seconds (0.0))
sinkApp.Stop(ns.core.Seconds (30.0))

'''
This tells every node on the network to start a flow to all other nodes on the network ...
'''

for i in range(MaxNodes):
	for j in range(MaxNodes):
		if i == j:
			continue
		remoteAddress = ns.network.Address(ns.network.utils.InetSocketAddress(ns.internet.ipv4Interfaces.GetAddress(j),servPort))
		clientHelper = ns.applications.OnOffApplication("ns3::TcpSocketFactory", remoteAddress)
		clientHelper.SetAttribute("OnTime", ns.Core.StringValue ("ns3::ConstantRandomVariable[Constant=1]"))
		clientHelper.SetAttribute("OFFTime", ns.Core.StringValue ("ns3::ConstantRandomVariable[Constant=0]"))

		clientApp = clientHelper.Install (n.Get (i))
		clientApp.Start(ns.Core.Seconds(float(j))
		clientApp.Stop(ns.Core.Seconds(float(j + runtime)))

csma.EnablePcapAll("tcp-nsc-zoo", False)

ns.core.Simulator.Stop (ns.core.Seconds (100.0))
ns.core.Simulator.Run ()
ns.core.Simulator.Destroy ()
print ("Done.")







