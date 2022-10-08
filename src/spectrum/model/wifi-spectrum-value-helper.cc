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

#include <cmath>
#include <map>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiSpectrumValueHelper");

///< Wifi Spectrum Model structure
struct WifiSpectrumModelId
{
    /**
     * Constructor
     * \param f the frequency (in MHz)
     * \param w the channel width (in MHz)
     * \param b the width of each band (in Hz)
     * \param g the guard band width (in MHz)
     */
    WifiSpectrumModelId(uint32_t f, uint16_t w, double b, uint16_t g);
    uint32_t m_centerFrequency; ///< center frequency (in MHz)
    uint16_t m_channelWidth;    ///< channel width (in MHz)
    double m_bandBandwidth;     ///< width of each band (in Hz)
    uint16_t m_guardBandwidth;  ///< guard band width (in MHz)
};

WifiSpectrumModelId::WifiSpectrumModelId(uint32_t f, uint16_t w, double b, uint16_t g)
    : m_centerFrequency(f),
      m_channelWidth(w),
      m_bandBandwidth(b),
      m_guardBandwidth(g)
{
    NS_LOG_FUNCTION(this << f << w << b << g);
}

/**
 * Less than operator
 * \param a the first wifi spectrum to compare
 * \param b the second wifi spectrum to compare
 * \returns true if the first spectrum is less than the second spectrum
 */
bool
operator<(const WifiSpectrumModelId& a, const WifiSpectrumModelId& b)
{
    return (
        (a.m_centerFrequency < b.m_centerFrequency) ||
        ((a.m_centerFrequency == b.m_centerFrequency) && (a.m_channelWidth < b.m_channelWidth)) ||
        ((a.m_centerFrequency == b.m_centerFrequency) && (a.m_channelWidth == b.m_channelWidth) &&
         (a.m_bandBandwidth < b.m_bandBandwidth)) // to cover coexistence of 11ax with legacy case
        || ((a.m_centerFrequency == b.m_centerFrequency) &&
            (a.m_channelWidth == b.m_channelWidth) && (a.m_bandBandwidth == b.m_bandBandwidth) &&
            (a.m_guardBandwidth <
             b.m_guardBandwidth))); // to cover 2.4 GHz case, where DSSS coexists with OFDM
}

static std::map<WifiSpectrumModelId, Ptr<SpectrumModel>>
    g_wifiSpectrumModelMap; ///< static initializer for the class

Ptr<SpectrumModel>
WifiSpectrumValueHelper::GetSpectrumModel(uint32_t centerFrequency,
                                          uint16_t channelWidth,
                                          uint32_t bandBandwidth,
                                          uint16_t guardBandwidth)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << bandBandwidth << guardBandwidth);
    Ptr<SpectrumModel> ret;
    WifiSpectrumModelId key(centerFrequency, channelWidth, bandBandwidth, guardBandwidth);
    std::map<WifiSpectrumModelId, Ptr<SpectrumModel>>::iterator it =
        g_wifiSpectrumModelMap.find(key);
    if (it != g_wifiSpectrumModelMap.end())
    {
        ret = it->second;
    }
    else
    {
        Bands bands;
        double centerFrequencyHz = centerFrequency * 1e6;
        double bandwidth = (channelWidth + (2.0 * guardBandwidth)) * 1e6;
        // For OFDM, the center subcarrier is null (at center frequency)
        uint32_t numBands = static_cast<uint32_t>((bandwidth / bandBandwidth) + 0.5);
        NS_ASSERT(numBands > 0);
        if (numBands % 2 == 0)
        {
            // round up to the nearest odd number of subbands so that bands
            // are symmetric around center frequency
            numBands += 1;
        }
        NS_ASSERT_MSG(numBands % 2 == 1, "Number of bands should be odd");
        NS_LOG_DEBUG("Num bands " << numBands << " band bandwidth " << bandBandwidth);
        // lay down numBands/2 bands symmetrically around center frequency
        // and place an additional band at center frequency
        double startingFrequencyHz =
            centerFrequencyHz - (numBands / 2 * bandBandwidth) - bandBandwidth / 2;
        for (size_t i = 0; i < numBands; i++)
        {
            BandInfo info;
            double f = startingFrequencyHz + (i * bandBandwidth);
            info.fl = f;
            f += bandBandwidth / 2;
            info.fc = f;
            f += bandBandwidth / 2;
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
WifiSpectrumValueHelper::CreateDsssTxPowerSpectralDensity(uint32_t centerFrequency,
                                                          double txPowerW,
                                                          uint16_t guardBandwidth)
{
    NS_LOG_FUNCTION(centerFrequency << txPowerW << +guardBandwidth);
    uint16_t channelWidth = 22; // DSSS channels are 22 MHz wide
    uint32_t bandBandwidth = 312500;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));
    Values::iterator vit = c->ValuesBegin();
    Bands::const_iterator bit = c->ConstBandsBegin();
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / bandBandwidth) + 0.5);
    uint32_t nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / bandBandwidth) + 0.5);
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
WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(uint32_t centerFrequency,
                                                          uint16_t channelWidth,
                                                          double txPowerW,
                                                          uint16_t guardBandwidth,
                                                          double minInnerBandDbr,
                                                          double minOuterBandDbr,
                                                          double lowestPointDbr)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                                    << minOuterBandDbr << lowestPointDbr);
    uint32_t bandBandwidth = 0;
    uint32_t innerSlopeWidth = 0;
    switch (channelWidth)
    {
    case 20:
        bandBandwidth = 312500;
        innerSlopeWidth = static_cast<uint32_t>((2e6 / bandBandwidth) + 0.5); // [-11;-9] & [9;11]
        break;
    case 10:
        bandBandwidth = 156250;
        innerSlopeWidth =
            static_cast<uint32_t>((1e6 / bandBandwidth) + 0.5); // [-5.5;-4.5] & [4.5;5.5]
        break;
    case 5:
        bandBandwidth = 78125;
        innerSlopeWidth =
            static_cast<uint32_t>((5e5 / bandBandwidth) + 0.5); // [-2.75;-2.5] & [2.5;2.75]
        break;
    default:
        NS_FATAL_ERROR("Channel width " << channelWidth << " should be correctly set.");
        return nullptr;
    }

    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / bandBandwidth) + 0.5);
    uint32_t nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / bandBandwidth) + 0.5);
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
    std::vector<WifiSpectrumBand> subBands{
        std::make_pair(start1, stop1),
        std::make_pair(start2, stop2),
    };
    WifiSpectrumBand maskBand(0, nAllocatedBands + nGuardBands);
    CreateSpectrumMaskForOfdm(c,
                              subBands,
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
    uint32_t centerFrequency,
    uint16_t channelWidth,
    double txPowerW,
    uint16_t guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                                    << minOuterBandDbr << lowestPointDbr);
    uint32_t bandBandwidth = 312500;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / bandBandwidth) + 0.5);
    uint32_t nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / bandBandwidth) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    std::size_t num20MhzBands = channelWidth / 20;
    std::size_t numAllocatedSubcarriersPer20MHz = 52;
    NS_ASSERT(puncturedSubchannels.empty() || (puncturedSubchannels.size() == num20MhzBands));
    double txPowerPerBandW = (txPowerW / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBandW << "W");

    std::size_t numSubcarriersPer20MHz = (20 * 1e6) / bandBandwidth;
    std::size_t numUnallocatedSubcarriersPer20MHz =
        numSubcarriersPer20MHz - numAllocatedSubcarriersPer20MHz;
    std::vector<WifiSpectrumBand> subBands; // list of data/pilot-containing subBands (sent at 0dBr)
    subBands.resize(num20MhzBands *
                    2); // the center subcarrier is skipped, hence 2 subbands per 20 MHz subchannel
    uint32_t start = (nGuardBands / 2) + (numUnallocatedSubcarriersPer20MHz / 2);
    uint32_t stop;
    uint8_t index = 0;
    for (auto it = subBands.begin(); it != subBands.end();)
    {
        if (!puncturedSubchannels.empty() && puncturedSubchannels.at(index++))
        {
            // if subchannel is punctured, skip it and go the next one
            NS_LOG_DEBUG("20 MHz subchannel " << +index << " is punctured");
            it += 2;
            continue;
        }
        stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
        *it = std::make_pair(start, stop);
        ++it;
        start = stop + 2; // skip center subcarrier
        stop = start + (numAllocatedSubcarriersPer20MHz / 2) - 1;
        *it = std::make_pair(start, stop);
        ++it;
        start = stop + numUnallocatedSubcarriersPer20MHz;
    }

    // Prepare spectrum mask specific variables
    uint32_t innerSlopeWidth = static_cast<uint32_t>(
        (2e6 / bandBandwidth) +
        0.5); // size in number of subcarriers of the 0dBr<->20dBr slope (2MHz for HT/VHT)
    WifiSpectrumBand maskBand(0, nAllocatedBands + nGuardBands);

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBands,
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
WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity(uint32_t centerFrequency,
                                                            uint16_t channelWidth,
                                                            double txPowerW,
                                                            uint16_t guardBandwidth,
                                                            double minInnerBandDbr,
                                                            double minOuterBandDbr,
                                                            double lowestPointDbr)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                                    << minOuterBandDbr << lowestPointDbr);
    uint32_t bandBandwidth = 312500;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / bandBandwidth) + 0.5);
    uint32_t nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / bandBandwidth) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1),
                  "Unexpected number of bands " << c->GetSpectrumModel()->GetNumBands());
    std::size_t num20MhzBands = channelWidth / 20;
    std::size_t numAllocatedSubcarriersPer20MHz = 56;
    double txPowerPerBandW = (txPowerW / numAllocatedSubcarriersPer20MHz) / num20MhzBands;
    NS_LOG_DEBUG("Power per band " << txPowerPerBandW << "W");

    std::size_t numSubcarriersPer20MHz = (20 * 1e6) / bandBandwidth;
    std::size_t numUnallocatedSubcarriersPer20MHz =
        numSubcarriersPer20MHz - numAllocatedSubcarriersPer20MHz;
    std::vector<WifiSpectrumBand> subBands; // list of data/pilot-containing subBands (sent at 0dBr)
    subBands.resize(num20MhzBands *
                    2); // the center subcarrier is skipped, hence 2 subbands per 20 MHz subchannel
    uint32_t start = (nGuardBands / 2) + (numUnallocatedSubcarriersPer20MHz / 2);
    uint32_t stop;
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

    // Prepare spectrum mask specific variables
    uint32_t innerSlopeWidth = static_cast<uint32_t>(
        (2e6 / bandBandwidth) +
        0.5); // size in number of subcarriers of the inner band (2MHz for HT/VHT)
    WifiSpectrumBand maskBand(0, nAllocatedBands + nGuardBands);

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBands,
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
    uint32_t centerFrequency,
    uint16_t channelWidth,
    double txPowerW,
    uint16_t guardBandwidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<bool>& puncturedSubchannels)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << minInnerBandDbr
                                    << minOuterBandDbr << lowestPointDbr);
    uint32_t bandBandwidth = 78125;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));
    uint32_t nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / bandBandwidth) + 0.5);
    uint32_t nAllocatedBands = static_cast<uint32_t>(((channelWidth * 1e6) / bandBandwidth) + 0.5);
    NS_ASSERT_MSG(c->GetSpectrumModel()->GetNumBands() == (nAllocatedBands + nGuardBands + 1),
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
    uint32_t innerSlopeWidth = static_cast<uint32_t>(
        (1e6 / bandBandwidth) + 0.5);       // size in number of subcarriers of the inner band
    std::vector<WifiSpectrumBand> subBands; // list of data/pilot-containing subBands (sent at 0dBr)
    WifiSpectrumBand maskBand(0, nAllocatedBands + nGuardBands);
    switch (channelWidth)
    {
    case 20:
        // 242 subcarriers (234 data + 8 pilot)
        txPowerPerBandW = txPowerW / 242;
        innerSlopeWidth =
            static_cast<uint32_t>((5e5 / bandBandwidth) + 0.5); // [-10.25;-9.75] & [9.75;10.25]
        // skip the guard band and 6 subbands, then place power in 121 subbands, then
        // skip 3 DC, then place power in 121 subbands, then skip
        // the final 5 subbands and the guard band.
        start1 = (nGuardBands / 2) + 6;
        stop1 = start1 + 121 - 1;
        start2 = stop1 + 4;
        stop2 = start2 + 121 - 1;
        subBands.emplace_back(start1, stop1);
        subBands.emplace_back(start2, stop2);
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
        subBands.emplace_back(start1, stop1);
        subBands.emplace_back(start2, stop2);
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
        subBands.emplace_back(start1, stop1);
        subBands.emplace_back(start2, stop2);
        break;
    case 160:
        // 2 x 996 subcarriers (2 x 80 MHZ bands)
        txPowerPerBandW = txPowerW / (2 * 996);
        start1 = (nGuardBands / 2) + 12;
        stop1 = start1 + 498 - 1;
        start2 = stop1 + 6;
        stop2 = start2 + 498 - 1;
        start3 = stop2 + (2 * 12);
        stop3 = start3 + 498 - 1;
        start4 = stop3 + 6;
        stop4 = start4 + 498 - 1;
        subBands.emplace_back(start1, stop1);
        subBands.emplace_back(start2, stop2);
        subBands.emplace_back(start3, stop3);
        subBands.emplace_back(start4, stop4);
        break;
    default:
        NS_FATAL_ERROR("ChannelWidth " << channelWidth << " unsupported");
        break;
    }

    // Create punctured bands
    uint32_t puncturedSlopeWidth = static_cast<uint32_t>(
        (500e3 / bandBandwidth) + 0.5); // size in number of subcarriers of the punctured slope band
    std::vector<WifiSpectrumBand> puncturedBands;
    std::size_t subcarriersPerSuband = (20 * 1e6 / bandBandwidth);
    uint32_t start = (nGuardBands / 2);
    uint32_t stop = start + subcarriersPerSuband - 1;
    for (auto puncturedSubchannel : puncturedSubchannels)
    {
        if (puncturedSubchannel)
        {
            puncturedBands.emplace_back(start, stop);
        }
        start = stop + 1;
        stop = start + subcarriersPerSuband - 1;
    }

    // Build transmit spectrum mask
    CreateSpectrumMaskForOfdm(c,
                              subBands,
                              maskBand,
                              txPowerPerBandW,
                              nGuardBands,
                              innerSlopeWidth,
                              minInnerBandDbr,
                              minOuterBandDbr,
                              lowestPointDbr,
                              puncturedBands,
                              puncturedSlopeWidth);
    NormalizeSpectrumMask(c, txPowerW);
    NS_ASSERT_MSG(std::abs(txPowerW - Integral(*c)) < 1e-6, "Power allocation failed");
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity(uint32_t centerFrequency,
                                                              uint16_t channelWidth,
                                                              double txPowerW,
                                                              uint16_t guardBandwidth,
                                                              WifiSpectrumBand ru)
{
    NS_LOG_FUNCTION(centerFrequency << channelWidth << txPowerW << guardBandwidth << ru.first
                                    << ru.second);
    uint32_t bandBandwidth = 78125;
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth));

    // Build spectrum mask
    Values::iterator vit = c->ValuesBegin();
    Bands::const_iterator bit = c->ConstBandsBegin();
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
WifiSpectrumValueHelper::CreateNoisePowerSpectralDensity(uint32_t centerFrequency,
                                                         uint16_t channelWidth,
                                                         uint32_t bandBandwidth,
                                                         double noiseFigure,
                                                         uint16_t guardBandwidth)
{
    Ptr<SpectrumModel> model =
        GetSpectrumModel(centerFrequency, channelWidth, bandBandwidth, guardBandwidth);
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

Ptr<SpectrumValue>
WifiSpectrumValueHelper::CreateRfFilter(uint32_t centerFrequency,
                                        uint16_t totalChannelWidth,
                                        uint32_t bandBandwidth,
                                        uint16_t guardBandwidth,
                                        WifiSpectrumBand band)
{
    uint32_t startIndex = band.first;
    uint32_t stopIndex = band.second;
    NS_LOG_FUNCTION(centerFrequency << totalChannelWidth << bandBandwidth << guardBandwidth
                                    << startIndex << stopIndex);
    Ptr<SpectrumValue> c = Create<SpectrumValue>(
        GetSpectrumModel(centerFrequency, totalChannelWidth, bandBandwidth, guardBandwidth));
    Bands::const_iterator bit = c->ConstBandsBegin();
    Values::iterator vit = c->ValuesBegin();
    vit += startIndex;
    bit += startIndex;
    for (size_t i = startIndex; i <= stopIndex; i++, vit++, bit++)
    {
        *vit = 1;
    }
    NS_LOG_LOGIC("Added subbands " << startIndex << " to " << stopIndex << " to filter");
    return c;
}

void
WifiSpectrumValueHelper::CreateSpectrumMaskForOfdm(
    Ptr<SpectrumValue> c,
    const std::vector<WifiSpectrumBand>& allocatedSubBands,
    WifiSpectrumBand maskBand,
    double txPowerPerBandW,
    uint32_t nGuardBands,
    uint32_t innerSlopeWidth,
    double minInnerBandDbr,
    double minOuterBandDbr,
    double lowestPointDbr,
    const std::vector<WifiSpectrumBand>& puncturedBands,
    uint32_t puncturedSlopeWidth)
{
    NS_LOG_FUNCTION(c << allocatedSubBands.front().first << allocatedSubBands.back().second
                      << maskBand.first << maskBand.second << txPowerPerBandW << nGuardBands
                      << innerSlopeWidth << minInnerBandDbr << minOuterBandDbr << lowestPointDbr
                      << puncturedSlopeWidth);
    uint32_t numSubBands = allocatedSubBands.size();
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
    WifiSpectrumBand outerBandLeft(maskBand.first, // to handle cases where allocated channel is
                                                   // under WifiPhy configured channel width.
                                   maskBand.first + outerSlopeWidth - 1);
    WifiSpectrumBand middleBandLeft(outerBandLeft.second + 1,
                                    outerBandLeft.second + middleSlopeWidth);
    WifiSpectrumBand innerBandLeft(allocatedSubBands.front().first - innerSlopeWidth,
                                   allocatedSubBands.front().first -
                                       1); // better to place slope based on allocated subcarriers
    WifiSpectrumBand flatJunctionLeft(middleBandLeft.second + 1,
                                      innerBandLeft.first -
                                          1); // in order to handle shift due to guard subcarriers
    WifiSpectrumBand outerBandRight(
        maskBand.second - outerSlopeWidth + 1,
        maskBand.second); // start from outer edge to be able to compute flat junction width
    WifiSpectrumBand middleBandRight(outerBandRight.first - middleSlopeWidth,
                                     outerBandRight.first - 1);
    WifiSpectrumBand innerBandRight(allocatedSubBands.back().second + 1,
                                    allocatedSubBands.back().second + innerSlopeWidth);
    WifiSpectrumBand flatJunctionRight(innerBandRight.second + 1, middleBandRight.first - 1);
    std::ostringstream ss;
    ss << "outerBandLeft=[" << outerBandLeft.first << ";" << outerBandLeft.second << "] "
       << "middleBandLeft=[" << middleBandLeft.first << ";" << middleBandLeft.second << "] "
       << "flatJunctionLeft=[" << flatJunctionLeft.first << ";" << flatJunctionLeft.second << "] "
       << "innerBandLeft=[" << innerBandLeft.first << ";" << innerBandLeft.second << "] "
       << "subBands=[" << allocatedSubBands.front().first << ";" << allocatedSubBands.back().second
       << "] ";
    if (!puncturedBands.empty())
    {
        ss << "puncturedBands=[" << puncturedBands.front().first << ";"
           << puncturedBands.back().second << "] ";
    }
    ss << "innerBandRight=[" << innerBandRight.first << ";" << innerBandRight.second << "] "
       << "flatJunctionRight=[" << flatJunctionRight.first << ";" << flatJunctionRight.second
       << "] "
       << "middleBandRight=[" << middleBandRight.first << ";" << middleBandRight.second << "] "
       << "outerBandRight=[" << outerBandRight.first << ";" << outerBandRight.second << "] ";
    NS_LOG_DEBUG(ss.str());
    NS_ASSERT(numMaskBands ==
              ((allocatedSubBands.back().second - allocatedSubBands.front().first +
                1) // equivalent to allocatedBand (includes notches and DC)
               + 2 * (innerSlopeWidth + middleSlopeWidth + outerSlopeWidth) +
               (flatJunctionLeft.second - flatJunctionLeft.first + 1) // flat junctions
               + (flatJunctionRight.second - flatJunctionRight.first + 1)));

    // Different slopes
    double innerSlope = (-1 * minInnerBandDbr) / innerSlopeWidth;
    double middleSlope = (-1 * (minOuterBandDbr - minInnerBandDbr)) / middleSlopeWidth;
    double outerSlope = (txPowerMiddleBandMinDbm - txPowerOuterBandMinDbm) / outerSlopeWidth;
    double puncturedSlope = (-1 * minInnerBandDbr) / puncturedSlopeWidth;

    // Build spectrum mask
    Values::iterator vit = c->ValuesBegin();
    Bands::const_iterator bit = c->ConstBandsBegin();
    double txPowerW = 0.0;
    double previousTxPowerW = 0.0;
    for (size_t i = 0; i < numBands; i++, vit++, bit++)
    {
        if (i < maskBand.first || i > maskBand.second) // outside the spectrum mask
        {
            txPowerW = 0.0;
        }
        else if (i <= outerBandLeft.second &&
                 i >= outerBandLeft.first) // better to put greater first (less computation)
        {
            txPowerW = DbmToW(txPowerOuterBandMinDbm + ((i - outerBandLeft.first) * outerSlope));
        }
        else if (i <= middleBandLeft.second && i >= middleBandLeft.first)
        {
            txPowerW = DbmToW(txPowerMiddleBandMinDbm + ((i - middleBandLeft.first) * middleSlope));
        }
        else if (i <= flatJunctionLeft.second && i >= flatJunctionLeft.first)
        {
            txPowerW = DbmToW(txPowerInnerBandMinDbm);
        }
        else if (i <= innerBandLeft.second && i >= innerBandLeft.first)
        {
            txPowerW =
                (!puncturedBands.empty() &&
                 (puncturedBands.front().first <= allocatedSubBands.front().first))
                    ? DbmToW(txPowerInnerBandMinDbm)
                    : // first 20 MHz band is punctured
                    DbmToW(txPowerInnerBandMinDbm + ((i - innerBandLeft.first) * innerSlope));
        }
        else if (i <= allocatedSubBands.back().second &&
                 i >= allocatedSubBands.front().first) // roughly in allocated band
        {
            bool insideSubBand = false;
            for (uint32_t j = 0; !insideSubBand && j < numSubBands;
                 j++) // continue until inside a sub-band
            {
                insideSubBand =
                    (i <= allocatedSubBands[j].second) && (i >= allocatedSubBands[j].first);
            }
            if (insideSubBand)
            {
                bool insidePuncturedSubBand = false;
                uint32_t j = 0;
                for (; !insidePuncturedSubBand && j < puncturedBands.size();
                     j++) // continue until inside a punctured sub-band
                {
                    insidePuncturedSubBand =
                        (i <= puncturedBands[j].second) && (i >= puncturedBands[j].first);
                }
                if (insidePuncturedSubBand)
                {
                    uint32_t startPuncturedSlope =
                        (puncturedBands[puncturedBands.size() - 1].second -
                         puncturedSlopeWidth); // only consecutive subchannels can be punctured
                    if (i >= startPuncturedSlope)
                    {
                        txPowerW = DbmToW(txPowerInnerBandMinDbm +
                                          ((i - startPuncturedSlope) * puncturedSlope));
                    }
                    else
                    {
                        txPowerW = std::max(DbmToW(txPowerInnerBandMinDbm),
                                            DbmToW(txPowerRefDbm - ((i - puncturedBands[0].first) *
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
        else if (i <= innerBandRight.second && i >= innerBandRight.first)
        {
            // take min to handle the case where last 20 MHz band is punctured
            txPowerW = std::min(
                previousTxPowerW,
                DbmToW(txPowerRefDbm - ((i - innerBandRight.first + 1) *
                                        innerSlope))); // +1 so as to be symmetric with left slope
        }
        else if (i <= flatJunctionRight.second && i >= flatJunctionRight.first)
        {
            txPowerW = DbmToW(txPowerInnerBandMinDbm);
        }
        else if (i <= middleBandRight.second && i >= middleBandRight.first)
        {
            txPowerW = DbmToW(txPowerInnerBandMinDbm -
                              ((i - middleBandRight.first + 1) *
                               middleSlope)); // +1 so as to be symmetric with left slope
        }
        else if (i <= outerBandRight.second && i >= outerBandRight.first)
        {
            txPowerW = DbmToW(txPowerMiddleBandMinDbm -
                              ((i - outerBandRight.first + 1) *
                               outerSlope)); // +1 so as to be symmetric with left slope
        }
        else
        {
            NS_FATAL_ERROR("Should have handled all cases");
        }
        double txPowerDbr = 10 * std::log10(txPowerW / txPowerPerBandW);
        NS_LOG_LOGIC(uint32_t(i) << " -> " << txPowerDbr);
        *vit = txPowerW / (bit->fh - bit->fl);
        previousTxPowerW = txPowerW;
    }
    NS_LOG_INFO("Added signal power to subbands " << allocatedSubBands.front().first << "-"
                                                  << allocatedSubBands.back().second);
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
    Values::iterator vit = c->ValuesBegin();
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
WifiSpectrumValueHelper::GetBandPowerW(Ptr<SpectrumValue> psd, const WifiSpectrumBand& band)
{
    double powerWattPerHertz = 0.0;
    auto valueIt = psd->ConstValuesBegin() + band.first;
    auto end = psd->ConstValuesBegin() + band.second;
    auto bandIt = psd->ConstBandsBegin() + band.first;
    while (valueIt <= end)
    {
        powerWattPerHertz += *valueIt;
        ++valueIt;
    }
    return powerWattPerHertz * (bandIt->fh - bandIt->fl);
}

static Ptr<SpectrumModel> g_WifiSpectrumModel5Mhz; ///< static initializer for the class

WifiSpectrumValueHelper::~WifiSpectrumValueHelper()
{
}

WifiSpectrumValue5MhzFactory::~WifiSpectrumValue5MhzFactory()
{
}

/**
 * Static class to initialize the values for the 2.4 GHz Wi-Fi spectrum model
 */
static class WifiSpectrumModel5MhzInitializer
{
  public:
    WifiSpectrumModel5MhzInitializer()
    {
        Bands bands;
        for (int i = -4; i < 13 + 7; i++)
        {
            BandInfo bi;
            bi.fl = 2407.0e6 + i * 5.0e6;
            bi.fh = 2407.0e6 + (i + 1) * 5.0e6;
            bi.fc = (bi.fl + bi.fh) / 2;
            bands.push_back(bi);
        }
        g_WifiSpectrumModel5Mhz = Create<SpectrumModel>(bands);
    }
} g_WifiSpectrumModel5MhzInitializerInstance; //!< initialization instance for WifiSpectrumModel5Mhz

Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateConstant(double v)
{
    Ptr<SpectrumValue> c = Create<SpectrumValue>(g_WifiSpectrumModel5Mhz);
    (*c) = v;
    return c;
}

Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateTxPowerSpectralDensity(double txPower, uint8_t channel)
{
    Ptr<SpectrumValue> txPsd = Create<SpectrumValue>(g_WifiSpectrumModel5Mhz);

    // since the spectrum model has a resolution of 5 MHz, we model
    // the transmitted signal with a constant density over a 20MHz
    // bandwidth centered on the center frequency of the channel. The
    // transmission power outside the transmission power density is
    // calculated considering the transmit spectrum mask, see IEEE
    // Std. 802.11-2007, Annex I

    double txPowerDensity = txPower / 20e6;

    NS_ASSERT(channel >= 1);
    NS_ASSERT(channel <= 13);

    (*txPsd)[channel - 1] = txPowerDensity * 1e-4;      // -40dB
    (*txPsd)[channel] = txPowerDensity * 1e-4;          // -40dB
    (*txPsd)[channel + 1] = txPowerDensity * 0.0015849; // -28dB
    (*txPsd)[channel + 2] = txPowerDensity * 0.0015849; // -28dB
    (*txPsd)[channel + 3] = txPowerDensity;
    (*txPsd)[channel + 4] = txPowerDensity;
    (*txPsd)[channel + 5] = txPowerDensity;
    (*txPsd)[channel + 6] = txPowerDensity;
    (*txPsd)[channel + 7] = txPowerDensity * 0.0015849; // -28dB
    (*txPsd)[channel + 8] = txPowerDensity * 0.0015849; // -28dB
    (*txPsd)[channel + 9] = txPowerDensity * 1e-4;      // -40dB
    (*txPsd)[channel + 10] = txPowerDensity * 1e-4;     // -40dB

    return txPsd;
}

Ptr<SpectrumValue>
WifiSpectrumValue5MhzFactory::CreateRfFilter(uint8_t channel)
{
    Ptr<SpectrumValue> rf = Create<SpectrumValue>(g_WifiSpectrumModel5Mhz);

    NS_ASSERT(channel >= 1);
    NS_ASSERT(channel <= 13);

    (*rf)[channel + 3] = 1;
    (*rf)[channel + 4] = 1;
    (*rf)[channel + 5] = 1;
    (*rf)[channel + 6] = 1;

    return rf;
}

} // namespace ns3
