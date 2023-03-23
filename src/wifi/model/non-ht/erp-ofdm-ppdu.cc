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
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ErpOfdmPpdu");

ErpOfdmPpdu::ErpOfdmPpdu(Ptr<const WifiPsdu> psdu,
                         const WifiTxVector& txVector,
                         const WifiPhyOperatingChannel& channel,
                         uint64_t uid)
    : OfdmPpdu(psdu, txVector, channel, uid, true) // add LSigHeader of OfdmPpdu
{
    NS_LOG_FUNCTION(this << psdu << txVector << channel << uid);
}

void
ErpOfdmPpdu::SetTxVectorFromLSigHeader(WifiTxVector& txVector, const LSigHeader& lSig) const
{
    txVector.SetMode(ErpOfdmPhy::GetErpOfdmRate(lSig.GetRate()));
    txVector.SetChannelWidth(20);
}

Ptr<WifiPpdu>
ErpOfdmPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new ErpOfdmPpdu(*this), false);
}

} // namespace ns3
