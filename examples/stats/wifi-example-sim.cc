/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 *
 * This program conducts a simple experiment: It places two nodes at a
 * parameterized distance apart.  One node generates packets and the
 * other node receives.  The stat framework collects data on packet
 * loss.  Outside of this program, a control script uses that data to
 * produce graphs presenting performance at the varying distances.
 * This isn't a typical simulation but is a common "experiment"
 * performed in real life and serves as an accessible exemplar for the
 * stat framework.  It also gives some intuition on the behavior and
 * basic reasonability of the NS-3 WiFi models.
 *
 * Applications used by this program are in test02-apps.h and
 * test02-apps.cc, which should be in the same place as this file.
 *
 */

#include "wifi-example-apps.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/stats-module.h"
#include "ns3/wifi-module.h"

#include <ctime>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WiFiDistanceExperiment");

/**
 * Function called when a packet is transmitted.
 *
 * @param datac The counter of the number of transmitted packets.
 * @param path The callback context.
 * @param packet The transmitted packet.
 */
void
TxCallback(Ptr<CounterCalculator<uint32_t>> datac, std::string path, Ptr<const Packet> packet)
{
    NS_LOG_INFO("Sent frame counted in " << datac->GetKey());
    datac->Update();
}

int
main(int argc, char* argv[])
{
    double distance = 50.0;
    std::string format("omnet");

    std::string experiment("wifi-distance-test");
    std::string strategy("wifi-default");
    std::string input;
    std::string runID;

    {
        std::stringstream sstr;
        sstr << "run-" << time(nullptr);
        runID = sstr.str();
    }

    // Set up command line parameters used to control the experiment.
    CommandLine cmd(__FILE__);
    cmd.AddValue("distance", "Distance apart to place nodes (in meters).", distance);
    cmd.AddValue("format", "Format to use for data output.", format);
    cmd.AddValue("experiment", "Identifier for experiment.", experiment);
    cmd.AddValue("strategy", "Identifier for strategy.", strategy);
    cmd.AddValue("run", "Identifier for run.", runID);
    cmd.Parse(argc, argv);

    if (format != "omnet" && format != "db")
    {
        NS_LOG_ERROR("Unknown output format '" << format << "'");
        return -1;
    }

#ifndef HAVE_SQLITE3
    if (format == "db")
    {
        NS_LOG_ERROR("sqlite support not compiled in.");
        return -1;
    }
#endif

    {
        std::stringstream sstr("");
        sstr << distance;
        input = sstr.str();
    }

    //--------------------------------------------
    //-- Create nodes and network stacks
    //--------------------------------------------
    NS_LOG_INFO("Creating nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Installing WiFi and Internet stack.");
    WifiHelper wifi;
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    NetDeviceContainer nodeDevices = wifi.Install(wifiPhy, wifiMac, nodes);

    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipAddrs;
    ipAddrs.SetBase("192.168.0.0", "255.255.255.0");
    ipAddrs.Assign(nodeDevices);

    //--------------------------------------------
    //-- Setup physical layout
    //--------------------------------------------
    NS_LOG_INFO("Installing static mobility; distance " << distance << " .");
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, distance, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nodes);

    //--------------------------------------------
    //-- Create a custom traffic source and sink
    //--------------------------------------------
    NS_LOG_INFO("Create traffic source & sink.");
    Ptr<Node> appSource = NodeList::GetNode(0);
    Ptr<Sender> sender = CreateObject<Sender>();
    appSource->AddApplication(sender);
    sender->SetStartTime(Seconds(1));

    Ptr<Node> appSink = NodeList::GetNode(1);
    Ptr<Receiver> receiver = CreateObject<Receiver>();
    appSink->AddApplication(receiver);
    receiver->SetStartTime(Seconds(0));

    Config::Set("/NodeList/*/ApplicationList/*/$Sender/Destination",
                Ipv4AddressValue("192.168.0.2"));

    //--------------------------------------------
    //-- Setup stats and data collection
    //--------------------------------------------

    // Create a DataCollector object to hold information about this run.
    DataCollector data;
    data.DescribeRun(experiment, strategy, input, runID);

    // Add any information we wish to record about this run.
    data.AddMetadata("author", "tjkopena");

    // Create a counter to track how many frames are generated.  Updates
    // are triggered by the trace signal generated by the WiFi MAC model
    // object.  Here we connect the counter to the signal via the simple
    // TxCallback() glue function defined above.
    Ptr<CounterCalculator<uint32_t>> totalTx = CreateObject<CounterCalculator<uint32_t>>();
    totalTx->SetKey("wifi-tx-frames");
    totalTx->SetContext("node[0]");
    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                    MakeBoundCallback(&TxCallback, totalTx));
    data.AddDataCalculator(totalTx);

    // This is similar, but creates a counter to track how many frames
    // are received.  Instead of our own glue function, this uses a
    // method of an adapter class to connect a counter directly to the
    // trace signal generated by the WiFi MAC.
    Ptr<PacketCounterCalculator> totalRx = CreateObject<PacketCounterCalculator>();
    totalRx->SetKey("wifi-rx-frames");
    totalRx->SetContext("node[1]");
    Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                    MakeCallback(&PacketCounterCalculator::PacketUpdate, totalRx));
    data.AddDataCalculator(totalRx);

    // This counter tracks how many packets---as opposed to frames---are
    // generated.  This is connected directly to a trace signal provided
    // by our Sender class.
    Ptr<PacketCounterCalculator> appTx = CreateObject<PacketCounterCalculator>();
    appTx->SetKey("sender-tx-packets");
    appTx->SetContext("node[0]");
    Config::Connect("/NodeList/0/ApplicationList/*/$Sender/Tx",
                    MakeCallback(&PacketCounterCalculator::PacketUpdate, appTx));
    data.AddDataCalculator(appTx);

    // Here a counter for received packets is directly manipulated by
    // one of the custom objects in our simulation, the Receiver
    // Application.  The Receiver object is given a pointer to the
    // counter and calls its Update() method whenever a packet arrives.
    Ptr<CounterCalculator<>> appRx = CreateObject<CounterCalculator<>>();
    appRx->SetKey("receiver-rx-packets");
    appRx->SetContext("node[1]");
    receiver->SetCounter(appRx);
    data.AddDataCalculator(appRx);

    // Just to show this is here...

    /*
    Ptr<MinMaxAvgTotalCalculator<uint32_t>> test =
        CreateObject<MinMaxAvgTotalCalculator<uint32_t>>();
    test->SetKey("test-dc");
    data.AddDataCalculator(test);

    test->Update(4);
    test->Update(8);
    test->Update(24);
    test->Update(12);
    */

    // This DataCalculator connects directly to the transmit trace
    // provided by our Sender Application.  It records some basic
    // statistics about the sizes of the packets received (min, max,
    // avg, total # bytes), although in this scenario they're fixed.
    Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts =
        CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
    appTxPkts->SetKey("tx-pkt-size");
    appTxPkts->SetContext("node[0]");
    Config::Connect("/NodeList/0/ApplicationList/*/$Sender/Tx",
                    MakeCallback(&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate, appTxPkts));
    data.AddDataCalculator(appTxPkts);

    // Here we directly manipulate another DataCollector tracking min,
    // max, total, and average propagation delays.  Check out the Sender
    // and Receiver classes to see how packets are tagged with
    // timestamps to do this.
    Ptr<TimeMinMaxAvgTotalCalculator> delayStat = CreateObject<TimeMinMaxAvgTotalCalculator>();
    delayStat->SetKey("delay");
    delayStat->SetContext(".");
    receiver->SetDelayTracker(delayStat);
    data.AddDataCalculator(delayStat);

    //--------------------------------------------
    //-- Run the simulation
    //--------------------------------------------
    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    //--------------------------------------------
    //-- Generate statistics output.
    //--------------------------------------------

    // Pick an output writer based in the requested format.
    Ptr<DataOutputInterface> output = nullptr;
    if (format == "omnet")
    {
        NS_LOG_INFO("Creating omnet formatted data output.");
        output = CreateObject<OmnetDataOutput>();
    }
    else if (format == "db")
    {
#ifdef HAVE_SQLITE3
        NS_LOG_INFO("Creating sqlite formatted data output.");
        output = CreateObject<SqliteDataOutput>();
#endif
    }
    else
    {
        NS_LOG_ERROR("Unknown output format " << format);
    }

    // Finally, have that writer interrogate the DataCollector and save
    // the results.
    if (output)
    {
        output->Output(data);
    }

    // Free any memory here at the end of this example.
    Simulator::Destroy();

    return 0;
}
