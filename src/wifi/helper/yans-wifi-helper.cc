/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "yans-wifi-helper.h"

#include "ns3/error-rate-model.h"
#include "ns3/frame-capture-model.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/preamble-detection-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("YansWifiHelper");

YansWifiChannelHelper::YansWifiChannelHelper()
{
}

YansWifiChannelHelper
YansWifiChannelHelper::Default()
{
    YansWifiChannelHelper helper;
    helper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    helper.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    return helper;
}

Ptr<YansWifiChannel>
YansWifiChannelHelper::Create() const
{
    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
    Ptr<PropagationLossModel> prev = nullptr;
    for (auto i = m_propagationLoss.begin(); i != m_propagationLoss.end(); ++i)
    {
        Ptr<PropagationLossModel> cur = (*i).Create<PropagationLossModel>();
        if (prev)
        {
            prev->SetNext(cur);
        }
        if (m_propagationLoss.begin() == i)
        {
            channel->SetPropagationLossModel(cur);
        }
        prev = cur;
    }
    Ptr<PropagationDelayModel> delay = m_propagationDelay.Create<PropagationDelayModel>();
    channel->SetPropagationDelayModel(delay);
    return channel;
}

int64_t
YansWifiChannelHelper::AssignStreams(Ptr<YansWifiChannel> c, int64_t stream)
{
    return c->AssignStreams(stream);
}

YansWifiPhyHelper::YansWifiPhyHelper()
    : WifiPhyHelper(1), // YANS phy is not used for 11be devices
      m_channel(nullptr)
{
    m_phys.front().SetTypeId("ns3::YansWifiPhy");
    SetInterferenceHelper("ns3::InterferenceHelper");
    SetErrorRateModel("ns3::TableBasedErrorRateModel");
}

void
YansWifiPhyHelper::SetChannel(Ptr<YansWifiChannel> channel)
{
    m_channel = channel;
}

void
YansWifiPhyHelper::SetChannel(std::string channelName)
{
    Ptr<YansWifiChannel> channel = Names::Find<YansWifiChannel>(channelName);
    m_channel = channel;
}

std::vector<Ptr<WifiPhy>>
YansWifiPhyHelper::Create(Ptr<Node> node, Ptr<WifiNetDevice> device) const
{
    Ptr<YansWifiPhy> phy = m_phys.front().Create<YansWifiPhy>();
    Ptr<InterferenceHelper> interference = m_interferenceHelper.Create<InterferenceHelper>();
    phy->SetInterferenceHelper(interference);
    Ptr<ErrorRateModel> error = m_errorRateModel.front().Create<ErrorRateModel>();
    phy->SetErrorRateModel(error);
    if (m_frameCaptureModel.front().IsTypeIdSet())
    {
        auto frameCapture = m_frameCaptureModel.front().Create<FrameCaptureModel>();
        phy->SetFrameCaptureModel(frameCapture);
    }
    if (m_preambleDetectionModel.front().IsTypeIdSet())
    {
        auto preambleDetection = m_preambleDetectionModel.front().Create<PreambleDetectionModel>();
        phy->SetPreambleDetectionModel(preambleDetection);
    }
    phy->SetChannel(m_channel);
    phy->SetDevice(device);
    return std::vector<Ptr<WifiPhy>>({phy});
}

} // namespace ns3
