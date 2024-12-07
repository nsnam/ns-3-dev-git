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
               TxPsdFlag flag)
    : OfdmPpdu(psdus.begin()->second,
               txVector,
               channel,
               uid,
               false), // don't instantiate LSigHeader of OfdmPpdu
      m_txPsdFlag(flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag);

    // overwrite with map (since only first element used by OfdmPpdu)
    m_psdus.begin()->second = nullptr;
    m_psdus.clear();
    m_psdus = psdus;
    SetPhyHeaders(txVector, ppduDuration);
}

HePpdu::HePpdu(Ptr<const WifiPsdu> psdu,
               const WifiTxVector& txVector,
               const WifiPhyOperatingChannel& channel,
               Time ppduDuration,
               uint64_t uid)
    : OfdmPpdu(psdu,
               txVector,
               channel,
               uid,
               false), // don't instantiate LSigHeader of OfdmPpdu
      m_txPsdFlag(PSD_NON_HE_PORTION)
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << ppduDuration << uid);
    NS_ASSERT(!IsMu());
    SetPhyHeaders(txVector, ppduDuration);
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
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
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
                (txVector.GetChannelWidth() >= MHz_u{80})
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
                         heSigHeader->m_ruAllocation,
                         heSigHeader->m_contentChannels,
                         heSigHeader->m_sigBCompression,
                         GetMuMimoUsersFromEncoding(heSigHeader->m_muMimoUsers));
        txVector.SetSigBMode(HePhy::GetVhtMcs(heSigHeader->m_sigBMcs));
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(MHz_u{20});
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

void
HePpdu::SetHeMuUserInfos(WifiTxVector& txVector,
                         const RuAllocation& ruAllocation,
                         const HeSigBContentChannels& contentChannels,
                         bool sigBcompression,
                         uint8_t numMuMimoUsers) const
{
    std::size_t contentChannelIndex = 0;
    for (const auto& contentChannel : contentChannels)
    {
        std::size_t numRusLeft = 0;
        std::size_t numUsersLeft = 0;
        std::size_t ruAllocIndex = contentChannelIndex;
        for (const auto& userInfo : contentChannel)
        {
            if (userInfo.staId == NO_USER_STA_ID)
            {
                continue;
            }
            if (ruAllocIndex >= ruAllocation.size())
            {
                break;
            }
            auto ruSpecs = HeRu::GetRuSpecs(ruAllocation.at(ruAllocIndex));
            if (ruSpecs.empty())
            {
                continue;
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
            auto ruSpec = ruSpecs.at(ruIndex);
            auto ruType = ruSpec.GetRuType();
            if ((ruAllocation.size() == 8) && (ruType == HeRu::RU_996_TONE) &&
                (((txVector.GetChannelWidth() == MHz_u{160}) && sigBcompression) ||
                 std::all_of(
                     contentChannel.cbegin(),
                     contentChannel.cend(),
                     [&userInfo](const auto& item) { return userInfo.staId == item.staId; })))
            {
                ruType = HeRu::RU_2x996_TONE;
            }
            const auto ruBw = HeRu::GetBandwidth(ruType);
            auto primary80 = ruAllocIndex < 4;
            const uint8_t num20MhzSubchannelsInRu =
                (ruBw < MHz_u{20}) ? 1 : Count20MHzSubchannels(ruBw);
            auto numRuAllocsInContentChannel = std::max(1, num20MhzSubchannelsInRu / 2);
            auto ruIndexOffset = (ruBw < MHz_u{20}) ? (ruSpecs.size() * ruAllocIndex)
                                                    : (ruAllocIndex / num20MhzSubchannelsInRu);
            if (!primary80)
            {
                ruIndexOffset -= HeRu::GetRusOfType(MHz_u{80}, ruType).size();
            }
            if (!txVector.IsAllocated(userInfo.staId))
            {
                txVector.SetHeMuUserInfo(userInfo.staId,
                                         {{ruType, ruSpec.GetIndex() + ruIndexOffset, primary80},
                                          userInfo.mcs,
                                          userInfo.nss});
            }
            if ((ruType == HeRu::RU_2x996_TONE) && !sigBcompression)
            {
                return;
            }
            numRusLeft--;
            numUsersLeft--;
            if (numRusLeft == 0 && numUsersLeft == 0)
            {
                ruAllocIndex += (2 * numRuAllocsInContentChannel);
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

MHz_u
HePpdu::GetTxChannelWidth() const
{
    if (const auto& txVector = GetTxVector();
        txVector.IsValid() && txVector.IsUlMu() && GetStaId() != SU_STA_ID)
    {
        TxPsdFlag flag = GetTxPsdFlag();
        const auto ruWidth = HeRu::GetBandwidth(txVector.GetRu(GetStaId()).GetRuType());
        MHz_u channelWidth =
            (flag == PSD_NON_HE_PORTION && ruWidth < MHz_u{20}) ? MHz_u{20} : ruWidth;
        NS_LOG_INFO("Use channelWidth=" << channelWidth << " MHz for HE TB from " << GetStaId()
                                        << " for " << flag);
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
            {{HeRu::GetRuType(m_txVector->GetChannelWidth()), 1, true}, 0, 1});
    }
}

std::pair<std::size_t, std::size_t>
HePpdu::GetNumRusPerHeSigBContentChannel(MHz_u channelWidth,
                                         const RuAllocation& ruAllocation,
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
        if (channelWidth == MHz_u{20})
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

    switch (static_cast<uint16_t>(channelWidth))
    {
    case 40:
        chSize.second += HeRu::GetRuSpecs(ruAllocation[1]).size();
        [[fallthrough]];
    case 20:
        chSize.first += HeRu::GetRuSpecs(ruAllocation[0]).size();
        break;
    default:
        for (std::size_t n = 0; n < Count20MHzSubchannels(channelWidth);)
        {
            chSize.first += HeRu::GetRuSpecs(ruAllocation[n]).size();
            if (ruAllocation[n] >= 208)
            {
                // 996 tone RU occupies 80 MHz
                n += 4;
                continue;
            }
            n += 2;
        }
        for (std::size_t n = 0; n < Count20MHzSubchannels(channelWidth);)
        {
            chSize.second += HeRu::GetRuSpecs(ruAllocation[n + 1]).size();
            if (ruAllocation[n + 1] >= 208)
            {
                // 996 tone RU occupies 80 MHz
                n += 4;
                continue;
            }
            n += 2;
        }
        break;
    }
    return chSize;
}

HePpdu::HeSigBContentChannels
HePpdu::GetHeSigBContentChannels(const WifiTxVector& txVector, uint8_t p20Index)
{
    HeSigBContentChannels contentChannels{{}};

    const auto channelWidth = txVector.GetChannelWidth();
    if (channelWidth > MHz_u{20})
    {
        contentChannels.emplace_back();
    }

    const auto& orderedMap = txVector.GetUserInfoMapOrderedByRus(p20Index);
    for (const auto& [ru, staIds] : orderedMap)
    {
        const auto ruType = ru.GetRuType();
        if ((ruType > HeRu::RU_242_TONE) && !txVector.IsSigBCompression())
        {
            for (auto i = 0; i < ((ruType == HeRu::RU_2x996_TONE) ? 2 : 1); ++i)
            {
                for (auto staId : staIds)
                {
                    const auto& userInfo = txVector.GetHeMuUserInfo(staId);
                    NS_ASSERT(ru == userInfo.ru);
                    contentChannels[0].push_back({staId, userInfo.nss, userInfo.mcs});
                    contentChannels[1].push_back({staId, userInfo.nss, userInfo.mcs});
                }
            }
            continue;
        }

        std::size_t numRus = (ruType >= HeRu::RU_242_TONE)
                                 ? 1
                                 : HeRu::m_heRuSubcarrierGroups.at({MHz_u{20}, ruType}).size();
        const auto ruIdx = ru.GetIndex();
        for (auto staId : staIds)
        {
            const auto& userInfo = txVector.GetHeMuUserInfo(staId);
            NS_ASSERT(ru == userInfo.ru);
            std::size_t ccIndex{0};
            if (channelWidth < MHz_u{40})
            {
                // only one content channel
                ccIndex = 0;
            }
            else if (txVector.IsSigBCompression())
            {
                // equal split
                ccIndex = (contentChannels.at(0).size() <= contentChannels.at(1).size()) ? 0 : 1;
            }
            else // MU-MIMO
            {
                ccIndex = (((ruIdx - 1) / numRus) % 2 == 0) ? 0 : 1;
            }
            contentChannels.at(ccIndex).push_back({staId, userInfo.nss, userInfo.mcs});
        }
    }

    const auto isSigBCompression = txVector.IsSigBCompression();
    if (!isSigBCompression)
    {
        // Add unassigned RUs
        auto numNumRusPerHeSigBContentChannel = GetNumRusPerHeSigBContentChannel(
            channelWidth,
            txVector.GetRuAllocation(p20Index),
            isSigBCompression,
            isSigBCompression ? txVector.GetHeMuUserInfoMap().size() : 0);
        std::size_t contentChannelIndex = 1;
        for (auto& contentChannel : contentChannels)
        {
            const auto totalUsersInContentChannel = (contentChannelIndex == 1)
                                                        ? numNumRusPerHeSigBContentChannel.first
                                                        : numNumRusPerHeSigBContentChannel.second;
            NS_ASSERT(contentChannel.size() <= totalUsersInContentChannel);
            std::size_t unallocatedRus = totalUsersInContentChannel - contentChannel.size();
            for (std::size_t i = 0; i < unallocatedRus; i++)
            {
                contentChannel.push_back({NO_USER_STA_ID, 0, 0});
            }
            contentChannelIndex++;
        }
    }

    return contentChannels;
}

uint32_t
HePpdu::GetSigBFieldSize(MHz_u channelWidth,
                         const RuAllocation& ruAllocation,
                         bool sigBCompression,
                         std::size_t numMuMimoUsers)
{
    // Compute the number of bits used by common field.
    uint32_t commonFieldSize = 0;
    if (!sigBCompression)
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

    auto numRusPerContentChannel = GetNumRusPerHeSigBContentChannel(channelWidth,
                                                                    ruAllocation,
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
HePpdu::GetChannelWidthEncodingFromMhz(MHz_u channelWidth)
{
    if (channelWidth == MHz_u{160})
    {
        return 3;
    }
    else if (channelWidth == MHz_u{80})
    {
        return 2;
    }
    else if (channelWidth == MHz_u{40})
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

MHz_u
HePpdu::GetChannelWidthMhzFromEncoding(uint8_t bandwidth)
{
    if (bandwidth == 3)
    {
        return MHz_u{160};
    }
    else if (bandwidth == 2)
    {
        return MHz_u{80};
    }
    else if (bandwidth == 1)
    {
        return MHz_u{40};
    }
    else
    {
        return MHz_u{20};
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
