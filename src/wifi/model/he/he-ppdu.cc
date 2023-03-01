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
               uint16_t txCenterFreq,
               Time ppduDuration,
               WifiPhyBand band,
               uint64_t uid,
               TxPsdFlag flag)
    : OfdmPpdu(psdus.begin()->second,
               txVector,
               txCenterFreq,
               band,
               uid,
               false) // don't instantiate LSigHeader of OfdmPpdu
{
    NS_LOG_FUNCTION(this << psdus << txVector << txCenterFreq << ppduDuration << band << uid
                         << flag);

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
        m_contentChannelAlloc = txVector.GetContentChannelAllocation();
        m_ruAllocation = txVector.GetRuAllocation();
    }
    SetPhyHeaders(txVector, ppduDuration);
    SetTxPsdFlag(flag);
}

HePpdu::HePpdu(Ptr<const WifiPsdu> psdu,
               const WifiTxVector& txVector,
               uint16_t txCenterFreq,
               Time ppduDuration,
               WifiPhyBand band,
               uint64_t uid)
    : OfdmPpdu(psdu,
               txVector,
               txCenterFreq,
               band,
               uid,
               false) // don't instantiate LSigHeader of OfdmPpdu
{
    NS_LOG_FUNCTION(this << psdu << txVector << txCenterFreq << ppduDuration << band << uid);
    NS_ASSERT(!IsMu());
    SetPhyHeaders(txVector, ppduDuration);
    SetTxPsdFlag(PSD_NON_HE_PORTION);
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
    if (m_band == WIFI_PHY_BAND_2_4GHZ)
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
    if (ns3::IsDlMu(m_preamble))
    {
        heSig.SetMuFlag(true);
        heSig.SetMcs(txVector.GetSigBMode().GetMcsValue());
    }
    else if (!ns3::IsUlMu(m_preamble))
    {
        heSig.SetMcs(txVector.GetMode().GetMcsValue());
        heSig.SetNStreams(txVector.GetNss());
    }
    heSig.SetBssColor(txVector.GetBssColor());
    heSig.SetChannelWidth(txVector.GetChannelWidth());
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

    HeSigHeader heSig;
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
        txVector.SetSigBMode(HePhy::GetVhtMcs(heSig.GetMcs()));
        txVector.SetRuAllocation(m_ruAllocation);
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
    if (m_band == WIFI_PHY_BAND_2_4GHZ)
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
    HeSigHeader heSig;
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

bool
HePpdu::IsAllocated(uint16_t staId) const
{
    return (m_muUserInfos.find(staId) != m_muUserInfos.cend());
}

bool
HePpdu::IsStaInContentChannel(uint16_t staId, std::size_t channelId) const
{
    NS_ASSERT_MSG(channelId < m_contentChannelAlloc.size(),
                  "Invalid content channel ID " << channelId);
    const auto& channelAlloc = m_contentChannelAlloc.at(channelId);
    return (std::find(channelAlloc.cbegin(), channelAlloc.cend(), staId) != channelAlloc.cend());
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
    : m_format(1),
      m_bssColor(0),
      m_ul_dl(0),
      m_mcs(0),
      m_spatialReuse(0),
      m_bandwidth(0),
      m_gi_ltf_size(0),
      m_nsts(0),
      m_mu(false)
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
        size += 1; // HE-SIG-B
    }
    return size;
}

void
HePpdu::HeSigHeader::SetMuFlag(bool mu)
{
    m_mu = mu;
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
HePpdu::HeSigHeader::Serialize(Buffer::Iterator start) const
{
    // HE-SIG-A1
    uint8_t byte = m_format & 0x01;
    byte |= ((m_ul_dl & 0x01) << 2);
    byte |= ((m_mcs & 0x0f) << 3);
    start.WriteU8(byte);
    uint16_t bytes = (m_bssColor & 0x3f);
    bytes |= (0x01 << 6); // Reserved set to 1
    bytes |= ((m_spatialReuse & 0x0f) << 7);
    bytes |= ((m_bandwidth & 0x03) << 11);
    bytes |= ((m_gi_ltf_size & 0x03) << 13);
    bytes |= ((m_nsts & 0x01) << 15);
    start.WriteU16(bytes);
    start.WriteU8((m_nsts >> 1) & 0x03);

    // HE-SIG-A2
    uint32_t sigA2 = 0;
    sigA2 |= (0x01 << 14); // Set Reserved bit #14 to 1
    start.WriteU32(sigA2);

    if (m_mu)
    {
        // HE-SIG-B
        start.WriteU8(0);
    }
}

uint32_t
HePpdu::HeSigHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // HE-SIG-A1
    uint8_t byte = i.ReadU8();
    m_format = (byte & 0x01);
    m_ul_dl = ((byte >> 2) & 0x01);
    m_mcs = ((byte >> 3) & 0x0f);
    uint16_t bytes = i.ReadU16();
    m_bssColor = (bytes & 0x3f);
    m_spatialReuse = ((bytes >> 7) & 0x0f);
    m_bandwidth = ((bytes >> 11) & 0x03);
    m_gi_ltf_size = ((bytes >> 13) & 0x03);
    m_nsts = ((bytes >> 15) & 0x01);
    byte = i.ReadU8();
    m_nsts |= (byte & 0x03) << 1;

    // HE-SIG-A2
    i.ReadU32();

    if (m_mu)
    {
        // HE-SIG-B
        i.ReadU8();
    }

    return i.GetDistanceFrom(start);
}

} // namespace ns3
