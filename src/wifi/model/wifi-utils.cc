/*
 * Copyright (c) 2016
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-utils.h"

#include "ap-wifi-mac.h"
#include "ctrl-headers.h"
#include "gcr-manager.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"
#include "wifi-phy-operating-channel.h"
#include "wifi-tx-vector.h"

#include "ns3/packet.h"

#include <cmath>

namespace ns3
{

double
DbToRatio(dB_t val)
{
    return val.to_linear();
}

Watt_t
DbmToW(dBm_t pow)
{
    return pow.to_Watt();
}

dBm_t
WToDbm(Watt_t power)
{
    return Watt_t{power}.to_dBm();
}

dB_t
RatioToDb(double ratio)
{
    return dB_t::from_linear(ratio);
}

uint32_t
GetAckSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_ACK).GetSize() + 4;

    return size;
}

uint32_t
GetBlockAckSize(BlockAckType type)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKRESP);
    CtrlBAckResponseHeader blockAck;
    blockAck.SetType(type);
    return hdr.GetSize() + blockAck.GetSerializedSize() + 4;
}

uint32_t
GetBlockAckRequestSize(BlockAckReqType type)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKREQ);
    CtrlBAckRequestHeader bar;
    bar.SetType(type);
    return hdr.GetSize() + bar.GetSerializedSize() + 4;
}

TriggerFrameVariant
GetTriggerFrameVariant(WifiModulationClass modClass)
{
    switch (modClass)
    {
    case WIFI_MOD_CLASS_HE:
        return TriggerFrameVariant::HE;
    case WIFI_MOD_CLASS_EHT:
        return TriggerFrameVariant::EHT;
    case WIFI_MOD_CLASS_UHR:
        return TriggerFrameVariant::UHR;
    default:
        NS_FATAL_ERROR("Unsupported modulation class: " << modClass);
        return TriggerFrameVariant::HE;
    }
}

uint32_t
GetMuBarSize(TriggerFrameVariant variant, MHz_t bw, const std::list<BlockAckReqType>& types)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    CtrlTriggerHeader trigger;
    trigger.SetType(TriggerFrameType::MU_BAR_TRIGGER);
    trigger.SetVariant(variant);
    trigger.SetUlBandwidth(bw);
    for (auto& t : types)
    {
        auto userInfo = trigger.AddUserInfoField();
        CtrlBAckRequestHeader bar;
        bar.SetType(t);
        userInfo.SetMuBarTriggerDepUserInfo(bar);
    }
    return hdr.GetSize() + trigger.GetSerializedSize() + 4;
}

uint32_t
GetRtsSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_RTS).GetSize() + 4;

    return size;
}

uint32_t
GetCtsSize()
{
    static const uint32_t size = WifiMacHeader(WIFI_MAC_CTL_CTS).GetSize() + 4;

    return size;
}

Time
GetEstimatedAckTxTime(const WifiTxVector& txVector)
{
    auto modClass = txVector.GetModulationClass();

    switch (modClass)
    {
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
        if (txVector.GetMode().GetDataRate(txVector) == 1e6)
        {
            return MicroSeconds(304);
        }
        else if (txVector.GetPreambleType() == WIFI_PREAMBLE_LONG)
        {
            return MicroSeconds(248);
        }
        else
        {
            return MicroSeconds(152);
        }
        break;
    case WIFI_MOD_CLASS_ERP_OFDM:
    case WIFI_MOD_CLASS_OFDM:
        if (auto constSize = txVector.GetMode().GetConstellationSize(); constSize == 2)
        {
            return MicroSeconds(44);
        }
        else if (constSize == 4)
        {
            return MicroSeconds(32);
        }
        else
        {
            return MicroSeconds(28);
        }
        break;
    default: {
        auto staId = (txVector.IsMu() ? txVector.GetHeMuUserInfoMap().begin()->first : SU_STA_ID);
        if (const auto constSize = txVector.GetMode(staId).GetConstellationSize(); constSize == 2)
        {
            return MicroSeconds(68);
        }
        else if (constSize == 4)
        {
            return MicroSeconds(44);
        }
        else
        {
            return MicroSeconds(32);
        }
    }
    }
}

bool
IsInWindow(uint16_t seq, uint16_t winstart, uint16_t winsize)
{
    return ((seq - winstart + 4096) % 4096) < winsize;
}

void
AddWifiMacTrailer(Ptr<Packet> packet)
{
    WifiMacTrailer fcs;
    packet->AddTrailer(fcs);
}

uint32_t
GetSize(Ptr<const Packet> packet, const WifiMacHeader* hdr, bool isAmpdu)
{
    uint32_t size;
    WifiMacTrailer fcs;
    if (isAmpdu)
    {
        size = packet->GetSize();
    }
    else
    {
        size = packet->GetSize() + hdr->GetSize() + fcs.GetSerializedSize();
    }
    return size;
}

bool
IsValidGuardInterval(Time gi, WifiStandard standard)
{
    const auto guardInterval = gi.GetNanoSeconds();
    switch (standard)
    {
    case WIFI_STANDARD_80211n:
    case WIFI_STANDARD_80211ac:
        return (gi.GetNanoSeconds() == 400) || (gi.GetNanoSeconds() == 800);
    case WIFI_STANDARD_80211ad:
        return (gi.GetPicoSeconds() == 48485) || (gi.GetPicoSeconds() == 96970);
    case WIFI_STANDARD_80211ax:
    case WIFI_STANDARD_80211be:
    case WIFI_STANDARD_80211bn:
        return (gi.GetNanoSeconds() == 800) || (gi.GetNanoSeconds() == 1600) ||
               (gi.GetNanoSeconds() == 3200);
    default:
        return guardInterval == 800;
    }
}

bool
TidToLinkMappingValidForNegType1(const WifiTidLinkMapping& dlLinkMapping,
                                 const WifiTidLinkMapping& ulLinkMapping)
{
    if (dlLinkMapping.empty() && ulLinkMapping.empty())
    {
        // default mapping is valid
        return true;
    }

    if (dlLinkMapping.size() != 8 || ulLinkMapping.size() != 8)
    {
        // not all TIDs have been mapped
        return false;
    }

    const auto& linkSet = dlLinkMapping.cbegin()->second;

    for (const auto& linkMapping : {std::cref(dlLinkMapping), std::cref(ulLinkMapping)})
    {
        for (const auto& [tid, links] : linkMapping.get())
        {
            if (links != linkSet)
            {
                // distinct link sets
                return false;
            }
        }
    }

    return true;
}

bool
IsGroupcast(const Mac48Address& adr)
{
    return adr.IsGroup() && !adr.IsBroadcast();
}

bool
IsGcr(Ptr<WifiMac> mac, const WifiMacHeader& hdr)
{
    auto apMac = DynamicCast<ApWifiMac>(mac);
    return apMac && apMac->UseGcr(hdr);
}

Mac48Address
GetIndividuallyAddressedRecipient(Ptr<WifiMac> mac, const WifiMacHeader& hdr)
{
    const auto isGcr = IsGcr(mac, hdr);
    const auto addr1 = hdr.GetAddr1();
    if (!isGcr)
    {
        return addr1;
    }
    auto apMac = DynamicCast<ApWifiMac>(mac);
    return apMac->GetGcrManager()->GetIndividuallyAddressedRecipient(addr1);
}

bool
DoesOverlap(const FrequencyRange& rangeChannel1, const FrequencyRange& rangeChannel2)
{
    /**
     * The channel 1 does not overlap the channel 2 in two cases.
     *
     * First non-overlapping case:
     *
     *                                        ┌─────────┐
     *                                        │channel 1│
     *                                        └─────────┘
     *         ┌──────────────────────────────┐
     *         │          Channel 2           │
     *         └──────────────────────────────┘
     *
     * Second non-overlapping case:
     *
     *         ┌─────────┐
     *         │channel 1│
     *         └─────────┘
     *                   ┌──────────────────────────────┐
     *                   │          Channel 2           │
     *                   └──────────────────────────────┘
     */
    return ((rangeChannel1.minFrequency < rangeChannel2.maxFrequency) &&
            (rangeChannel1.maxFrequency > rangeChannel2.minFrequency));
}

FrequencyRange
GetFrequencyRange(MHz_t centerFrequency, MHz_t bandwidth)
{
    return {centerFrequency - bandwidth / 2, centerFrequency + bandwidth / 2};
}

bool
DoesOverlap(const std::set<MHz_t>& channel1CenterFreqs,
            MHz_t channel1Bw,
            const std::set<MHz_t>& channel2CenterFreqs,
            MHz_t channel2Bw)
{
    for (const auto& freq1 : channel1CenterFreqs)
    {
        for (const auto& freq2 : channel2CenterFreqs)
        {
            if (DoesOverlap(GetFrequencyRange(freq1, channel1Bw),
                            GetFrequencyRange(freq2, channel2Bw)))
            {
                return true;
            }
        }
    }
    return false;
}

bool
DoesOverlap(const WifiPhyOperatingChannel& channel1, const WifiPhyOperatingChannel& channel2)
{
    // assume all segments have the same width for now
    const std::set<MHz_t> channel1CenterFreqs(channel1.GetFrequencies().cbegin(),
                                              channel1.GetFrequencies().cend());
    const auto channel1Bw{channel1.GetTotalWidth() / channel1CenterFreqs.size()};
    const std::set<MHz_t> channel2CenterFreqs(channel2.GetFrequencies().cbegin(),
                                              channel2.GetFrequencies().cend());
    const auto channel2Bw{channel2.GetTotalWidth() / channel2CenterFreqs.size()};
    return DoesOverlap({channel1CenterFreqs}, channel1Bw, {channel2CenterFreqs}, channel2Bw);
}

bool
DoesOverlap(const WifiPhyOperatingChannel& opChannel, const FrequencyRange& freqRange)
{
    NS_ASSERT_MSG(opChannel.IsSet(), "Operating channel is not set");
    const auto centerFreqs = opChannel.GetFrequencies();
    const auto bw = opChannel.GetTotalWidth();
    const auto rangeBw = freqRange.maxFrequency - freqRange.minFrequency;
    const auto rangeFreq = freqRange.minFrequency + (rangeBw / 2);
    return DoesOverlap(std::set<MHz_t>(centerFreqs.cbegin(), centerFreqs.cend()),
                       bw,
                       {rangeFreq},
                       rangeBw);
}

/**
 * @brief Get the operating channel based on the center frequency segments and primary 20 MHz
 * channel number.
 *
 * @param ccfs0 the center frequency segment 0.
 * @param ccfs1 the center frequency segment 1 (0 if not applicable).
 * @param p20ChannelNumber the primary 20 MHz channel number.
 * @param bw the bandwidth of the operating channel.
 * @param band the band of the operating channel (5 GHz or 6 GHz).
 * @param standard the standard.
 * @return The operating channel
 */
WifiPhyOperatingChannel
GetOperatingChannel(uint8_t ccfs0,
                    uint8_t ccfs1,
                    uint8_t p20ChannelNumber,
                    MHz_t bw,
                    WifiPhyBand band,
                    WifiStandard standard)
{
    NS_ASSERT_MSG(band == WIFI_PHY_BAND_5GHZ || band == WIFI_PHY_BAND_6GHZ,
                  "GetOperatingChannel is only supported for 5 and 6 GHz band");
    WifiPhyOperatingChannel opChannel;

    // Table 9-317 in IEEE Std 802.11-2024 for 5 GHz band and Table 26-15 in IEEE Std 802.11-2024
    // for 6 GHz band
    const auto is80Plus80 = (ccfs1 > 0) ? (std::abs(ccfs1 - ccfs0) > 16) : false;

    if (is80Plus80)
    {
        WifiPhyOperatingChannel::ConstIteratorSet chanIts;
        const auto chanIt0 =
            WifiPhyOperatingChannel::FindFirst(ccfs0, MHz_t{0}, MHz_t{80}, standard, band);
        NS_ASSERT_MSG(chanIt0 != WifiPhyOperatingChannel::m_frequencyChannels.end(),
                      "Operating channel segment0 (" << +ccfs0 << ") not found");
        chanIts.emplace(chanIt0);
        const auto chanIt1 =
            WifiPhyOperatingChannel::FindFirst(ccfs1, MHz_t{0}, MHz_t{80}, standard, band);
        NS_ASSERT_MSG(chanIt1 != WifiPhyOperatingChannel::m_frequencyChannels.end(),
                      "AP operating channel segment1 (" << +ccfs1 << ") not found");
        chanIts.emplace(chanIt1);
        opChannel = WifiPhyOperatingChannel(chanIts);
    }
    else
    {
        const auto ccfs = (ccfs1 > 0) ? ccfs1 : ccfs0;
        const auto chanIt = WifiPhyOperatingChannel::FindFirst(ccfs, MHz_t{0}, bw, standard, band);
        NS_ASSERT_MSG(chanIt != WifiPhyOperatingChannel::m_frequencyChannels.end(),
                      "Operating channel not found for " << +ccfs);
        opChannel = WifiPhyOperatingChannel(chanIt);
    }
    opChannel.SetPrimary20ChannelNumber(p20ChannelNumber);
    return opChannel;
}

WifiPhyOperatingChannel
GetApOperatingChannel(MHz_t apBw,
                      WifiPhyBand band,
                      WifiStandard standard,
                      const MgtResponseFrameType& frame)
{
    NS_ASSERT_MSG(standard >= WIFI_STANDARD_80211n,
                  "GetApOperatingChannel is only supported for 802.11n and later standards");

    uint8_t ccfs0{0};
    uint8_t ccfs1{0};
    uint8_t p20ChannelNumber{0};
    auto readFromOpIes = [&](auto&& frame) {
        if (band == WIFI_PHY_BAND_5GHZ)
        {
            const auto& htOperation = frame.template Get<HtOperation>();
            NS_ASSERT_MSG(htOperation.has_value(),
                          "HT Operation should be present in management response for 5 GHz band");
            p20ChannelNumber = htOperation->GetPrimaryChannel();

            const auto& vhtOperation = frame.template Get<VhtOperation>();
            NS_ASSERT_MSG(vhtOperation.has_value(),
                          "VHT Operation should be present in management response for 5 GHz band");

            ccfs0 = vhtOperation->GetChannelCenterFrequencySegment0();
            ccfs1 = vhtOperation->GetChannelCenterFrequencySegment1();
        }
        else // 6 GHz band
        {
            const auto& heOperation = frame.template Get<HeOperation>();
            NS_ASSERT_MSG(heOperation.has_value(),
                          "HE Operation should be present in management response");
            NS_ASSERT_MSG(heOperation->m_6GHzOpInfo.has_value(),
                          "6 GHz Operation Information field should be present if band is 6 GHz");
            p20ChannelNumber = heOperation->m_6GHzOpInfo->m_primCh;

            if (apBw == MHz_t{320})
            {
                // In case of 320 MHz, we need to check the EHT Operation Information to determine
                // the channelization
                const auto& ehtOperation = frame.template Get<EhtOperation>();
                NS_ASSERT_MSG(
                    ehtOperation.has_value(),
                    "EHT Operation should be present in management response for 6 GHz band");
                NS_ASSERT_MSG(
                    ehtOperation->m_opInfo.has_value(),
                    "EHT Operation Information should be present if operating channel is 320 MHz");
                ccfs0 = ehtOperation->m_opInfo->ccfs0;
                ccfs1 = ehtOperation->m_opInfo->ccfs1;
            }
            else
            {
                ccfs0 = heOperation->m_6GHzOpInfo->m_chCntrFreqSeg0;
                ccfs1 = heOperation->m_6GHzOpInfo->m_chCntrFreqSeg1;
            }
        }
    };

    std::visit(readFromOpIes, frame);
    return GetOperatingChannel(ccfs0, ccfs1, p20ChannelNumber, apBw, band, standard);
}

} // namespace ns3
