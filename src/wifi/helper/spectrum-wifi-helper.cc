/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
#include "ns3/wifi-utils.h"

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
                                  const FrequencyRange& freqRange,
                                  const std::vector<SpectrumSubchannel>& subchannels)
{
    m_channels[freqRange] = SpectrumChannelInfo{
        .channel = channel,
        .subchannels = std::set<SpectrumSubchannel>(subchannels.begin(), subchannels.end())};
    AddWifiBandwidthFilter(channel);
}

void
SpectrumWifiPhyHelper::AddChannel(const std::string& channelName,
                                  const FrequencyRange& freqRange,
                                  const std::vector<SpectrumSubchannel>& subchannels)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    AddChannel(channel, freqRange, subchannels);
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
    NS_LOG_FUNCTION(this << linkId << freqRange);
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
    NS_LOG_FUNCTION(this);
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
        phy->ConfigureStandard(device->GetStandard());
        InstallPhyInterfaces(i, phy);
        phy->SetChannelSwitchedCallback(
            MakeCallback(&SpectrumWifiPhyHelper::SpectrumChannelSwitched).Bind(phy));
        phy->SetDevice(device);
        phy->SetMobility(node->GetObject<MobilityModel>());
        ret.emplace_back(phy);
    }

    return ret;
}

void
SpectrumWifiPhyHelper::InstallPhyInterfaces(uint8_t linkId, Ptr<SpectrumWifiPhy> phy) const
{
    NS_LOG_FUNCTION(this << linkId << phy->GetPhyId());
    if (!m_interfacesMap.contains(linkId))
    {
        // default setup: set all interfaces to this link
        for (const auto& [freqRange, channelInfo] : m_channels)
        {
            ConfigureChannel(phy, freqRange, channelInfo);
        }
    }
    else
    {
        for (const auto& freqRange : m_interfacesMap.at(linkId))
        {
            ConfigureChannel(phy, freqRange, m_channels.at(freqRange));
        }
    }
}

void
SpectrumWifiPhyHelper::ConfigureChannel(Ptr<SpectrumWifiPhy> phy,
                                        const FrequencyRange& freqRange,
                                        const SpectrumChannelInfo& channelInfo) const
{
    NS_LOG_FUNCTION(phy << phy << freqRange << channelInfo.channel
                        << channelInfo.subchannels.size());

    // If multiple subchannels have to be configured, we need to split the whole range.
    // For that purpose, we need to calculate the frequencies at which each split should happen.
    std::vector<MHz_t> splitFrequencies;
    if (!channelInfo.subchannels.empty())
    {
        std::optional<MHz_t> prevStopFreq;
        for (const auto& subchannel : channelInfo.subchannels)
        {
            // check there is no overlap with the other subchannels
            std::vector<MHz_t> subchannelFreqs(subchannel.centerFrequencies.begin(),
                                               subchannel.centerFrequencies.end());
            NS_ABORT_MSG_IF(std::any_of(channelInfo.subchannels.cbegin(),
                                        channelInfo.subchannels.cend(),
                                        [&subchannel](const auto& subchan) {
                                            const auto isSameSubchannel =
                                                std::equal(subchan.centerFrequencies.cbegin(),
                                                           subchan.centerFrequencies.cend(),
                                                           subchannel.centerFrequencies.cbegin()) &&
                                                (subchan.totalWidth == subchannel.totalWidth);
                                            return !isSameSubchannel &&
                                                   DoesOverlap(subchan.centerFrequencies,
                                                               subchan.totalWidth,
                                                               subchannel.centerFrequencies,
                                                               subchannel.totalWidth);
                                        }),
                            "Subchannel overlap detected");
            const auto width = subchannel.totalWidth / subchannel.centerFrequencies.size();
            const auto startFreq = *subchannel.centerFrequencies.cbegin() - (width / 2);
            const auto stopFreq = *subchannel.centerFrequencies.crbegin() + (width / 2);
            if (prevStopFreq)
            {
                splitFrequencies.emplace_back(*prevStopFreq + ((startFreq - *prevStopFreq) / 2));
            }
            prevStopFreq = stopFreq;
        }
    }

    // Split the frequency range into subranges starting from the minimum frequency
    // and ending at the maximum frequency, using the split frequencies calculated above.
    auto startFreq = freqRange.minFrequency;
    std::vector<FrequencyRange> splitRanges;
    for (const auto& freq : splitFrequencies)
    {
        splitRanges.emplace_back(startFreq, freq);
        startFreq = freq;
    }
    splitRanges.emplace_back(startFreq, freqRange.maxFrequency);
    for (const auto& splitRange : splitRanges)
    {
        phy->AddChannel(channelInfo.channel, splitRange);
    }

    // Finally, configure the interface of each subchannel, unless it is disabled.
    if (!channelInfo.subchannels.empty())
    {
        for (const auto& subchannel : channelInfo.subchannels)
        {
            if (!subchannel.enabled)
            {
                continue;
            }
            phy->ConfigureInterface(
                {subchannel.centerFrequencies.cbegin(), subchannel.centerFrequencies.cend()},
                subchannel.totalWidth);
        }
    }
}

void
SpectrumWifiPhyHelper::SpectrumChannelSwitched(Ptr<SpectrumWifiPhy> phy)
{
    NS_LOG_FUNCTION(phy->GetPhyId());
    for (const auto& otherPhy : phy->GetDevice()->GetPhys())
    {
        auto spectrumPhy = DynamicCast<SpectrumWifiPhy>(otherPhy);
        NS_ASSERT(spectrumPhy);
        if (spectrumPhy == phy)
        {
            // this is the PHY that has switched, so we do not need to configure anything
            NS_LOG_DEBUG("PHY " << +otherPhy->GetPhyId()
                                << " is the one that has switched: do nothing");
            continue;
        }
        if (DoesOverlap(spectrumPhy->GetCurrentFrequencyRange(), phy->GetCurrentFrequencyRange()))
        {
            // The frequency range covered by the active interface of the other PHY overlaps
            // with the frequency range of the PHY that has switched, so there is no inactive
            // interface to configure.
            NS_LOG_DEBUG("Range " << phy->GetCurrentFrequencyRange() << " overlaps with the range "
                                  << spectrumPhy->GetCurrentFrequencyRange()
                                  << " of the active interface for PHY " << +otherPhy->GetPhyId()
                                  << ": do nothing");
            continue;
        }
        if (const auto& interfaces = spectrumPhy->GetSpectrumPhyInterfaces();
            std::find_if(interfaces.cbegin(), interfaces.cend(), [phy](const auto& item) {
                return DoesOverlap(item.first, phy->GetCurrentFrequencyRange());
            }) == interfaces.cend())
        {
            // The other PHY does not have any interface that covers (fully or partially) the
            // frequency range of the PHY that has switched, so there is no interface to configure.
            NS_LOG_DEBUG("No interface covering range " << phy->GetCurrentFrequencyRange()
                                                        << " for PHY " << +otherPhy->GetPhyId()
                                                        << ": do nothing");
            continue;
        }
        NS_LOG_DEBUG("Configure interface for PHY " << +otherPhy->GetPhyId());
        const auto primaryWidth = std::min(otherPhy->GetChannelWidth(), phy->GetChannelWidth());
        const auto primaryChannel = phy->GetOperatingChannel().GetPrimaryChannel(primaryWidth);
        spectrumPhy->ConfigureInterface(primaryChannel.GetFrequencies(),
                                        primaryChannel.GetTotalWidth());
    }
}

} // namespace ns3
