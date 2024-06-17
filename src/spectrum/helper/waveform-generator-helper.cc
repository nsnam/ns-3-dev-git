/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */
#include "waveform-generator-helper.h"

#include "ns3/antenna-model.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/waveform-generator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WaveformGeneratorHelper");

WaveformGeneratorHelper::WaveformGeneratorHelper()
{
    m_phy.SetTypeId("ns3::WaveformGenerator");
    m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
    m_antenna.SetTypeId("ns3::IsotropicAntennaModel");
}

WaveformGeneratorHelper::~WaveformGeneratorHelper()
{
}

void
WaveformGeneratorHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
WaveformGeneratorHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
WaveformGeneratorHelper::SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd)
{
    NS_LOG_FUNCTION(this << txPsd);
    m_txPsd = txPsd;
}

void
WaveformGeneratorHelper::SetPhyAttribute(std::string name, const AttributeValue& v)
{
    m_phy.Set(name, v);
}

void
WaveformGeneratorHelper::SetDeviceAttribute(std::string name, const AttributeValue& v)
{
    m_device.Set(name, v);
}

NetDeviceContainer
WaveformGeneratorHelper::Install(NodeContainer c) const
{
    NetDeviceContainer devices;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;

        Ptr<NonCommunicatingNetDevice> dev =
            m_device.Create()->GetObject<NonCommunicatingNetDevice>();

        Ptr<WaveformGenerator> phy = m_phy.Create()->GetObject<WaveformGenerator>();
        NS_ASSERT(phy);

        dev->SetPhy(phy);

        NS_ASSERT(node);
        phy->SetMobility(node->GetObject<MobilityModel>());

        NS_ASSERT(dev);
        phy->SetDevice(dev);

        NS_ASSERT_MSG(m_txPsd,
                      "you forgot to call WaveformGeneratorHelper::SetTxPowerSpectralDensity ()");
        phy->SetTxPowerSpectralDensity(m_txPsd);

        NS_ASSERT_MSG(m_channel, "you forgot to call WaveformGeneratorHelper::SetChannel ()");
        phy->SetChannel(m_channel);
        dev->SetChannel(m_channel);

        Ptr<AntennaModel> antenna = (m_antenna.Create())->GetObject<AntennaModel>();
        NS_ASSERT_MSG(antenna, "error in creating the AntennaModel object");
        phy->SetAntenna(antenna);

        node->AddDevice(dev);
        devices.Add(dev);
    }
    return devices;
}

NetDeviceContainer
WaveformGeneratorHelper::Install(Ptr<Node> node) const
{
    return Install(NodeContainer(node));
}

NetDeviceContainer
WaveformGeneratorHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

} // namespace ns3
