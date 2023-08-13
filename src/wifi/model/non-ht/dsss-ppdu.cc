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
 *         Sébastien Deronne <sebastien.deronne@gmail.com> (DsssSigHeader)
 */

#include "dsss-ppdu.h"

#include "dsss-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsssPpdu");

DsssPpdu::DsssPpdu(Ptr<const WifiPsdu> psdu,
                   const WifiTxVector& txVector,
                   const WifiPhyOperatingChannel& channel,
                   Time ppduDuration,
                   uint64_t uid)
    : WifiPpdu(psdu, txVector, channel, uid)
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << ppduDuration << uid);
    SetPhyHeaders(txVector, ppduDuration);
}

void
DsssPpdu::SetPhyHeaders(const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << txVector);
    SetDsssHeader(m_dsssSig, txVector, ppduDuration);
}

void
DsssPpdu::SetDsssHeader(DsssSigHeader& dsssSig,
                        const WifiTxVector& txVector,
                        Time ppduDuration) const
{
    dsssSig.SetRate(txVector.GetMode().GetDataRate(22));
    Time psduDuration = ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    dsssSig.SetLength(psduDuration.GetMicroSeconds());
}

WifiTxVector
DsssPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    txVector.SetChannelWidth(22);
    SetTxVectorFromDsssHeader(txVector, m_dsssSig);
    return txVector;
}

void
DsssPpdu::SetTxVectorFromDsssHeader(WifiTxVector& txVector, const DsssSigHeader& dsssSig) const
{
    txVector.SetMode(DsssPhy::GetDsssRate(dsssSig.GetRate()));
}

Time
DsssPpdu::GetTxDuration() const
{
    const auto& txVector = GetTxVector();
    const auto length = m_dsssSig.GetLength();
    return (MicroSeconds(length) + WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector));
}

Ptr<WifiPpdu>
DsssPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new DsssPpdu(*this), false);
}

DsssPpdu::DsssSigHeader::DsssSigHeader()
    : m_rate(0b00001010),
      m_length(0)
{
}

void
DsssPpdu::DsssSigHeader::SetRate(uint64_t rate)
{
    /* Here is the binary representation for a given rate:
     * 1 Mbit/s: 00001010
     * 2 Mbit/s: 00010100
     * 5.5 Mbit/s: 00110111
     * 11 Mbit/s: 01101110
     */
    switch (rate)
    {
    case 1000000:
        m_rate = 0b00001010;
        break;
    case 2000000:
        m_rate = 0b00010100;
        break;
    case 5500000:
        m_rate = 0b00110111;
        break;
    case 11000000:
        m_rate = 0b01101110;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid rate");
        break;
    }
}

uint64_t
DsssPpdu::DsssSigHeader::GetRate() const
{
    uint64_t rate = 0;
    switch (m_rate)
    {
    case 0b00001010:
        rate = 1000000;
        break;
    case 0b00010100:
        rate = 2000000;
        break;
    case 0b00110111:
        rate = 5500000;
        break;
    case 0b01101110:
        rate = 11000000;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid rate");
        break;
    }
    return rate;
}

void
DsssPpdu::DsssSigHeader::SetLength(uint16_t length)
{
    m_length = length;
}

uint16_t
DsssPpdu::DsssSigHeader::GetLength() const
{
    return m_length;
}

} // namespace ns3
