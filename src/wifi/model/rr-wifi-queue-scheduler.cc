/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "rr-wifi-queue-scheduler.h"

#include "qos-txop.h"
#include "wifi-mac-queue.h"
#include "wifi-mac.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RrWifiQueueScheduler");

NS_OBJECT_ENSURE_REGISTERED(RrWifiQueueScheduler);

TypeId
RrWifiQueueScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RrWifiQueueScheduler")
                            .SetParent<WifiMacQueueSchedulerImpl<WifiSchedPrecedence<Time>>>()
                            .SetGroupName("Wifi")
                            .AddConstructor<RrWifiQueueScheduler>();
    return tid;
}

RrWifiQueueScheduler::RrWifiQueueScheduler()
    : NS_LOG_TEMPLATE_DEFINE("RrWifiQueueScheduler")
{
}

void
RrWifiQueueScheduler::SetWifiMac(Ptr<WifiMac> mac)
{
    WifiMacQueueSchedulerImpl<WifiSchedPrecedence<Time>>::SetWifiMac(mac);

    const auto dev = mac->GetDevice();
    for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
    {
        dev->GetPhy(phyId)->TraceConnectWithoutContext(
            "PhyTxPsduBegin",
            MakeCallback(&RrWifiQueueScheduler::PsduTransmitted, this).Bind(phyId));
    }
}

void
RrWifiQueueScheduler::PsduTransmitted(uint8_t phyId,
                                      WifiConstPsduMap psduMap,
                                      WifiTxVector txVector,
                                      Watt_u txPower)
{
    NS_LOG_FUNCTION(this << phyId << txVector << txPower);

    const auto now = Simulator::Now();
    const auto linkId = GetMac()->GetLinkForPhy(phyId);

    NS_ASSERT_MSG(linkId,
                  "PHY " << +phyId << " transmitted a PSDU but it is not operating on any link");

    for (const auto& [staId, psdu] : psduMap)
    {
        if (const auto mpdu = *psdu->begin(); mpdu->IsQueued())
        {
            const auto ac = mpdu->GetQueueAc();

            if (auto txop = GetMac()->GetTxopFor(ac);
                txop->IsQosTxop() && StaticCast<QosTxop>(txop)->GetTxopStartTime(*linkId))
            {
                const auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);
                m_perAcQueueIdLastTxTime[ac][queueId] = now;
                SetPriority(ac, queueId, {now, queueId.type});
            }
        }
    }
}

void
RrWifiQueueScheduler::DoNotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << ac << *mpdu);

    // if the container queue was empty before the insertion of this MPDU, we need to assign a
    // priority to the container queue; let's use the last priority value we assigned to the
    // container queue (which equals the current priority if the container queue was not empty),
    // if any, or Time{0}, otherwise
    const auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);
    const auto [it, inserted] = m_perAcQueueIdLastTxTime[ac].insert({queueId, Time{0}});
    SetPriority(ac, queueId, {it->second, queueId.type});
}

void
RrWifiQueueScheduler::DoNotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << ac << mpdus.size());
    // do nothing, priority is set when a PSDU is transmitted in a TXOP
}

void
RrWifiQueueScheduler::DoNotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << ac << mpdus.size());
    // do nothing, priority is set when a PSDU is transmitted in a TXOP
}

} // namespace ns3
