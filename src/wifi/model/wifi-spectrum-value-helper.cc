/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 * Copyright (c) 2017 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Giuseppe Piro  <g.piro@poliba.it>
 *          Rediet <getachew.redieteab@orange.com>
 */

#include "wifi-spectrum-value-helper.h"

#include "wifi-utils.h"

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
auto printFrequencies = [](const std::vector<ns3::MHz_u>& v) {
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
    std::vector<MHz_u> centerFrequencies; ///< center frequency per segment
    MHz_u channelWidth;                   ///< channel width
    Hz_u carrierSpacing;                  ///< carrier spacing
    MHz_u guardBandwidth;                 ///< guard band width
};

/**
 * Less than operator
 * @param lhs the left hand side wifi spectrum to compare
 * @param rhs the right hand side wifi spectrum to compare
 * @returns true if the left hand side spectrum is less than the right hand side spectrum
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
WifiSpectrumValueHelper::GetSpectrumModel(const std::vector<MHz_u>& centerFrequencies,
                                          MHz_u channelWidth,
                                          Hz_u carrierSpacing,
                                          MHz_u guardBandwidth)
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
    // assume all frequency segments have the same width, hence split the guard bandwidth
    // accordingly over the segments
    guardBandwidth /= centerFrequencies.size();
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
                ? MHz_u{0}
                : (*maxCenterFrequency - *minCenterFrequency - (channelWidth / 2));
        NS_ASSERT(separationWidth == MHz_u{0} || centerFrequencies.size() == 2);
        const auto bandwidth = MHzToHz((channelWidth + (2 * guardBandwidth) + separationWidth));
        // For OFDM, the center subcarrier is null (at center frequency)
        uint32_t numBands = std::ceil(bandwidth / carrierSpacing);
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
        const auto startingFrequency = MHzToHz(*minCenterFrequency) -
                                       (MHzToHz(channelWidth) / (2 * centerFrequencies.size())) -
                                       MHzToHz(guardBandwidth) - (carrierSpacing / 2);
        for (size_t i = 0; i < numBands; i++)
        {
            BandInfo info;
            auto f = startingFrequency + (i * carrierSpacing);
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
WifiSpectrumValueHelper::CreateDsssTxPowerSpectralDensity(MHz_u centerFrequency,
                                                          Watt_u txPower,
                                                          MHz_u guardBandwidth)
{
    NS_LOG_FUNCTION(centerFrequency << txPower << +guardBandwidth);
    MHz_u channelWidth{22}; // DSSS channels are 22 MHz wide
    Hz_u carrierSpacing{312500};
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel({centerFrequency}, channelWidth, carrierSpacing, guardBandwidth));
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    auto nGuardBands =
        static_cast<uint32_t>(((MHzToHz(2 * guardBandwidth)) / carrierSpacing) + 0.5);
    auto nAllocatedBands = static_cast<uint32_t>((MHzToHz(channelWidth) / carrierSpacing) + 0.5);
    NS_ASSERT(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1));
    // Evenly spread power across 22 MHz
    const auto txPowerPerBand = txPower / nAllocatedBands;
    const auto psd = txPowerPerBand / (bit->fh - bit->fl);
    for (size_t i = 0; i < c->GetSpectrumModel()->GetNumBands(); i++, vit++, bit++)
    {
        if ((i >= (nGuardBands / 2)) && (i <= ((nGuardBands / 2) + nAllocatedBands - 1)))
        {
            *vit = psd;
        }
    }
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(MHz_u centerFrequency,
                                                          MHz_u channelWidth,
                                                          Watt_u txPower,
                                                          MHz_u guardBandwidth,
                                                          dBr_u minInnerBand,
                                                          dBr_u minOuterBand,
                                                          dBr_u lowestPoint)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPower << guardBandwidth << minInnerBand
                                    << minOuterBand << lowestPoint);
    Hz_u carrierSpacing{0};
    uint32_t innerSlopeWidth = 0;
    switch (static_cast<uint16_t>(channelWidth))
    {
    case 20:
        carrierSpacing = Hz_u{312500};
        innerSlopeWidth =
            static_cast<uint32_t>((Hz_u{2e6} / carrierSpacing) + 0.5); // [-11;-9] & [9;11]
        break;
    case 10:
        carrierSpacing = Hz_u{156250};
        innerSlopeWidth =
            static_cast<uint32_t>((Hz_u{1e6} / carrierSpacing) + 0.5); // [-5.5;-4.5] & [4.5;5.5]
        break;
    case 5:
        carrierSpacing = Hz_u{78125};
        innerSlopeWidth =
            static_cast<uint32_t>((Hz_u{5e5} / carrierSpacing) + 0.5); // [-2.75;-2.5] & [2.5;2.75]
        break;
    default:
        NS_FATAL_ERROR("Channel width " << channelWidth << " should be correctly set.");
        return nullptr;
    }

    auto c = Create<SpectrumValue>(
        GetSpectrumModel({centerFrequency}, channelWidth, carrierSpacing, guardBandwidth));
    auto nGuardBands =
        static_cast<uint32_t>(((MHzToHz(2 * guardBandwidth)) / carrierSpacing) + 0.5);
    auto nAllocatedBands = static_cast<uint32_t>((MHzToHz(channelWidth) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    // 52 subcarriers (48 data + 4 pilot)
    // skip guard band and 6 subbands, then place power in 26 subbands, then
    // skip the center subband, then place power in 26 subbands, then skip
    // the final 6 subbands and the guard band.
    const auto txPowerPerBand = txPower / 52;
    NS_LOG_DEBUG("Power per band " << txPowerPerBand << "W");
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
                              txPowerPerBand,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBand,
                              minOuterBand,
                              lowestPoint);
    NormalizeSpectrumMask(c, txPower);
    NS_ASSERT_MSG(std::abs(txPower - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateDuplicated20MhzTxPowerSpectralDensity(
    const std::vector<MHz_u>& centerFrequencies,
    MHz_u channelWidth,
    Watt_u txPower,
    MHz_u guardBandwidth,
    dBr_u minInnerBand,
    dBr_u minOuterBand,
    dBr_u lowestPoint,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_ASSERT_MSG(centerFrequencies.size() == 1 ||
                      (channelWidth == MHz_u{160} && centerFrequencies.size() <= 2),
                  "PSD for non-contiguous channels is only possible when the total width is 160 "
                  "MHz and cannot be made of more than 2 segments");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPower << guardBandwidth << minInnerBand << minOuterBand
                    << lowestPoint);
    const Hz_u carrierSpacing{312500};
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    // assume all frequency segments have the same width, hence split the guard bandwidth
    // accordingly over the segments
    guardBandwidth /= centerFrequencies.size();
    const auto nGuardBands =
        static_cast<uint32_t>(((MHzToHz(2 * guardBandwidth)) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>((MHzToHz(channelWidth) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth =
        separationWidth > Hz_u{0} ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>((MHzToHz(unallocatedWidth) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    auto num20MhzBands = Count20MHzSubchannels(channelWidth);
    std::size_t numAllocatedSubcarriersPer20MHz = 52;
    NS_ASSERT(puncturedSubchannels.empty() || (puncturedSubchannels.size() == num20MhzBands));
    const auto txPowerPerBand = (txPower / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBand << "W");

    std::size_t numSubcarriersPer20MHz = MHzToHz(MHz_u{20}) / carrierSpacing;
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
        (Hz_u{2e6} / carrierSpacing) +
        0.5); // size in number of subcarriers of the 0dBr<->20dBr slope (2MHz for HT/VHT)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);
    const auto puncturedSlopeWidth =
        static_cast<uint32_t>((Hz_u{500e3} / carrierSpacing) +
                              0.5); // size in number of subcarriers of the punctured slope band

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBandsPerSegment,
                              maskBand,
                              txPowerPerBand,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBand,
                              minOuterBand,
                              lowestPoint,
                              puncturedBandsPerSegment,
                              puncturedSlopeWidth);
    NormalizeSpectrumMask(c, txPower);
    NS_ASSERT_MSG(std::abs(txPower - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(
    const std::vector<MHz_u>& centerFrequencies,
    MHz_u channelWidth,
    Watt_u txPower,
    MHz_u guardBandwidth,
    dBr_u minInnerBand,
    dBr_u minOuterBand,
    dBr_u lowestPoint)
{
    NS_ASSERT_MSG(centerFrequencies.size() == 1 ||
                      (channelWidth == MHz_u{160} && centerFrequencies.size() <= 2),
                  "PSD for non-contiguous channels is only possible when the total width is 160 "
                  "MHz and cannot be made of more than 2 segments");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPower << guardBandwidth << minInnerBand << minOuterBand
                    << lowestPoint);
    const Hz_u carrierSpacing{312500};
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    // assume all frequency segments have the same width, hence split the guard bandwidth
    // accordingly over the segments
    guardBandwidth /= centerFrequencies.size();
    const auto nGuardBands =
        static_cast<uint32_t>(((MHzToHz(2 * guardBandwidth)) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>((MHzToHz(channelWidth) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth =
        separationWidth > Hz_u{0} ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>((MHzToHz(unallocatedWidth) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    auto num20MhzBands = Count20MHzSubchannels(channelWidth);
    std::size_t numAllocatedSubcarriersPer20MHz = 56;
    const auto txPowerPerBand = (txPower / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBand << "W");

    std::size_t numSubcarriersPer20MHz = MHzToHz(MHz_u{20}) / carrierSpacing;
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
        (Hz_u{2e6} / carrierSpacing) +
        0.5); // size in number of subcarriers of the inner band (2MHz for HT/VHT)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBandsPerSegment,
                              maskBand,
                              txPowerPerBand,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBand,
                              minOuterBand,
                              lowestPoint);
    NormalizeSpectrumMask(c, txPower);
    NS_ASSERT_MSG(std::abs(txPower - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
    MHz_u centerFrequency,
    MHz_u channelWidth,
    Watt_u txPower,
    MHz_u guardBandwidth,
    dBr_u minInnerBand,
    dBr_u minOuterBand,
    dBr_u lowestPoint,
    const std::vector<bool>& puncturedSubchannels)
{
    return CreateHeOfdmTxPowerSpectralDensity(std::vector<MHz_u>{centerFrequency},
                                              channelWidth,
                                              txPower,
                                              guardBandwidth,
                                              minInnerBand,
                                              minOuterBand,
                                              lowestPoint,
                                              puncturedSubchannels);
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
    const std::vector<MHz_u>& centerFrequencies,
    MHz_u channelWidth,
    Watt_u txPower,
    MHz_u guardBandwidth,
    dBr_u minInnerBand,
    dBr_u minOuterBand,
    dBr_u lowestPoint,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_ASSERT_MSG(
        centerFrequencies.size() == 1 || channelWidth == MHz_u{160},
        "PSD for non-contiguous channels is only possible when the total width is 160 MHz");
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPower << guardBandwidth << minInnerBand << minOuterBand
                    << lowestPoint);
    const Hz_u carrierSpacing{78125};
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));
    // assume all frequency segments have the same width, hence split the guard bandwidth
    // accordingly over the segments
    guardBandwidth /= centerFrequencies.size();
    const auto nGuardBands =
        static_cast<uint32_t>(((MHzToHz(2 * guardBandwidth)) / carrierSpacing) + 0.5);
    const auto separationWidth = std::abs(centerFrequencies.back() - centerFrequencies.front());
    const auto unallocatedWidth =
        separationWidth > Hz_u{0} ? (separationWidth - (channelWidth / 2)) : 0;
    const auto nUnallocatedBands =
        static_cast<uint32_t>((MHzToHz(unallocatedWidth) / carrierSpacing) + 0.5);
    const auto nAllocatedBands =
        static_cast<uint32_t>((MHzToHz(channelWidth) / carrierSpacing) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() ==
                      (nAllocatedBands + nGuardBands + nUnallocatedBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    Watt_u txPowerPerBand{0.0};
    uint32_t start1;
    uint32_t stop1;
    uint32_t start2;
    uint32_t stop2;
    uint32_t start3;
    uint32_t stop3;
    uint32_t start4;
    uint32_t stop4;
    // Prepare spectrum mask specific variables
    auto innerSlopeWidth =
        static_cast<uint32_t>((MHzToHz(MHz_u{1}) / carrierSpacing) +
                              0.5); // size in number of subcarriers of the inner band
    std::vector<std::vector<WifiSpectrumBandIndices>> subBandsPerSegment(
        centerFrequencies.size()); // list of data/pilot-containing subBands (sent at 0dBr)
    WifiSpectrumBandIndices maskBand(0, nAllocatedBands + nGuardBands + nUnallocatedBands);
    switch (static_cast<uint16_t>(channelWidth))
    {
    case 20:
        // 242 subcarriers (234 data + 8 pilot)
        txPowerPerBand = txPower / 242;
        innerSlopeWidth = static_cast<uint32_t>((Hz_u{5e5} / carrierSpacing) +
                                                0.5); // [-10.25;-9.75] & [9.75;10.25]
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
        txPowerPerBand = txPower / 484;
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
        txPowerPerBand = txPower / 996;
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
        txPowerPerBand = txPower / (2 * 996);
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
        static_cast<uint32_t>((Hz_u{500e3} / carrierSpacing) +
                              0.5); // size in number of subcarriers of the punctured slope band
    std::vector<std::vector<WifiSpectrumBandIndices>> puncturedBandsPerSegment;
    std::size_t subcarriersPerSuband = (MHzToHz(MHz_u{20}) / carrierSpacing);
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
                              txPowerPerBand,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBand,
                              minOuterBand,
                              lowestPoint,
                              puncturedBandsPerSegment,
                              puncturedSlopeWidth);
    NormalizeSpectrumMask(c, txPower);
    NS_ASSERT_MSG(std::abs(txPower - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity(
    const std::vector<MHz_u>& centerFrequencies,
    MHz_u channelWidth,
    Watt_u txPower,
    MHz_u guardBandwidth,
    const std::vector<WifiSpectrumBandIndices>& ru)
{
    auto printRuIndices = [](const std::vector<WifiSpectrumBandIndices>& v) {
        std::stringstream ss;
        for (const auto& [start, stop] : v)
        {
            ss << start << "-" << stop << " ";
        }
        return ss.str();
    };
    NS_LOG_FUNCTION(printFrequencies(centerFrequencies)
                    << channelWidth << txPower << guardBandwidth << printRuIndices(ru));
    const Hz_u carrierSpacing{78125};
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequencies, channelWidth, carrierSpacing, guardBandwidth));

    // Build spectrum mask
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    const auto numSubcarriers =
        std::accumulate(ru.cbegin(), ru.cend(), 0, [](uint32_t sum, const auto& p) {
            return sum + (p.second - p.first) + 1;
        });
    const auto txPowerPerBand = (txPower / numSubcarriers); // FIXME: null subcarriers
    uint32_t numBands = c->GetSpectrumModel()->GetNumBands();
    const auto psd = txPowerPerBand / (bit->fh - bit->fl);
    for (size_t i = 0; i < numBands; i++, vit++, bit++)
    {
        const auto allocated = std::any_of(ru.cbegin(), ru.cend(), [i](const auto& p) {
            return (i >= p.first && i <= p.second);
        });
        *vit = allocated ? psd : 0.0;
    }

    return c;
}

void
WifiSpectrumValueHelper::CreateSpectrumMaskForOfdm(
    Ptr<SpectrumValue> c,
    const std::vector<std::vector<WifiSpectrumBandIndices>>& allocatedSubBandsPerSegment,
    const WifiSpectrumBandIndices& maskBand,
    Watt_u txPowerPerBand,
    uint32_t nGuardBands,
    uint32_t innerSlopeWidth,
    dBr_u minInnerBand,
    dBr_u minOuterBand,
    dBr_u lowestPoint,
    const std::vector<std::vector<WifiSpectrumBandIndices>>& puncturedBandsPerSegment,
    uint32_t puncturedSlopeWidth)
{
    NS_ASSERT_MSG(allocatedSubBandsPerSegment.size() <= 2,
                  "Only PSDs for up to 2 frequency segments are supported");
    NS_ASSERT(puncturedBandsPerSegment.empty() ||
              (puncturedBandsPerSegment.size() == allocatedSubBandsPerSegment.size()));
    NS_LOG_FUNCTION(c << allocatedSubBandsPerSegment.front().front().first
                      << allocatedSubBandsPerSegment.front().back().second << maskBand.first
                      << maskBand.second << txPowerPerBand << nGuardBands << innerSlopeWidth
                      << minInnerBand << minOuterBand << lowestPoint << puncturedSlopeWidth);
    uint32_t numSubBands = allocatedSubBandsPerSegment.front().size();
    uint32_t numBands = c->GetSpectrumModel()->GetNumBands();
    uint32_t numMaskBands = maskBand.second - maskBand.first + 1;
    NS_ASSERT(numSubBands && numBands && numMaskBands);
    NS_LOG_LOGIC("Power per band " << txPowerPerBand << "W");

    // Different power levels
    dBm_u txPowerRef{10.0 * std::log10(txPowerPerBand * 1000.0)};
    dBm_u txPowerInnerBandMin{txPowerRef + minInnerBand};
    dBm_u txPowerMiddleBandMin{txPowerRef + minOuterBand};
    dBm_u txPowerOuterBandMin{txPowerRef +
                              lowestPoint}; // TODO also take into account dBm/MHz constraints

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
    double innerSlope = (-1.0 * minInnerBand) / innerSlopeWidth;
    double middleSlope = (-1.0 * (minOuterBand - minInnerBand)) / middleSlopeWidth;
    double outerSlope = (txPowerMiddleBandMin - txPowerOuterBandMin) / outerSlopeWidth;
    double puncturedSlope = (-1.0 * minInnerBand) / puncturedSlopeWidth;

    // Build spectrum mask
    Watt_u previousTxPower{0.0};
    std::vector<Watt_u> txPowerValues(numBands);
    NS_ASSERT(txPowerValues.size() == numBands);
    for (size_t i = 0; i < numBands; ++i)
    {
        size_t psdIndex =
            (allocatedSubBandsPerSegment.size() == 1) ? 0 : ((i < (numBands / 2)) ? 0 : 1);
        Watt_u txPower{0.0};
        if (i < maskBand.first || i > maskBand.second) // outside the spectrum mask
        {
            txPower = Watt_u{0.0};
        }
        else if (betweenPsdsBand.has_value() &&
                 (i <= betweenPsdsBand->second && i >= betweenPsdsBand->first))
        {
            // value for PSD mask 1
            std::vector<Watt_u> txPowerWPsds(2);
            if (i <= middleBandsRight.at(0).second && i >= middleBandsRight.at(0).first)
            {
                txPowerWPsds.at(0) =
                    DbmToW(txPowerInnerBandMin -
                           ((i - middleBandsRight.at(0).first + 1) *
                            middleSlope)); // +1 so as to be symmetric with left slope
            }
            else if (i <= outerBandsRight.at(0).second && i >= outerBandsRight.at(0).first)
            {
                txPowerWPsds.at(0) =
                    DbmToW(txPowerMiddleBandMin -
                           ((i - outerBandsRight.at(0).first + 1) *
                            outerSlope)); // +1 so as to be symmetric with left slope
            }
            else if (i > outerBandsRight.at(0).second)
            {
                txPower = DbmToW(txPowerOuterBandMin);
            }
            else
            {
                NS_ASSERT(false);
            }

            // value for PSD mask 2
            if (i < outerBandsLeft.at(1).first)
            {
                txPower = DbmToW(txPowerOuterBandMin);
            }
            else if (i <= outerBandsLeft.at(1).second && i >= outerBandsLeft.at(1).first)
            {
                txPowerWPsds.at(1) =
                    DbmToW(txPowerOuterBandMin + ((i - outerBandsLeft.at(1).first) * outerSlope));
            }
            else if (i <= middleBandsLeft.at(1).second && i >= middleBandsLeft.at(1).first)
            {
                txPowerWPsds.at(1) = DbmToW(txPowerMiddleBandMin +
                                            ((i - middleBandsLeft.at(1).first) * middleSlope));
            }
            else
            {
                NS_ASSERT(false);
            }

            txPower = std::accumulate(txPowerWPsds.cbegin(), txPowerWPsds.cend(), Watt_u{0.0});
            txPower = std::max(DbmToW(txPowerRef - 25.0), txPower);
            txPower = std::min(DbmToW(txPowerRef - 20.0), txPower);
        }
        else if (i <= outerBandsLeft.at(psdIndex).second &&
                 i >= outerBandsLeft.at(psdIndex)
                          .first) // better to put greater first (less computation)
        {
            txPower = DbmToW(txPowerOuterBandMin +
                             ((i - outerBandsLeft.at(psdIndex).first) * outerSlope));
        }
        else if (i <= middleBandsLeft.at(psdIndex).second &&
                 i >= middleBandsLeft.at(psdIndex).first)
        {
            txPower = DbmToW(txPowerMiddleBandMin +
                             ((i - middleBandsLeft.at(psdIndex).first) * middleSlope));
        }
        else if ((i <= flatJunctionsLeft.at(psdIndex).second &&
                  i >= flatJunctionsLeft.at(psdIndex).first) ||
                 (i <= flatJunctionsRight.at(psdIndex).second &&
                  i >= flatJunctionsRight.at(psdIndex).first))
        {
            txPower = DbmToW(txPowerInnerBandMin);
        }
        else if (i <= innerBandsLeft.at(psdIndex).second && i >= innerBandsLeft.at(psdIndex).first)
        {
            txPower = (!puncturedBandsPerSegment.empty() &&
                       !puncturedBandsPerSegment.at(psdIndex).empty() &&
                       (puncturedBandsPerSegment.at(psdIndex).front().first <=
                        allocatedSubBandsPerSegment.at(psdIndex).front().first))
                          ? DbmToW(txPowerInnerBandMin)
                          : // first 20 MHz band is punctured
                          DbmToW(txPowerInnerBandMin +
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
                        txPower = DbmToW(txPowerInnerBandMin +
                                         ((i - startPuncturedSlope) * puncturedSlope));
                    }
                    else
                    {
                        txPower = std::max(
                            DbmToW(txPowerInnerBandMin),
                            DbmToW(txPowerRef -
                                   ((i - puncturedBandsPerSegment.at(psdIndex).at(0).first) *
                                    puncturedSlope)));
                    }
                }
                else
                {
                    txPower = txPowerPerBand;
                }
            }
            else
            {
                txPower = DbmToW(txPowerInnerBandMin);
            }
        }
        else if (i <= innerBandsRight.at(psdIndex).second &&
                 i >= innerBandsRight.at(psdIndex).first)
        {
            // take min to handle the case where last 20 MHz band is punctured
            txPower = std::min(
                previousTxPower,
                DbmToW(txPowerRef - ((i - innerBandsRight.at(psdIndex).first + 1) *
                                     innerSlope))); // +1 so as to be symmetric with left slope
        }
        else if (i <= middleBandsRight.at(psdIndex).second &&
                 i >= middleBandsRight.at(psdIndex).first)
        {
            txPower = DbmToW(txPowerInnerBandMin -
                             ((i - middleBandsRight.at(psdIndex).first + 1) *
                              middleSlope)); // +1 so as to be symmetric with left slope
        }
        else if (i <= outerBandsRight.at(psdIndex).second &&
                 i >= outerBandsRight.at(psdIndex).first)
        {
            txPower = DbmToW(txPowerMiddleBandMin -
                             ((i - outerBandsRight.at(psdIndex).first + 1) *
                              outerSlope)); // +1 so as to be symmetric with left slope
        }
        else
        {
            NS_FATAL_ERROR("Should have handled all cases");
        }
        NS_LOG_LOGIC(i << " -> " << (10 * std::log10(txPower / txPowerPerBand)));
        previousTxPower = txPower;
        txPowerValues.at(i) = txPower;
    }

    // fill in spectrum mask
    auto vit = c->ValuesBegin();
    auto bit = c->ConstBandsBegin();
    const auto invBandwidth = 1 / (bit->fh - bit->fl);
    for (auto txPowerValue : txPowerValues)
    {
        *vit = txPowerValue * invBandwidth;
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
WifiSpectrumValueHelper::NormalizeSpectrumMask(Ptr<SpectrumValue> c, Watt_u txPower)
{
    NS_LOG_FUNCTION(c << txPower);
    // Normalize power so that total signal power equals transmit power
    Watt_u currentTxPower{Integral(*c)};
    double normalizationRatio [[maybe_unused]] = currentTxPower / txPower;
    double invNormalizationRatio = txPower / currentTxPower;
    NS_LOG_LOGIC("Current power: " << currentTxPower << "W vs expected power: " << txPower << "W"
                                   << " -> ratio (C/E) = " << normalizationRatio);
    auto vit = c->ValuesBegin();
    for (size_t i = 0; i < c->GetSpectrumModel()->GetNumBands(); i++, vit++)
    {
        *vit = (*vit) * invNormalizationRatio;
    }
}

Watt_u
WifiSpectrumValueHelper::GetBandPowerW(Ptr<SpectrumValue> psd,
                                       const std::vector<WifiSpectrumBandIndices>& segments)
{
    auto powerWattPerHertz{0.0};
    auto bandIt = psd->ConstBandsBegin() + segments.front().first; // all bands have same width
    const auto bandWidth = (bandIt->fh - bandIt->fl);
    NS_ASSERT_MSG(bandWidth >= 0.0,
                  "Invalid width for subband [" << bandIt->fl << ";" << bandIt->fh << "]");
    for (const auto& [start, stop] : segments)
    {
        auto valueIt = psd->ConstValuesBegin() + start;
        auto end = psd->ConstValuesBegin() + stop;
        uint32_t index [[maybe_unused]] = 0;
        while (valueIt <= end)
        {
            NS_ASSERT_MSG(*valueIt >= 0.0,
                          "Invalid power value " << *valueIt << " in subband " << index);
            powerWattPerHertz += *valueIt;
            ++valueIt;
            ++index;
        }
    }
    const Watt_u power{powerWattPerHertz * bandWidth};
    NS_ASSERT_MSG(power >= 0.0, "Invalid calculated power " << power);
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
