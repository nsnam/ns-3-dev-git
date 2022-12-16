/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "spectrum-wifi-helper.h"

#include "ns3/error-rate-model.h"
#include "ns3/frame-capture-model.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/preamble-detection-model.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-transmit-filter.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/wifi-bandwidth-filter.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-spectrum-value-helper.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumWifiHelper");

SpectrumWifiPhyHelper::SpectrumWifiPhyHelper(uint8_t nLinks)
    : WifiPhyHelper(nLinks)
{
    NS_ABORT_IF(m_phy.size() != nLinks);
    for (auto& phy : m_phy)
    {
        phy.SetTypeId("ns3::SpectrumWifiPhy");
    }
    SetInterferenceHelper("ns3::InterferenceHelper");
    SetErrorRateModel("ns3::TableBasedErrorRateModel");
}

void
SpectrumWifiPhyHelper::SetChannel(const Ptr<SpectrumChannel> channel)
{
    m_channels[WHOLE_WIFI_SPECTRUM] = channel;
    AddWifiBandwidthFilter(channel);
}

void
SpectrumWifiPhyHelper::SetChannel(const std::string& channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channels[WHOLE_WIFI_SPECTRUM] = channel;
    AddWifiBandwidthFilter(channel);
}

void
SpectrumWifiPhyHelper::AddChannel(const Ptr<SpectrumChannel> channel,
                                  const FrequencyRange& freqRange)
{
    m_channels[freqRange] = channel;
    AddWifiBandwidthFilter(channel);
}

void
SpectrumWifiPhyHelper::AddChannel(const std::string& channelName, const FrequencyRange& freqRange)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    AddChannel(channel, freqRange);
    AddWifiBandwidthFilter(channel);
}

void
SpectrumWifiPhyHelper::AddWifiBandwidthFilter(Ptr<SpectrumChannel> channel)
{
    Ptr<const SpectrumTransmitFilter> p = channel->GetSpectrumTransmitFilter();
    bool found = false;
    while (p && !found)
    {
        if (DynamicCast<const WifiBandwidthFilter>(p))
        {
            NS_LOG_DEBUG("Found existing WifiBandwidthFilter for channel " << channel);
            found = true;
        }
        else
        {
            NS_LOG_DEBUG("Found different SpectrumTransmitFilter for channel " << channel);
            p = p->GetNext();
        }
    }
    if (!found)
    {
        Ptr<WifiBandwidthFilter> pWifi = CreateObject<WifiBandwidthFilter>();
        channel->AddSpectrumTransmitFilter(pWifi);
        NS_LOG_DEBUG("Adding WifiBandwidthFilter to channel " << channel);
    }
}

std::vector<Ptr<WifiPhy>>
SpectrumWifiPhyHelper::Create(Ptr<Node> node, Ptr<WifiNetDevice> device) const
{
    std::vector<Ptr<WifiPhy>> ret;

    for (std::size_t i = 0; i < m_phy.size(); i++)
    {
        Ptr<SpectrumWifiPhy> phy = m_phy.at(i).Create<SpectrumWifiPhy>();
        auto interference = m_interferenceHelper.Create<InterferenceHelper>();
        phy->SetInterferenceHelper(interference);
        Ptr<ErrorRateModel> error = m_errorRateModel.at(i).Create<ErrorRateModel>();
        phy->SetErrorRateModel(error);
        if (m_frameCaptureModel.at(i).IsTypeIdSet())
        {
            auto frameCapture = m_frameCaptureModel.at(i).Create<FrameCaptureModel>();
            phy->SetFrameCaptureModel(frameCapture);
        }
        if (m_preambleDetectionModel.at(i).IsTypeIdSet())
        {
            auto preambleDetection =
                m_preambleDetectionModel.at(i).Create<PreambleDetectionModel>();
            phy->SetPreambleDetectionModel(preambleDetection);
        }
        for (const auto& [freqRange, channel] : m_channels)
        {
            phy->AddChannel(channel, freqRange);
        }
        phy->SetDevice(device);
        phy->SetMobility(node->GetObject<MobilityModel>());
        ret.emplace_back(phy);
    }

    return ret;
}

} // namespace ns3
