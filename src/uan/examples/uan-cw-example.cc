/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

/**
 * @file uan-cw-example.cc
 * @ingroup uan
 *
 * This example showcases the "CW-MAC" described in System Design Considerations
 * for Undersea Networks article in the IEEE Journal on Selected Areas of
 * Communications 2008 by Nathan Parrish, Leonard Tracy and Sumit Roy.
 * The MAC protocol is implemented in the class UanMacCw.  CW-MAC is similar
 * in nature to the IEEE 802.11 DCF with a constant backoff window.
 * It requires two parameters to be set, the slot time and
 * the contention window size.  The contention window size is
 * the backoff window size in slots, and the slot time is
 * the duration of each slot.  These parameters should be set
 * according to the overall network size, internode spacing and
 * the number of nodes in the network.
 *
 * This example deploys nodes randomly (according to RNG seed of course)
 * in a finite square region with the X and Y coordinates of the nodes
 * distributed uniformly.  The CW parameter is varied throughout
 * the simulation in order to show the variation in throughput
 * with respect to changes in CW.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/stats-module.h"
#include "ns3/uan-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UanCwExample");

/** Unnamed namespace, to disambiguate class Experiment. */
namespace
{

/**
 * @ingroup uan
 * @brief Helper class for UAN CW MAC example.
 *
 * An experiment measures the average throughput for a series of CW values.
 *
 * @see uan-cw-example.cc
 */
class Experiment
{
  public:
    /**
     * Run an experiment across a range of congestion window values.
     *
     * @param uan The Uan stack helper to configure nodes in the model.
     * @return The data set of CW values and measured throughput
     */
    Gnuplot2dDataset Run(UanHelper& uan);
    /**
     * Receive all available packets from a socket.
     *
     * @param socket The receive socket.
     */
    void ReceivePacket(Ptr<Socket> socket);
    /**
     * Assign new random positions to a set of nodes.  New positions
     * are randomly assigned within the bounding box.
     *
     * @param nodes The nodes to reposition.
     */
    void UpdatePositions(NodeContainer& nodes) const;
    /** Save the throughput from a single run. */
    void ResetData();
    /**
     * Compute average throughput for a set of runs, then increment CW.
     *
     * @param cw CW value for completed runs.
     */
    void IncrementCw(uint32_t cw);

    uint32_t m_numNodes;   //!< Number of transmitting nodes.
    uint32_t m_dataRate;   //!< DataRate in bps.
    double m_depth;        //!< Depth of transmitting and sink nodes.
    double m_boundary;     //!< Size of boundary in meters.
    uint32_t m_packetSize; //!< Generated packet size in bytes.
    uint32_t m_bytesTotal; //!< Total bytes received.
    uint32_t m_cwMin;      //!< Min CW to simulate.
    uint32_t m_cwMax;      //!< Max CW to simulate.
    uint32_t m_cwStep;     //!< CW step size, default 10.
    uint32_t m_avgs;       //!< Number of topologies to test for each cw point.

    Time m_slotTime; //!< Slot time duration.
    Time m_simTime;  //!< Simulation run time, default 1000 s.

    std::string m_gnudatfile;     //!< Name for GNU Plot output, default uan-cw-example.gpl.
    std::string m_asciitracefile; //!< Name for ascii trace file, default uan-cw-example.asc.
    std::string m_bhCfgFile;      //!< (Unused)

    Gnuplot2dDataset m_data;           //!< Container for the simulation data.
    std::vector<double> m_throughputs; //!< Throughput for each run.

    /** Default constructor. */
    Experiment();
};

Experiment::Experiment()
    : m_numNodes(15),
      m_dataRate(80),
      m_depth(70),
      m_boundary(500),
      m_packetSize(32),
      m_bytesTotal(0),
      m_cwMin(10),
      m_cwMax(400),
      m_cwStep(10),
      m_avgs(3),
      m_slotTime(Seconds(0.2)),
      m_simTime(Seconds(1000)),
      m_gnudatfile("uan-cw-example.gpl"),
      m_asciitracefile("uan-cw-example.asc"),
      m_bhCfgFile("uan-apps/dat/default.cfg")
{
}

void
Experiment::ResetData()
{
    NS_LOG_DEBUG(Now().As(Time::S) << "  Resetting data");
    m_throughputs.push_back(m_bytesTotal * 8.0 / m_simTime.GetSeconds());
    m_bytesTotal = 0;
}

void
Experiment::IncrementCw(uint32_t cw)
{
    NS_ASSERT(m_throughputs.size() == m_avgs);

    double avgThroughput = 0.0;
    for (uint32_t i = 0; i < m_avgs; i++)
    {
        avgThroughput += m_throughputs[i];
    }
    avgThroughput /= m_avgs;
    m_data.Add(cw, avgThroughput);
    m_throughputs.clear();

    Config::Set("/NodeList/*/DeviceList/*/Mac/CW", UintegerValue(cw + m_cwStep));

    SeedManager::SetRun(SeedManager::GetRun() + 1);

    NS_LOG_DEBUG("Average for cw=" << cw << " over " << m_avgs << " runs: " << avgThroughput);
}

void
Experiment::UpdatePositions(NodeContainer& nodes) const
{
    NS_LOG_DEBUG(Now().As(Time::S) << " Updating positions");
    auto it = nodes.Begin();
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    for (; it != nodes.End(); it++)
    {
        Ptr<MobilityModel> mp = (*it)->GetObject<MobilityModel>();
        mp->SetPosition(Vector(uv->GetValue(0, m_boundary), uv->GetValue(0, m_boundary), 70.0));
    }
}

void
Experiment::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv()))
    {
        m_bytesTotal += packet->GetSize();
    }
    packet = nullptr;
}

Gnuplot2dDataset
Experiment::Run(UanHelper& uan)
{
    uan.SetMac("ns3::UanMacCw", "CW", UintegerValue(m_cwMin), "SlotTime", TimeValue(m_slotTime));
    NodeContainer nc = NodeContainer();
    NodeContainer sink = NodeContainer();
    nc.Create(m_numNodes);
    sink.Create(1);

    PacketSocketHelper socketHelper;
    socketHelper.Install(nc);
    socketHelper.Install(sink);

#ifdef UAN_PROP_BH_INSTALLED
    Ptr<UanPropModelBh> prop =
        CreateObjectWithAttributes<UanPropModelBh>("ConfigFile", StringValue("exbhconfig.cfg"));
#else
    Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal>();
#endif // UAN_PROP_BH_INSTALLED
    Ptr<UanChannel> channel =
        CreateObjectWithAttributes<UanChannel>("PropagationModel", PointerValue(prop));

    // Create net device and nodes with UanHelper
    NetDeviceContainer devices = uan.Install(nc, channel);
    NetDeviceContainer sinkdev = uan.Install(sink, channel);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();

    {
        Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable>();
        pos->Add(Vector(m_boundary / 2.0, m_boundary / 2.0, m_depth));
        double rsum = 0;

        double minr = 2 * m_boundary;
        for (uint32_t i = 0; i < m_numNodes; i++)
        {
            double x = urv->GetValue(0, m_boundary);
            double y = urv->GetValue(0, m_boundary);
            double newr = std::sqrt((x - m_boundary / 2.0) * (x - m_boundary / 2.0) +
                                    (y - m_boundary / 2.0) * (y - m_boundary / 2.0));
            rsum += newr;
            minr = std::min(minr, newr);
            pos->Add(Vector(x, y, m_depth));
        }
        NS_LOG_DEBUG("Mean range from gateway: " << rsum / m_numNodes << "    min. range " << minr);

        mobility.SetPositionAllocator(pos);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(sink);

        NS_LOG_DEBUG(
            "Position of sink: " << sink.Get(0)->GetObject<MobilityModel>()->GetPosition());
        mobility.Install(nc);

        PacketSocketAddress socket;
        socket.SetSingleDevice(sinkdev.Get(0)->GetIfIndex());
        socket.SetPhysicalAddress(sinkdev.Get(0)->GetAddress());
        socket.SetProtocol(0);

        OnOffHelper app("ns3::PacketSocketFactory", Address(socket));
        app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        app.SetAttribute("DataRate", DataRateValue(m_dataRate));
        app.SetAttribute("PacketSize", UintegerValue(m_packetSize));

        ApplicationContainer apps = app.Install(nc);
        apps.Start(Seconds(0.5));
        Time nextEvent = Seconds(0.5);

        for (uint32_t cw = m_cwMin; cw <= m_cwMax; cw += m_cwStep)
        {
            for (uint32_t an = 0; an < m_avgs; an++)
            {
                nextEvent += m_simTime;
                Simulator::Schedule(nextEvent, &Experiment::ResetData, this);
                Simulator::Schedule(nextEvent, &Experiment::UpdatePositions, this, nc);
            }
            Simulator::Schedule(nextEvent, &Experiment::IncrementCw, this, cw);
        }
        apps.Stop(nextEvent + m_simTime);

        Ptr<Node> sinkNode = sink.Get(0);
        TypeId psfid = TypeId::LookupByName("ns3::PacketSocketFactory");
        if (!sinkNode->GetObject<SocketFactory>(psfid))
        {
            Ptr<PacketSocketFactory> psf = CreateObject<PacketSocketFactory>();
            sinkNode->AggregateObject(psf);
        }
        Ptr<Socket> sinkSocket = Socket::CreateSocket(sinkNode, psfid);
        sinkSocket->Bind(socket);
        sinkSocket->SetRecvCallback(MakeCallback(&Experiment::ReceivePacket, this));

        m_bytesTotal = 0;

        std::ofstream ascii(m_asciitracefile);
        if (!ascii.is_open())
        {
            NS_FATAL_ERROR("Could not open ascii trace file: " << m_asciitracefile);
        }
        UanHelper::EnableAsciiAll(ascii);

        Simulator::Run();
        sinkNode = nullptr;
        sinkSocket = nullptr;
        pos = nullptr;
        channel = nullptr;
        prop = nullptr;
        for (uint32_t i = 0; i < nc.GetN(); i++)
        {
            nc.Get(i) = nullptr;
        }
        for (uint32_t i = 0; i < sink.GetN(); i++)
        {
            sink.Get(i) = nullptr;
        }

        for (uint32_t i = 0; i < devices.GetN(); i++)
        {
            devices.Get(i) = nullptr;
        }
        for (uint32_t i = 0; i < sinkdev.GetN(); i++)
        {
            sinkdev.Get(i) = nullptr;
        }

        Simulator::Destroy();
        return m_data;
    }
}

} // unnamed namespace

int
main(int argc, char** argv)
{
    Experiment exp;
    bool quiet = false;

    std::string gnudatfile("cwexpgnuout.dat");
    std::string perModel = "ns3::UanPhyPerGenDefault";
    std::string sinrModel = "ns3::UanPhyCalcSinrDefault";

    CommandLine cmd(__FILE__);
    cmd.AddValue("NumNodes", "Number of transmitting nodes", exp.m_numNodes);
    cmd.AddValue("Depth", "Depth of transmitting and sink nodes", exp.m_depth);
    cmd.AddValue("RegionSize", "Size of boundary in meters", exp.m_boundary);
    cmd.AddValue("PacketSize", "Generated packet size in bytes", exp.m_packetSize);
    cmd.AddValue("DataRate", "DataRate in bps", exp.m_dataRate);
    cmd.AddValue("CwMin", "Min CW to simulate", exp.m_cwMin);
    cmd.AddValue("CwMax", "Max CW to simulate", exp.m_cwMax);
    cmd.AddValue("SlotTime", "Slot time duration", exp.m_slotTime);
    cmd.AddValue("Averages", "Number of topologies to test for each cw point", exp.m_avgs);
    cmd.AddValue("GnuFile", "Name for GNU Plot output", exp.m_gnudatfile);
    cmd.AddValue("PerModel", "PER model name", perModel);
    cmd.AddValue("SinrModel", "SINR model name", sinrModel);
    cmd.AddValue("Quiet", "Run in quiet mode (disable logging)", quiet);
    cmd.Parse(argc, argv);

    if (!quiet)
    {
        LogComponentEnable("UanCwExample", LOG_LEVEL_ALL);
    }

    ObjectFactory obf;
    obf.SetTypeId(perModel);
    Ptr<UanPhyPer> per = obf.Create<UanPhyPer>();
    obf.SetTypeId(sinrModel);
    Ptr<UanPhyCalcSinr> sinr = obf.Create<UanPhyCalcSinr>();

    UanHelper uan;
    UanTxMode mode;
    mode = UanTxModeFactory::CreateMode(UanTxMode::FSK,
                                        exp.m_dataRate,
                                        exp.m_dataRate,
                                        12000,
                                        exp.m_dataRate,
                                        2,
                                        "Default mode");
    UanModesList myModes;
    myModes.AppendMode(mode);

    uan.SetPhy("ns3::UanPhyGen",
               "PerModel",
               PointerValue(per),
               "SinrModel",
               PointerValue(sinr),
               "SupportedModes",
               UanModesListValue(myModes));

    Gnuplot gp;
    Gnuplot2dDataset ds;
    ds = exp.Run(uan);

    gp.AddDataset(ds);

    std::ofstream of(exp.m_gnudatfile);
    if (!of.is_open())
    {
        NS_FATAL_ERROR("Can not open GNU Plot outfile: " << exp.m_gnudatfile);
    }
    gp.GenerateOutput(of);

    per = nullptr;
    sinr = nullptr;

    return 0;
}
