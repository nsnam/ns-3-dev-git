/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */
#include "spectrum-helper.h"

#include "ns3/config.h"
#include "ns3/half-duplex-ideal-phy.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/names.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-phy.h"

namespace ns3
{

SpectrumChannelHelper
SpectrumChannelHelper::Default()
{
    SpectrumChannelHelper h;
    h.SetChannel("ns3::SingleModelSpectrumChannel");
    h.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    h.AddSpectrumPropagationLoss("ns3::FriisSpectrumPropagationLossModel");
    return h;
}

void
SpectrumChannelHelper::AddPropagationLoss(Ptr<PropagationLossModel> m)
{
    m->SetNext(m_propagationLossModel);
    m_propagationLossModel = m;
}

void
SpectrumChannelHelper::AddSpectrumPropagationLoss(Ptr<SpectrumPropagationLossModel> m)
{
    m->SetNext(m_spectrumPropagationLossModel);
    m_spectrumPropagationLossModel = m;
}

Ptr<SpectrumChannel>
SpectrumChannelHelper::Create() const
{
    Ptr<SpectrumChannel> channel = (m_channel.Create())->GetObject<SpectrumChannel>();
    channel->AddSpectrumPropagationLossModel(m_spectrumPropagationLossModel);
    channel->AddPropagationLossModel(m_propagationLossModel);
    Ptr<PropagationDelayModel> delay = m_propagationDelay.Create<PropagationDelayModel>();
    channel->SetPropagationDelayModel(delay);
    return channel;
}

void
SpectrumPhyHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
SpectrumPhyHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
SpectrumPhyHelper::SetPhyAttribute(std::string name, const AttributeValue& v)
{
    m_phy.Set(name, v);
}

Ptr<SpectrumPhy>
SpectrumPhyHelper::Create(Ptr<Node> node, Ptr<NetDevice> device) const
{
    NS_ASSERT(m_channel);
    Ptr<SpectrumPhy> phy = (m_phy.Create())->GetObject<SpectrumPhy>();
    phy->SetChannel(m_channel);
    phy->SetMobility(node->GetObject<MobilityModel>());
    phy->SetDevice(device);
    return phy;
}

} // namespace ns3
