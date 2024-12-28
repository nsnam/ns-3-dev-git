/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-ppdu.h"

#include "uhr-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrPpdu");

UhrPpdu::UhrPpdu(const WifiConstPsduMap& psdus,
                 const WifiTxVector& txVector,
                 const WifiPhyOperatingChannel& channel,
                 Time ppduDuration,
                 uint64_t uid,
                 TxPsdFlag flag)
    : EhtPpdu(psdus, txVector, channel, ppduDuration, uid, flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag);
}

WifiPpduType
UhrPpdu::GetType() const
{
    if (m_psdus.contains(SU_STA_ID))
    {
        return WIFI_PPDU_TYPE_SU;
    }
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_UHR_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_UHR_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    case WIFI_PREAMBLE_UHR_ELR:
        return WIFI_PPDU_TYPE_SU;
    default:
        NS_ASSERT_MSG(false, "invalid preamble " << m_preamble);
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
UhrPpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_UHR_MU) && !m_psdus.contains(SU_STA_ID);
}

bool
UhrPpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_UHR_TB) && !m_psdus.contains(SU_STA_ID);
}

WifiMode
UhrPpdu::GetMcs(uint8_t mcs) const
{
    return UhrPhy::GetUhrMcs(mcs);
}

Ptr<WifiPpdu>
UhrPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new UhrPpdu(*this), false);
}

} // namespace ns3
