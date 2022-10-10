/*
 * Copyright (c) 2020 Orange Labs
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
    if (txVector.IsMu())
    {
        for (const auto& heMuUserInfo : txVector.GetHeMuUserInfoMap())
        {
            auto [it, ret] = m_muUserInfos.emplace(heMuUserInfo);
            NS_ABORT_MSG_IF(!ret, "STA-ID " << heMuUserInfo.first << " already present");
        }
    }
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

#ifdef NS3_BUILD_PROFILE_DEBUG
    LSigHeader lSig;
    SetLSigHeader(lSig, ppduDuration);

    HeSigHeader heSig;
    SetHeSigHeader(heSig, txVector);

    m_phyHeaders->AddHeader(heSig);
    m_phyHeaders->AddHeader(lSig);
#else
    SetLSigHeader(m_lSig, ppduDuration);
    SetHeSigHeader(m_heSig, txVector);
#endif
}

void
HePpdu::SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const
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
    lSig.SetLength(length);
}

void
HePpdu::SetHeSigHeader(HeSigHeader& heSig, const WifiTxVector& txVector) const
{
    heSig.SetFormat(m_preamble);
    heSig.SetChannelWidth(txVector.GetChannelWidth());
    // TODO: EHT PHY headers not implemented yet, hence we do not fill in HE-SIG-B for EHT SU
    /* See section 36.3.12.8.2 of IEEE 802.11be D3.0 (EHT-SIG content channels):
     * In non-OFDMA transmission, the Common field of the EHT-SIG content channel does not contain
     * the RU Allocation subfield. For non-OFDMA transmission except for EHT sounding NDP, the
     * Common field of the EHT-SIG content channel is encoded together with the first User field and
     * this encoding block contains a CRC and Tail, referred to as a common encoding block. */
    if (txVector.IsDlMu())
    {
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(20);
        heSig.SetMuFlag(true);
        heSig.SetSigBMcs(txVector.GetSigBMode().GetMcsValue());
        heSig.SetRuAllocation(txVector.GetRuAllocation(p20Index));
        heSig.SetHeSigBContentChannels(txVector.GetContentChannels(p20Index));
        if (txVector.GetChannelWidth() >= 80)
        {
            heSig.SetCenter26ToneRuIndication(txVector.GetCenter26ToneRuIndication());
        }
    }
    else
    {
        heSig.SetMuFlag(false);
        if (!ns3::IsUlMu(m_preamble))
        {
            heSig.SetMcs(txVector.GetMode().GetMcsValue());
            heSig.SetNStreams(txVector.GetNss());
        }
    }
    heSig.SetBssColor(txVector.GetBssColor());
    if (!txVector.IsUlMu())
    {
        heSig.SetGuardIntervalAndLtfSize(txVector.GetGuardInterval(), 2 /*NLTF currently unused*/);
    }
}

WifiTxVector
HePpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);

#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();

    LSigHeader lSig;
    if (phyHeaders->RemoveHeader(lSig) == 0)
    {
        NS_FATAL_ERROR("Missing L-SIG header in HE PPDU");
    }

    HeSigHeader heSig(IsDlMu());
    if (phyHeaders->PeekHeader(heSig) == 0)
    {
        NS_FATAL_ERROR("Missing HE-SIG header in HE PPDU");
    }

    SetTxVectorFromPhyHeaders(txVector, lSig, heSig);
#else
    SetTxVectorFromPhyHeaders(txVector, m_lSig, m_heSig);
#endif

    return txVector;
}

void
HePpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                  const LSigHeader& lSig,
                                  const HeSigHeader& heSig) const
{
    txVector.SetChannelWidth(heSig.GetChannelWidth());
    txVector.SetBssColor(heSig.GetBssColor());
    txVector.SetLength(lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    if (!IsMu())
    {
        txVector.SetMode(HePhy::GetHeMcs(heSig.GetMcs()));
        txVector.SetNss(heSig.GetNStreams());
    }
    if (!IsUlMu())
    {
        txVector.SetGuardInterval(heSig.GetGuardInterval());
    }
    if (IsDlMu())
    {
        for (const auto& muUserInfo : m_muUserInfos)
        {
            txVector.SetHeMuUserInfo(muUserInfo.first, muUserInfo.second);
        }
    }
    if (txVector.IsDlMu())
    {
        txVector.SetSigBMode(HePhy::GetVhtMcs(heSig.GetSigBMcs()));
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(20);
        txVector.SetRuAllocation(heSig.GetRuAllocation(), p20Index);
        auto center26ToneRuIndication = heSig.GetCenter26ToneRuIndication();
        if (center26ToneRuIndication.has_value())
        {
            txVector.SetCenter26ToneRuIndication(center26ToneRuIndication.value());
        }
    }
}

Time
HePpdu::GetTxDuration() const
{
    Time ppduDuration = Seconds(0);
    const WifiTxVector& txVector = GetTxVector();

    uint16_t length = 0;
#ifdef NS3_BUILD_PROFILE_DEBUG
    LSigHeader lSig;
    m_phyHeaders->PeekHeader(lSig);
    length = lSig.GetLength();
#else
    length = m_lSig.GetLength();
#endif

    Time tSymbol = NanoSeconds(12800 + txVector.GetGuardInterval());
    Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    uint8_t sigExtension = 0;
    NS_ASSERT(m_operatingChannel.IsSet());
    if (m_operatingChannel.GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
    {
        sigExtension = 6;
    }
    uint8_t m = IsDlMu() ? 1 : 2;
    // Equation 27-11 of IEEE P802.11ax/D4.0
    Time calculatedDuration =
        MicroSeconds(((ceil(static_cast<double>(length + 3 + m) / 3)) * 4) + 20 + sigExtension);
    NS_ASSERT(calculatedDuration > preambleDuration);
    uint32_t nSymbols =
        floor(static_cast<double>((calculatedDuration - preambleDuration).GetNanoSeconds() -
                                  (sigExtension * 1000)) /
              tSymbol.GetNanoSeconds());
    ppduDuration = preambleDuration + (nSymbols * tSymbol) + MicroSeconds(sigExtension);
    return ppduDuration;
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

    uint8_t ppduBssColor = 0;
#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();
    LSigHeader lSig;
    phyHeaders->RemoveHeader(lSig);
    HeSigHeader heSig(IsDlMu());
    phyHeaders->RemoveHeader(heSig);
    ppduBssColor = heSig.GetBssColor();
#else
    ppduBssColor = m_heSig.GetBssColor();
#endif

    if (IsUlMu())
    {
        NS_ASSERT(m_psdus.size() == 1);
        if (bssColor == 0 || ppduBssColor == 0 || (bssColor == ppduBssColor))
        {
            return m_psdus.cbegin()->second;
        }
    }
    else
    {
        if (bssColor == 0 || ppduBssColor == 0 || (bssColor == ppduBssColor))
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

uint16_t
HePpdu::GetTransmissionChannelWidth() const
{
    const WifiTxVector& txVector = GetTxVector();
    if (txVector.IsUlMu() && GetStaId() != SU_STA_ID)
    {
        TxPsdFlag flag = GetTxPsdFlag();
        uint16_t ruWidth = HeRu::GetBandwidth(txVector.GetRu(GetStaId()).GetRuType());
        uint16_t channelWidth = (flag == PSD_NON_HE_PORTION && ruWidth < 20) ? 20 : ruWidth;
        NS_LOG_INFO("Use channelWidth=" << channelWidth << " MHz for HE TB from " << GetStaId()
                                        << " for " << flag);
        return channelWidth;
    }
    else
    {
        return OfdmPpdu::GetTransmissionChannelWidth();
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
        (trigVector->GetHeMuUserInfoMap().count(staId) > 0))
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
HePpdu::GetNumRusPerHeSigBContentChannel(uint16_t channelWidth,
                                         const std::vector<uint8_t>& ruAllocation)
{
    // MU-MIMO is not handled for now, i.e. one station per RU
    NS_ASSERT_MSG(!ruAllocation.empty(), "RU allocation is not set");
    NS_ASSERT_MSG(ruAllocation.size() == channelWidth / 20,
                  "RU allocation is not consistent with packet bandwidth");

    std::pair<std::size_t /* number of RUs in content channel 1 */,
              std::size_t /* number of RUs in content channel 2 */>
        chSize{0, 0};

    switch (channelWidth)
    {
    case 40:
        chSize.second += HeRu::GetRuSpecs(ruAllocation[1]).size();
        [[fallthrough]];
    case 20:
        chSize.first += HeRu::GetRuSpecs(ruAllocation[0]).size();
        break;
    default:
        for (auto n = 0; n < channelWidth / 20;)
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
        for (auto n = 0; n < channelWidth / 20;)
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

uint32_t
HePpdu::GetSigBFieldSize(uint16_t channelWidth, const RuAllocation& ruAllocation)
{
    // Compute the number of bits used by common field.
    // Assume that compression bit in HE-SIG-A is not set (i.e. not
    // full band MU-MIMO); the field is present.
    auto commonFieldSize = 4 /* CRC */ + 6 /* tail */;
    if (channelWidth <= 40)
    {
        commonFieldSize += 8; // only one allocation subfield
    }
    else
    {
        commonFieldSize +=
            8 * (channelWidth / 40) /* one allocation field per 40 MHz */ + 1 /* center RU */;
    }

    auto numStaPerContentChannel = GetNumRusPerHeSigBContentChannel(channelWidth, ruAllocation);
    auto maxNumStaPerContentChannel =
        std::max(numStaPerContentChannel.first, numStaPerContentChannel.second);
    auto maxNumUserBlockFields = maxNumStaPerContentChannel /
                                 2; // handle last user block with single user, if any, further down
    std::size_t userSpecificFieldSize =
        maxNumUserBlockFields * (2 * 21 /* user fields (2 users) */ + 4 /* tail */ + 6 /* CRC */);
    if (maxNumStaPerContentChannel % 2 != 0)
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

HePpdu::HeSigHeader::HeSigHeader()
    : HeSigHeader(false)
{
}

HePpdu::HeSigHeader::HeSigHeader(bool mu)
    : m_format(0),
      m_bssColor(0),
      m_mcs(0),
      m_bandwidth(0),
      m_gi_ltf_size(0),
      m_nsts(0),
      m_sigBMcs(0),
      m_mu(mu)
{
}

TypeId
HePpdu::HeSigHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HeSigHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<HeSigHeader>();
    return tid;
}

TypeId
HePpdu::HeSigHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
HePpdu::HeSigHeader::Print(std::ostream& os) const
{
    os << "MCS=" << +m_mcs << " CHANNEL_WIDTH=" << GetChannelWidth() << " GI=" << GetGuardInterval()
       << " NSTS=" << +m_nsts << " BSSColor=" << +m_bssColor << " MU=" << +m_mu;
}

uint32_t
HePpdu::HeSigHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 4; // HE-SIG-A1
    size += 4; // HE-SIG-A2
    if (m_mu)
    {
        size += GetSigBSize(); // HE-SIG-B
    }
    return size;
}

void
HePpdu::HeSigHeader::SetMuFlag(bool mu)
{
    m_mu = mu;
}

uint32_t
HePpdu::HeSigHeader::GetSigBSize() const
{
    return HePpdu::GetSigBFieldSize(GetChannelWidth(), m_ruAllocation);
}

void
HePpdu::HeSigHeader::SetFormat(WifiPreamble preamble)
{
    m_format = preamble == WIFI_PREAMBLE_HE_TB ? 0 : 1;
}

void
HePpdu::HeSigHeader::SetMcs(uint8_t mcs)
{
    // NS_ASSERT (mcs <= 11); // TODO: reactivate once EHT PHY headers are implemented
    m_mcs = mcs;
}

uint8_t
HePpdu::HeSigHeader::GetMcs() const
{
    return m_mcs;
}

void
HePpdu::HeSigHeader::SetBssColor(uint8_t bssColor)
{
    NS_ASSERT(bssColor < 64);
    m_bssColor = bssColor;
}

uint8_t
HePpdu::HeSigHeader::GetBssColor() const
{
    return m_bssColor;
}

void
HePpdu::HeSigHeader::SetChannelWidth(uint16_t channelWidth)
{
    if (channelWidth == 160)
    {
        m_bandwidth = 3;
    }
    else if (channelWidth == 80)
    {
        m_bandwidth = 2;
    }
    else if (channelWidth == 40)
    {
        m_bandwidth = 1;
    }
    else
    {
        m_bandwidth = 0;
    }
}

uint16_t
HePpdu::HeSigHeader::GetChannelWidth() const
{
    if (m_bandwidth == 3)
    {
        return 160;
    }
    else if (m_bandwidth == 2)
    {
        return 80;
    }
    else if (m_bandwidth == 1)
    {
        return 40;
    }
    else
    {
        return 20;
    }
}

void
HePpdu::HeSigHeader::SetGuardIntervalAndLtfSize(uint16_t gi, uint8_t ltf)
{
    if (gi == 800 && ltf == 1)
    {
        m_gi_ltf_size = 0;
    }
    else if (gi == 800 && ltf == 2)
    {
        m_gi_ltf_size = 1;
    }
    else if (gi == 1600 && ltf == 2)
    {
        m_gi_ltf_size = 2;
    }
    else
    {
        m_gi_ltf_size = 3;
    }
}

uint16_t
HePpdu::HeSigHeader::GetGuardInterval() const
{
    if (m_gi_ltf_size == 3)
    {
        // we currently do not consider DCM nor STBC fields
        return 3200;
    }
    else if (m_gi_ltf_size == 2)
    {
        return 1600;
    }
    else
    {
        return 800;
    }
}

void
HePpdu::HeSigHeader::SetNStreams(uint8_t nStreams)
{
    NS_ASSERT(nStreams <= 8);
    m_nsts = (nStreams - 1);
}

uint8_t
HePpdu::HeSigHeader::GetNStreams() const
{
    return (m_nsts + 1);
}

void
HePpdu::HeSigHeader::SetSigBMcs(uint8_t mcs)
{
    NS_ASSERT(mcs <= 5);
    m_sigBMcs = mcs;
}

uint8_t
HePpdu::HeSigHeader::GetSigBMcs() const
{
    return m_sigBMcs;
}

void
HePpdu::HeSigHeader::SetRuAllocation(const RuAllocation& ruAlloc)
{
    m_ruAllocation = ruAlloc;
}

const RuAllocation&
HePpdu::HeSigHeader::GetRuAllocation() const
{
    return m_ruAllocation;
}

void
HePpdu::HeSigHeader::SetHeSigBContentChannels(const HeSigBContentChannels& contentChannels)
{
    m_contentChannels = contentChannels;
}

const HeSigBContentChannels&
HePpdu::HeSigHeader::GetHeSigBContentChannels() const
{
    return m_contentChannels;
}

void
HePpdu::HeSigHeader::SetCenter26ToneRuIndication(
    std::optional<Center26ToneRuIndication> center26ToneRuIndication)
{
    m_center26ToneRuIndication = center26ToneRuIndication;
}

std::optional<Center26ToneRuIndication>
HePpdu::HeSigHeader::GetCenter26ToneRuIndication() const
{
    return m_center26ToneRuIndication;
}

void
HePpdu::HeSigHeader::SerializeSigBContentChannel(
    Buffer::Iterator& start,
    uint8_t contentChannelId,
    const std::vector<uint32_t>& userBlockFieldsContentChannel) const
{
    NS_ASSERT(contentChannelId <= WIFI_MAX_NUM_HE_SIGB_CONTENT_CHANNELS);
    NS_ASSERT(!userBlockFieldsContentChannel.empty());

    // Common field
    const auto bw = GetChannelWidth();
    switch (bw)
    {
    case 20:
        NS_ASSERT(contentChannelId == 1);
        NS_ASSERT(m_ruAllocation.size() == 1);
        start.WriteU8(m_ruAllocation.at(0));
        break;
    case 40:
        NS_ASSERT(m_ruAllocation.size() == 2);
        start.WriteU8(m_ruAllocation.at(contentChannelId - 1));
        break;
    case 80:
        NS_ASSERT(m_ruAllocation.size() == 4);
        start.WriteU8(m_ruAllocation.at(contentChannelId - 1));
        start.WriteU8(m_ruAllocation.at(contentChannelId + 1));
        break;
    case 160:
        NS_ASSERT(m_ruAllocation.size() == 8);
        start.WriteU8(m_ruAllocation.at(contentChannelId - 1));
        start.WriteU8(m_ruAllocation.at(contentChannelId + 1));
        start.WriteU8(m_ruAllocation.at(contentChannelId + 3));
        start.WriteU8(m_ruAllocation.at(contentChannelId + 5));
        break;
    default:
        NS_FATAL_ERROR("Invalid channel width: " << bw << " MHz");
    }
    uint8_t byte = 0;
    uint8_t offset = 0;
    if (m_center26ToneRuIndication.has_value())
    {
        NS_ASSERT(bw >= 80);
        if ((m_center26ToneRuIndication.value() ==
             CENTER_26_TONE_RU_LOW_AND_HIGH_80_MHZ_ALLOCATED) ||
            ((contentChannelId == 1) &&
             (m_center26ToneRuIndication.value() == CENTER_26_TONE_RU_LOW_80_MHZ_ALLOCATED)) ||
            ((contentChannelId == 2) &&
             (m_center26ToneRuIndication.value() == CENTER_26_TONE_RU_HIGH_80_MHZ_ALLOCATED)))
        {
            byte |= 0x01;
        }
        offset++;
    }

    // CRC and tail (set to 0 for simplification)
    offset += 10;
    start.WriteU8(byte);
    byte = 0;
    offset -= 8;

    // User fields
    for (auto userField : userBlockFieldsContentChannel)
    {
        std::size_t nBits = 0;
        const uint8_t bitsPerUserField = 21 + 4 /* CRC */ + 6 /* tail */;
        while (nBits < bitsPerUserField)
        {
            NS_ASSERT(offset < 8);
            const uint8_t nBitsLeftToWrite = bitsPerUserField - nBits;
            const uint8_t bitsLeft = (8 - offset);
            if (nBitsLeftToWrite >= 8)
            {
                const uint8_t bitMask = (0x01 << bitsLeft) - 0x01;
                byte |= ((userField >> nBits) & bitMask) << offset;
                start.WriteU8(byte);
                nBits += bitsLeft;
                byte = 0;
                offset = 0;
            }
            else
            {
                // less than one byte to write, set bits to be used by next user field
                const uint8_t bitMask = (0x01 << (8 - offset)) - 0x01;
                byte |= (userField & (bitMask << nBits)) << offset;
                offset = nBitsLeftToWrite;
                nBits = bitsPerUserField;
            }
        }
    }
}

uint16_t
HePpdu::HeSigHeader::DeserializeSigBContentChannel(
    Buffer::Iterator start,
    uint8_t contentChannelId,
    std::vector<uint32_t>& userBlockFieldsContentChannel)
{
    NS_ASSERT(contentChannelId <= WIFI_MAX_NUM_HE_SIGB_CONTENT_CHANNELS);
    Buffer::Iterator i = start;
    uint16_t count = 0;

    // Common field
    const auto bw = GetChannelWidth();
    switch (bw)
    {
    case 20:
        NS_ASSERT(contentChannelId == 1);
        NS_ASSERT(m_ruAllocation.size() == 1);
        m_ruAllocation.at(0) = i.ReadU8();
        count += 1;
        break;
    case 40:
        NS_ASSERT(m_ruAllocation.size() == 2);
        m_ruAllocation.at(contentChannelId - 1) = i.ReadU8();
        count += 1;
        break;
    case 80:
        NS_ASSERT(m_ruAllocation.size() == 4);
        m_ruAllocation.at(contentChannelId - 1) = i.ReadU8();
        m_ruAllocation.at(contentChannelId + 1) = i.ReadU8();
        count += 2;
        break;
    case 160:
        NS_ASSERT(m_ruAllocation.size() == 8);
        m_ruAllocation.at(contentChannelId - 1) = i.ReadU8();
        m_ruAllocation.at(contentChannelId + 1) = i.ReadU8();
        m_ruAllocation.at(contentChannelId + 3) = i.ReadU8();
        m_ruAllocation.at(contentChannelId + 5) = i.ReadU8();
        count += 4;
        break;
    default:
        NS_FATAL_ERROR("Invalid channel width: " << bw << " MHz");
    }
    uint8_t offset = 0;
    uint8_t byte = i.ReadU8();
    count += 1;
    if (bw >= 80)
    {
        NS_ASSERT(m_center26ToneRuIndication.has_value());
        auto val = static_cast<uint8_t>(m_center26ToneRuIndication.value());
        val |= (byte & 0x01) << (contentChannelId - 1);
        m_center26ToneRuIndication = static_cast<Center26ToneRuIndication>(val);
        offset++;
    }

    // CRC and tail
    offset += 10;
    byte = i.ReadU8();
    count += 1;
    offset -= 8;

    // User fields
    const auto numRusPerHeSigBContentChannel = GetNumRusPerHeSigBContentChannel(bw, m_ruAllocation);
    std::size_t nUserFields = (contentChannelId == 1) ? numRusPerHeSigBContentChannel.first
                                                      : numRusPerHeSigBContentChannel.second;
    userBlockFieldsContentChannel.reserve(nUserFields);
    for (std::size_t field = 0; field < nUserFields; ++field)
    {
        uint8_t nBits = 0;
        uint32_t userField = 0;
        const uint8_t bitsPerUserField = 21 + 4 /* CRC */ + 6 /* tail */;
        while (nBits < bitsPerUserField)
        {
            NS_ASSERT(offset < 8);
            const uint8_t bitsLeft = std::min(8 - offset, bitsPerUserField - nBits);
            const uint8_t bitMask = (0x01 << bitsLeft) - 0x01;
            userField |= ((byte >> offset) & bitMask) << nBits;
            offset += bitsLeft;
            nBits += bitsLeft;
            if ((bitsPerUserField - nBits < 8) && (field == nUserFields - 1))
            {
                // reached padding, can stop deserializing user fields
                break;
            }
            if (offset == 8)
            {
                // reached end of byte, read next one
                byte = i.ReadU8();
                count += 1;
                offset = 0;
            }
        }
        userBlockFieldsContentChannel.push_back(userField);
    }

    return count;
}

void
HePpdu::HeSigHeader::Serialize(Buffer::Iterator start) const
{
    // HE-SIG-A1
    uint32_t sigA1 = 0;
    if (!m_mu)
    {
        sigA1 |= m_format & 0x01;
        if (m_format == 1) // HE SU or HE ER SU PPDU
        {
            sigA1 |= ((m_mcs & 0x0f) << 3);
            sigA1 |= ((m_bssColor & 0x3f) << 8);
            sigA1 |= (0x01 << 14); // Reserved set to 1
            sigA1 |= ((m_bandwidth & 0x03) << 19);
            sigA1 |= ((m_gi_ltf_size & 0x03) << 21);
            sigA1 |= ((m_nsts & 0x07) << 23);
        }
        else
        {
            // HE TB PPDU
            sigA1 |= ((m_bssColor & 0x3f) << 1);
            sigA1 |= (0x01 << 23); // Reserved set to 1
            sigA1 |= ((m_bandwidth & 0x03) << 24);
        }
    }
    else
    {
        // HE MU PPDU
        sigA1 |= ((m_sigBMcs & 0x07) << 1);
        sigA1 |= ((m_bssColor & 0x3f) << 5);
        sigA1 |= ((m_bandwidth & 0x03) << 15);
        sigA1 |= ((m_gi_ltf_size & 0x03) << 23);
    }
    start.WriteU32(sigA1);

    // HE-SIG-A2
    uint32_t sigA2 = 0;
    if (!m_mu && (m_format == 1))
    {
        // HE SU or HE ER SU PPDU
        sigA2 |= (0x01 << 14); // Set Reserved bit #14 to 1
    }
    start.WriteU32(sigA2);

    if (m_mu)
    {
        // HE-SIG-B
        std::vector<uint32_t> userBlockFieldsContentChannel1;
        std::vector<uint32_t> userBlockFieldsContentChannel2;
        for (const auto& userInfo : m_contentChannels.at(0))
        {
            uint32_t userField = userInfo.staId;
            userField |= ((userInfo.nss & 0x07) << 11);
            userField |= ((userInfo.mcs & 0x0f) << 15);
            // CRC and tail set to 0 for simplification
            userBlockFieldsContentChannel1.push_back(userField);
        }
        SerializeSigBContentChannel(start, 1, userBlockFieldsContentChannel1);
        if (m_contentChannels.size() > 1)
        {
            NS_ASSERT(GetChannelWidth() > 20);
            for (const auto& userInfo : m_contentChannels.at(1))
            {
                uint32_t userField = userInfo.staId;
                userField |= ((userInfo.nss & 0x07) << 11);
                userField |= ((userInfo.mcs & 0x0f) << 15);
                // CRC and tail set to 0 for simplification
                userBlockFieldsContentChannel2.push_back(userField);
            }
            SerializeSigBContentChannel(start, 2, userBlockFieldsContentChannel2);
        }
    }
}

uint32_t
HePpdu::HeSigHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // HE-SIG-A1
    uint32_t sigA1 = i.ReadU32();
    if (!m_mu)
    {
        m_format = (sigA1 & 0x01);
        if (m_format == 1)
        {
            // HE SU or HE ER SU PPDU
            m_mcs = ((sigA1 >> 3) & 0x0f);
            m_bssColor = ((sigA1 >> 8) & 0x3f);
            m_bandwidth = ((sigA1 >> 19) & 0x03);
            m_gi_ltf_size = ((sigA1 >> 21) & 0x03);
            m_nsts = ((sigA1 >> 23) & 0x07);
        }
        else
        {
            // HE TB PPDU
            m_bssColor = ((sigA1 >> 1) & 0x3f);
            m_bandwidth = ((sigA1 >> 24) & 0x03);
        }
    }
    else
    {
        // HE MU PPDU
        m_sigBMcs = ((sigA1 >> 1) & 0x07);
        m_bssColor = ((sigA1 >> 5) & 0x3f);
        m_bandwidth = ((sigA1 >> 15) & 0x03);
        m_gi_ltf_size = ((sigA1 >> 23) & 0x03);
    }

    // HE-SIG-A2
    i.ReadU32();

    if (m_mu)
    {
        // HE-SIG-B
        m_ruAllocation.clear();
        m_contentChannels.clear();
        std::size_t ruAllocationSize = 0;
        const auto bw = GetChannelWidth();
        switch (bw)
        {
        case 20:
            ruAllocationSize = 1;
            break;
        case 40:
            ruAllocationSize = 2;
            break;
        case 80:
            ruAllocationSize = 4;
            break;
        case 160:
            ruAllocationSize = 8;
            break;
        default:
            NS_FATAL_ERROR("Invalid channel width: " << bw << " MHz");
        }
        m_ruAllocation.resize(ruAllocationSize);
        std::vector<uint32_t> userBlockFieldsContentChannel1;
        std::vector<uint32_t> userBlockFieldsContentChannel2;
        if (bw < 80)
        {
            m_center26ToneRuIndication = std::nullopt;
        }
        else
        {
            m_center26ToneRuIndication = CENTER_26_TONE_RU_UNALLOCATED;
        }
        uint16_t nBytes = DeserializeSigBContentChannel(i, 1, userBlockFieldsContentChannel1);
        i.Next(nBytes);
        m_contentChannels.push_back({});
        for (auto userField : userBlockFieldsContentChannel1)
        {
            uint16_t staId = (userField & 0x07ff);
            uint8_t nss = (userField >> 11) & 0x07;
            uint8_t mcs = (userField >> 15) & 0x0f;
            m_contentChannels.at(0).push_back({staId, nss, mcs});
        }
        if (bw >= 40)
        {
            nBytes = DeserializeSigBContentChannel(i, 2, userBlockFieldsContentChannel2);
            i.Next(nBytes);
            m_contentChannels.push_back({});
            for (auto userField : userBlockFieldsContentChannel2)
            {
                uint16_t staId = (userField & 0x07ff);
                uint8_t nss = (userField >> 11) & 0x07;
                uint8_t mcs = (userField >> 15) & 0x0f;
                m_contentChannels.at(1).push_back({staId, nss, mcs});
            }
        }
    }

    return i.GetDistanceFrom(start);
}

} // namespace ns3
