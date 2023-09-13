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
    NS_ABORT_IF(m_phys.size() != nLinks);
    for (auto& phy : m_phys)
    {
        phy.SetTypeId("ns3::SpectrumWifiPhy");
    }
    SetInterferenceHelper("ns3::InterferenceHelper");
    SetErrorRateModel("ns3::TableBasedErrorRateModel");
}

void
SpectrumWifiPhyHelper::SetChannel(const Ptr<SpectrumChannel> channel)
{
    AddChannel(channel);
}

void
SpectrumWifiPhyHelper::SetChannel(const std::string& channelName)
{
    AddChannel(channelName);
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

void
SpectrumWifiPhyHelper::AddPhyToFreqRangeMapping(uint8_t linkId, const FrequencyRange& freqRange)
{
    if (auto it = m_interfacesMap.find(linkId); it == m_interfacesMap.end())
    {
        m_interfacesMap.insert({linkId, {freqRange}});
    }
    else
    {
        it->second.emplace(freqRange);
    }
}

void
SpectrumWifiPhyHelper::ResetPhyToFreqRangeMapping()
{
    m_interfacesMap.clear();
}

std::vector<Ptr<WifiPhy>>
SpectrumWifiPhyHelper::Create(Ptr<Node> node, Ptr<WifiNetDevice> device) const
{
    std::vector<Ptr<WifiPhy>> ret;

    for (std::size_t i = 0; i < m_phys.size(); i++)
    {
        auto phy = m_phys.at(i).Create<SpectrumWifiPhy>();
        auto interference = m_interferenceHelper.Create<InterferenceHelper>();
        phy->SetInterferenceHelper(interference);
        auto error = m_errorRateModel.at(i).Create<ErrorRateModel>();
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
        InstallPhyInterfaces(i, phy);
        phy->SetChannelSwitchedCallback(
            MakeCallback(&SpectrumWifiPhyHelper::SpectrumChannelSwitched, this).Bind(phy));
        phy->SetDevice(device);
        phy->SetMobility(node->GetObject<MobilityModel>());
        ret.emplace_back(phy);
    }

    return ret;
}

void
SpectrumWifiPhyHelper::InstallPhyInterfaces(uint8_t linkId, Ptr<SpectrumWifiPhy> phy) const
{
    if (m_interfacesMap.count(linkId) == 0)
    {
        // default setup: set all interfaces to this link
        for (const auto& [freqRange, channel] : m_channels)
        {
            phy->AddChannel(channel, freqRange);
        }
    }
    else
    {
        for (const auto& freqRange : m_interfacesMap.at(linkId))
        {
            phy->AddChannel(m_channels.at(freqRange), freqRange);
        }
    }
}

void
SpectrumWifiPhyHelper::SpectrumChannelSwitched(Ptr<SpectrumWifiPhy> phy) const
{
    NS_LOG_FUNCTION(this << phy);
    for (const auto& otherPhy : phy->GetDevice()->GetPhys())
    {
        auto spectrumPhy = DynamicCast<SpectrumWifiPhy>(otherPhy);
        NS_ASSERT(spectrumPhy);
        if (spectrumPhy == phy)
        {
            // this is the PHY that has switched
            continue;
        }
        if (spectrumPhy->GetCurrentFrequencyRange() == phy->GetCurrentFrequencyRange())
        {
            // this is the active interface
            continue;
        }
        if (const auto& interfaces = spectrumPhy->GetSpectrumPhyInterfaces();
            interfaces.count(phy->GetCurrentFrequencyRange()) == 0)
        {
            // no interface attached to that channel
            continue;
        }
        spectrumPhy->ConfigureInterface(phy->GetFrequency(), phy->GetChannelWidth());
    }
}

} // namespace ns3
