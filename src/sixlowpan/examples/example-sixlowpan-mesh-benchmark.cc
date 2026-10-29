/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

/**
 * @file
 * @ingroup sixlowpan
 *
 * Benchmark comparing the two 6LoWPAN mesh-under forwarding policies:
 * Simple Flooding (SixLowPanSimpleFlooding, the default) against Adaptive
 * Flooding (SixLowPanAdaptiveFlooding, based on the Trickle algorithm).
 *
 * Unlike example-ping-lr-wpan-mesh-under.cc, which shows basic mesh-under
 * reachability through a CSMA gateway, this example quantitatively compares
 * the control overhead of the two forwarding policies on a flat /64.
 *
 * A static IEEE 802.15.4 / 6LoWPAN network is built, a single source floods
 * packets towards a sink, and the example reports the figures the forwarding
 * policy actually influences:
 *   - PHY transmissions: every frame put on the air, i.e. the original send
 *     plus all the redundant re-broadcasts that Trickle is meant to suppress.
 *     This is the mesh-under control overhead.
 *   - PHY Rx drops: receptions discarded by the PHY (collisions and
 *     interference), a proxy for broadcast-storm congestion.
 *   - Delivery: application packets sent vs. received (ICMP echo replies at the
 *     source, or UDP packets at the sink).
 *
 * Two topologies are provided (--topology):
 *
 * @verbatim
   dense: N nodes in a small square, roughly in mutual range. The source
          and the sink are pinned at opposite corners (a consistent
          worst-case pair); the relays in between are placed randomly and
          maximise redundancy -> shows where Adaptive Flooding wins by
          collapsing the storm.

          S . o   o
            o   o  o               src  = node 0 (fixed corner)
            o  o   o               sink = node N-1 (opposite corner)
                 o . D

   bridge: two clusters joined ONLY through a single bridge node. The
           clusters are out of mutual range, so every packet from A to B
           must traverse the bridge (a cut vertex) -> stresses Adaptive
           Flooding on a single thin path, where over-suppression can
           reduce delivery. Source and sink are pinned at the far edges
           of their clusters (worst case); the other nodes are random.

           cluster A         bridge          cluster B
             o o                                o o
           S o o ---------- [ B ] ---------- o  o  D
             o                                  o
           src = node 0 (far edge)        sink = node N-1 (far edge)
   @endverbatim
 *
 * The policy is toggled with --adaptive-flooding (false = Simple Flooding,
 * true = Adaptive Flooding), the traffic with --traffic (icmp | udp), and the
 * node count with --nodes, so a driver script can sweep them and collect the
 * "CSV," line printed at the end. Results are reproducible for a given
 * --RngRun; pass --RngRun=2,3,... to obtain independent replications.
 *
 * Example:
 *   ./ns3 run "example-sixlowpan-mesh-benchmark --adaptive-flooding=1 --topology=dense"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/spectrum-module.h"

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SixLowPanMeshBenchmark");

namespace
{
uint64_t g_phyTx = 0;       //!< Total PHY transmissions (re-broadcasts included).
uint64_t g_phyRxDrop = 0;   //!< Receptions dropped by the PHY (collisions/interference).
uint32_t g_appSent = 0;     //!< Application packets sent by the source.
uint32_t g_appReceived = 0; //!< Application packets received (echo replies, or UDP at the sink).

/**
 * Count a PHY transmission.
 * @param p the transmitted packet (unused).
 */
void
CountPhyTx([[maybe_unused]] Ptr<const Packet> p)
{
    ++g_phyTx;
}

/**
 * Count a dropped reception.
 * @param p the dropped packet (unused).
 */
void
CountPhyRxDrop([[maybe_unused]] Ptr<const Packet> p)
{
    ++g_phyRxDrop;
}

/**
 * Capture the final ping delivery figures.
 * @param report the ping application's end-of-run report.
 */
void
CapturePingReport(const Ping::PingReport& report)
{
    g_appSent = report.m_transmitted;
    g_appReceived = report.m_received;
}
} // namespace

int
main(int argc, char** argv)
{
    bool adaptiveFlooding = false;
    uint32_t nNodes = 10;
    std::string topology = "dense";
    std::string traffic = "icmp";
    double simTime = 30.0;
    uint32_t numPackets = 10;
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("adaptive-flooding",
                 "Use Adaptive Flooding (true) instead of Simple Flooding (false)",
                 adaptiveFlooding);
    cmd.AddValue("nodes", "Number of WSN nodes", nNodes);
    cmd.AddValue("topology", "Node layout: dense | bridge", topology);
    cmd.AddValue("traffic", "Traffic type: icmp (ping) | udp", traffic);
    cmd.AddValue("simTime", "Total simulation time (s)", simTime);
    cmd.AddValue("packets", "Number of packets sent by the source", numPackets);
    cmd.AddValue("verbose", "Enable model logging", verbose);
    cmd.Parse(argc, argv);

    const bool bridge = (topology == "bridge");
    NS_ABORT_MSG_IF(topology != "dense" && topology != "bridge",
                    "Unknown topology '" << topology << "', use dense | bridge");
    NS_ABORT_MSG_IF(traffic != "icmp" && traffic != "udp",
                    "Unknown traffic '" << traffic << "', use icmp | udp");
    // dense needs >= 3 to be meaningful; bridge needs >= 2 nodes per cluster
    // plus the bridge, so >= 5, otherwise the "two clusters" case is degenerate.
    NS_ABORT_MSG_IF(!bridge && nNodes < 3, "dense topology needs at least 3 nodes");
    NS_ABORT_MSG_IF(bridge && nNodes < 5, "bridge topology needs at least 5 nodes");

    if (verbose)
    {
        LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_INFO);
        LogComponentEnable("SixLowPanAdaptiveFlooding", LOG_LEVEL_INFO);
        LogComponentEnable("SixLowPanSimpleFlooding", LOG_LEVEL_INFO);
    }

    int64_t streamNumber = 100;

    NodeContainer wsnNodes;
    wsnNodes.Create(nNodes);

    // ---- Static placement ------------------------------------------------
    // The source and the sink are pinned at the opposite extremes of the
    // deployment area, so every run measures the same worst-case pair and
    // sweeping --nodes only changes the relay population, not the
    // source-sink geometry. The relay nodes are scattered randomly,
    // reproducibly for a given --RngRun (the stream below is pinned).
    // The chosen distances keep intra-cluster and cluster-to-bridge links
    // within the LR-WPAN range under the default LogDistance loss model,
    // while the two clusters (~120 m apart) are out of mutual range, so
    // the bridge is the only path.
    Ptr<UniformRandomVariable> jitter = CreateObject<UniformRandomVariable>();
    jitter->SetStream(streamNumber++);

    Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();
    uint32_t sourceIdx = 0;
    uint32_t sinkIdx = nNodes - 1;

    if (bridge)
    {
        const uint32_t bridgeIdx = nNodes / 2; // a single node bridges the two clusters
        positions->Add(Vector(0.0, 7.0, 0.0)); // source at the far edge of cluster A
        for (uint32_t i = 1; i < bridgeIdx; ++i)
        {
            positions->Add(Vector(jitter->GetValue(0.0, 15.0), jitter->GetValue(0.0, 15.0), 0.0));
        }
        positions->Add(Vector(60.0, 7.0, 0.0)); // bridge, between the clusters
        for (uint32_t i = bridgeIdx + 1; i < nNodes - 1; ++i)
        {
            positions->Add(
                Vector(120.0 + jitter->GetValue(0.0, 15.0), jitter->GetValue(0.0, 15.0), 0.0));
        }
        positions->Add(Vector(135.0, 7.0, 0.0)); // sink at the far edge of cluster B
    }
    else // dense
    {
        positions->Add(Vector(0.0, 0.0, 0.0)); // source at one corner
        for (uint32_t i = 1; i < nNodes - 1; ++i)
        {
            positions->Add(Vector(jitter->GetValue(0.0, 40.0), jitter->GetValue(0.0, 40.0), 0.0));
        }
        positions->Add(Vector(40.0, 40.0, 0.0)); // sink at the opposite corner
    }

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positions);
    mobility.Install(wsnNodes);

    // ---- IEEE 802.15.4 ---------------------------------------------------
    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(wsnNodes);
    streamNumber += lrWpanHelper.AssignStreams(lrwpanDevices, streamNumber);
    streamNumber += lrWpanHelper.GetChannel()->AssignStreams(streamNumber);
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

    // ---- IPv6 + 6LoWPAN --------------------------------------------------
    InternetStackHelper internetv6;
    internetv6.Install(wsnNodes);
    streamNumber += internetv6.AssignStreams(wsnNodes, streamNumber);

    SixLowPanHelper sixLowPanHelper;
    if (adaptiveFlooding)
    {
        sixLowPanHelper.SetMeshUnderRouting("ns3::SixLowPanAdaptiveFlooding");
    }
    sixLowPanHelper.SetDeviceAttribute("UseMeshUnder", BooleanValue(true));
    // Hops-left must cover the network diameter; nNodes is a safe bound.
    sixLowPanHelper.SetDeviceAttribute("MeshUnderRadius", UintegerValue(nNodes));
    NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install(lrwpanDevices);
    streamNumber += sixLowPanHelper.AssignStreams(sixLowPanDevices, streamNumber);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:f00d::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer wsnInterfaces = ipv6.Assign(sixLowPanDevices);

    // ---- Counters --------------------------------------------------------
    for (uint32_t i = 0; i < lrwpanDevices.GetN(); ++i)
    {
        Ptr<lrwpan::LrWpanNetDevice> dev =
            DynamicCast<lrwpan::LrWpanNetDevice>(lrwpanDevices.Get(i));
        dev->GetPhy()->TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&CountPhyTx));
        dev->GetPhy()->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&CountPhyRxDrop));
    }

    // ---- Traffic: one source floods the sink -----------------------------
    const Ipv6Address sinkAddr = wsnInterfaces.GetAddress(sinkIdx, 1);
    const uint16_t udpPort = 9;
    Ptr<UdpServer> udpServer;
    if (traffic == "udp")
    {
        UdpServerHelper server(udpPort);
        ApplicationContainer sinkApp = server.Install(wsnNodes.Get(sinkIdx));
        udpServer = DynamicCast<UdpServer>(sinkApp.Get(0));
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(simTime));

        UdpClientHelper client(sinkAddr, udpPort);
        client.SetAttribute("MaxPackets", UintegerValue(numPackets));
        client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        client.SetAttribute("PacketSize", UintegerValue(16));
        ApplicationContainer clientApp = client.Install(wsnNodes.Get(sourceIdx));
        clientApp.Start(Seconds(2.0));
        clientApp.Stop(Seconds(simTime - 1.0));
    }
    else // icmp
    {
        PingHelper ping(sinkAddr);
        ping.SetAttribute("Count", UintegerValue(numPackets));
        ping.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        ping.SetAttribute("Size", UintegerValue(16));
        ApplicationContainer clientApp = ping.Install(wsnNodes.Get(sourceIdx));
        clientApp.Get(0)->TraceConnectWithoutContext("Report", MakeCallback(&CapturePingReport));
        clientApp.Start(Seconds(2.0));
        clientApp.Stop(Seconds(simTime - 1.0));
    }

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    if (traffic == "udp")
    {
        g_appSent = numPackets;
        g_appReceived = udpServer->GetReceived();
    }
    Simulator::Destroy();

    // ---- Report ----------------------------------------------------------
    const std::string policy = adaptiveFlooding ? "adaptive" : "simple";
    std::cout << "\n=== 6LoWPAN mesh-under benchmark ===\n"
              << "  policy         : " << policy << "\n"
              << "  topology         : " << topology << "\n"
              << "  traffic          : " << traffic << "\n"
              << "  nodes            : " << nNodes << "\n"
              << "  source -> sink   : " << sourceIdx << " -> " << sinkIdx << "\n"
              << "  sent / received  : " << g_appSent << " / " << g_appReceived << "\n"
              << "  PHY transmissions: " << g_phyTx << "\n"
              << "  PHY Rx drops     : " << g_phyRxDrop << "\n";
    // Machine-readable line for a sweep/plot script (column order documented here).
    std::cout << "CSV_HEADER,topology,traffic,nodes,policy,sent,recv,phyTx,phyRxDrop\n";
    std::cout << "CSV," << topology << "," << traffic << "," << nNodes << "," << policy << ","
              << g_appSent << "," << g_appReceived << "," << g_phyTx << "," << g_phyRxDrop << "\n";

    return 0;
}
