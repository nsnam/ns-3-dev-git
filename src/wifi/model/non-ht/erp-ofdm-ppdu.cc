/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
    txVector.SetChannelWidth(MHz_u{20});
}

Ptr<WifiPpdu>
ErpOfdmPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new ErpOfdmPpdu(*this), false);
}

} // namespace ns3
