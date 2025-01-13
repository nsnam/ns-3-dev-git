/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */

/*
 * Try to send data end-to-end through a LrWpanMac <-> LrWpanPhy <->
 * SpectrumChannel <-> LrWpanPhy <-> LrWpanMac chain
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;

/**
 * Function called when a Data indication is invoked
 * @param params MCPS data indication parameters
 * @param p packet
 */
static void
DataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND("Received packet of size " << p->GetSize());
}

/**
 * Function called when a Data confirm is invoked
 * @param params MCPS data confirm parameters
 */
static void
DataConfirm(McpsDataConfirmParams params)
{
    NS_LOG_UNCOND("LrWpanMcpsDataConfirmStatus = " << params.m_status);
}

/**
 * Function called when a the PHY state changes
 * @param context context
 * @param now time at which the function is called
 * @param oldState old PHY state
 * @param newState new PHY state
 */
static void
StateChangeNotification(std::string context,
                        Time now,
                        PhyEnumeration oldState,
                        PhyEnumeration newState)
{
    NS_LOG_UNCOND(context << " state change at " << now.As(Time::S) << " from "
                          << LrWpanHelper::LrWpanPhyEnumerationPrinter(oldState) << " to "
                          << LrWpanHelper::LrWpanPhyEnumerationPrinter(newState));
}

int
main(int argc, char* argv[])
{
    bool verbose = false;
    bool extended = false;

    CommandLine cmd(__FILE__);

    cmd.AddValue("verbose", "turn on all log components", verbose);
    cmd.AddValue("extended", "use extended addressing", extended);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));
        LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL);
    }

    // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices
    // or wireshark. GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    // Create 2 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();

    // Each device must be attached to the same channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);

    // Note: This setup, which has been done manually here, can be simplified using the LrWpanHelper
    // class. The LrWpanHelper can be used to set up the propagation loss and delay models in many
    // devices in a simpler way. The following is an equivalent, simplified setup:
    //
    //    LrWpanHelper lrWpanHelper;
    //    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    //    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    //    NodeContainer nodes;
    //    nodes.Create(2);
    //    NetDeviceContainer devices = lrWpanHelper.Install(nodes);
    //    Ptr<LrWpanNetDevice> dev0 = devices.Get(0)->GetObject<LrWpanNetDevice>();
    //    Ptr<LrWpanNetDevice> dev1 = devices.Get(1)->GetObject<LrWpanNetDevice>();

    // Set 16-bit short addresses if extended is false, otherwise use 64-bit extended addresses
    if (!extended)
    {
        dev0->SetAddress(Mac16Address("00:01"));
        dev1->SetAddress(Mac16Address("00:02"));
    }
    else
    {
        Ptr<LrWpanMac> mac0 = dev0->GetMac();
        Ptr<LrWpanMac> mac1 = dev1->GetMac();
        mac0->SetExtendedAddress(Mac64Address("00:00:00:00:00:00:00:01"));
        mac1->SetExtendedAddress(Mac64Address("00:00:00:00:00:00:00:02"));
    }

    // Trace state changes in the phy
    dev0->GetPhy()->TraceConnect("TrxState",
                                 std::string("phy0"),
                                 MakeCallback(&StateChangeNotification));
    dev1->GetPhy()->TraceConnect("TrxState",
                                 std::string("phy1"),
                                 MakeCallback(&StateChangeNotification));

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);
    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    // Configure position 10 m distance
    sender1Mobility->SetPosition(Vector(0, 10, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);

    McpsDataConfirmCallback cb0;
    cb0 = MakeCallback(&DataConfirm);
    dev0->GetMac()->SetMcpsDataConfirmCallback(cb0);

    McpsDataIndicationCallback cb1;
    cb1 = MakeCallback(&DataIndication);
    dev0->GetMac()->SetMcpsDataIndicationCallback(cb1);

    McpsDataConfirmCallback cb2;
    cb2 = MakeCallback(&DataConfirm);
    dev1->GetMac()->SetMcpsDataConfirmCallback(cb2);

    McpsDataIndicationCallback cb3;
    cb3 = MakeCallback(&DataIndication);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb3);

    // The below should trigger two callbacks when end-to-end data is working
    // 1) DataConfirm callback is called
    // 2) DataIndication callback is called with value of 50
    Ptr<Packet> p0 = Create<Packet>(50); // 50 bytes of dummy data
    McpsDataRequestParams params;
    params.m_dstPanId = 0;
    if (!extended)
    {
        params.m_srcAddrMode = SHORT_ADDR;
        params.m_dstAddrMode = SHORT_ADDR;
        params.m_dstAddr = Mac16Address("00:02");
    }
    else
    {
        params.m_srcAddrMode = EXT_ADDR;
        params.m_dstAddrMode = EXT_ADDR;
        params.m_dstExtAddr = Mac64Address("00:00:00:00:00:00:00:02");
    }
    params.m_msduHandle = 0;
    params.m_txOptions = TX_OPTION_ACK;
    //  dev0->GetMac ()->McpsDataRequest (params, p0);
    Simulator::ScheduleWithContext(1,
                                   Seconds(0),
                                   &LrWpanMac::McpsDataRequest,
                                   dev0->GetMac(),
                                   params,
                                   p0);

    // Send a packet back at time 2 seconds
    Ptr<Packet> p2 = Create<Packet>(60); // 60 bytes of dummy data
    if (!extended)
    {
        params.m_dstAddr = Mac16Address("00:01");
    }
    else
    {
        params.m_dstExtAddr = Mac64Address("00:00:00:00:00:00:00:01");
    }
    Simulator::ScheduleWithContext(2,
                                   Seconds(2),
                                   &LrWpanMac::McpsDataRequest,
                                   dev1->GetMac(),
                                   params,
                                   p2);

    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
