/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 * Copyright (c) 2017 Orange Labs
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Giuseppe Piro  <g.piro@poliba.it>
 *          Rediet <getachew.redieteab@orange.com>
 */

#include "wifi-spectrum-value-helper.h"

#include "ns3/assert.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>

namespace
{

/**
 * Lambda to print a vector of frequencies.
 */
auto printFrequencies = [](const std::vector<uint16_t>& v) {
    std::stringstream ss;
    for (const auto& centerFrequency : v)
    {
        ss << centerFrequency << " ";
    }
    return ss.str();
};
} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiSpectrumValueHelper");

///< Wifi Spectrum Model structure
struct WifiSpectrumModelId
{
    std::vector<uint16_t> centerFrequencies; ///< center frequency per segment (in MHz)
    ChannelWidthMhz channelWidth;            ///< channel width (in MHz)
    uint32_t carrierSpacing;                 ///< carrier spacing (in Hz)
    ChannelWidthMhz guardBandwidth;          ///< guard band width (in MHz)
};

/**
 * Less than operator
 * \param lhs the left hand side wifi spectrum to compare
 * \param rhs the right hand side wifi spectrum to compare
 * \returns true if the left hand side spectrum is less than the right hand side spectrum
 */
bool
operator<(const WifiSpectrumModelId& lhs, const WifiSpectrumModelId& rhs)
{
    return std::tie(lhs.centerFrequencies,
                    lhs.channelWidth,
                    lhs.carrierSpacing,
                    lhs.guardBandwidth) < std::tie(rhs.centerFrequencies,
                                                   rhs.channelWidth,
                                                   rhs.carrierSpacing,
                                                   rhs.guardBandwidth);
    // TODO: replace with default spaceship operator, but it seems currently not working with all
    // compilers
}

static std::map<WifiSpectrumModelId, Ptr<SpectrumModel>>
    g_wifiSpectrumModelMap; ///< static initializer for the class

Ptr<SpectrumModel>
WifiSpectrumValueHelper::GetSpectrumModel(const std::vector<uint16_t>& centerFrequencies,
                                          ChannelWidthMhz channelWidth,
                                          uint32_t carrierSpacing,
                                          ChannelWidthMhz guardBandwidth)
{
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << carrierSpacing << guardBandwidth);
    NS_ASSERT_MSG(centerFrequencies.size() <= 2,
                  "Spectrum model does not support more than 2 segments");
    if (centerFrequencies.size() != 1)
    {
        NS_ASSERT_MSG(centerFrequencies.front() != centerFrequencies.back(),
                      "Center frequency of each segment shall be different");
    }
    Ptr<SpectrumModel> ret;
    WifiSpectrumModelId key{centerFrequencies, channelWidth, carrierSpacing, guardBandwidth};
    if (const auto it = g_wifiSpectrumModelMap.find(key); it != g_wifiSpectrumModelMap.cend())
    {
        ret = it->second;
    }
    else
    {
        Bands bands;
        const auto [minCenterFrequency, maxCenterFrequency] =
            std::minmax_element(centerFrequencies.cbegin(), centerFrequencies.cend());
        const auto separationWidth =
            (minCenterFrequency == maxCenterFrequency)
                ? 0
                : (*maxCenterFrequency - *minCenterFrequency - (channelWidth / 2));
        NS_ASSERT(separationWidth == 0 || centerFrequencies.size() == 2);
        double bandwidth = (channelWidth + (2 * guardBandwidth) + separationWidth) * 1e6;
        // For OFDM, the center subcarrier is null (at center frequency)
        auto numBands = static_cast<uint32_t>((bandwidth / carrierSpacing) + 0.5);
        NS_ASSERT(numBands > 0);
        if (numBands % 2 == 0)
        {
            // round up to the nearest odd number of subbands so that bands
            // are symmetric around center frequency
            numBands += 1;
        }
        NS_ASSERT_MSG(numBands % 2 == 1, "Number of bands should be odd");
        NS_LOG_DEBUG("Num bands " << numBands << " band bandwidth " << carrierSpacing);

        // The lowest frequency is obtained from the minimum center frequency among the segment(s).
        // Then, we subtract half the channel width to retrieve the starting frequency of the
        // operating channel. If the channel is made of 2 segments, since the channel width is the
        // total width, only a quarter of the channel width has to be subtracted. Finally, we
        // remove the guard band width to get the center frequency of the first band and half the
        // carrier spacing to get the effective starting frequency of the first band.
        const auto startingFrequencyHz = *minCenterFrequency * 1e6 -
                                         ((channelWidth * 1e6) / (2 * centerFrequencies.size())) -
                                         (guardBandwidth * 1e6) - (carrierSpacing / 2);
        for (size_t i = 0; i < numBands; i++)
        {
            BandInfo info;
            double f = startingFrequencyHz + (i * carrierSpacing);
            info.fl = f;
            f += carrierSpacing / 2;
            info.fc = f;
            f += carrierSpacing / 2;
            info.fh = f;
            NS_LOG_DEBUG("creating band " << i << " (" << info.fl << ":" << info.fc << ":"
                                          << info.fh << ")");
            bands.push_back(info);
        }
        ret = Create<SpectrumModel>(std::move(bands));
        g_wifiSpectrumModelMap.insert(std::pair<WifiSpectrumModelId, Ptr<SpectrumModel>>(key, ret));
    }
    NS_LOG_LOGIC("returning SpectrumModel::GetUid () == " << ret->GetUid());
    return ret;
}

// Power allocated to 71 center subbands out of 135 total subbands in the band
Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateDsssTxPowerSpectralDensity(uint16_t centerFrequency,
                                                          double txPowerW,
                                                          ChannelWidthMhz guardBandwidth)
{
    NS_LOG_FUNCTION(centerFrequency << txPowerW << +guardBandwidth);
    ChannelWidthMhz channelWidth = 22; // DSSS channels are 22 MHz wide
    uint32_t carrierSpacing = 312500;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel({centerFrequency}, channelWidth, carrierSpacing, guardBandwidth));
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    auto nGuardBands = static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / carrierSpacing) + 0.5);
    auto nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / carrierSpacing) + 0.5);
    NS_ASSERT(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1));
    // Evenly spread power across 22 MHz
    double txPowerPerBand = txPowerW / nAllocatedBands;
    for (size_t i = 0; i < c->GetSpectrumModel()->GetNumBands(); i++, vit++, bit++)
    {
        if ((i >= (nGuardBands / 2)) && (i <= ((nGuardBands / 2) + nAllocatedBands - 1)))
        {
            *vit = txPowerPerBand / (bit->fh - bit->fl);
        }
    }
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(uint16_t centerFrequency,
                                                          ChannelWidthMhz channelWidth,
                                                          double txPowerW,
                                                          ChannelWidthMhz guardBandwidth,
                                                          double minInnerBandDbr,
                                                          double minOuterBandDbr,
                                                          double lowestPointDbr)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                                    << minOuterBandDbr << lowestPointDbr);
    uint32_t carrierSpacing = 0;
    uint32_t innerSlopeWidth = 0;
    switch (channelWidth)
    {
    case 20:
        carrierSpacing = 312500;
        innerSlopeWidth = static_cast<uint32_t>((2e6 / carrierSpacing) + 0.5); // [-11;-9] & [9;11]
        break;
    case 10:
        carrierSpacing = 156250;
        innerSlopeWidth =
            static_cast<uint32_t>((1e6 / carrierSpacing) + 0.5); // [-5.5;-4.5] & [4.5;5.5]
        break;
    case 5:
        carrierSpacing = 78125;
        innerSlopeWidth =
            static_cast<uint32_t>((5e5 / carrierSpacing) + 0.5); // [-2.75;-2.5] & [2.5;2.75]
        break;
    default:
        NS_FATAL_ERROR("Channel width " << channelWidth << " should be correctly set.");
        return nullptr;
    }

    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel({centerFrequency}, channelWidth, carrierSpacing, guardBandwidth));
    auto nGuardBands = static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / carrierSpacing) + 0.5);
    auto nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    // 52 subcarriers (48 data + 4 pilot)
    // skip guard band and 6 subbands, then place power in 26 subbands, then
    // skip the center subband, then place power in 26 subbands, then skip
    // the final 6 subbands and the guard band.
    double txPowerPerBandW = txPowerW / 52;
    NS_LOG_DEBUG("Power per band " << txPowerPerBandW << "W");
    uint32_t start1 = (nGuardBands / 2) + 6;
    uint32_t stop1 = start1 + 26 - 1;
    uint32_t start2 = stop1 + 2;
    uint32_t stop2 = start2 + 26 - 1;

    // Build transmit spectrum mask
    std::vector<WifiSpectrumBandIndices> subBands{
        std::make_pair(start1, stop1),
        std::make_pair(start2, stop2),
    };
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands);
    CreateSpectrumMaskForOfdm(c,
                              {subBands},
                              maskBand,
                              txPowerPerBandW,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBandDbr,
                              minOuterBandDbr,
                              lowestPointDbr);
    NormalizeSpectrumMask(c, txPowerW);
    NS_ASSERT_MSG(std::abs(txPowerW - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateDuplicated20MhzTxPowerSpectralDensity(
    const std::vector<uint16_t>& centerFrequencies,
    ChannelWidthMhz channelWidth,
    double txPowerW,
    ChannelWidthMhz guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_ASSERT_MSG(centerFrequencies.size() == 1 ||
                      (channelWidth == 160 && centerFrequencies.size() <= 2),
                  "PSD for non-contiguous channels is only possible when the total width is 160 "
                  "MHz and cannot be made of more than 2 segments");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                    << minOuterBandDbr << lowestPointDbr);
    uint32_t carrierSpacing = 312500;
    guardBandwidth /= centerFrequencies.size();
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    const auto nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>(((channelWidth * 1e6) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth = separationWidth > 0 ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>(((unallocatedWidth * 1e6) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    std::size_t num20MhzBands = channelWidth / 20;
    std::size_t numAllocatedSubcarriersPer20MHz = 52;
    NS_ASSERT(puncturedSubchannels.empty() || (puncturedSubchannels.size() == num20MhzBands));
    double txPowerPerBandW = (txPowerW / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBandW << "W");

    std::size_t numSubcarriersPer20MHz = (20 * 1e6) / carrierSpacing;
    std::size_t numUnallocatedSubcarriersPer20MHz =
        numSubcarriersPer20MHz - numAllocatedSubcarriersPer20MHz;
    std::vector<std::vector<WifiSpectrumBandIndices>> subBandsPerSegment(
        centerFrequencies.size()); // list of data/pilot-containing subBands (sent at 0dBr)
    for (std::size_t i = 0; i < centerFrequencies.size(); ++i)
    {
        subBandsPerSegment.at(i).resize(
            num20MhzBands * 2 / centerFrequencies.size()); // the center subcarrier is skipped,
                                                           // hence 2 subbands per 20 MHz subchannel
    }
    std::vector<std::vector<WifiSpectrumBandIndices>> puncturedBandsPerSegment;
    uint32_t start = (nGuardBands / 2) + (numUnallocatedSubcarriersPer20MHz / 2);
    uint32_t stop;
    uint8_t index = 0;
    for (auto& subBands : subBandsPerSegment)
    {
        puncturedBandsPerSegment.emplace_back();
        for (auto it = subBands.begin(); it != subBands.end();)
        {
            stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
            *it = std::make_pair(start, stop);
            ++it;
            uint32_t puncturedStart = start;
            start = stop + 2; // skip center subcarrier
            stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
            *it = std::make_pair(start, stop);
            ++it;
            start = stop + numUnallocatedSubcarriersPer20MHz;
            uint32_t puncturedStop = stop;
            if (!puncturedSubchannels.empty() && puncturedSubchannels.at(index++))
            {
                puncturedBandsPerSegment.back().emplace_back(puncturedStart, puncturedStop);
            }
        }
        start += nUnallocatedBands;
    }

    // Prepare spectrum mask specific variables
    auto innerSlopeWidth = static_cast<uint32_t>(
        (2e6 / carrierSpacing) +
        0.5); // size in number of subcarriers of the 0dBr<->20dBr slope (2MHz for HT/VHT)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);
    const auto puncturedSlopeWidth =
        static_cast<uint32_t>((500e3 / carrierSpacing) +
                              0.5); // size in number of subcarriers of the punctured slope band

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBandsPerSegment,
                              maskBand,
                              txPowerPerBandW,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBandDbr,
                              minOuterBandDbr,
                              lowestPointDbr,
                              puncturedBandsPerSegment,
                              puncturedSlopeWidth);
    NormalizeSpectrumMask(c, txPowerW);
    NS_ASSERT_MSG(std::abs(txPowerW - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(
    const std::vector<uint16_t>& centerFrequencies,
    ChannelWidthMhz channelWidth,
    double txPowerW,
    ChannelWidthMhz guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr)
{
    NS_ASSERT_MSG(centerFrequencies.size() == 1 ||
                      (channelWidth == 160 && centerFrequencies.size() <= 2),
                  "PSD for non-contiguous channels is only possible when the total width is 160 "
                  "MHz and cannot be made of more than 2 segments");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                    << minOuterBandDbr << lowestPointDbr);
    uint32_t carrierSpacing = 312500;
    guardBandwidth /= centerFrequencies.size();
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    const auto nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>(((channelWidth * 1e6) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth = separationWidth > 0 ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>(((unallocatedWidth * 1e6) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    std::size_t num20MhzBands = channelWidth / 20;
    std::size_t numAllocatedSubcarriersPer20MHz = 56;
    double txPowerPerBandW = (txPowerW / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBandW << "W");

    std::size_t numSubcarriersPer20MHz = (20 * 1e6) / carrierSpacing;
    std::size_t numUnallocatedSubcarriersPer20MHz =
        numSubcarriersPer20MHz - numAllocatedSubcarriersPer20MHz;
    std::vector<std::vector<WifiSpectrumBandIndices>> subBandsPerSegment(
        centerFrequencies.size()); // list of data/pilot-containing subBands (sent at 0dBr)
    for (std::size_t i = 0; i < centerFrequencies.size(); ++i)
    {
        subBandsPerSegment.at(i).resize(
            num20MhzBands * 2 / centerFrequencies.size()); // the center subcarrier is skipped,
                                                           // hence 2 subbands per 20 MHz subchannel
    }
    uint32_t start = (nGuardBands / 2) + (numUnallocatedSubcarriersPer20MHz / 2);
    uint32_t stop;
    for (auto& subBands : subBandsPerSegment)
    {
        for (auto it = subBands.begin(); it != subBands.end();)
        {
            stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
            *it = std::make_pair(start, stop);
            ++it;
            start = stop + 2; // skip center subcarrier
            stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
            *it = std::make_pair(start, stop);
            ++it;
            start = stop + numUnallocatedSubcarriersPer20MHz;
        }
        start += nUnallocatedBands;
    }

    // Prepare spectrum mask specific variables
    auto innerSlopeWidth = static_cast<uint32_t>(
        (2e6 / carrierSpacing) +
        0.5); // size in number of subcarriers of the inner band (2MHz for HT/VHT)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBandsPerSegment,
                              maskBand,
                              txPowerPerBandW,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBandDbr,
                              minOuterBandDbr,
                              lowestPointDbr);
    NormalizeSpectrumMask(c, txPowerW);
    NS_ASSERT_MSG(std::abs(txPowerW - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
    uint16_t centerFrequency,
    ChannelWidthMhz channelWidth,
    double txPowerW,
    ChannelWidthMhz guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<bool>& puncturedSubchannels)
{
    return CreateHeOfdmTxPowerSpectralDensity(std::vector<uint16_t>{centerFrequency},
                                              channelWidth,
                                              txPowerW,
                                              guardBandwidth,
                                              minInnerBandDbr,
                                              minOuterBandDbr,
                                              lowestPointDbr,
                                              puncturedSubchannels);
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
    const std::vector<uint16_t>& centerFrequencies,
    uint16_t channelWidth,
    double txPowerW,
    uint16_t guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_ASSERT_MSG(
        centerFrequencies.size() == 1 || channelWidth == 160,
        "PSD for non-contiguous channels is only possible when the total width is 160 MHz");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                    << minOuterBandDbr << lowestPointDbr);
    uint32_t carrierSpacing = 78125;
    guardBandwidth /= centerFrequencies.size();
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    const auto nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth = separationWidth > 0 ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>(((unallocatedWidth * 1e6) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>(((channelWidth * 1e6) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    double txPowerPerBandW = 0.0;
    uint32_t start1;
    uint32_t stop1;
    uint32_t start2;
    uint32_t stop2;
    uint32_t start3;
    uint32_t stop3;
    uint32_t start4;
    uint32_t stop4;
    // Prepare spectrum mask specific variables
    auto innerSlopeWidth = static_cast<uint32_t>(
        (1e6 / carrierSpacing) + 0.5); // size in number of subcarriers of the inner band
    std::vector<std::vector<WifiSpectrumBandIndices>> subBandsPerSegment(
        centerFrequencies.size()); // list of data/pilot-containing subBands (sent at 0dBr)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);
    switch (channelWidth)
    {
    case 20:
        // 242 subcarriers (234 data + 8 pilot)
        txPowerPerBandW = txPowerW / 242;
        innerSlopeWidth =
            static_cast<uint32_t>((5e5 / carrierSpacing) + 0.5); // [-10.25;-9.75] & [9.75;10.25]
        // skip the guard band and 6 subbands, then place power in 121 subbands, then
        // skip 3 DC, then place power in 121 subbands, then skip
        // the final 5 subbands and the guard band.
        start1 = (nGuardBands / 2) + 6;
        stop1 = start1 + 121 - 1;
        start2 = stop1 + 4;
        stop2 = start2 + 121 - 1;
        subBandsPerSegment.at(0).emplace_back(start1, stop1);
        subBandsPerSegment.at(0).emplace_back(start2, stop2);
        break;
    case 40:
        // 484 subcarriers (468 data + 16 pilot)
        txPowerPerBandW = txPowerW / 484;
        // skip the guard band and 12 subbands, then place power in 242 subbands, then
        // skip 5 DC, then place power in 242 subbands, then skip
        // the final 11 subbands and the guard band.
        start1 = (nGuardBands / 2) + 12;
        stop1 = start1 + 242 - 1;
        start2 = stop1 + 6;
        stop2 = start2 + 242 - 1;
        subBandsPerSegment.at(0).emplace_back(start1, stop1);
        subBandsPerSegment.at(0).emplace_back(start2, stop2);
        break;
    case 80:
        // 996 subcarriers (980 data + 16 pilot)
        txPowerPerBandW = txPowerW / 996;
        // skip the guard band and 12 subbands, then place power in 498 subbands, then
        // skip 5 DC, then place power in 498 subbands, then skip
        // the final 11 subbands and the guard band.
        start1 = (nGuardBands / 2) + 12;
        stop1 = start1 + 498 - 1;
        start2 = stop1 + 6;
        stop2 = start2 + 498 - 1;
        subBandsPerSegment.at(0).emplace_back(start1, stop1);
        subBandsPerSegment.at(0).emplace_back(start2, stop2);
        break;
    case 160: {
        NS_ASSERT_MSG(centerFrequencies.size() <= 2,
                      "It is not possible to create a PSD made of more than 2 segments for a width "
                      "of 160 MHz");
        // 2 x 996 subcarriers (2 x 80 MHZ bands)
        txPowerPerBandW = txPowerW / (2 * 996);
        start1 = (nGuardBands / 2) + 12;
        stop1 = start1 + 498 - 1;
        start2 = stop1 + 6;
        stop2 = start2 + 498 - 1;
        start3 = stop2 + (2 * 12) + nUnallocatedBands;
        stop3 = start3 + 498 - 1;
        start4 = stop3 + 6;
        stop4 = start4 + 498 - 1;
        subBandsPerSegment.at(0).emplace_back(start1, stop1);
        subBandsPerSegment.at(0).emplace_back(start2, stop2);
        subBandsPerSegment.at(subBandsPerSegment.size() - 1).emplace_back(start3, stop3);
        subBandsPerSegment.at(subBandsPerSegment.size() - 1).emplace_back(start4, stop4);
        break;
    }
    default:
        NS_FATAL_ERROR("ChannelWidth " << channelWidth << " unsupported");
        break;
    }

    // Create punctured bands
    auto puncturedSlopeWidth =
        static_cast<uint32_t>((500e3 / carrierSpacing) +
                              0.5); // size in number of subcarriers of the punctured slope band
    std::vector<std::vector<WifiSpectrumBandIndices>> puncturedBandsPerSegment;
    std::size_t subcarriersPerSuband = (20 * 1e6 / carrierSpacing);
    uint32_t start = (nGuardBands / 2);
    uint32_t stop = start + subcarriersPerSuband - 1;
    if (!puncturedSubchannels.empty())
    {
        for (std::size_t i = 0; i < subBandsPerSegment.size(); ++i)
        {
            puncturedBandsPerSegment.emplace_back();
        }
    }
    std::size_t prevPsdIndex = 0;
    for (std::size_t i = 0; i < puncturedSubchannels.size(); ++i)
    {
        std::size_t psdIndex = (puncturedBandsPerSegment.size() == 1)
                                   ? 0
                                   : ((i < (puncturedSubchannels.size() / 2)) ? 0 : 1);
        if (psdIndex != prevPsdIndex)
        {
            start += nUnallocatedBands;
            stop += nUnallocatedBands;
        }
        if (puncturedSubchannels.at(i))
        {
            puncturedBandsPerSegment.at(psdIndex).emplace_back(start, stop);
        }
        start = stop + 1;
        stop = start + subcarriersPerSuband - 1;
        prevPsdIndex = psdIndex;
    }

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBandsPerSegment,
                              maskBand,
                              txPowerPerBandW,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBandDbr,
                              minOuterBandDbr,
                              lowestPointDbr,
                              puncturedBandsPerSegment,
                              puncturedSlopeWidth);
    NormalizeSpectrumMask(c, txPowerW);
    NS_ASSERT_MSG(std::abs(txPowerW - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity(
    const std::vector<uint16_t>& centerFrequencies,
    ChannelWidthMhz channelWidth,
    double txPowerW,
    ChannelWidthMhz guardBandwidth,
    const WifiSpectrumBandIndices& ru)
{
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPowerW << guardBandwidth << ru.first << ru.second);
    uint32_t carrierSpacing = 78125;
    guardBandwidth /= centerFrequencies.size();
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));

    // Build spectrum mask
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    double txPowerPerBandW = (txPowerW / (ru.second - ru.first + 1)); // FIXME: null subcarriers
    uint32_t numBands = c->GetSpectrumModel()->GetNumBands();
    for (size_t i = 0; i < numBands; i++, vit++, bit++)
    {
        if (i < ru.first || i > ru.second) // outside the spectrum mask
        {
            *vit = 0.0;
        }
        else
        {
            *vit = (txPowerPerBandW / (bit->fh - bit->fl));
        }
    }

    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateNoisePowerSpectralDensity(uint16_t centerFrequency,
                                                         ChannelWidthMhz channelWidth,
                                                         uint32_t carrierSpacing,
                                                         double noiseFigure,
                                                         ChannelWidthMhz guardBandwidth)
{
    Ptr<SpectrumModel> model =
        GetSpectrumModel({centerFrequency}, channelWidth, carrierSpacing, guardBandwidth);
    return CreateNoisePowerSpectralDensity(noiseFigure, model);
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateNoisePowerSpectralDensity(double noiseFigureDb,
                                                         Ptr<SpectrumModel> spectrumModel)
{
    NS_LOG_FUNCTION(noiseFigureDb << spectrumModel);

    // see "LTE - From theory to practice"
    // Section 22.4.4.2 Thermal Noise and Receiver Noise Figure
    const double kT_dBm_Hz = -174.0; // dBm/Hz
    double kT_W_Hz = DbmToW(kT_dBm_Hz);
    double noiseFigureLinear = std::pow(10.0, noiseFigureDb / 10.0);
    double noisePowerSpectralDensity = kT_W_Hz * noiseFigureLinear;

    Ptr<SpectrumValue> noisePsd = Create<SpectrumValue>(spectrumModel);
    (*noisePsd) = noisePowerSpectralDensity;
    NS_LOG_INFO("NoisePowerSpectralDensity has integrated power of " << Integral(*noisePsd));
    return noisePsd;
}

void
WifiSpectrumValueHelper::CreateSpectrumMaskForOfdm(
    Ptr<SpectrumValue> c,
    const std::vector<std::vector<WifiSpectrumBandIndices>>& allocatedSubBandsPerSegment,
    const WifiSpectrumBandIndices& maskBand,
    double txPowerPerBandW,
    uint32_t nGuardBands,
    uint32_t innerSlopeWidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<std::vector<WifiSpectrumBandIndices>>& puncturedBandsPerSegment,
    uint32_t puncturedSlopeWidth)
{
    NS_ASSERT_MSG(allocatedSubBandsPerSegment.size() <= 2,
                  "Only PSDs for up to 2 frequency segments are supported");
    NS_ASSERT(puncturedBandsPerSegment.empty() ||
              (puncturedBandsPerSegment.size() == allocatedSubBandsPerSegment.size()));
    NS_LOG_FUNCTION(c << allocatedSubBandsPerSegment.front().front().first
                      << allocatedSubBandsPerSegment.front().back().second << maskBand.first
                      << maskBand.second << txPowerPerBandW << nGuardBands << innerSlopeWidth
                      << minInnerBandDbr << minOuterBandDbr << lowestPointDbr
                      << puncturedSlopeWidth);
    uint32_t numSubBands = allocatedSubBandsPerSegment.front().size();
    uint32_t numBands = c->GetSpectrumModel()->GetNumBands();
    uint32_t numMaskBands = maskBand.second - maskBand.first + 1;
    NS_ASSERT(numSubBands && numBands && numMaskBands);
    NS_LOG_LOGIC("Power per band " << txPowerPerBandW << "W");

    // Different power levels
    double txPowerRefDbm = (10.0 * std::log10(txPowerPerBandW * 1000.0));
    double txPowerInnerBandMinDbm = txPowerRefDbm + minInnerBandDbr;
    double txPowerMiddleBandMinDbm = txPowerRefDbm + minOuterBandDbr;
    double txPowerOuterBandMinDbm =
        txPowerRefDbm + lowestPointDbr; // TODO also take into account dBm/MHz constraints

    // Different widths (in number of bands)
    uint32_t outerSlopeWidth =
        nGuardBands / 4; // nGuardBands is the total left+right guard band. The left/right outer
                         // part is half of the left/right guard band.
    uint32_t middleSlopeWidth = outerSlopeWidth - (innerSlopeWidth / 2);

    std::vector<WifiSpectrumBandIndices> outerBandsLeft;
    std::vector<WifiSpectrumBandIndices> middleBandsLeft;
    std::vector<WifiSpectrumBandIndices> flatJunctionsLeft;
    std::vector<WifiSpectrumBandIndices> innerBandsLeft;
    std::vector<WifiSpectrumBandIndices> allocatedSubBands;
    std::vector<WifiSpectrumBandIndices> innerBandsRight;
    std::vector<WifiSpectrumBandIndices> flatJunctionsRight;
    std::vector<WifiSpectrumBandIndices> middleBandsRight;
    std::vector<WifiSpectrumBandIndices> outerBandsRight;
    std::optional<WifiSpectrumBandIndices> betweenPsdsBand;

    allocatedSubBands.emplace_back(allocatedSubBandsPerSegment.front().front().first,
                                   allocatedSubBandsPerSegment.front().back().second);
    outerBandsLeft.emplace_back(maskBand.first, // to handle cases where allocated channel is
                                                // under WifiPhy configured channel width.
                                maskBand.first + outerSlopeWidth - 1);
    middleBandsLeft.emplace_back(outerBandsLeft.front().second + 1,
                                 outerBandsLeft.front().second + middleSlopeWidth);
    innerBandsLeft.emplace_back(allocatedSubBands.front().first - innerSlopeWidth,
                                allocatedSubBands.front().first -
                                    1); // better to place slope based on allocated subcarriers
    flatJunctionsLeft.emplace_back(middleBandsLeft.front().second + 1,
                                   innerBandsLeft.front().first -
                                       1); // in order to handle shift due to guard subcarriers
    uint32_t flatJunctionWidth =
        flatJunctionsLeft.front().second - flatJunctionsLeft.front().first + 1;
    innerBandsRight.emplace_back(allocatedSubBands.front().second + 1,
                                 allocatedSubBands.front().second + innerSlopeWidth);
    flatJunctionsRight.emplace_back(innerBandsRight.front().second + 1,
                                    innerBandsRight.front().second + flatJunctionWidth);
    middleBandsRight.emplace_back(flatJunctionsRight.front().second + 1,
                                  flatJunctionsRight.front().second + middleSlopeWidth);
    outerBandsRight.emplace_back(middleBandsRight.front().second + 1,
                                 middleBandsRight.front().second + outerSlopeWidth);

    if (allocatedSubBandsPerSegment.size() > 1)
    {
        const auto offset = (((allocatedSubBandsPerSegment.front().back().second -
                               allocatedSubBandsPerSegment.front().front().first) /
                              2) +
                             (allocatedSubBandsPerSegment.back().front().first -
                              allocatedSubBandsPerSegment.front().back().second) +
                             ((allocatedSubBandsPerSegment.back().back().second -
                               allocatedSubBandsPerSegment.back().front().first) /
                              2));
        outerBandsLeft.emplace_back(outerBandsLeft.front().first + offset,
                                    outerBandsLeft.front().second + offset);
        middleBandsLeft.emplace_back(middleBandsLeft.front().first + offset,
                                     middleBandsLeft.front().second + offset);
        flatJunctionsLeft.emplace_back(flatJunctionsLeft.front().first + offset,
                                       flatJunctionsLeft.front().second + offset);
        innerBandsLeft.emplace_back(innerBandsLeft.front().first + offset,
                                    innerBandsLeft.front().second + offset);
        allocatedSubBands.emplace_back(allocatedSubBands.front().first + offset,
                                       allocatedSubBands.front().second + offset);
        innerBandsRight.emplace_back(innerBandsRight.front().first + offset,
                                     innerBandsRight.front().second + offset);
        flatJunctionsRight.emplace_back(flatJunctionsRight.front().first + offset,
                                        flatJunctionsRight.front().second + offset);
        middleBandsRight.emplace_back(middleBandsRight.front().first + offset,
                                      middleBandsRight.front().second + offset);
        outerBandsRight.emplace_back(outerBandsRight.front().first + offset,
                                     outerBandsRight.front().second + offset);
        betweenPsdsBand.emplace(middleBandsRight.front().first, middleBandsLeft.back().second);
    }

    std::ostringstream ss;
    for (std::size_t i = 0; i < allocatedSubBandsPerSegment.size(); ++i)
    {
        if (allocatedSubBandsPerSegment.size() > 1)
        {
            ss << "PSD" << i + 1 << ": ";
        }
        ss << "outerBandLeft=[" << outerBandsLeft.at(i).first << ";" << outerBandsLeft.at(i).second
           << "] "
           << "middleBandLeft=[" << middleBandsLeft.at(i).first << ";"
           << middleBandsLeft.at(i).second << "] "
           << "flatJunctionLeft=[" << flatJunctionsLeft.at(i).first << ";"
           << flatJunctionsLeft.at(i).second << "] "
           << "innerBandLeft=[" << innerBandsLeft.at(i).first << ";" << innerBandsLeft.at(i).second
           << "] "
           << "allocatedBand=[" << allocatedSubBands.at(i).first << ";"
           << allocatedSubBands.at(i).second << "] ";
        if (!puncturedBandsPerSegment.empty() && !puncturedBandsPerSegment.at(i).empty())
        {
            ss << "puncturedBands=[" << puncturedBandsPerSegment.at(i).front().first << ";"
               << puncturedBandsPerSegment.at(i).back().second << "] ";
        }
        ss << "innerBandRight=[" << innerBandsRight.at(i).first << ";"
           << innerBandsRight.at(i).second << "] "
           << "flatJunctionRight=[" << flatJunctionsRight.at(i).first << ";"
           << flatJunctionsRight.at(i).second << "] "
           << "middleBandRight=[" << middleBandsRight.at(i).first << ";"
           << middleBandsRight.at(i).second << "] "
           << "outerBandRight=[" << outerBandsRight.at(i).first << ";"
           << outerBandsRight.at(i).second << "] ";
    }
    if (allocatedSubBandsPerSegment.size() > 1)
    {
        ss << "=> PSD: "
           << "outerBandLeft=[" << outerBandsLeft.front().first << ";"
           << outerBandsLeft.front().second << "] "
           << "middleBandLeft=[" << middleBandsLeft.front().first << ";"
           << middleBandsLeft.front().second << "] "
           << "flatJunctionLeft=[" << flatJunctionsLeft.front().first << ";"
           << flatJunctionsLeft.front().second << "] "
           << "innerBandLeft=[" << innerBandsLeft.front().first << ";"
           << innerBandsLeft.front().second << "] "
           << "allocatedBandInPsd1=[" << allocatedSubBands.front().first << ";"
           << allocatedSubBands.front().second << "] ";
        if (!puncturedBandsPerSegment.empty() && !puncturedBandsPerSegment.front().empty())
        {
            ss << "puncturedBandsInPsd1=[" << puncturedBandsPerSegment.front().front().first << ";"
               << puncturedBandsPerSegment.front().back().second << "] ";
        }
        ss << "flatJunctionRightPsd1=[" << flatJunctionsRight.front().first << ";"
           << flatJunctionsRight.front().second << "] "
           << "linearSum=[" << betweenPsdsBand->first << ";" << betweenPsdsBand->second << "] "
           << "flatJunctionLeftPsd2=[" << flatJunctionsLeft.back().first << ";"
           << flatJunctionsLeft.back().second << "] "
           << "innerBandLeftPsd2=[" << innerBandsLeft.back().first << ";"
           << innerBandsLeft.back().second << "] "
           << "allocatedBandInPsd2=[" << allocatedSubBands.back().first << ";"
           << allocatedSubBands.back().second << "] ";
        if (!puncturedBandsPerSegment.empty() && !puncturedBandsPerSegment.back().empty())
        {
            ss << "puncturedBandsInPsd2=[" << puncturedBandsPerSegment.back().front().first << ";"
               << puncturedBandsPerSegment.back().back().second << "] ";
        }
        ss << "innerBandRight=[" << innerBandsRight.back().first << ";"
           << innerBandsRight.back().second << "] "
           << "flatJunctionRight=[" << flatJunctionsRight.back().first << ";"
           << flatJunctionsRight.back().second << "] "
           << "middleBandRight=[" << middleBandsRight.back().first << ";"
           << middleBandsRight.back().second << "] "
           << "outerBandRight=[" << outerBandsRight.back().first << ";"
           << outerBandsRight.back().second << "] ";
    }
    NS_LOG_DEBUG(ss.str());
    NS_ASSERT(maskBand.second == outerBandsRight.back().second);
    NS_ASSERT(numMaskBands ==
              ((allocatedSubBandsPerSegment.back().back().second -
                allocatedSubBandsPerSegment.front().front().first +
                1) // equivalent to allocatedBand (includes notches and DC)
               + 2 * (innerSlopeWidth + middleSlopeWidth + outerSlopeWidth + flatJunctionWidth)));

    // Different slopes
    double innerSlope = (-1 * minInnerBandDbr) / innerSlopeWidth;
    double middleSlope = (-1 * (minOuterBandDbr - minInnerBandDbr)) / middleSlopeWidth;
    double outerSlope = (txPowerMiddleBandMinDbm - txPowerOuterBandMinDbm) / outerSlopeWidth;
    double puncturedSlope = (-1 * minInnerBandDbr) / puncturedSlopeWidth;

    // Build spectrum mask
    double previousTxPowerW = 0.0;
    std::vector<double> txPowerValues(numBands);
    NS_ASSERT(txPowerValues.size() == numBands);
    for (size_t i = 0; i < numBands; ++i)
    {
        size_t psdIndex =
            (allocatedSubBandsPerSegment.size() == 1) ? 0 : ((i < (numBands / 2)) ? 0 : 1);
        double txPowerW = 0.0;
        if (i < maskBand.first || i > maskBand.second) // outside the spectrum mask
        {
            txPowerW = 0.0;
        }
        else if (betweenPsdsBand.has_value() &&
                 (i <= betweenPsdsBand->second && i >= betweenPsdsBand->first))
        {
            // value for PSD mask 1
            std::vector<double> txPowerWPsds(2);
            if (i <= middleBandsRight.at(0).second && i >= middleBandsRight.at(0).first)
            {
                txPowerWPsds.at(0) =
                    DbmToW(txPowerInnerBandMinDbm -
                           ((i - middleBandsRight.at(0).first + 1) *
                            middleSlope)); // +1 so as to be symmetric with left slope
            }
            else if (i <= outerBandsRight.at(0).second && i >= outerBandsRight.at(0).first)
            {
                txPowerWPsds.at(0) =
                    DbmToW(txPowerMiddleBandMinDbm -
                           ((i - outerBandsRight.at(0).first + 1) *
                            outerSlope)); // +1 so as to be symmetric with left slope
            }
            else if (i > outerBandsRight.at(0).second)
            {
                txPowerW = DbmToW(txPowerOuterBandMinDbm);
            }
            else
            {
                NS_ASSERT(false);
            }

            // value for PSD mask 2
            if (i < outerBandsLeft.at(1).first)
            {
                txPowerW = DbmToW(txPowerOuterBandMinDbm);
            }
            else if (i <= outerBandsLeft.at(1).second && i >= outerBandsLeft.at(1).first)
            {
                txPowerWPsds.at(1) = DbmToW(txPowerOuterBandMinDbm +
                                            ((i - outerBandsLeft.at(1).first) * outerSlope));
            }
            else if (i <= middleBandsLeft.at(1).second && i >= middleBandsLeft.at(1).first)
            {
                txPowerWPsds.at(1) = DbmToW(txPowerMiddleBandMinDbm +
                                            ((i - middleBandsLeft.at(1).first) * middleSlope));
            }
            else
            {
                NS_ASSERT(false);
            }

            txPowerW = std::accumulate(txPowerWPsds.cbegin(), txPowerWPsds.cend(), 0.0);
            txPowerW = std::max(DbmToW(txPowerRefDbm - 25.0), txPowerW);
            txPowerW = std::min(DbmToW(txPowerRefDbm - 20.0), txPowerW);
        }
        else if (i <= outerBandsLeft.at(psdIndex).second &&
                 i >= outerBandsLeft.at(psdIndex)
                          .first) // better to put greater first (less computation)
        {
            txPowerW = DbmToW(txPowerOuterBandMinDbm +
                              ((i - outerBandsLeft.at(psdIndex).first) * outerSlope));
        }
        else if (i <= middleBandsLeft.at(psdIndex).second &&
                 i >= middleBandsLeft.at(psdIndex).first)
        {
            txPowerW = DbmToW(txPowerMiddleBandMinDbm +
                              ((i - middleBandsLeft.at(psdIndex).first) * middleSlope));
        }
        else if ((i <= flatJunctionsLeft.at(psdIndex).second &&
                  i >= flatJunctionsLeft.at(psdIndex).first) ||
                 (i <= flatJunctionsRight.at(psdIndex).second &&
                  i >= flatJunctionsRight.at(psdIndex).first))
        {
            txPowerW = DbmToW(txPowerInnerBandMinDbm);
        }
        else if (i <= innerBandsLeft.at(psdIndex).second && i >= innerBandsLeft.at(psdIndex).first)
        {
            txPowerW = (!puncturedBandsPerSegment.empty() &&
                        !puncturedBandsPerSegment.at(psdIndex).empty() &&
                        (puncturedBandsPerSegment.at(psdIndex).front().first <=
                         allocatedSubBandsPerSegment.at(psdIndex).front().first))
                           ? DbmToW(txPowerInnerBandMinDbm)
                           : // first 20 MHz band is punctured
                           DbmToW(txPowerInnerBandMinDbm +
                                  ((i - innerBandsLeft.at(psdIndex).first) * innerSlope));
        }
        else if ((i <= allocatedSubBandsPerSegment.at(psdIndex).back().second &&
                  i >= allocatedSubBandsPerSegment.at(psdIndex).front().first)) // roughly in
                                                                                // allocated band
        {
            bool insideSubBand = false;
            for (uint32_t j = 0; !insideSubBand && j < numSubBands;
                 j++) // continue until inside a sub-band
            {
                insideSubBand = (i <= allocatedSubBandsPerSegment.at(psdIndex)[j].second) &&
                                (i >= allocatedSubBandsPerSegment.at(psdIndex)[j].first);
            }
            if (insideSubBand)
            {
                bool insidePuncturedSubBand = false;
                std::size_t j = 0;
                std::size_t puncturedBandSize = !puncturedBandsPerSegment.empty()
                                                    ? puncturedBandsPerSegment.at(psdIndex).size()
                                                    : 0;
                for (; !insidePuncturedSubBand && j < puncturedBandSize;
                     j++) // continue until inside a punctured sub-band
                {
                    insidePuncturedSubBand =
                        (i <= puncturedBandsPerSegment.at(psdIndex).at(j).second) &&
                        (i >= puncturedBandsPerSegment.at(psdIndex).at(j).first);
                }
                if (insidePuncturedSubBand)
                {
                    uint32_t startPuncturedSlope =
                        (puncturedBandsPerSegment.at(psdIndex)
                             .at(puncturedBandsPerSegment.at(psdIndex).size() - 1)
                             .second -
                         puncturedSlopeWidth); // only consecutive subchannels can be punctured
                    if (i >= startPuncturedSlope)
                    {
                        txPowerW = DbmToW(txPowerInnerBandMinDbm +
                                          ((i - startPuncturedSlope) * puncturedSlope));
                    }
                    else
                    {
                        txPowerW = std::max(
                            DbmToW(txPowerInnerBandMinDbm),
                            DbmToW(txPowerRefDbm -
                                   ((i - puncturedBandsPerSegment.at(psdIndex).at(0).first) *
                                    puncturedSlope)));
                    }
                }
                else
                {
                    txPowerW = txPowerPerBandW;
                }
            }
            else
            {
                txPowerW = DbmToW(txPowerInnerBandMinDbm);
            }
        }
        else if (i <= innerBandsRight.at(psdIndex).second &&
                 i >= innerBandsRight.at(psdIndex).first)
        {
            // take min to handle the case where last 20 MHz band is punctured
            txPowerW = std::min(
                previousTxPowerW,
                DbmToW(txPowerRefDbm - ((i - innerBandsRight.at(psdIndex).first + 1) *
                                        innerSlope))); // +1 so as to be symmetric with left slope
        }
        else if (i <= middleBandsRight.at(psdIndex).second &&
                 i >= middleBandsRight.at(psdIndex).first)
        {
            txPowerW = DbmToW(txPowerInnerBandMinDbm -
                              ((i - middleBandsRight.at(psdIndex).first + 1) *
                               middleSlope)); // +1 so as to be symmetric with left slope
        }
        else if (i <= outerBandsRight.at(psdIndex).second &&
                 i >= outerBandsRight.at(psdIndex).first)
        {
            txPowerW = DbmToW(txPowerMiddleBandMinDbm -
                              ((i - outerBandsRight.at(psdIndex).first + 1) *
                               outerSlope)); // +1 so as to be symmetric with left slope
        }
        else
        {
            NS_FATAL_ERROR("Should have handled all cases");
        }
        double txPowerDbr = 10 * std::log10(txPowerW / txPowerPerBandW);
        NS_LOG_LOGIC(uint32_t(i) << " -> " << txPowerDbr);
        previousTxPowerW = txPowerW;
        txPowerValues.at(i) = txPowerW;
    }

    // fill in spectrum mask
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    for (auto txPowerValue : txPowerValues)
    {
        *vit = txPowerValue / (bit->fh - bit->fl);
        vit++;
        bit++;
    }

    for (const auto& allocatedSubBands : allocatedSubBandsPerSegment)
    {
        NS_LOG_INFO("Added signal power to subbands " << allocatedSubBands.front().first << "-"
                                                      << allocatedSubBands.back().second);
    }
}

void
WifiSpectrumValueHelper::NormalizeSpectrumMask(Ptr<SpectrumValue> c, double txPowerW)
{
    NS_LOG_FUNCTION(c << txPowerW);
    // Normalize power so that total signal power equals transmit power
    double currentTxPowerW = Integral(*c);
    double normalizationRatio = currentTxPowerW / txPowerW;
    NS_LOG_LOGIC("Current power: " << currentTxPowerW << "W vs expected power: " << txPowerW << "W"
                                   << " -> ratio (C/E) = " << normalizationRatio);
    auto vit = c->ValuesBegin();
    for (size_t i = 0; i < c->GetSpectrumModel()->GetNumBands(); i++, vit++)
    {
        *vit = (*vit) / normalizationRatio;
    }
}

double
WifiSpectrumValueHelper::DbmToW(double dBm)
{
    return std::pow(10.0, 0.1 * (dBm - 30.0));
}

double
WifiSpectrumValueHelper::GetBandPowerW(Ptr<SpectrumValue> psd, const WifiSpectrumBandIndices& band)
{
    auto valueIt = psd->ConstValuesBegin() + band.first;
    auto end = psd->ConstValuesBegin() + band.second;
    auto bandIt = psd->ConstBandsBegin() + band.first; // all bands have same width
    const auto bandWidth = (bandIt->fh - bandIt->fl);
    NS_ASSERT_MSG(bandWidth >= 0.0,
                  "Invalid width for subband [" << bandIt->fl << ";" << bandIt->fh << "]");
    uint32_t index [[maybe_unused]] = 0;
    auto powerWattPerHertz{0.0};
    while (valueIt <= end)
    {
        NS_ASSERT_MSG(*valueIt >= 0.0,
                      "Invalid power value " << *valueIt << " in subband " << index);
        powerWattPerHertz += *valueIt;
        ++valueIt;
        ++index;
    }
    const auto power = powerWattPerHertz * bandWidth;
    NS_ASSERT_MSG(power >= 0.0,
                  "Invalid calculated power " << power << " for band [" << band.first << ";"
                                              << band.second << "]");
    return power;
}

bool
operator<(const FrequencyRange& left, const FrequencyRange& right)
{
    return left.minFrequency < right.minFrequency;
}

bool
operator==(const FrequencyRange& left, const FrequencyRange& right)
{
    return (left.minFrequency == right.minFrequency) && (left.maxFrequency == right.maxFrequency);
}

bool
operator!=(const FrequencyRange& left, const FrequencyRange& right)
{
    return !(left == right);
}

std::ostream&
operator<<(std::ostream& os, const FrequencyRange& freqRange)
{
    os << "[" << freqRange.minFrequency << " MHz - " << freqRange.maxFrequency << " MHz]";
    return os;
}

} // namespace ns3
