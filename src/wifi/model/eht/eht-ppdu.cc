/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-ppdu.h"

#include "eht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtPpdu");

EhtPpdu::EhtPpdu(const WifiConstPsduMap& psdus,
                 const WifiTxVector& txVector,
                 uint16_t txCenterFreq,
                 Time ppduDuration,
                 WifiPhyBand band,
                 uint64_t uid,
                 TxPsdFlag flag,
                 uint8_t p20Index)
    : HePpdu(psdus, txVector, txCenterFreq, ppduDuration, band, uid, flag, p20Index)
{
    NS_LOG_FUNCTION(this << psdus << txVector << txCenterFreq << ppduDuration << band << uid << flag
                         << p20Index);
}

EhtPpdu::~EhtPpdu()
{
    NS_LOG_FUNCTION(this);
}

WifiPpduType
EhtPpdu::GetType() const
{
    if (m_muUserInfos.empty())
    {
        return WIFI_PPDU_TYPE_SU;
    }
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_EHT_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_EHT_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    default:
        NS_ASSERT_MSG(false, "invalid preamble " << m_preamble);
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
EhtPpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_MU) && !m_muUserInfos.empty();
}

bool
EhtPpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_TB) && !m_muUserInfos.empty();
}

WifiTxVector
EhtPpdu::DoGetTxVector() const
{
    // FIXME: define EHT PHY headers
    WifiTxVector txVector;
    txVector.SetPreambleType(m_preamble);
    txVector.SetMode(EhtPhy::GetEhtMcs(m_heSig.GetMcs()));
    txVector.SetChannelWidth(m_heSig.GetChannelWidth());
    txVector.SetNss(m_heSig.GetNStreams());
    txVector.SetGuardInterval(m_heSig.GetGuardInterval());
    txVector.SetBssColor(m_heSig.GetBssColor());
    txVector.SetLength(m_lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    for (const auto& muUserInfo : m_muUserInfos)
    {
        txVector.SetHeMuUserInfo(muUserInfo.first, muUserInfo.second);
    }
    return txVector;
}

Ptr<WifiPpdu>
EhtPpdu::Copy() const
{
    return ns3::Copy(Ptr(this));
}

} // namespace ns3
