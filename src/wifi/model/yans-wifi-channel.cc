/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#include "yans-wifi-channel.h"

#include "wifi-net-device.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-utils.h"
#include "yans-wifi-phy.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("YansWifiChannel");

NS_OBJECT_ENSURE_REGISTERED(YansWifiChannel);

TypeId
YansWifiChannel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::YansWifiChannel")
            .SetParent<Channel>()
            .SetGroupName("Wifi")
            .AddConstructor<YansWifiChannel>()
            .AddAttribute("PropagationLossModel",
                          "A pointer to the propagation loss model attached to this channel.",
                          PointerValue(),
                          MakePointerAccessor(&YansWifiChannel::m_loss),
                          MakePointerChecker<PropagationLossModel>())
            .AddAttribute("PropagationDelayModel",
                          "A pointer to the propagation delay model attached to this channel.",
                          PointerValue(),
                          MakePointerAccessor(&YansWifiChannel::m_delay),
                          MakePointerChecker<PropagationDelayModel>());
    return tid;
}

YansWifiChannel::YansWifiChannel()
{
    NS_LOG_FUNCTION(this);
}

YansWifiChannel::~YansWifiChannel()
{
    NS_LOG_FUNCTION(this);
    m_phyList.clear();
}

void
YansWifiChannel::SetPropagationLossModel(const Ptr<PropagationLossModel> loss)
{
    NS_LOG_FUNCTION(this << loss);
    m_loss = loss;
}

void
YansWifiChannel::SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay)
{
    NS_LOG_FUNCTION(this << delay);
    m_delay = delay;
}

void
YansWifiChannel::Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const
{
    NS_LOG_FUNCTION(this << sender << ppdu << txPower);
    Ptr<MobilityModel> senderMobility = sender->GetMobility();
    NS_ASSERT(senderMobility);
    for (auto i = m_phyList.begin(); i != m_phyList.end(); i++)
    {
        if (sender != (*i))
        {
            // For now don't account for inter channel interference nor channel bonding
            if ((*i)->GetChannelNumber() != sender->GetChannelNumber())
            {
                continue;
            }

            auto receiverMobility = (*i)->GetMobility()->GetObject<MobilityModel>();
            const auto delay = m_delay->GetDelay(senderMobility, receiverMobility);
            const dBm_u rxPower{m_loss->CalcRxPower(txPower, senderMobility, receiverMobility)};
            NS_LOG_DEBUG("propagation: txPower="
                         << txPower << "dBm, rxPower=" << rxPower << "dBm, "
                         << "distance=" << senderMobility->GetDistanceFrom(receiverMobility)
                         << "m, delay=" << delay);
            auto dstNetDevice = (*i)->GetDevice();
            uint32_t dstNode;
            if (!dstNetDevice)
            {
                dstNode = 0xffffffff;
            }
            else
            {
                dstNode = dstNetDevice->GetNode()->GetId();
            }

            Simulator::ScheduleWithContext(dstNode,
                                           delay,
                                           &YansWifiChannel::Receive,
                                           (*i),
                                           ppdu,
                                           rxPower);
        }
    }
}

void
YansWifiChannel::Receive(Ptr<YansWifiPhy> phy, Ptr<const WifiPpdu> ppdu, dBm_u rxPower)
{
    NS_LOG_FUNCTION(phy << ppdu << rxPower);
    const auto totalRxPower = rxPower + phy->GetRxGain();
    phy->TraceSignalArrival(ppdu, totalRxPower, ppdu->GetTxDuration());
    // Do no further processing if signal is too weak
    // Current implementation assumes constant RX power over the PPDU duration
    // Compare received TX power per MHz to normalized RX sensitivity
    const auto txWidth = ppdu->GetTxChannelWidth();
    if (totalRxPower < phy->GetRxSensitivity() + RatioToDb(txWidth / MHz_u{20}))
    {
        NS_LOG_INFO("Received signal too weak to process: " << rxPower << " dBm");
        return;
    }
    RxPowerWattPerChannelBand rxPowerW;

    rxPowerW.insert(
        {{{{0, 0}}, {{Hz_u{0}, Hz_u{0}}}}, (DbmToW(totalRxPower))}); // dummy band for YANS
    phy->StartReceivePreamble(ppdu, rxPowerW, ppdu->GetTxDuration());
}

std::size_t
YansWifiChannel::GetNDevices() const
{
    return m_phyList.size();
}

Ptr<NetDevice>
YansWifiChannel::GetDevice(std::size_t i) const
{
    return m_phyList[i]->GetDevice();
}

void
YansWifiChannel::Add(Ptr<YansWifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    m_phyList.push_back(phy);
}

int64_t
YansWifiChannel::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    int64_t currentStream = stream;
    currentStream += m_loss->AssignStreams(stream);
    return (currentStream - stream);
}

} // namespace ns3
