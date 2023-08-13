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
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (LSigHeader)
 */

#include "ofdm-ppdu.h"

#include "ofdm-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OfdmPpdu");

OfdmPpdu::OfdmPpdu(Ptr<const WifiPsdu> psdu,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   uint64_t uid,
                   bool instantiateLSig /* = true */)
    : WifiPpdu(psdu, txVector, channel, uid),
      m_channelWidth(txVector.IsNonHtDuplicate() ? 20 : txVector.GetChannelWidth())
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << uid);
    if (instantiateLSig)
    {
        SetPhyHeaders(txVector, psdu->GetSize());
    }
}

void
OfdmPpdu::SetPhyHeaders(const WifiTxVector& txVector, std::size_t psduSize)
{
    NS_LOG_FUNCTION(this << txVector << psduSize);
    SetLSigHeader(m_lSig, txVector, psduSize);
}

void
OfdmPpdu::SetLSigHeader(LSigHeader& lSig, const WifiTxVector& txVector, std::size_t psduSize) const
{
    lSig.SetRate(txVector.GetMode().GetDataRate(txVector), m_channelWidth);
    lSig.SetLength(psduSize);
}

WifiTxVector
OfdmPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    SetTxVectorFromLSigHeader(txVector, m_lSig);
    return txVector;
}

void
OfdmPpdu::SetTxVectorFromLSigHeader(WifiTxVector& txVector, const LSigHeader& lSig) const
{
    NS_ASSERT(m_channelWidth <= 20);
    // OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
    txVector.SetMode(OfdmPhy::GetOfdmRate(lSig.GetRate(m_channelWidth), m_channelWidth));
    txVector.SetChannelWidth(m_channelWidth);
}

Time
OfdmPpdu::GetTxDuration() const
{
    const auto& txVector = GetTxVector();
    const auto length = m_lSig.GetLength();
    NS_ASSERT(m_operatingChannel.IsSet());
    return WifiPhy::CalculateTxDuration(length, txVector, m_operatingChannel.GetPhyBand());
}

Ptr<WifiPpdu>
OfdmPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new OfdmPpdu(*this), false);
}

OfdmPpdu::LSigHeader::LSigHeader()
    : m_rate(0b1101),
      m_length(0)
{
}

void
OfdmPpdu::LSigHeader::SetRate(uint64_t rate, uint16_t channelWidth)
{
    if (channelWidth == 5)
    {
        rate *= 4; // corresponding 20 MHz rate if 5 MHz is used
    }
    else if (channelWidth == 10)
    {
        rate *= 2; // corresponding 20 MHz rate if 10 MHz is used
    }
    /* Here is the binary representation for a given rate:
     * 6 Mbit/s: 1101
     * 9 Mbit/s: 1111
     * 12 Mbit/s: 0101
     * 18 Mbit/s: 0111
     * 24 Mbit/s: 1001
     * 36 Mbit/s: 1011
     * 48 Mbit/s: 0001
     * 54 Mbit/s: 0011
     */
    switch (rate)
    {
    case 6000000:
        m_rate = 0b1101;
        break;
    case 9000000:
        m_rate = 0b1111;
        break;
    case 12000000:
        m_rate = 0b0101;
        break;
    case 18000000:
        m_rate = 0b0111;
        break;
    case 24000000:
        m_rate = 0b1001;
        break;
    case 36000000:
        m_rate = 0b1011;
        break;
    case 48000000:
        m_rate = 0b0001;
        break;
    case 54000000:
        m_rate = 0b0011;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid rate");
        break;
    }
}

uint64_t
OfdmPpdu::LSigHeader::GetRate(uint16_t channelWidth) const
{
    uint64_t rate = 0;
    switch (m_rate)
    {
    case 0b1101:
        rate = 6000000;
        break;
    case 0b1111:
        rate = 9000000;
        break;
    case 0b0101:
        rate = 12000000;
        break;
    case 0b0111:
        rate = 18000000;
        break;
    case 0b1001:
        rate = 24000000;
        break;
    case 0b1011:
        rate = 36000000;
        break;
    case 0b0001:
        rate = 48000000;
        break;
    case 0b0011:
        rate = 54000000;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid rate");
        break;
    }
    if (channelWidth == 5)
    {
        rate /= 4; // compute corresponding 5 MHz rate
    }
    else if (channelWidth == 10)
    {
        rate /= 2; // compute corresponding 10 MHz rate
    }
    return rate;
}

void
OfdmPpdu::LSigHeader::SetLength(uint16_t length)
{
    NS_ASSERT_MSG(length < 4096, "Invalid length");
    m_length = length;
}

uint16_t
OfdmPpdu::LSigHeader::GetLength() const
{
    return m_length;
}

} // namespace ns3
