/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (HtSigHeader)
 */

#include "ht-ppdu.h"

#include "ht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HtPpdu");

HtPpdu::HtPpdu(Ptr<const WifiPsdu> psdu,
               const WifiTxVector& txVector,
               const WifiPhyOperatingChannel& channel,
               Time ppduDuration,
               uint64_t uid)
    : OfdmPpdu(psdu,
               txVector,
               channel,
               uid,
               false) // don't instantiate LSigHeader of OfdmPpdu
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << ppduDuration << uid);
    SetPhyHeaders(txVector, ppduDuration, psdu->GetSize());
}

void
HtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration, std::size_t psduSize)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration << psduSize);
    SetLSigHeader(m_lSig, ppduDuration);
    SetHtSigHeader(m_htSig, txVector, psduSize);
}

void
HtPpdu::SetLSigHeader(LSigHeader& lSig, Time ppduDuration) const
{
    uint8_t sigExtension = 0;
    NS_ASSERT(m_operatingChannel.IsSet());
    if (m_operatingChannel.GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
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
    htSig.SetShortGuardInterval(txVector.GetGuardInterval().GetNanoSeconds() == 400);
}

WifiTxVector
HtPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    SetTxVectorFromPhyHeaders(txVector, m_lSig, m_htSig);
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
    txVector.SetGuardInterval(NanoSeconds(htSig.GetShortGuardInterval() ? 400 : 800));
    txVector.SetAggregation(htSig.GetAggregation());
}

Time
HtPpdu::GetTxDuration() const
{
    const auto& txVector = GetTxVector();
    const auto htLength = m_htSig.GetHtLength();
    NS_ASSERT(m_operatingChannel.IsSet());
    return WifiPhy::CalculateTxDuration(htLength, txVector, m_operatingChannel.GetPhyBand());
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
HtPpdu::HtSigHeader::SetChannelWidth(MHz_u channelWidth)
{
    m_cbw20_40 = (channelWidth > MHz_u{20}) ? 1 : 0;
}

MHz_u
HtPpdu::HtSigHeader::GetChannelWidth() const
{
    return m_cbw20_40 ? MHz_u{40} : MHz_u{20};
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

} // namespace ns3
