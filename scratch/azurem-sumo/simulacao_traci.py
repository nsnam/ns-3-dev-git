import os
import sys
from ns import ns

# --- SUMO SETUP ---
if 'SUMO_HOME' in os.environ:
    tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
    sys.path.append(tools)
else:
    sys.exit("Please set SUMO_HOME environment variable")

import traci

sumo_binary = "sumo" # Change to "sumo" for faster headless simulation
sumo_cfg_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "campus.sumocfg")
sumo_cmd = [sumo_binary, "-c", sumo_cfg_path, "--start"]

# --- NS-3 CALLBACK SETUP ---
ns.cppyy.cppdef("""
    using namespace ns3;
    Callback<void,Ptr<const Packet>,const Address&,const Address&>
    make_sinktrace_callback(void(*func)(Ptr<const Packet>, const Address&,const Address&))
    {
        return MakeCallback(func);
    }
""")

def SinkTracer(packet: ns.Packet, src_address: ns.Address, dst_address: ns.Address) -> None:
    print(f"At {ns.Simulator.Now().GetSeconds():.2f}s, '{dst_address}' received packet "
          f"from '{src_address}'")

# --- SIMULATION CONFIGURATION ---
TOTAL_SECONDS = 20
STEP_SIZE = 0.1 # 100ms steps for smooth mobility

def run_combined_simulation():
    # 1. Start SUMO
    traci.start(sumo_cmd)

    # 2. Setup NS-3 Nodes (CSMA for simplicity, but representing V2I)
    nodes = ns.NodeContainer()
    nodes.Create(2)

    csma = ns.CsmaHelper()
    csma.SetChannelAttribute("DataRate", ns.StringValue("100Mbps"))
    csma.SetChannelAttribute("Delay", ns.TimeValue(ns.NanoSeconds(6560)))
    devices = csma.Install(nodes)

    stack = ns.InternetStackHelper()
    stack.Install(nodes)

    address = ns.Ipv4AddressHelper()
    address.SetBase(ns.Ipv4Address("10.1.2.0"), ns.Ipv4Mask("255.255.255.0"))
    interfaces = address.Assign(devices)

    # 3. Setup UDP Echo (Simulating V2I Data)
    echoServer = ns.UdpEchoServerHelper(9)
    serverApps = echoServer.Install(nodes.Get(0))
    serverApps.Start(ns.Seconds(0))
    serverApps.Stop(ns.Seconds(TOTAL_SECONDS))

    echoClient = ns.UdpEchoClientHelper(interfaces.GetAddress(0).ConvertTo(), 9)
    echoClient.SetAttribute("MaxPackets", ns.UintegerValue(1000))
    echoClient.SetAttribute("Interval", ns.TimeValue(ns.Seconds(0.5)))
    echoClient.SetAttribute("PacketSize", ns.UintegerValue(1024))

    clientApps = echoClient.Install(nodes.Get(1))
    clientApps.Start(ns.Seconds(1))
    clientApps.Stop(ns.Seconds(TOTAL_SECONDS))

    # 4. Connect Trace
    sinkTraceCallback = ns.cppyy.gbl.make_sinktrace_callback(SinkTracer)
    serverApps.Get(0).__deref__().TraceConnectWithoutContext("RxWithAddresses", sinkTraceCallback)

    # 5. THE SYNCHRONIZED LOOP
    print("Starting Synchronized Simulation...")

    current_time = 0.0
    while current_time < TOTAL_SECONDS:
        # Advance ns-3 to the next step
        current_time += STEP_SIZE
        ns.Simulator.Stop(ns.Seconds(current_time))
        ns.Simulator.Run()

        # Advance SUMO
        traci.simulationStep()

        # Bridge Logic: Get SUMO position and print (or feed into ns-3 Mobility)
        veiculos = traci.vehicle.getIDList()
        if veiculos:
            vid = veiculos[0]
            pos = traci.vehicle.getPosition(vid)
            # LOGIC: Here you would update ns-3 Node Mobility using pos
            # e.g., nodes.Get(1).GetObject[ns.MobilityModel]().SetPosition(...)
            print(f"Time: {current_time:.1f}s | Vehicle {vid} at X:{pos[0]:.1f}")

    # 6. Cleanup
    ns.Simulator.Destroy()
    traci.close()
    print("Simulation Finished.")

if __name__ == "__main__":
    run_combined_simulation()