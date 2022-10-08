/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wave-frame-exchange-manager.h"

#include "higher-tx-tag.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/qos-blocked-destinations.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-protection.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WaveFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(WaveFrameExchangeManager);

TypeId
WaveFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WaveFrameExchangeManager")
                            .SetParent<QosFrameExchangeManager>()
                            .AddConstructor<WaveFrameExchangeManager>()
                            .SetGroupName("Wave");
    return tid;
}

WaveFrameExchangeManager::WaveFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

WaveFrameExchangeManager::~WaveFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WaveFrameExchangeManager::SetWaveNetDevice(Ptr<WaveNetDevice> device)
{
    m_scheduler = device->GetChannelScheduler();
    m_coordinator = device->GetChannelCoordinator();
    NS_ASSERT(m_scheduler && m_coordinator);
}

WifiTxVector
WaveFrameExchangeManager::GetDataTxVector(Ptr<const WifiMpdu> item) const
{
    NS_LOG_FUNCTION(this << *item);
    HigherLayerTxVectorTag datatag;
    bool found;
    found = ConstCast<Packet>(item->GetPacket())->PeekPacketTag(datatag);
    // if high layer has not controlled transmit parameters, the real transmit parameters
    // will be determined by MAC layer itself.
    if (!found)
    {
        return m_mac->GetWifiRemoteStationManager()->GetDataTxVector(item->GetHeader(),
                                                                     m_allowedWidth);
    }

    // if high layer has set the transmit parameters with non-adaption mode,
    // the real transmit parameters are determined by high layer.
    if (!datatag.IsAdaptable())
    {
        return datatag.GetTxVector();
    }

    // if high layer has set the transmit parameters with non-adaption mode,
    // the real transmit parameters are determined by both high layer and MAC layer.
    WifiTxVector txHigher = datatag.GetTxVector();
    WifiTxVector txMac =
        m_mac->GetWifiRemoteStationManager()->GetDataTxVector(item->GetHeader(), m_allowedWidth);
    WifiTxVector txAdapter;
    txAdapter.SetChannelWidth(10);
    // the DataRate set by higher layer is the minimum data rate
    // which is the lower bound for the actual data rate.
    if (txHigher.GetMode().GetDataRate(txHigher.GetChannelWidth()) >
        txMac.GetMode().GetDataRate(txMac.GetChannelWidth()))
    {
        txAdapter.SetMode(txHigher.GetMode());
        txAdapter.SetPreambleType(txHigher.GetPreambleType());
    }
    else
    {
        txAdapter.SetMode(txMac.GetMode());
        txAdapter.SetPreambleType(txMac.GetPreambleType());
    }
    // the TxPwr_Level set by higher layer is the maximum transmit
    // power which is the upper bound for the actual transmit power;
    txAdapter.SetTxPowerLevel(std::min(txHigher.GetTxPowerLevel(), txMac.GetTxPowerLevel()));

    return txAdapter;
}

bool
WaveFrameExchangeManager::StartTransmission(Ptr<Txop> dcf, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << dcf << allowedWidth);

    uint32_t curChannel = m_phy->GetChannelNumber();
    // if current channel access is not AlternatingAccess, just do as FrameExchangeManager.
    if (!m_scheduler || !m_scheduler->IsAlternatingAccessAssigned(curChannel))
    {
        return FrameExchangeManager::StartTransmission(dcf, allowedWidth);
    }

    m_txTimer.Cancel();
    m_dcf = dcf;
    m_allowedWidth = allowedWidth;

    Ptr<WifiMacQueue> queue = dcf->GetWifiMacQueue();

    if (queue->IsEmpty())
    {
        NS_LOG_DEBUG("Queue empty");
        m_dcf->NotifyChannelReleased(0);
        m_dcf = nullptr;
        return false;
    }

    m_dcf->NotifyChannelAccessed(0);
    Ptr<WifiMpdu> mpdu = queue->PeekFirstAvailable(0);
    NS_ASSERT(mpdu);

    // assign a sequence number if this is not a fragment nor a retransmission
    if (!mpdu->IsFragment() && !mpdu->GetHeader().IsRetry())
    {
        uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor(&mpdu->GetHeader());
        mpdu->GetHeader().SetSequenceNumber(sequence);
    }

    WifiTxParameters txParams;
    txParams.m_txVector = GetDataTxVector(mpdu);
    Time remainingTime = m_coordinator->NeedTimeToGuardInterval();

    if (!TryAddMpdu(mpdu, txParams, remainingTime))
    {
        // The attempt for this transmission will be canceled;
        // and this packet will be pending for next transmission by QosTxop class
        NS_LOG_DEBUG("Because the required transmission time exceeds the remainingTime = "
                     << remainingTime.As(Time::MS)
                     << ", currently this packet will not be transmitted.");
    }
    else
    {
        SendMpduWithProtection(mpdu, txParams);
        return true;
    }
    return false;
}

void
WaveFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_scheduler = nullptr;
    m_coordinator = nullptr;
    FrameExchangeManager::DoDispose();
}

} // namespace ns3
