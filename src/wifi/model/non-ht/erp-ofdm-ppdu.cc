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
 */

#include "erp-ofdm-ppdu.h"

#include "erp-ofdm-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ErpOfdmPpdu");

ErpOfdmPpdu::ErpOfdmPpdu(Ptr<const WifiPsdu> psdu,
                         const WifiTxVector& txVector,
                         uint16_t txCenterFreq,
                         WifiPhyBand band,
                         uint64_t uid)
    : OfdmPpdu(psdu, txVector, txCenterFreq, band, uid, true) // instantiate LSigHeader of OfdmPpdu
{
    NS_LOG_FUNCTION(this << psdu << txVector << txCenterFreq << band << uid);
}

ErpOfdmPpdu::~ErpOfdmPpdu()
{
}

WifiTxVector
ErpOfdmPpdu::DoGetTxVector() const
{
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    NS_ASSERT(m_channelWidth == 20);
    txVector.SetMode(ErpOfdmPhy::GetErpOfdmRate(m_lSig.GetRate()));
    txVector.SetChannelWidth(m_channelWidth);
    return txVector;
}

Ptr<WifiPpdu>
ErpOfdmPpdu::Copy() const
{
    return Create<ErpOfdmPpdu>(GetPsdu(), GetTxVector(), m_txCenterFreq, m_band, m_uid);
}

} // namespace ns3
