/*
 * Copyright (c) 2019 Orange Labs
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
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (VhtSigHeader)
 */

#include "vht-ppdu.h"

#include "vht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VhtPpdu");

VhtPpdu::VhtPpdu(Ptr<const WifiPsdu> psdu,
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
    SetPhyHeaders(txVector, ppduDuration);
}

void
VhtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);

#ifdef NS3_BUILD_PROFILE_DEBUG
    LSigHeader lSig;
    SetLSigHeader(lSig, ppduDuration);

    VhtSigHeader vhtSig;
    SetVhtSigHeader(vhtSig, txVector, ppduDuration);

    m_phyHeaders->AddHeader(vhtSig);
    m_phyHeaders->AddHeader(lSig);
#else
    SetLSigHeader(m_lSig, ppduDuration);
    SetVhtSigHeader(m_vhtSig, txVector, ppduDuration);
#endif
}

void
VhtPpdu::SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const
{
    uint16_t length =
        ((ceil((static_cast<double>(ppduDuration.GetNanoSeconds() - (20 * 1000)) / 1000) / 4.0) *
          3) -
         3);
    lSig.SetLength(length);
}

void
VhtPpdu::SetVhtSigHeader(VhtSigHeader& vhtSig,
                         const WifiTxVector& txVector,
                         Time ppduDuration) const
{
    vhtSig.SetMuFlag(m_preamble == WIFI_PREAMBLE_VHT_MU);
    vhtSig.SetChannelWidth(txVector.GetChannelWidth());
    vhtSig.SetShortGuardInterval(txVector.GetGuardInterval() == 400);
    uint32_t nSymbols =
        (static_cast<double>(
             (ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector))
                 .GetNanoSeconds()) /
         (3200 + txVector.GetGuardInterval()));
    if (txVector.GetGuardInterval() == 400)
    {
        vhtSig.SetShortGuardIntervalDisambiguation((nSymbols % 10) == 9);
    }
    vhtSig.SetSuMcs(txVector.GetMode().GetMcsValue());
    vhtSig.SetNStreams(txVector.GetNss());
}

WifiTxVector
VhtPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);

#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();

    LSigHeader lSig;
    if (phyHeaders->RemoveHeader(lSig) == 0)
    {
        NS_FATAL_ERROR("Missing L-SIG header in VHT PPDU");
    }

    VhtSigHeader vhtSig;
    if (phyHeaders->RemoveHeader(vhtSig) == 0)
    {
        NS_FATAL_ERROR("Missing VHT-SIG header in VHT PPDU");
    }

    SetTxVectorFromPhyHeaders(txVector, lSig, vhtSig);
#else
    SetTxVectorFromPhyHeaders(txVector, m_lSig, m_vhtSig);
#endif

    return txVector;
}

void
VhtPpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                   const LSigHeader& lSig,
                                   const VhtSigHeader& vhtSig) const
{
    txVector.SetMode(VhtPhy::GetVhtMcs(vhtSig.GetSuMcs()));
    txVector.SetChannelWidth(vhtSig.GetChannelWidth());
    txVector.SetNss(vhtSig.GetNStreams());
    txVector.SetGuardInterval(vhtSig.GetShortGuardInterval() ? 400 : 800);
    txVector.SetAggregation(GetPsdu()->IsAggregate());
}

Time
VhtPpdu::GetTxDuration() const
{
    Time ppduDuration = Seconds(0);
    const WifiTxVector& txVector = GetTxVector();

    uint16_t length = 0;
    bool sgi = false;
    bool sgiDisambiguation = false;
#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();

    LSigHeader lSig;
    phyHeaders->RemoveHeader(lSig);
    VhtSigHeader vhtSig;
    phyHeaders->RemoveHeader(vhtSig);

    length = lSig.GetLength();
    sgi = vhtSig.GetShortGuardInterval();
    sgiDisambiguation = vhtSig.GetShortGuardIntervalDisambiguation();
#else
    length = m_lSig.GetLength();
    sgi = m_vhtSig.GetShortGuardInterval();
    sgiDisambiguation = m_vhtSig.GetShortGuardIntervalDisambiguation();
#endif

    Time tSymbol = NanoSeconds(3200 + txVector.GetGuardInterval());
    Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    Time calculatedDuration = MicroSeconds(((ceil(static_cast<double>(length + 3) / 3)) * 4) + 20);
    uint32_t nSymbols =
        floor(static_cast<double>((calculatedDuration - preambleDuration).GetNanoSeconds()) /
              tSymbol.GetNanoSeconds());
    if (sgi && sgiDisambiguation)
    {
        nSymbols--;
    }
    ppduDuration = preambleDuration + (nSymbols * tSymbol);
    return ppduDuration;
}

Ptr<WifiPpdu>
VhtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new VhtPpdu(*this), false);
}

WifiPpduType
VhtPpdu::GetType() const
{
    return (m_preamble == WIFI_PREAMBLE_VHT_MU) ? WIFI_PPDU_TYPE_DL_MU : WIFI_PPDU_TYPE_SU;
}

VhtPpdu::VhtSigHeader::VhtSigHeader()
    : m_bw(0),
      m_nsts(0),
      m_sgi(0),
      m_sgi_disambiguation(0),
      m_suMcs(0),
      m_mu(false)
{
}

TypeId
VhtPpdu::VhtSigHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::VhtSigHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<VhtSigHeader>();
    return tid;
}

TypeId
VhtPpdu::VhtSigHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
VhtPpdu::VhtSigHeader::Print(std::ostream& os) const
{
    os << "SU_MCS=" << +m_suMcs << " CHANNEL_WIDTH=" << GetChannelWidth() << " SGI=" << +m_sgi
       << " NSTS=" << +m_nsts << " MU=" << +m_mu;
}

uint32_t
VhtPpdu::VhtSigHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 3; // VHT-SIG-A1
    size += 3; // VHT-SIG-A2
    if (m_mu)
    {
        size += 4; // VHT-SIG-B
    }
    return size;
}

void
VhtPpdu::VhtSigHeader::SetMuFlag(bool mu)
{
    m_mu = mu;
}

void
VhtPpdu::VhtSigHeader::SetChannelWidth(uint16_t channelWidth)
{
    if (channelWidth == 160)
    {
        m_bw = 3;
    }
    else if (channelWidth == 80)
    {
        m_bw = 2;
    }
    else if (channelWidth == 40)
    {
        m_bw = 1;
    }
    else
    {
        m_bw = 0;
    }
}

uint16_t
VhtPpdu::VhtSigHeader::GetChannelWidth() const
{
    if (m_bw == 3)
    {
        return 160;
    }
    else if (m_bw == 2)
    {
        return 80;
    }
    else if (m_bw == 1)
    {
        return 40;
    }
    else
    {
        return 20;
    }
}

void
VhtPpdu::VhtSigHeader::SetNStreams(uint8_t nStreams)
{
    NS_ASSERT(nStreams <= 8);
    m_nsts = (nStreams - 1);
}

uint8_t
VhtPpdu::VhtSigHeader::GetNStreams() const
{
    return (m_nsts + 1);
}

void
VhtPpdu::VhtSigHeader::SetShortGuardInterval(bool sgi)
{
    m_sgi = sgi ? 1 : 0;
}

bool
VhtPpdu::VhtSigHeader::GetShortGuardInterval() const
{
    return m_sgi;
}

void
VhtPpdu::VhtSigHeader::SetShortGuardIntervalDisambiguation(bool disambiguation)
{
    m_sgi_disambiguation = disambiguation ? 1 : 0;
}

bool
VhtPpdu::VhtSigHeader::GetShortGuardIntervalDisambiguation() const
{
    return m_sgi_disambiguation;
}

void
VhtPpdu::VhtSigHeader::SetSuMcs(uint8_t mcs)
{
    NS_ASSERT(mcs <= 9);
    m_suMcs = mcs;
}

uint8_t
VhtPpdu::VhtSigHeader::GetSuMcs() const
{
    return m_suMcs;
}

void
VhtPpdu::VhtSigHeader::Serialize(Buffer::Iterator start) const
{
    // VHT-SIG-A1
    uint8_t byte = m_bw;
    byte |= (0x01 << 2); // Set Reserved bit #2 to 1
    start.WriteU8(byte);
    uint16_t bytes = (m_nsts & 0x07) << 2;
    bytes |= (0x01 << (23 - 8)); // Set Reserved bit #23 to 1
    start.WriteU16(bytes);

    // VHT-SIG-A2
    byte = m_sgi & 0x01;
    byte |= ((m_sgi_disambiguation & 0x01) << 1);
    byte |= ((m_suMcs & 0x0f) << 4);
    start.WriteU8(byte);
    bytes = (0x01 << (9 - 8)); // Set Reserved bit #9 to 1
    start.WriteU16(bytes);

    if (m_mu)
    {
        // VHT-SIG-B
        start.WriteU32(0);
    }
}

uint32_t
VhtPpdu::VhtSigHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // VHT-SIG-A1
    uint8_t byte = i.ReadU8();
    m_bw = byte & 0x03;
    uint16_t bytes = i.ReadU16();
    m_nsts = ((bytes >> 2) & 0x07);

    // VHT-SIG-A2
    byte = i.ReadU8();
    m_sgi = byte & 0x01;
    m_sgi_disambiguation = ((byte >> 1) & 0x01);
    m_suMcs = ((byte >> 4) & 0x0f);
    i.ReadU16();

    if (m_mu)
    {
        // VHT-SIG-B
        i.ReadU32();
    }

    return i.GetDistanceFrom(start);
}

} // namespace ns3
