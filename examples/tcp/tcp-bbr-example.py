# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (c) 2018-20 NITK Surathkal
# Copyright (c) 2025 CSPIT, CHARUSAT
#
# Authors:
#   Aarti Nandagiri <aarti.nandagiri@gmail.com>
#   Vivek Jain <jain.vivek.anand@gmail.com>
#   Mohit P. Tahiliani <tahiliani@nitk.edu.in>
#   Urval Kheni <kheniurval777@gmail.com>
#   Ritesh Patel <riteshpatel.ce@charusat.ac.in>

"""
Python port of tcp-bbr-example.cc

Simulates the following topology:

    Sender ---- R1 ---- R2 ---- Receiver
     1000Mbps    10Mbps   1000Mbps
       5ms        10ms       5ms

Outputs:
- cwnd.dat        (congestion window)
- throughput.dat  (throughput)
- queueSize.dat   (queue length)
"""

import argparse
import os
import time

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found. "
        "Bindings may not be enabled or PYTHONPATH is incorrect."
    )

# Global trace variables
prev_tx = 0
prev_time = None
throughput_file = None
queue_file = None
cwnd_file = None
outdir = ""


def trace_throughput_callback():
    """Throughput trace callback."""
    global prev_tx, prev_time, throughput_file

    stats = ns.cppyy.gbl.monitor_ptr.GetFlowStats()
    if stats and not stats.empty():
        flow_stats = stats.begin().__deref__().second
        cur_time = ns.Now()

        bits = 8 * (flow_stats.txBytes - prev_tx)
        us = (cur_time - prev_time).ToDouble(ns.Time.US)

        if us > 0:
            throughput_file.write(f"{cur_time.GetSeconds():.6g}s {bits / us:.6g} Mbps\n")
            throughput_file.flush()

        prev_time = cur_time
        prev_tx = flow_stats.txBytes


def check_queue_size_callback():
    """Queue length tracer."""
    global queue_file

    q = ns.cppyy.gbl.qdisc_ptr.GetCurrentSize().GetValue()
    queue_file.write(f"{ns.Simulator.Now().GetSeconds():.6g} {q}\n")
    queue_file.flush()


def trace_cwnd_callback(oldval, newval):
    """Congestion window tracer."""
    global cwnd_file

    cwnd_file.write(f"{ns.Simulator.Now().GetSeconds():.6g} {newval / 1448.0:.6g}\n")
    cwnd_file.flush()


# C++ helper functions for scheduling Python callbacks
ns.cppyy.cppdef(r"""
#include "ns3/simulator.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/queue-disc.h"
#include "ns3/node.h"
#include "ns3/config.h"

using namespace ns3;

Ptr<FlowMonitor> monitor_ptr;
Ptr<QueueDisc> qdisc_ptr;

// Helper to create event from Python callback without parameters
EventImpl* pythonMakeEvent(void (*f)())
{
    return MakeEvent(f);
}

// Helper to connect cwnd callback and create event
EventImpl* pythonConnectCwndTrace(void (*f)(uint32_t, uint32_t))
{
    Config::ConnectWithoutContext(
        "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
        MakeCallback(f));
    return nullptr;
}
""")


def main():
    global throughput_file, queue_file, cwnd_file, outdir, prev_time
    global prev_tx

    parser = argparse.ArgumentParser(description="tcp-bbr-example (Python)")
    parser.add_argument(
        "--tcpTypeId",
        default="TcpBbr",
        help="Transport protocol: TcpNewReno, TcpBbr",
    )
    parser.add_argument("--delAckCount", type=int, default=2, help="Delayed ACK count")
    parser.add_argument("--enablePcap", action="store_true", help="Enable pcap traces")
    parser.add_argument("--stopTime", type=float, default=100.0, help="Simulation stop time")
    args = parser.parse_args()

    ts = time.strftime("%d-%m-%Y-%I-%M-%S")
    outdir = f"bbr-results/{ts}/"
    os.makedirs(outdir, exist_ok=True)

    tcpType = args.tcpTypeId
    queueDisc = "ns3::FifoQueueDisc"
    delAck = args.delAckCount
    enablePcap = args.enablePcap
    stopTime = ns.Seconds(args.stopTime)

    ns.Config.SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        ns.StringValue("ns3::" + tcpType),
    )
    ns.Config.SetDefault("ns3::TcpSocket::SndBufSize", ns.UintegerValue(4194304))
    ns.Config.SetDefault("ns3::TcpSocket::RcvBufSize", ns.UintegerValue(6291456))
    ns.Config.SetDefault("ns3::TcpSocket::InitialCwnd", ns.UintegerValue(10))
    ns.Config.SetDefault("ns3::TcpSocket::DelAckCount", ns.UintegerValue(delAck))
    ns.Config.SetDefault("ns3::TcpSocket::SegmentSize", ns.UintegerValue(1448))
    ns.Config.SetDefault(
        "ns3::DropTailQueue<Packet>::MaxSize", ns.QueueSizeValue(ns.QueueSize("1p"))
    )
    ns.Config.SetDefault(
        queueDisc + "::MaxSize",
        ns.QueueSizeValue(ns.QueueSize("100p")),
    )

    sender = ns.NodeContainer()
    receiver = ns.NodeContainer()
    routers = ns.NodeContainer()
    sender.Create(1)
    receiver.Create(1)
    routers.Create(2)

    bottleneck = ns.PointToPointHelper()
    bottleneck.SetDeviceAttribute("DataRate", ns.StringValue("10Mbps"))
    bottleneck.SetChannelAttribute("Delay", ns.StringValue("10ms"))

    edge = ns.PointToPointHelper()
    edge.SetDeviceAttribute("DataRate", ns.StringValue("1000Mbps"))
    edge.SetChannelAttribute("Delay", ns.StringValue("5ms"))

    senderEdge = edge.Install(sender.Get(0), routers.Get(0))
    r1r2 = bottleneck.Install(routers.Get(0), routers.Get(1))
    receiverEdge = edge.Install(routers.Get(1), receiver.Get(0))

    inet = ns.InternetStackHelper()
    inet.Install(sender)
    inet.Install(receiver)
    inet.Install(routers)

    tch = ns.TrafficControlHelper()
    tch.SetRootQueueDisc(queueDisc)
    tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", ns.StringValue("1000ms"))
    tch.Install(senderEdge)
    tch.Install(receiverEdge)

    ipv4 = ns.Ipv4AddressHelper()
    ipv4.SetBase(ns.Ipv4Address("10.0.0.0"), ns.Ipv4Mask("255.255.255.0"))

    _ = ipv4.Assign(r1r2)

    ipv4.NewNetwork()
    ipv4.Assign(senderEdge)

    ipv4.NewNetwork()
    ir1 = ipv4.Assign(receiverEdge)

    ns.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

    port = 50001

    source = ns.BulkSendHelper(
        "ns3::TcpSocketFactory",
        ns.InetSocketAddress(ir1.GetAddress(1), port).ConvertTo(),
    )
    source.SetAttribute("MaxBytes", ns.UintegerValue(0))
    sourceApps = source.Install(sender.Get(0))
    sourceApps.Start(ns.Seconds(0.1))
    sourceApps.Stop(stopTime)

    sink = ns.PacketSinkHelper(
        "ns3::TcpSocketFactory",
        ns.InetSocketAddress(ns.Ipv4Address.GetAny(), port).ConvertTo(),
    )
    sinkApps = sink.Install(receiver.Get(0))
    sinkApps.Start(ns.Seconds(0))
    sinkApps.Stop(stopTime)

    if enablePcap:
        os.makedirs(os.path.join(outdir, "pcap"), exist_ok=True)

    throughput_file = open(os.path.join(outdir, "throughput.dat"), "w")
    queue_file = open(os.path.join(outdir, "queueSize.dat"), "w")
    cwnd_file = open(os.path.join(outdir, "cwnd.dat"), "w")

    if enablePcap:
        bottleneck.EnablePcapAll(os.path.join(outdir, "pcap", "bbr"), True)

    flowmon = ns.FlowMonitorHelper()
    monitor = flowmon.InstallAll()

    tch.Uninstall(routers.Get(0).GetDevice(1))
    qdc = tch.Install(routers.Get(0).GetDevice(1))

    prev_time = ns.Now()
    prev_tx = 0

    ns.cppyy.gbl.monitor_ptr = monitor
    ns.cppyy.gbl.qdisc_ptr = qdc.Get(0)

    # Define wrapper functions that reschedule themselves
    def throughput_tracer():
        """Periodic throughput tracer that reschedules itself."""
        trace_throughput_callback()
        event = ns.cppyy.gbl.pythonMakeEvent(throughput_tracer)
        ns.Simulator.Schedule(ns.Seconds(0.2), event)

    def queue_size_tracer():
        """Periodic queue size tracer that reschedules itself."""
        check_queue_size_callback()
        event = ns.cppyy.gbl.pythonMakeEvent(queue_size_tracer)
        ns.Simulator.Schedule(ns.Seconds(0.2), event)

    # Schedule initial events using pythonMakeEvent
    event = ns.cppyy.gbl.pythonMakeEvent(throughput_tracer)
    ns.Simulator.Schedule(ns.Seconds(0.000001), event)

    event = ns.cppyy.gbl.pythonMakeEvent(queue_size_tracer)
    ns.Simulator.ScheduleNow(event)

    # Connect cwnd trace after socket is created
    def connect_cwnd_trace():
        """Connect the cwnd trace callback."""
        ns.cppyy.gbl.pythonConnectCwndTrace(trace_cwnd_callback)

    event = ns.cppyy.gbl.pythonMakeEvent(connect_cwnd_trace)
    ns.Simulator.Schedule(ns.Seconds(0.1) + ns.MilliSeconds(1), event)

    ns.Simulator.Stop(stopTime + ns.TimeStep(1))
    ns.Simulator.Run()
    ns.Simulator.Destroy()

    throughput_file.close()
    queue_file.close()
    cwnd_file.close()


if __name__ == "__main__":
    main()
