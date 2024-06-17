/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */
#include "adhoc-aloha-noack-ideal-phy-helper.h"

#include "ns3/aloha-noack-net-device.h"
#include "ns3/antenna-model.h"
#include "ns3/config.h"
#include "ns3/half-duplex-ideal-phy.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-propagation-loss-model.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdhocAlohaNoackIdealPhyHelper");

AdhocAlohaNoackIdealPhyHelper::AdhocAlohaNoackIdealPhyHelper()
{
    m_phy.SetTypeId("ns3::HalfDuplexIdealPhy");
    m_device.SetTypeId("ns3::AlohaNoackNetDevice");
    m_queue.SetTypeId("ns3::DropTailQueue<Packet>");
    m_antenna.SetTypeId("ns3::IsotropicAntennaModel");
}

AdhocAlohaNoackIdealPhyHelper::~AdhocAlohaNoackIdealPhyHelper()
{
}

void
AdhocAlohaNoackIdealPhyHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
AdhocAlohaNoackIdealPhyHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
AdhocAlohaNoackIdealPhyHelper::SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd)
{
    NS_LOG_FUNCTION(this << txPsd);
    m_txPsd = txPsd;
}

void
AdhocAlohaNoackIdealPhyHelper::SetNoisePowerSpectralDensity(Ptr<SpectrumValue> noisePsd)
{
    NS_LOG_FUNCTION(this << noisePsd);
    m_noisePsd = noisePsd;
}

void
AdhocAlohaNoackIdealPhyHelper::SetPhyAttribute(std::string name, const AttributeValue& v)
{
    m_phy.Set(name, v);
}

void
AdhocAlohaNoackIdealPhyHelper::SetDeviceAttribute(std::string name, const AttributeValue& v)
{
    m_device.Set(name, v);
}

NetDeviceContainer
AdhocAlohaNoackIdealPhyHelper::Install(NodeContainer c) const
{
    NetDeviceContainer devices;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;

        Ptr<AlohaNoackNetDevice> dev = (m_device.Create())->GetObject<AlohaNoackNetDevice>();
        dev->SetAddress(Mac48Address::Allocate());
        Ptr<Queue<Packet>> q = (m_queue.Create())->GetObject<Queue<Packet>>();
        dev->SetQueue(q);

        // note that we could have used a SpectrumPhyHelper here, but
        // given that it is straightforward to handle the configuration
        // in this helper here, we avoid asking the user to pass us a
        // SpectrumPhyHelper, so to spare him some typing.

        Ptr<HalfDuplexIdealPhy> phy = (m_phy.Create())->GetObject<HalfDuplexIdealPhy>();
        NS_ASSERT(phy);

        dev->SetPhy(phy);

        NS_ASSERT(node);
        phy->SetMobility(node->GetObject<MobilityModel>());

        NS_ASSERT(dev);
        phy->SetDevice(dev);

        NS_ASSERT_MSG(
            m_txPsd,
            "you forgot to call AdhocAlohaNoackIdealPhyHelper::SetTxPowerSpectralDensity ()");
        phy->SetTxPowerSpectralDensity(m_txPsd);

        NS_ASSERT_MSG(
            m_noisePsd,
            "you forgot to call AdhocAlohaNoackIdealPhyHelper::SetNoisePowerSpectralDensity ()");
        phy->SetNoisePowerSpectralDensity(m_noisePsd);

        NS_ASSERT_MSG(m_channel, "you forgot to call AdhocAlohaNoackIdealPhyHelper::SetChannel ()");
        phy->SetChannel(m_channel);
        dev->SetChannel(m_channel);
        m_channel->AddRx(phy);

        phy->SetGenericPhyTxEndCallback(
            MakeCallback(&AlohaNoackNetDevice::NotifyTransmissionEnd, dev));
        phy->SetGenericPhyRxStartCallback(
            MakeCallback(&AlohaNoackNetDevice::NotifyReceptionStart, dev));
        phy->SetGenericPhyRxEndOkCallback(
            MakeCallback(&AlohaNoackNetDevice::NotifyReceptionEndOk, dev));
        dev->SetGenericPhyTxStartCallback(MakeCallback(&HalfDuplexIdealPhy::StartTx, phy));

        Ptr<AntennaModel> antenna = (m_antenna.Create())->GetObject<AntennaModel>();
        NS_ASSERT_MSG(antenna, "error in creating the AntennaModel object");
        phy->SetAntenna(antenna);

        node->AddDevice(dev);
        devices.Add(dev);
    }
    return devices;
}

NetDeviceContainer
AdhocAlohaNoackIdealPhyHelper::Install(Ptr<Node> node) const
{
    return Install(NodeContainer(node));
}

NetDeviceContainer
AdhocAlohaNoackIdealPhyHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

} // namespace ns3
