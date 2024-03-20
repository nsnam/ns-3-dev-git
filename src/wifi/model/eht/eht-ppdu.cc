/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-ppdu.h"

#include "eht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"

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
    : HePpdu(psdus, txVector, channel, ppduDuration, uid, flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag);
    SetPhyHeaders(txVector, ppduDuration);
}

void
EhtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);
    SetEhtPhyHeader(txVector);
}

void
EhtPpdu::SetEhtPhyHeader(const WifiTxVector& txVector)
{
    const auto bssColor = txVector.GetBssColor();
    NS_ASSERT(bssColor < 64);
    if (ns3::IsDlMu(m_preamble))
    {
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(20);
        m_ehtPhyHeader.emplace<EhtMuPhyHeader>(EhtMuPhyHeader{
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth()),
            .m_bssColor = bssColor,
            .m_ppduType = txVector.GetEhtPpduType(),
            .m_ehtSigMcs = txVector.GetSigBMode().GetMcsValue(),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            /* See section 36.3.12.8.2 of IEEE 802.11be D3.0 (EHT-SIG content channels):
             * In non-OFDMA transmission, the Common field of the EHT-SIG content channel does not
             * contain the RU Allocation subfield. For non-OFDMA transmission except for EHT
             * sounding NDP, the Common field of the EHT-SIG content channel is encoded together
             * with the first User field and this encoding block contains a CRC and Tail, referred
             * to as a common encoding block. */
            .m_ruAllocationA =
                txVector.IsMu() ? std::optional{txVector.GetRuAllocation(p20Index)} : std::nullopt,
            // TODO: RU Allocation-B not supported yet
            .m_contentChannels = GetEhtSigContentChannels(txVector, p20Index)});
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        m_ehtPhyHeader.emplace<EhtTbPhyHeader>(EhtTbPhyHeader{
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth()),
            .m_bssColor = bssColor,
            .m_ppduType = txVector.GetEhtPpduType()});
    }
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
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(ehtPhyHeader->m_bandwidth));
        txVector.SetBssColor(ehtPhyHeader->m_bssColor);
        txVector.SetEhtPpduType(ehtPhyHeader->m_ppduType);
        txVector.SetSigBMode(HePhy::GetVhtMcs(ehtPhyHeader->m_ehtSigMcs));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(ehtPhyHeader->m_giLtfSize));
        const auto ruAllocation = ehtPhyHeader->m_ruAllocationA; // RU Allocation-B not supported
                                                                 // yet
        if (const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(20);
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
                             ruAllocation.value(),
                             ehtPhyHeader->m_contentChannels,
                             ehtPhyHeader->m_ppduType == 2,
                             muMimoUsers);
        }
        if (ehtPhyHeader->m_ppduType == 1) // EHT SU
        {
            NS_ASSERT(ehtPhyHeader->m_contentChannels.size() == 1 &&
                      ehtPhyHeader->m_contentChannels.front().size() == 1);
            txVector.SetMode(
                EhtPhy::GetEhtMcs(ehtPhyHeader->m_contentChannels.front().front().mcs));
            txVector.SetNss(ehtPhyHeader->m_contentChannels.front().front().nss);
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
EhtPpdu::GetNumRusPerEhtSigBContentChannel(uint16_t channelWidth,
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
                                                    ruAllocation,
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
EhtPpdu::GetEhtSigFieldSize(uint16_t channelWidth,
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
        if (channelWidth <= 40)
        {
            commonFieldSize += 8; // only one allocation subfield
        }
        else
        {
            commonFieldSize +=
                8 * (channelWidth / 40) /* one allocation field per 40 MHz */ + 1 /* center RU */;
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

Ptr<WifiPpdu>
EhtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new EhtPpdu(*this), false);
}

} // namespace ns3
