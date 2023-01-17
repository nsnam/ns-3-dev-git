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
                 TxPsdFlag flag)
    : HePpdu(psdus, txVector, txCenterFreq, ppduDuration, band, uid, flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << txCenterFreq << ppduDuration << band << uid
                         << flag);

    // For EHT SU transmissions (carried in EHT MU PPDUs), we have to:
    // - store the EHT-SIG content channels
    // - store the MCS and the number of streams for the data field
    // because this is not done by the parent class.
    // This is a workaround needed until we properly implement 11be PHY headers.
    if (ns3::IsDlMu(m_preamble) && !txVector.IsDlMu())
    {
        m_contentChannelAlloc = txVector.GetContentChannelAllocation();
        m_ruAllocation = txVector.GetRuAllocation();
        m_ehtSuMcs = txVector.GetMode().GetMcsValue();
        m_ehtSuNStreams = txVector.GetNss();
    }
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
    txVector.SetMode(EhtPhy::GetEhtMcs(m_ehtSuMcs));
    txVector.SetChannelWidth(m_heSig.GetChannelWidth());
    txVector.SetNss(m_ehtSuNStreams);
    txVector.SetGuardInterval(m_heSig.GetGuardInterval());
    txVector.SetBssColor(m_heSig.GetBssColor());
    txVector.SetLength(m_lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    if (!m_muUserInfos.empty())
    {
        txVector.SetEhtPpduType(0); // FIXME set to 2 for DL MU-MIMO (non-OFDMA) transmission
    }
    for (const auto& muUserInfo : m_muUserInfos)
    {
        txVector.SetHeMuUserInfo(muUserInfo.first, muUserInfo.second);
    }
    if (ns3::IsDlMu(m_preamble))
    {
        txVector.SetSigBMode(HePhy::GetVhtMcs(m_heSig.GetMcs()));
        txVector.SetRuAllocation(m_ruAllocation);
    }
    return txVector;
}

Ptr<WifiPpdu>
EhtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new EhtPpdu(*this), false);
}

} // namespace ns3
