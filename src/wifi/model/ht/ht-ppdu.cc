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
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HtSigHeader)
 */

#include "ht-ppdu.h"

#include "ht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HtPpdu");

HtPpdu::HtPpdu(Ptr<const WifiPsdu> psdu,
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
    SetPhyHeaders(txVector, ppduDuration, psdu->GetSize());
}

void
HtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration, std::size_t psduSize)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration << psduSize);

#ifdef NS3_BUILD_PROFILE_DEBUG
    LSigHeader lSig;
    SetLSigHeader(lSig, ppduDuration);

    HtSigHeader htSig;
    SetHtSigHeader(htSig, txVector, psduSize);

    m_phyHeaders->AddHeader(htSig);
    m_phyHeaders->AddHeader(lSig);
#else
    SetLSigHeader(m_lSig, ppduDuration);
    SetHtSigHeader(m_htSig, txVector, psduSize);
#endif
}

void
HtPpdu::SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const
{
    uint8_t sigExtension = 0;
    if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
        sigExtension = 6;
    }
    uint16_t length = ((ceil((static_cast<double>(ppduDuration.GetNanoSeconds() - (20 * 1000) -
                                                  (sigExtension * 1000)) /
                              1000) /
                             4.0) *
                        3) -
                       3);
    lSig.SetLength(length);
}

void
HtPpdu::SetHtSigHeader(HtSigHeader& htSig, const WifiTxVector& txVector, std::size_t psduSize) const
{
    htSig.SetMcs(txVector.GetMode().GetMcsValue());
    htSig.SetChannelWidth(txVector.GetChannelWidth());
    htSig.SetHtLength(psduSize);
    htSig.SetAggregation(txVector.IsAggregation());
    htSig.SetShortGuardInterval(txVector.GetGuardInterval() == 400);
}

WifiTxVector
HtPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);

#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();

    LSigHeader lSig;
    if (phyHeaders->RemoveHeader(lSig) == 0)
    {
        NS_FATAL_ERROR("Missing L-SIG header in HT PPDU");
    }

    HtSigHeader htSig;
    if (phyHeaders->RemoveHeader(htSig) == 0)
    {
        NS_FATAL_ERROR("Missing HT-SIG header in HT PPDU");
    }

    SetTxVectorFromPhyHeaders(txVector, lSig, htSig);
#else
    SetTxVectorFromPhyHeaders(txVector, m_lSig, m_htSig);
#endif

    return txVector;
}

void
HtPpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector,
                                  const LSigHeader& lSig,
                                  const HtSigHeader& htSig) const
{
    txVector.SetMode(HtPhy::GetHtMcs(htSig.GetMcs()));
    txVector.SetChannelWidth(htSig.GetChannelWidth());
    txVector.SetNss(1 + (htSig.GetMcs() / 8));
    txVector.SetGuardInterval(htSig.GetShortGuardInterval() ? 400 : 800);
    txVector.SetAggregation(htSig.GetAggregation());
}

Time
HtPpdu::GetTxDuration() const
{
    Time ppduDuration = Seconds(0);
    const WifiTxVector& txVector = GetTxVector();

    uint16_t htLength = 0;
#ifdef NS3_BUILD_PROFILE_DEBUG
    auto phyHeaders = m_phyHeaders->Copy();

    LSigHeader lSig;
    phyHeaders->RemoveHeader(lSig);
    HtSigHeader htSig;
    phyHeaders->RemoveHeader(htSig);

    htLength = htSig.GetHtLength();
#else
    htLength = m_htSig.GetHtLength();
#endif

    ppduDuration = WifiPhy::CalculateTxDuration(htLength, txVector, m_band);
    return ppduDuration;
}

Ptr<WifiPpdu>
HtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new HtPpdu(*this), false);
}

HtPpdu::HtSigHeader::HtSigHeader()
    : m_mcs(0),
      m_cbw20_40(0),
      m_htLength(0),
      m_aggregation(0),
      m_sgi(0)
{
}

TypeId
HtPpdu::HtSigHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HtSigHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<HtSigHeader>();
    return tid;
}

TypeId
HtPpdu::HtSigHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
HtPpdu::HtSigHeader::Print(std::ostream& os) const
{
    os << "MCS=" << +m_mcs << " HT_LENGTH=" << m_htLength << " CHANNEL_WIDTH=" << GetChannelWidth()
       << " SGI=" << +m_sgi << " AGGREGATION=" << +m_aggregation;
}

uint32_t
HtPpdu::HtSigHeader::GetSerializedSize() const
{
    return 6;
}

void
HtPpdu::HtSigHeader::SetMcs(uint8_t mcs)
{
    NS_ASSERT(mcs <= 31);
    m_mcs = mcs;
}

uint8_t
HtPpdu::HtSigHeader::GetMcs() const
{
    return m_mcs;
}

void
HtPpdu::HtSigHeader::SetChannelWidth(uint16_t channelWidth)
{
    m_cbw20_40 = (channelWidth > 20) ? 1 : 0;
}

uint16_t
HtPpdu::HtSigHeader::GetChannelWidth() const
{
    return m_cbw20_40 ? 40 : 20;
}

void
HtPpdu::HtSigHeader::SetHtLength(uint16_t length)
{
    m_htLength = length;
}

uint16_t
HtPpdu::HtSigHeader::GetHtLength() const
{
    return m_htLength;
}

void
HtPpdu::HtSigHeader::SetAggregation(bool aggregation)
{
    m_aggregation = aggregation ? 1 : 0;
}

bool
HtPpdu::HtSigHeader::GetAggregation() const
{
    return m_aggregation;
}

void
HtPpdu::HtSigHeader::SetShortGuardInterval(bool sgi)
{
    m_sgi = sgi ? 1 : 0;
}

bool
HtPpdu::HtSigHeader::GetShortGuardInterval() const
{
    return m_sgi;
}

void
HtPpdu::HtSigHeader::Serialize(Buffer::Iterator start) const
{
    uint8_t byte = m_mcs;
    byte |= ((m_cbw20_40 & 0x01) << 7);
    start.WriteU8(byte);
    start.WriteU16(m_htLength);
    byte = (0x01 << 2); // Set Reserved bit #2 to 1
    byte |= ((m_aggregation & 0x01) << 3);
    byte |= ((m_sgi & 0x01) << 7);
    start.WriteU8(byte);
    start.WriteU16(0);
}

uint32_t
HtPpdu::HtSigHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint8_t byte = i.ReadU8();
    m_mcs = byte & 0x7f;
    m_cbw20_40 = ((byte >> 7) & 0x01);
    m_htLength = i.ReadU16();
    byte = i.ReadU8();
    m_aggregation = ((byte >> 3) & 0x01);
    m_sgi = ((byte >> 7) & 0x01);
    i.ReadU16();
    return i.GetDistanceFrom(start);
}

} // namespace ns3
