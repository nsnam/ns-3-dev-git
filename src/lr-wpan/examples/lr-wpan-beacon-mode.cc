/*
 * Copyright (c) 2019 Ritsumeikan University, Shiga, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */

/*
 *   Coordinator              End Device
 *       N0   <----------------  N1
 *      (dev0)                 (dev1)
 *
 * This example demonstrate the usage of the MAC primitives involved in
 * direct transmissions for the beacon enabled mode of IEEE 802.15.4-2011.
 * A single packet is sent from an end device to the coordinator during the CAP
 * of the first incoming superframe.
 *
 * This example do not demonstrate a full protocol stack usage.
 * For full protocol stack usage refer to 6lowpan examples.
 *
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

void
BeaconIndication(MlmeBeaconNotifyIndicationParams params)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " secs | Received BEACON packet of size ");
}

void
DataIndication(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << " secs | Received DATA packet of size " << p->GetSize());
}

void
TransEndIndication(McpsDataConfirmParams params)
{
    // In the case of transmissions with the Ack flag activated, the transaction is only
    // successful if the Ack was received.
    if (params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << " secs | Transmission successfully sent");
    }
}

void
DataIndicationCoordinator(McpsDataIndicationParams params, Ptr<Packet> p)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                  << "s Coordinator Received DATA packet (size " << p->GetSize() << " bytes)");
}

void
StartConfirm(MlmeStartConfirmParams params)
{
    if (params.m_status == MacStatus::SUCCESS)
    {
        NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "Beacon status SUCCESSFUL");
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnable("LrWpanMac", LOG_LEVEL_INFO);
    LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_INFO);

    LrWpanHelper lrWpanHelper;

    // Create 2 nodes, and a NetDevice for each one

    NodeContainer nodes;
    nodes.Create(2);

    // Use LrWpanHelper to create devices and assign them to nodes
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    NetDeviceContainer devices = lrWpanHelper.Install(nodes);
    Ptr<LrWpanNetDevice> dev0 = devices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = devices.Get(1)->GetObject<LrWpanNetDevice>();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));

    ///////////////// Mobility   ///////////////////////
    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);
    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();

    sender1Mobility->SetPosition(Vector(0, 10, 0)); // 10 m distance
    dev1->GetPhy()->SetMobility(sender1Mobility);

    /////// MAC layer Callbacks hooks/////////////

    MlmeStartConfirmCallback cb0;
    cb0 = MakeCallback(&StartConfirm);
    dev0->GetMac()->SetMlmeStartConfirmCallback(cb0);

    McpsDataConfirmCallback cb1;
    cb1 = MakeCallback(&TransEndIndication);
    dev1->GetMac()->SetMcpsDataConfirmCallback(cb1);

    MlmeBeaconNotifyIndicationCallback cb3;
    cb3 = MakeCallback(&BeaconIndication);
    dev1->GetMac()->SetMlmeBeaconNotifyIndicationCallback(cb3);

    McpsDataIndicationCallback cb4;
    cb4 = MakeCallback(&DataIndication);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb4);

    McpsDataIndicationCallback cb5;
    cb5 = MakeCallback(&DataIndicationCoordinator);
    dev0->GetMac()->SetMcpsDataIndicationCallback(cb5);

    //////////// Manual device association ////////////////////
    // Note: We manually associate the devices to a PAN coordinator
    //       (i.e. bootstrap is not used);
    //       The PAN COORDINATOR does not need to associate or set its
    //       PAN Id or its own coordinator id, these are set
    //       by the MLME-start.request primitive when used.

    dev1->GetMac()->SetPanId(5);
    dev1->GetMac()->SetAssociatedCoor(Mac16Address("00:01"));

    ///////////////////// Start transmitting beacons from coordinator ////////////////////////

    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 14;
    params.m_sfrmOrd = 6;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev0->GetMac(),
                                   params);

    ///////////////////// Transmission of data Packets from end device //////////////////////

    Ptr<Packet> p1 = Create<Packet>(5);
    McpsDataRequestParams params2;
    params2.m_dstPanId = 5;
    params2.m_srcAddrMode = SHORT_ADDR;
    params2.m_dstAddrMode = SHORT_ADDR;
    params2.m_dstAddr = Mac16Address("00:01");
    params2.m_msduHandle = 0;
    // params2.m_txOptions = TX_OPTION_ACK;  // Enable direct transmission with Ack

    /////////////////////////////////////////////////////////////////////////////////////
    // Examples of time parameters for transmissions in the first incoming superframe. //
    /////////////////////////////////////////////////////////////////////////////////////

    // 2.981 sec      No time to finish CCA in CAP, the transmission at this time will cause
    //                the packet to be deferred to the next superframe.

    // 2.982 sec      No time to finish random backoff delay in CAP, the  transmission at this
    //                time will cause the packet to be deferred to the next superframe.

    // 2.93 sec       Enough time, the packet can be transmitted within the CAP of the first
    // superframe

    // MCPS-DATA.request Beacon enabled Direct Transmission (dev1)
    // Frame transmission from End Device to Coordinator (Direct transmission)
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.93),
                                   &LrWpanMac::McpsDataRequest,
                                   dev1->GetMac(),
                                   params2,
                                   p1);

    Simulator::Stop(Seconds(600));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
