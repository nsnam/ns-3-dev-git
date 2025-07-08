/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HeSigHeader)
 */

#include "he-ppdu.h"

#include "he-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HePpdu");

std::ostream&
operator<<(std::ostream& os, const HePpdu::TxPsdFlag& flag)
{
    switch (flag)
    {
    case HePpdu::PSD_NON_HE_PORTION:
        return (os << "PSD_NON_HE_PORTION");
    case HePpdu::PSD_HE_PORTION:
        return (os << "PSD_HE_PORTION");
    default:
        NS_FATAL_ERROR("Invalid PSD flag");
        return (os << "INVALID");
    }
}

HePpdu::HePpdu(const WifiConstPsduMap& psdus,
               const WifiTxVector& txVector,
               const WifiPhyOperatingChannel& channel,
               Time ppduDuration,
               uint64_t uid,
               TxPsdFlag flag,
               bool instantiateHeaders /* = true */)
    : OfdmPpdu(psdus.begin()->second,
               txVector,
               channel,
               uid,
               false), // don't instantiate LSigHeader of OfdmPpdu
      m_txPsdFlag(flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag
                         << instantiateHeaders);

    // overwrite with map (since only first element used by OfdmPpdu)
    m_psdus.begin()->second = nullptr;
    m_psdus.clear();
    m_psdus = psdus;
    if (instantiateHeaders)
    {
        SetPhyHeaders(txVector, ppduDuration);
    }
}

HePpdu::HePpdu(Ptr<const WifiPsdu> psdu,
               const WifiTxVector& txVector,
               const WifiPhyOperatingChannel& channel,
               Time ppduDuration,
               uint64_t uid,
               bool instantiateHeaders /* = true */)
    : OfdmPpdu(psdu,
               txVector,
               channel,
               uid,
               false), // don't instantiate LSigHeader of OfdmPpdu
      m_txPsdFlag(PSD_NON_HE_PORTION)
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << ppduDuration << uid
                         << instantiateHeaders);
    NS_ASSERT(!IsMu());
    if (instantiateHeaders)
    {
        SetPhyHeaders(txVector, ppduDuration);
    }
}

void
HePpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);
    SetLSigHeader(ppduDuration);
    SetHeSigHeader(txVector);
}

void
HePpdu::SetLSigHeader(Time ppduDuration)
{
    uint8_t sigExtension = 0;
    NS_ASSERT(m_operatingChannel.IsSet());
    if (m_operatingChannel.GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
    {
        sigExtension = 6;
    }
    uint8_t m = IsDlMu() ? 1 : 2;
    uint16_t length = ((ceil((static_cast<double>(ppduDuration.GetNanoSeconds() - (20 * 1000) -
                                                  (sigExtension * 1000)) /
                              1000) /
                             4.0) *
                        3) -
                       3 - m);
    m_lSig.SetLength(length);
}

void
HePpdu::SetHeSigHeader(const WifiTxVector& txVector)
{
    const auto bssColor = txVector.GetBssColor();
    NS_ASSERT(bssColor < 64);
    if (ns3::IsUlMu(m_preamble))
    {
        m_heSig.emplace<HeTbSigHeader>(HeTbSigHeader{
            .m_bssColor = bssColor,
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth())});
    }
    else if (ns3::IsDlMu(m_preamble))
    {
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_t{20});
        const uint8_t noMuMimoUsers{0};
        m_heSig.emplace<HeMuSigHeader>(HeMuSigHeader{
            .m_bssColor = bssColor,
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth()),
            .m_sigBMcs = txVector.GetSigBMode().GetMcsValue(),
            .m_muMimoUsers = (txVector.IsSigBCompression()
                                  ? GetMuMimoUsersEncoding(txVector.GetHeMuUserInfoMap().size())
                                  : noMuMimoUsers),
            .m_sigBCompression = txVector.IsSigBCompression(),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            .m_ruAllocation = txVector.GetRuAllocation(p20Index),
            .m_contentChannels = GetHeSigBContentChannels(txVector, p20Index),
            .m_center26ToneRuIndication =
                (txVector.GetChannelWidth() >= MHz_t{80})
                    ? std::optional{txVector.GetCenter26ToneRuIndication()}
                    : std::nullopt});
    }
    else
    {
        const auto mcs = txVector.GetMode().GetMcsValue();
        NS_ASSERT(mcs <= 11);
        m_heSig.emplace<HeSuSigHeader>(HeSuSigHeader{
            .m_bssColor = bssColor,
            .m_mcs = mcs,
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth()),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            .m_nsts = GetNstsEncodingFromNss(txVector.GetNss())});
    }
}

WifiTxVector
HePpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    SetTxVectorFromPhyHeaders(txVector);
    return txVector;
}

void
HePpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const
{
    txVector.SetLength(m_lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    if (!IsMu())
    {
        auto heSigHeader = std::get_if<HeSuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader && (heSigHeader->m_format == 1));
        txVector.SetMode(HePhy::GetHeMcs(heSigHeader->m_mcs));
        txVector.SetNss(GetNssFromNstsEncoding(heSigHeader->m_nsts));
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(heSigHeader->m_giLtfSize));
        txVector.SetBssColor(heSigHeader->m_bssColor);
    }
    else if (IsUlMu())
    {
        auto heSigHeader = std::get_if<HeTbSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader && (heSigHeader->m_format == 0));
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetBssColor(heSigHeader->m_bssColor);
    }
    else if (IsDlMu())
    {
        auto heSigHeader = std::get_if<HeMuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(heSigHeader->m_giLtfSize));
        txVector.SetBssColor(heSigHeader->m_bssColor);
        SetHeMuUserInfos(txVector,
                         WIFI_MOD_CLASS_HE,
                         heSigHeader->m_ruAllocation,
                         heSigHeader->m_center26ToneRuIndication,
                         heSigHeader->m_contentChannels,
                         heSigHeader->m_sigBCompression,
                         GetMuMimoUsersFromEncoding(heSigHeader->m_muMimoUsers));
        txVector.SetSigBMode(HePhy::GetVhtMcs(heSigHeader->m_sigBMcs));
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_t{20});
        txVector.SetRuAllocation(heSigHeader->m_ruAllocation, p20Index);
        if (heSigHeader->m_center26ToneRuIndication.has_value())
        {
            txVector.SetCenter26ToneRuIndication(heSigHeader->m_center26ToneRuIndication.value());
        }
        if (heSigHeader->m_sigBCompression)
        {
            NS_ASSERT(GetMuMimoUsersFromEncoding(heSigHeader->m_muMimoUsers) ==
                      txVector.GetHeMuUserInfoMap().size());
        }
    }
}

WifiRu::RuSpec
HePpdu::GetRuSpec(std::size_t ruAllocIndex, MHz_t bw, RuType ruType, std::size_t phyIndex) const
{
    auto isPrimary80{true};
    auto index{phyIndex};
    if (bw > MHz_t{80})
    {
        const auto isLow80 = ruAllocIndex < 4;
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_t{20});
        const auto primary80IsLower80 = (p20Index < bw / MHz_t{40});
        if (!isLow80)
        {
            const auto numRusP80 = HeRu::GetRusOfType(MHz_t{80}, ruType).size();
            index -= (ruType == RuType::RU_26_TONE) ? (numRusP80 - 1) : numRusP80;
        }
        isPrimary80 = ((primary80IsLower80 && isLow80) || (!primary80IsLower80 && !isLow80));
    }
    if ((ruType == RuType::RU_26_TONE) && (ruAllocIndex >= 2) && (index >= 19))
    {
        index++;
    }
    return HeRu::RuSpec{ruType, index, isPrimary80};
}

void
HePpdu::SetHeMuUserInfos(WifiTxVector& txVector,
                         WifiModulationClass mc,
                         const RuAllocation& ruAllocation,
                         std::optional<Center26ToneRuIndication> center26ToneRuIndication,
                         const HeSigBContentChannels& contentChannels,
                         bool sigBcompression,
                         uint8_t numMuMimoUsers) const
{
    NS_ASSERT(ruAllocation.size() == Count20MHzSubchannels(txVector.GetChannelWidth()));
    std::vector<uint8_t> remainingRuAllocIndices(ruAllocation.size());
    std::iota(remainingRuAllocIndices.begin(), remainingRuAllocIndices.end(), 0);
    std::size_t contentChannelIndex = 0;
    std::size_t ruAllocIndex = 0;
    for (const auto& contentChannel : contentChannels)
    {
        std::size_t numRusLeft = 0;
        std::size_t numUsersLeft = 0;
        ruAllocIndex = remainingRuAllocIndices.front();
        std::size_t numUsersLeftInCc = contentChannel.size();
        if (contentChannel.empty())
        {
            const auto pos = std::find(remainingRuAllocIndices.cbegin(),
                                       remainingRuAllocIndices.cend(),
                                       ruAllocIndex);
            remainingRuAllocIndices.erase(pos);
            ++contentChannelIndex;
            continue;
        }
        for (const auto& userInfo : contentChannel)
        {
            if (center26ToneRuIndication && (numUsersLeftInCc == 1))
            {
                // handle central 26 tones
                if ((contentChannelIndex == 0) &&
                    ((*center26ToneRuIndication ==
                      Center26ToneRuIndication::CENTER_26_TONE_RU_LOW_80_MHZ_ALLOCATED) ||
                     (*center26ToneRuIndication ==
                      Center26ToneRuIndication::CENTER_26_TONE_RU_LOW_AND_HIGH_80_MHZ_ALLOCATED)))
                {
                    txVector.SetHeMuUserInfo(
                        userInfo.staId,
                        {HeRu::RuSpec{RuType::RU_26_TONE, 19, true}, userInfo.mcs, userInfo.nss});
                    continue;
                }
                else if ((contentChannelIndex == 1) &&
                         ((*center26ToneRuIndication ==
                           Center26ToneRuIndication::CENTER_26_TONE_RU_HIGH_80_MHZ_ALLOCATED) ||
                          (*center26ToneRuIndication ==
                           Center26ToneRuIndication::
                               CENTER_26_TONE_RU_LOW_AND_HIGH_80_MHZ_ALLOCATED)))
                {
                    txVector.SetHeMuUserInfo(
                        userInfo.staId,
                        {HeRu::RuSpec{RuType::RU_26_TONE, 19, false}, userInfo.mcs, userInfo.nss});
                    continue;
                }
            }
            NS_ASSERT(ruAllocIndex < ruAllocation.size());
            auto ruSpecs = WifiRu::GetRuSpecs(ruAllocation.at(ruAllocIndex), mc);
            while (ruSpecs.empty() && (ruAllocIndex < ruAllocation.size()))
            {
                const auto pos = std::find(remainingRuAllocIndices.cbegin(),
                                           remainingRuAllocIndices.cend(),
                                           ruAllocIndex);
                remainingRuAllocIndices.erase(pos);
                ruAllocIndex += 2;
                NS_ASSERT(ruAllocIndex < ruAllocation.size());
                ruSpecs = WifiRu::GetRuSpecs(ruAllocation.at(ruAllocIndex), mc);
            }
            if (numRusLeft == 0)
            {
                numRusLeft = ruSpecs.size();
            }
            if (numUsersLeft == 0)
            {
                if (sigBcompression)
                {
                    numUsersLeft = numMuMimoUsers;
                }
                else
                {
                    // not MU-MIMO
                    numUsersLeft = 1;
                }
            }
            auto ruIndex = (ruSpecs.size() - numRusLeft);
            const auto ruSpec = ruSpecs.at(ruIndex);
            auto ruType = WifiRu::GetRuType(ruSpec);
            if (sigBcompression)
            {
                ruType = WifiRu::GetRuType(ruAllocation.size() * MHz_t{20});
            }
            const auto num20MhzSubchannelsInRu = WifiRu::GetNum20MHzSubchannelsInRu(ruType);
            if (userInfo.staId != NO_USER_STA_ID)
            {
                const auto ruBw = WifiRu::GetBandwidth(ruType);
                const std::size_t numRus =
                    (ruBw > MHz_t{20}) ? 1 : HeRu::GetNRus(MHz_t{20}, ruType);
                const std::size_t ruIndexOffset = (ruBw < MHz_t{20})
                                                      ? (numRus * ruAllocIndex)
                                                      : (ruAllocIndex / num20MhzSubchannelsInRu);
                std::size_t phyIndex = WifiRu::GetIndex(ruSpecs.at(ruIndex)) + ruIndexOffset;
                const auto ru{
                    GetRuSpec(ruAllocIndex, txVector.GetChannelWidth(), ruType, phyIndex)};
                txVector.SetHeMuUserInfo(userInfo.staId, {ru, userInfo.mcs, userInfo.nss});
            }
            numRusLeft--;
            numUsersLeft--;
            numUsersLeftInCc--;
            if (numRusLeft == 0 && numUsersLeft == 0)
            {
                const auto pos = std::find(remainingRuAllocIndices.cbegin(),
                                           remainingRuAllocIndices.cend(),
                                           ruAllocIndex);
                remainingRuAllocIndices.erase(pos);
                ruAllocIndex += num20MhzSubchannelsInRu;
                if (ruAllocIndex % 2 != contentChannelIndex)
                {
                    ++ruAllocIndex;
                }
            }
        }
        contentChannelIndex++;
    }
}

Time
HePpdu::GetTxDuration() const
{
    Time ppduDuration;
    const auto& txVector = GetTxVector();
    const auto length = m_lSig.GetLength();
    const auto tSymbol = HePhy::GetSymbolDuration(txVector.GetGuardInterval());
    const auto preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    NS_ASSERT(m_operatingChannel.IsSet());
    uint8_t sigExtension = (m_operatingChannel.GetPhyBand() == WIFI_PHY_BAND_2_4GHZ) ? 6 : 0;
    uint8_t m = IsDlMu() ? 1 : 2;
    // Equation 27-11 of IEEE P802.11ax/D4.0
    const auto calculatedDuration =
        MicroSeconds(((ceil(static_cast<double>(length + 3 + m) / 3)) * 4) + 20 + sigExtension);
    NS_ASSERT(calculatedDuration > preambleDuration);
    uint32_t nSymbols =
        floor(static_cast<double>((calculatedDuration - preambleDuration).GetNanoSeconds() -
                                  (sigExtension * 1000)) /
              tSymbol.GetNanoSeconds());
    return (preambleDuration + (nSymbols * tSymbol) + MicroSeconds(sigExtension));
}

Ptr<WifiPpdu>
HePpdu::Copy() const
{
    return Ptr<WifiPpdu>(new HePpdu(*this), false);
}

WifiPpduType
HePpdu::GetType() const
{
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_HE_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_HE_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    default:
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
HePpdu::IsMu() const
{
    return (IsDlMu() || IsUlMu());
}

bool
HePpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_HE_MU);
}

bool
HePpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_HE_TB);
}

Ptr<const WifiPsdu>
HePpdu::GetPsdu(uint8_t bssColor, uint16_t staId /* = SU_STA_ID */) const
{
    if (!IsMu())
    {
        NS_ASSERT(m_psdus.size() == 1);
        return m_psdus.at(SU_STA_ID);
    }

    if (IsUlMu())
    {
        auto heSigHeader = std::get_if<HeTbSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        NS_ASSERT(m_psdus.size() == 1);
        if ((bssColor == 0) || (heSigHeader->m_bssColor == 0) ||
            (bssColor == heSigHeader->m_bssColor))
        {
            return m_psdus.cbegin()->second;
        }
    }
    else
    {
        auto heSigHeader = std::get_if<HeMuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        if ((bssColor == 0) || (heSigHeader->m_bssColor == 0) ||
            (bssColor == heSigHeader->m_bssColor))
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

uint16_t
HePpdu::GetStaId() const
{
    NS_ASSERT(IsUlMu());
    return m_psdus.begin()->first;
}

MHz_t
HePpdu::GetTxChannelWidth() const
{
    if (const auto& txVector = GetTxVector();
        txVector.IsValid() && txVector.IsUlMu() && GetStaId() != SU_STA_ID)
    {
        TxPsdFlag flag = GetTxPsdFlag();
        const auto ruWidth = WifiRu::GetBandwidth(WifiRu::GetRuType(txVector.GetRu(GetStaId())));
        MHz_t channelWidth =
            (flag == PSD_NON_HE_PORTION && ruWidth < MHz_t{20}) ? MHz_t{20} : ruWidth;
        NS_LOG_INFO("Use " << channelWidth << " MHz for TB PPDU from " << GetStaId() << " for "
                           << flag);
        return channelWidth;
    }
    else
    {
        return OfdmPpdu::GetTxChannelWidth();
    }
}

HePpdu::TxPsdFlag
HePpdu::GetTxPsdFlag() const
{
    return m_txPsdFlag;
}

void
HePpdu::SetTxPsdFlag(TxPsdFlag flag) const
{
    NS_LOG_FUNCTION(this << flag);
    m_txPsdFlag = flag;
}

void
HePpdu::UpdateTxVectorForUlMu(const std::optional<WifiTxVector>& trigVector) const
{
    if (trigVector.has_value())
    {
        NS_LOG_FUNCTION(this << trigVector.value());
    }
    else
    {
        NS_LOG_FUNCTION(this);
    }
    if (!m_txVector.has_value())
    {
        m_txVector = GetTxVector();
    }
    NS_ASSERT(GetModulation() >= WIFI_MOD_CLASS_HE);
    NS_ASSERT(GetType() == WIFI_PPDU_TYPE_UL_MU);
    // HE TB PPDU reception needs information from the TRIGVECTOR to be able to receive the PPDU
    const auto staId = GetStaId();
    if (trigVector.has_value() && trigVector->IsUlMu() &&
        (trigVector->GetHeMuUserInfoMap().contains(staId)))
    {
        // These information are not carried in HE-SIG-A for a HE TB PPDU,
        // but they are carried in the Trigger frame soliciting the HE TB PPDU
        m_txVector->SetGuardInterval(trigVector->GetGuardInterval());
        m_txVector->SetHeMuUserInfo(staId, trigVector->GetHeMuUserInfo(staId));
    }
    else
    {
        // Set dummy user info, PPDU will be dropped later after decoding PHY headers.
        m_txVector->SetHeMuUserInfo(
            staId,
            {HeRu::RuSpec{(WifiRu::GetRuType(m_txVector->GetChannelWidth())), 1, true}, 0, 1});
    }
}

std::pair<std::size_t, std::size_t>
HePpdu::GetNumRusPerHeSigBContentChannel(
    MHz_t channelWidth,
    WifiModulationClass mc,
    const RuAllocation& ruAllocation,
    std::optional<Center26ToneRuIndication> center26ToneRuIndication,
    bool sigBCompression,
    uint8_t numMuMimoUsers)
{
    std::pair<std::size_t /* number of RUs in content channel 1 */,
              std::size_t /* number of RUs in content channel 2 */>
        chSize{0, 0};

    if (sigBCompression)
    {
        // If the HE-SIG-B Compression field in the HE-SIG-A field of an HE MU PPDU is 1,
        // for bandwidths larger than 20 MHz, the AP performs an equitable split of
        // the User fields between two HE-SIG-B content channels
        if (channelWidth == MHz_t{20})
        {
            return {numMuMimoUsers, 0};
        }
        chSize.first = numMuMimoUsers / 2;
        chSize.second = numMuMimoUsers / 2;
        if (numMuMimoUsers != (chSize.first + chSize.second))
        {
            chSize.first++;
        }
        return chSize;
    }

    NS_ASSERT_MSG(!ruAllocation.empty(), "RU allocation is not set");
    NS_ASSERT_MSG(ruAllocation.size() == Count20MHzSubchannels(channelWidth),
                  "RU allocation is not consistent with packet bandwidth");

    switch (static_cast<uint16_t>(channelWidth.in_MHz()))
    {
    case 40:
        chSize.second += WifiRu::GetRuSpecs(ruAllocation[1], mc).size();
        [[fallthrough]];
    case 20:
        chSize.first += WifiRu::GetRuSpecs(ruAllocation[0], mc).size();
        break;
    default:
        for (std::size_t n = 0; n < Count20MHzSubchannels(channelWidth);)
        {
            std::size_t ccIndex;
            const auto ruAlloc = ruAllocation.at(n);
            std::size_t num20MHz{1};
            const auto ruSpecs = WifiRu::GetRuSpecs(ruAlloc, mc);
            const auto nRuSpecs = ruSpecs.size();
            if (nRuSpecs == 1)
            {
                const auto ruBw = WifiRu::GetBandwidth(WifiRu::GetRuType(ruSpecs.front()));
                num20MHz = Count20MHzSubchannels(ruBw);
            }
            if (nRuSpecs == 0)
            {
                ++n;
                continue;
            }
            if (num20MHz > 1)
            {
                ccIndex = (chSize.first <= chSize.second) ? 0 : 1;
            }
            else
            {
                ccIndex = (n % 2 == 0) ? 0 : 1;
            }
            if (ccIndex == 0)
            {
                chSize.first += nRuSpecs;
            }
            else
            {
                chSize.second += nRuSpecs;
            }
            if (num20MHz > 1)
            {
                const auto skipNumIndices = (ccIndex == 0) ? num20MHz : num20MHz - 1;
                n += skipNumIndices;
            }
            else
            {
                ++n;
            }
        }
        break;
    }
    if (center26ToneRuIndication)
    {
        switch (*center26ToneRuIndication)
        {
        case Center26ToneRuIndication::CENTER_26_TONE_RU_LOW_80_MHZ_ALLOCATED:
            chSize.first++;
            break;
        case Center26ToneRuIndication::CENTER_26_TONE_RU_HIGH_80_MHZ_ALLOCATED:
            chSize.second++;
            break;
        case Center26ToneRuIndication::CENTER_26_TONE_RU_LOW_AND_HIGH_80_MHZ_ALLOCATED:
            chSize.first++;
            chSize.second++;
            break;
        case Center26ToneRuIndication::CENTER_26_TONE_RU_UNALLOCATED:
        default:
            break;
        }
    }
    return chSize;
}

HePpdu::HeSigBContentChannels
HePpdu::GetHeSigBContentChannels(const WifiTxVector& txVector, uint8_t p20Index)
{
    HeSigBContentChannels contentChannels{{}};

    const auto channelWidth = txVector.GetChannelWidth();
    if (channelWidth > MHz_t{20})
    {
        contentChannels.emplace_back();
    }

    const auto mc = txVector.GetModulationClass();
    const auto& ruAllocs = txVector.GetRuAllocation(p20Index);
    std::vector<std::vector<WifiRu::RuSpec>> ruSpecs;
    ruSpecs.reserve(ruAllocs.size());
    std::transform(ruAllocs.cbegin(),
                   ruAllocs.cend(),
                   std::back_inserter(ruSpecs),
                   [mc](const auto ruAlloc) { return WifiRu::GetRuSpecs(ruAlloc, mc); });

    std::optional<HeSigBUserSpecificField> cc1Central26ToneRu;
    std::optional<HeSigBUserSpecificField> cc2Central26ToneRu;

    const auto& orderedMap = txVector.GetUserInfoMapOrderedByRus(p20Index);
    std::optional<std::size_t> prevRuSpecsIdx;
    std::size_t nAllocatedUsers{0};
    for (const auto& [ru, staIds] : orderedMap)
    {
        const auto ruType = WifiRu::GetRuType(ru);
        auto ruIdx = WifiRu::GetIndex(ru);
        auto ruPhyIndex = WifiRu::GetPhyIndex(ru, channelWidth, p20Index);
        if ((ruType == RuType::RU_26_TONE) && (ruIdx == 19))
        {
            NS_ASSERT(WifiRu::IsHe(ru));
            const auto staId = *staIds.cbegin();
            const auto& userInfo = txVector.GetHeMuUserInfo(staId);
            if (std::get<HeRu::RuSpec>(ru).GetPrimary80MHz())
            {
                NS_ASSERT(!cc1Central26ToneRu);
                cc1Central26ToneRu = HeSigBUserSpecificField{staId, userInfo.nss, userInfo.mcs};
                ++nAllocatedUsers;
            }
            else
            {
                NS_ASSERT(!cc2Central26ToneRu);
                cc2Central26ToneRu = HeSigBUserSpecificField{staId, userInfo.nss, userInfo.mcs};
                ++nAllocatedUsers;
            }
            continue;
        }

        // special case for RUs that are larger than 20 MHz: equally split the users
        // between the two content channels, since they are not allocated in a specific 20 MHz
        if (ruType >= RuType::RU_484_TONE)
        {
            for (auto staId : staIds)
            {
                const auto ccIndex =
                    (contentChannels.at(0).size() <= contentChannels.at(1).size()) ? 0 : 1;
                const auto& userInfo = txVector.GetHeMuUserInfo(staId);
                NS_ASSERT(ru == userInfo.ru);
                contentChannels[ccIndex].push_back({staId, userInfo.nss, userInfo.mcs});
            }
            continue;
        }

        if ((ruType == RuType::RU_26_TONE) && (channelWidth >= MHz_t{80}))
        {
            // "ignore" the center 26-tone RUs in 80 MHz channels
            ruIdx = (ruIdx > 19) ? (ruIdx - 1) : ruIdx;
            ruPhyIndex = (ruPhyIndex > 19) ? (ruPhyIndex - 1) : ruPhyIndex;
            if (ruPhyIndex > 37)
            {
                ruPhyIndex -= (ruPhyIndex - 19) / 37;
            }
        }

        const auto numRus = WifiRu::GetNRus(MHz_t{20}, ruType, mc);
        std::size_t ccIndex{0};
        if (channelWidth < MHz_t{40})
        {
            // only one content channel
            ccIndex = 0;
        }
        else if (txVector.IsSigBCompression())
        {
            // MU-MIMO: equal split
            ccIndex = (contentChannels.at(0).size() <= contentChannels.at(1).size()) ? 0 : 1;
        }
        else
        {
            ccIndex = (((ruIdx - 1) / numRus) % 2 == 0) ? 0 : 1;
        }
        const auto ruSpecsIdxIn80MHz = (ruIdx - 1) / numRus;
        const auto ruOffsetIn80MHz = ruSpecsIdxIn80MHz * numRus;
        auto ruSpecsIdx = (ruPhyIndex - 1) / numRus;
        if (prevRuSpecsIdx && *prevRuSpecsIdx != ruSpecsIdx)
        {
            // we are in a different 20 MHz subchannel, we need to fill the remaining RUs in
            // previous 20 MHz subchannel that are not allocated (if any)
            auto& rus = ruSpecs.at(*prevRuSpecsIdx);
            for (std::size_t i = 0; i < rus.size(); ++i)
            {
                // since we are in a different 20 MHz subchannel than the one being processed, we
                // need to use the other content channel index
                const auto otherCcIndex = (ccIndex == 0) ? 1 : 0;
                contentChannels.at(otherCcIndex).push_back({.staId = NO_USER_STA_ID});
            }
        }
        prevRuSpecsIdx = ruSpecsIdx;
        auto& rus = ruSpecs.at(ruSpecsIdx);
        // loop over the RUs in the current 20 MHz subchannel and fill in the RUs that are not
        // allocated between the previous allocated RU and the current one
        while (!rus.empty() && ((WifiRu::GetRuType(rus.front()) != ruType) ||
                                (WifiRu::GetIndex(rus.front()) != (ruIdx - ruOffsetIn80MHz))))
        {
            contentChannels.at(ccIndex).push_back({.staId = NO_USER_STA_ID});
            // allocated current RU to an empty user, so we can remove it from the list
            rus.erase(rus.cbegin());
        }
        // fill the content channel with the current RU and the user(s) allocated to it
        for (auto staId : staIds)
        {
            const auto& userInfo = txVector.GetHeMuUserInfo(staId);
            NS_ASSERT(ru == userInfo.ru);
            contentChannels.at(ccIndex).push_back({staId, userInfo.nss, userInfo.mcs});
        }
        // remove the current RU from the list, since it has been allocated
        if (!rus.empty())
        {
            rus.erase(rus.cbegin());
        }
        ++nAllocatedUsers;
        // we reached the last allocated user in the last 20 MHz subchannel, we need to fill the
        // remaining RUs that are not allocated (if any)
        if (nAllocatedUsers == orderedMap.size())
        {
            while (!rus.empty())
            {
                contentChannels.at(ccIndex).push_back({.staId = NO_USER_STA_ID});
                auto rusBegin = rus.begin();
                rus.erase(rusBegin);
            }
        }
    }

    // finally, add the center 26-tone RUs, if any, to the content channels
    if (cc1Central26ToneRu)
    {
        contentChannels.at(0).push_back(*cc1Central26ToneRu);
    }
    if (cc2Central26ToneRu)
    {
        contentChannels.at(1).push_back(*cc2Central26ToneRu);
    }

#ifdef NS3_BUILD_PROFILE_DEBUG
    // check that the content channels returned by the function match with
    // GetNumRusPerHeSigBContentChannel
    const auto isSigBCompression = txVector.IsSigBCompression();
    if (!isSigBCompression)
    {
        auto numNumRusPerHeSigBContentChannel = GetNumRusPerHeSigBContentChannel(
            channelWidth,
            mc,
            txVector.GetRuAllocation(p20Index),
            txVector.GetCenter26ToneRuIndication(),
            isSigBCompression,
            isSigBCompression ? txVector.GetHeMuUserInfoMap().size() : 0);
        std::size_t contentChannelIndex = 1;
        for (auto& contentChannel : contentChannels)
        {
            const auto totalUsersInContentChannel = (contentChannelIndex == 1)
                                                        ? numNumRusPerHeSigBContentChannel.first
                                                        : numNumRusPerHeSigBContentChannel.second;
            NS_ASSERT(contentChannel.size() == totalUsersInContentChannel);
            contentChannelIndex++;
        }
    }
#endif // NS3_BUILD_PROFILE_DEBUG

    return contentChannels;
}

uint32_t
HePpdu::GetSigBFieldSize(MHz_t channelWidth,
                         WifiModulationClass mc,
                         const RuAllocation& ruAllocation,
                         std::optional<Center26ToneRuIndication> center26ToneRuIndication,
                         bool sigBCompression,
                         std::size_t numMuMimoUsers)
{
    // Compute the number of bits used by common field.
    uint32_t commonFieldSize = 0;
    if (!sigBCompression)
    {
        commonFieldSize = 4 /* CRC */ + 6 /* tail */;
        if (channelWidth <= MHz_t{40})
        {
            commonFieldSize += 8; // only one allocation subfield
        }
        else
        {
            commonFieldSize +=
                8 * (channelWidth / MHz_t{40}) /* one allocation field per 40 MHz */ +
                1 /* center RU */;
        }
    }

    auto numRusPerContentChannel = GetNumRusPerHeSigBContentChannel(channelWidth,
                                                                    mc,
                                                                    ruAllocation,
                                                                    center26ToneRuIndication,
                                                                    sigBCompression,
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

std::string
HePpdu::PrintPayload() const
{
    std::ostringstream ss;
    if (IsMu())
    {
        ss << m_psdus;
        ss << ", " << m_txPsdFlag;
    }
    else
    {
        ss << "PSDU=" << m_psdus.at(SU_STA_ID) << " ";
    }
    return ss.str();
}

uint8_t
HePpdu::GetChannelWidthEncodingFromMhz(MHz_t channelWidth)
{
    if (channelWidth == MHz_t{160})
    {
        return 3;
    }
    else if (channelWidth == MHz_t{80})
    {
        return 2;
    }
    else if (channelWidth == MHz_t{40})
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

MHz_t
HePpdu::GetChannelWidthMhzFromEncoding(uint8_t bandwidth)
{
    if (bandwidth == 3)
    {
        return MHz_t{160};
    }
    else if (bandwidth == 2)
    {
        return MHz_t{80};
    }
    else if (bandwidth == 1)
    {
        return MHz_t{40};
    }
    else
    {
        return MHz_t{20};
    }
}

uint8_t
HePpdu::GetGuardIntervalAndNltfEncoding(Time guardInterval, uint8_t nltf)
{
    const auto gi = guardInterval.GetNanoSeconds();
    if ((gi == 800) && (nltf == 1))
    {
        return 0;
    }
    else if ((gi == 800) && (nltf == 2))
    {
        return 1;
    }
    else if ((gi == 1600) && (nltf == 2))
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

Time
HePpdu::GetGuardIntervalFromEncoding(uint8_t giAndNltfSize)
{
    if (giAndNltfSize == 3)
    {
        // we currently do not consider DCM nor STBC fields
        return NanoSeconds(3200);
    }
    else if (giAndNltfSize == 2)
    {
        return NanoSeconds(1600);
    }
    else
    {
        return NanoSeconds(800);
    }
}

uint8_t
HePpdu::GetNstsEncodingFromNss(uint8_t nss)
{
    NS_ASSERT(nss <= 8);
    return nss - 1;
}

uint8_t
HePpdu::GetNssFromNstsEncoding(uint8_t nsts)
{
    return nsts + 1;
}

uint8_t
HePpdu::GetMuMimoUsersEncoding(uint8_t nUsers)
{
    NS_ASSERT(nUsers <= 8);
    return (nUsers - 1);
}

uint8_t
HePpdu::GetMuMimoUsersFromEncoding(uint8_t encoding)
{
    return (encoding + 1);
}

} // namespace ns3
