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
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VhtPpdu");

VhtPpdu::VhtPpdu(Ptr<const WifiPsdu> psdu,
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
    SetPhyHeaders(txVector, ppduDuration);
}

void
VhtPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector << ppduDuration);
    SetLSigHeader(m_lSig, ppduDuration);
    SetVhtSigHeader(m_vhtSig, txVector, ppduDuration);
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
    SetTxVectorFromPhyHeaders(txVector, m_lSig, m_vhtSig);
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
    const auto& txVector = GetTxVector();
    const auto length = m_lSig.GetLength();
    const auto sgi = m_vhtSig.GetShortGuardInterval();
    const auto sgiDisambiguation = m_vhtSig.GetShortGuardIntervalDisambiguation();
    const auto tSymbol = NanoSeconds(3200 + txVector.GetGuardInterval());
    const auto preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    const auto calculatedDuration =
        MicroSeconds(((ceil(static_cast<double>(length + 3) / 3)) * 4) + 20);
    uint32_t nSymbols =
        floor(static_cast<double>((calculatedDuration - preambleDuration).GetNanoSeconds()) /
              tSymbol.GetNanoSeconds());
    if (sgi && sgiDisambiguation)
    {
        nSymbols--;
    }
    return (preambleDuration + (nSymbols * tSymbol));
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

} // namespace ns3
