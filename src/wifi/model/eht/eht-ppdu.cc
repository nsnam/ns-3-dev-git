/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-ppdu.h"

#include "eht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <initializer_list>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtPpdu");

EhtPpdu::EhtPpdu(const WifiConstPsduMap& psdus,
                 const WifiTxVector& txVector,
                 const WifiPhyOperatingChannel& channel,
                 Time ppduDuration,
                 uint64_t uid,
                 TxPsdFlag flag)
    : HePpdu(psdus, txVector, channel, ppduDuration, uid, flag, false)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag);
    SetPhyHeaders(txVector, ppduDuration);
}

void
EhtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);
    SetLSigHeader(ppduDuration);
    SetEhtPhyHeader(txVector);
}

void
EhtPpdu::SetEhtPhyHeader(const WifiTxVector& txVector)
{
    const auto bssColor = txVector.GetBssColor();
    NS_ASSERT(bssColor < 64);
    if (ns3::IsDlMu(m_preamble))
    {
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
        m_ehtPhyHeader.emplace<EhtMuPhyHeader>(EhtMuPhyHeader{
            .m_bandwidth =
                GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth(), m_operatingChannel),
            .m_bssColor = bssColor,
            .m_ppduType = txVector.GetEhtPpduType(),
            // TODO: EHT PPDU should store U-SIG per 20 MHz band, assume it is the lowest 20 MHz
            // band for now
            .m_puncturedChannelInfo =
                GetPuncturedInfo(txVector.GetInactiveSubchannels(),
                                 txVector.GetEhtPpduType(),
                                 (txVector.IsDlMu() && (txVector.GetChannelWidth() > MHz_u{80}))
                                     ? std::optional{true}
                                     : std::nullopt),
            .m_ehtSigMcs = txVector.GetSigBMode().GetMcsValue(),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            /* See section 36.3.12.8.2 of IEEE 802.11be D3.0 (EHT-SIG content channels):
             * In non-OFDMA transmission, the Common field of the EHT-SIG content channel does not
             * contain the RU Allocation subfield. For non-OFDMA transmission except for EHT
             * sounding NDP, the Common field of the EHT-SIG content channel is encoded together
             * with the first User field and this encoding block contains a CRC and Tail, referred
             * to as a common encoding block. */
            .m_ruAllocationA = txVector.IsMu() && !txVector.IsSigBCompression()
                                   ? std::optional{txVector.GetRuAllocation(p20Index)}
                                   : std::nullopt,
            // TODO: RU Allocation-B not supported yet
            .m_contentChannels = GetEhtSigContentChannels(txVector, p20Index)});
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        m_ehtPhyHeader.emplace<EhtTbPhyHeader>(
            EhtTbPhyHeader{.m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth(),
                                                                         m_operatingChannel),
                           .m_bssColor = bssColor,
                           .m_ppduType = txVector.GetEhtPpduType()});
    }
}

uint8_t
EhtPpdu::GetChannelWidthEncodingFromMhz(MHz_u channelWidth, const WifiPhyOperatingChannel& channel)
{
    NS_ASSERT(channel.GetTotalWidth() >= channelWidth);
    if (channelWidth == MHz_u{320})
    {
        switch (channel.GetNumber())
        {
        case 31:
        case 95:
        case 159:
            return 4;
        case 63:
        case 127:
        case 191:
            return 5;
        default:
            NS_ASSERT_MSG(false, "Invalid 320 MHz channel number " << +channel.GetNumber());
            return 4;
        }
    }
    return HePpdu::GetChannelWidthEncodingFromMhz(channelWidth);
}

MHz_u
EhtPpdu::GetChannelWidthMhzFromEncoding(uint8_t bandwidth)
{
    if ((bandwidth == 4) || (bandwidth == 5))
    {
        return MHz_u{320};
    }
    return HePpdu::GetChannelWidthMhzFromEncoding(bandwidth);
}

WifiPpduType
EhtPpdu::GetType() const
{
    if (m_psdus.contains(SU_STA_ID))
    {
        return WIFI_PPDU_TYPE_SU;
    }
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_EHT_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_EHT_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    default:
        NS_ASSERT_MSG(false, "invalid preamble " << m_preamble);
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
EhtPpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_MU) && !m_psdus.contains(SU_STA_ID);
}

bool
EhtPpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_TB) && !m_psdus.contains(SU_STA_ID);
}

void
EhtPpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const
{
    txVector.SetLength(m_lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    if (ns3::IsDlMu(m_preamble))
    {
        auto ehtPhyHeader = std::get_if<EhtMuPhyHeader>(&m_ehtPhyHeader);
        NS_ASSERT(ehtPhyHeader);
        const auto bw = GetChannelWidthMhzFromEncoding(ehtPhyHeader->m_bandwidth);
        txVector.SetChannelWidth(bw);
        txVector.SetBssColor(ehtPhyHeader->m_bssColor);
        txVector.SetEhtPpduType(ehtPhyHeader->m_ppduType);
        if (bw > MHz_u{80})
        {
            // TODO: use punctured channel information
        }
        txVector.SetSigBMode(EhtPhy::GetVhtMcs(ehtPhyHeader->m_ehtSigMcs));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(ehtPhyHeader->m_giLtfSize));
        const auto ruAllocation = ehtPhyHeader->m_ruAllocationA; // RU Allocation-B not supported
                                                                 // yet
        if (const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
            ruAllocation.has_value())
        {
            txVector.SetRuAllocation(ruAllocation.value(), p20Index);
            const auto isMuMimo = (ehtPhyHeader->m_ppduType == 2);
            const auto muMimoUsers =
                isMuMimo
                    ? std::accumulate(ehtPhyHeader->m_contentChannels.cbegin(),
                                      ehtPhyHeader->m_contentChannels.cend(),
                                      0,
                                      [](uint8_t prev, const auto& cc) { return prev + cc.size(); })
                    : 0;
            SetHeMuUserInfos(txVector,
                             WIFI_MOD_CLASS_EHT,
                             ruAllocation.value(),
                             std::nullopt,
                             ehtPhyHeader->m_contentChannels,
                             ehtPhyHeader->m_ppduType == 2,
                             muMimoUsers);
        }
        else if (ehtPhyHeader->m_ppduType == 1) // EHT SU
        {
            NS_ASSERT(ehtPhyHeader->m_contentChannels.size() == 1 &&
                      ehtPhyHeader->m_contentChannels.front().size() == 1);
            txVector.SetMode(
                EhtPhy::GetEhtMcs(ehtPhyHeader->m_contentChannels.front().front().mcs));
            txVector.SetNss(ehtPhyHeader->m_contentChannels.front().front().nss);
        }
        else
        {
            const auto fullBwRu{EhtRu::RuSpec(WifiRu::GetRuType(bw), 1, true, true)};
            txVector.SetHeMuUserInfo(ehtPhyHeader->m_contentChannels.front().front().staId,
                                     {fullBwRu,
                                      ehtPhyHeader->m_contentChannels.front().front().mcs,
                                      ehtPhyHeader->m_contentChannels.front().front().nss});
        }
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        auto ehtPhyHeader = std::get_if<EhtTbPhyHeader>(&m_ehtPhyHeader);
        NS_ASSERT(ehtPhyHeader);
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(ehtPhyHeader->m_bandwidth));
        txVector.SetBssColor(ehtPhyHeader->m_bssColor);
        txVector.SetEhtPpduType(ehtPhyHeader->m_ppduType);
    }
}

std::pair<std::size_t, std::size_t>
EhtPpdu::GetNumRusPerEhtSigBContentChannel(MHz_u channelWidth,
                                           uint8_t ehtPpduType,
                                           const RuAllocation& ruAllocation,
                                           bool compression,
                                           std::size_t numMuMimoUsers)
{
    if (ehtPpduType == 1)
    {
        return {1, 0};
    }
    return HePpdu::GetNumRusPerHeSigBContentChannel(channelWidth,
                                                    WIFI_MOD_CLASS_EHT,
                                                    ruAllocation,
                                                    std::nullopt,
                                                    compression,
                                                    numMuMimoUsers);
}

HePpdu::HeSigBContentChannels
EhtPpdu::GetEhtSigContentChannels(const WifiTxVector& txVector, uint8_t p20Index)
{
    if (txVector.GetEhtPpduType() == 1)
    {
        // according to spec the TXVECTOR shall have a correct STA-ID even for SU transmission,
        // but this is not set by the MAC for simplification, so set to 0 for now.
        return HeSigBContentChannels{{{0, txVector.GetNss(), txVector.GetMode().GetMcsValue()}}};
    }
    return HePpdu::GetHeSigBContentChannels(txVector, p20Index);
}

uint32_t
EhtPpdu::GetEhtSigFieldSize(MHz_u channelWidth,
                            const RuAllocation& ruAllocation,
                            uint8_t ehtPpduType,
                            bool compression,
                            std::size_t numMuMimoUsers)
{
    // FIXME: EHT-SIG is not implemented yet, hence this is a copy of HE-SIG-B
    uint32_t commonFieldSize = 0;
    if (!compression)
    {
        commonFieldSize = 4 /* CRC */ + 6 /* tail */;
        if (channelWidth <= MHz_u{40})
        {
            commonFieldSize += 8; // only one allocation subfield
        }
        else
        {
            commonFieldSize +=
                8 * (channelWidth / MHz_u{40}) /* one allocation field per 40 MHz */ +
                1 /* center RU */;
        }
    }

    auto numRusPerContentChannel = GetNumRusPerEhtSigBContentChannel(channelWidth,
                                                                     ehtPpduType,
                                                                     ruAllocation,
                                                                     compression,
                                                                     numMuMimoUsers);
    auto maxNumRusPerContentChannel =
        std::max(numRusPerContentChannel.first, numRusPerContentChannel.second);
    auto maxNumUserBlockFields = maxNumRusPerContentChannel /
                                 2; // handle last user block with single user, if any, further down
    std::size_t userSpecificFieldSize =
        maxNumUserBlockFields * (2 * 21 /* user fields (2 users) */ + 4 /* tail */ + 6 /* CRC */);
    if (maxNumRusPerContentChannel % 2 != 0)
    {
        userSpecificFieldSize += 21 /* last user field */ + 4 /* CRC */ + 6 /* tail */;
    }

    return commonFieldSize + userSpecificFieldSize;
}

uint8_t
EhtPpdu::GetPuncturedInfo(const std::vector<bool>& inactiveSubchannels,
                          uint8_t ehtPpduType,
                          std::optional<bool> isLow80MHz)
{
    if (inactiveSubchannels.size() < 4)
    {
        // no puncturing if less than 80 MHz
        return 0;
    }
    NS_ASSERT_MSG(inactiveSubchannels.size() <= 8,
                  "Puncturing over more than 160 MHz is not supported");
    if (ehtPpduType == 0)
    {
        // IEEE 802.11be D5.0 Table 36-28
        NS_ASSERT(inactiveSubchannels.size() <= 4 || isLow80MHz.has_value());
        const auto startIndex = (inactiveSubchannels.size() <= 4) ? 0 : (*isLow80MHz ? 0 : 4);
        const auto stopIndex =
            (inactiveSubchannels.size() <= 4) ? inactiveSubchannels.size() : (*isLow80MHz ? 4 : 8);
        uint8_t puncturedInfoField = 0;
        for (std::size_t i = startIndex; i < stopIndex; ++i)
        {
            if (!inactiveSubchannels.at(i))
            {
                puncturedInfoField |= 1 << (i / 4);
            }
        }
        return puncturedInfoField;
    }
    // IEEE 802.11be D5.0 Table 36-30
    const auto numPunctured = std::count_if(inactiveSubchannels.cbegin(),
                                            inactiveSubchannels.cend(),
                                            [](bool punctured) { return punctured; });
    if (numPunctured == 0)
    {
        // no puncturing
        return 0;
    }
    const auto firstPunctured = std::find_if(inactiveSubchannels.cbegin(),
                                             inactiveSubchannels.cend(),
                                             [](bool punctured) { return punctured; });
    const auto firstIndex = std::distance(inactiveSubchannels.cbegin(), firstPunctured);
    switch (numPunctured)
    {
    case 1:
        return firstIndex + 1;
    case 2:
        NS_ASSERT_MSG(((firstIndex % 2) == 0) && inactiveSubchannels.at(firstIndex + 1),
                      "invalid 40 MHz puncturing pattern");
        return 9 + (firstIndex / 2);
    default:
        break;
    }
    NS_ASSERT_MSG(false, "invalid puncturing pattern");
    return 0;
}

Ptr<const WifiPsdu>
EhtPpdu::GetPsdu(uint8_t bssColor, uint16_t staId /* = SU_STA_ID */) const
{
    if (m_psdus.contains(SU_STA_ID))
    {
        NS_ASSERT(m_psdus.size() == 1);
        return m_psdus.at(SU_STA_ID);
    }

    if (IsUlMu())
    {
        auto ehtPhyHeader = std::get_if<EhtTbPhyHeader>(&m_ehtPhyHeader);
        NS_ASSERT(ehtPhyHeader);
        NS_ASSERT(m_psdus.size() == 1);
        if ((bssColor == 0) || (ehtPhyHeader->m_bssColor == 0) ||
            (bssColor == ehtPhyHeader->m_bssColor))
        {
            return m_psdus.cbegin()->second;
        }
    }
    else if (IsDlMu())
    {
        auto ehtPhyHeader = std::get_if<EhtMuPhyHeader>(&m_ehtPhyHeader);
        NS_ASSERT(ehtPhyHeader);
        if ((bssColor == 0) || (ehtPhyHeader->m_bssColor == 0) ||
            (bssColor == ehtPhyHeader->m_bssColor))
        {
            const auto it = m_psdus.find(staId);
            if (it != m_psdus.cend())
            {
                return it->second;
            }
        }
    }

    return nullptr;
}

WifiRu::RuSpec
EhtPpdu::GetRuSpec(std::size_t ruAllocIndex, MHz_u bw, RuType ruType, std::size_t phyIndex) const
{
    if (ruType == RuType::RU_26_TONE)
    {
        for (const auto undefinedRu : std::initializer_list<std::size_t>{19, 56, 93, 130})
        {
            if (phyIndex >= undefinedRu)
            {
                ++phyIndex;
            }
        }
    }
    const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
    const auto& [primary160, primary80OrLow80] =
        EhtRu::GetPrimaryFlags(bw, ruType, phyIndex, p20Index);
    const auto index = EhtRu::GetIndexIn80MHzSegment(bw, ruType, phyIndex);
    return EhtRu::RuSpec{ruType, index, primary160, primary80OrLow80};
}

Ptr<WifiPpdu>
EhtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new EhtPpdu(*this), false);
}

} // namespace ns3
