/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "airtime-metric.h"

#include "ns3/wifi-phy.h"

namespace ns3
{
namespace dot11s
{
NS_OBJECT_ENSURE_REGISTERED(AirtimeLinkMetricCalculator);

TypeId
AirtimeLinkMetricCalculator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::dot11s::AirtimeLinkMetricCalculator")
            .SetParent<Object>()
            .SetGroupName("Mesh")
            .AddConstructor<AirtimeLinkMetricCalculator>()
            .AddAttribute("TestLength",
                          "Number of bytes in test frame (a constant 1024 in the standard)",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&AirtimeLinkMetricCalculator::SetTestLength),
                          MakeUintegerChecker<uint16_t>(1))
            .AddAttribute("Dot11MetricTid",
                          "TID used to calculate metric (data rate)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&AirtimeLinkMetricCalculator::SetHeaderTid),
                          MakeUintegerChecker<uint8_t>(0));
    return tid;
}

AirtimeLinkMetricCalculator::AirtimeLinkMetricCalculator()
{
}

void
AirtimeLinkMetricCalculator::SetHeaderTid(uint8_t tid)
{
    m_testHeader.SetType(WIFI_MAC_DATA);
    m_testHeader.SetDsFrom();
    m_testHeader.SetDsTo();
    m_testHeader.SetQosTid(tid);
}

void
AirtimeLinkMetricCalculator::SetTestLength(uint16_t testLength)
{
    m_testFrame = Create<Packet>(testLength + 6 /*Mesh header*/ + 36 /*802.11 header*/);
}

uint32_t
AirtimeLinkMetricCalculator::CalculateMetric(Mac48Address peerAddress,
                                             Ptr<MeshWifiInterfaceMac> mac)
{
    /* Airtime link metric is defined in Section 13.9 of 802.11-2012 as:
     *
     * airtime = (O + Bt/r) /  (1 - frame error rate), where
     * o  -- the PHY dependent channel access which includes frame headers, training sequences,
     *       access protocol frames, etc.
     * bt -- the test packet length in bits (8192 by default),
     * r  -- the current bitrate of the packet,
     *
     * Final result is expressed in units of 0.01 Time Unit = 10.24 us (as required by 802.11s
     * draft)
     */
    NS_ASSERT(!peerAddress.IsGroup());
    // obtain current rate:
    WifiMode mode = mac->GetWifiRemoteStationManager()
                        ->GetDataTxVector(m_testHeader, mac->GetWifiPhy()->GetChannelWidth())
                        .GetMode();
    // obtain frame error rate:
    double failAvg = mac->GetWifiRemoteStationManager()->GetInfo(peerAddress).GetFrameErrorRate();
    if (failAvg == 1)
    {
        // Return max metric value when frame error rate equals to 1
        return (uint32_t)0xffffffff;
    }
    NS_ASSERT(failAvg < 1.0);
    WifiTxVector txVector;
    txVector.SetMode(mode);
    txVector.SetPreambleType(WIFI_PREAMBLE_LONG);
    // calculate metric
    uint32_t metric =
        (uint32_t)((double)(/*Overhead + payload*/
                            // DIFS + SIFS + AckTxTime = 2 * SIFS + 2 * SLOT + AckTxTime
                            2 * mac->GetWifiPhy()->GetSifs() + 2 * mac->GetWifiPhy()->GetSlot() +
                            mac->GetWifiPhy()->GetAckTxTime() +
                            WifiPhy::CalculateTxDuration(m_testFrame->GetSize(),
                                                         txVector,
                                                         mac->GetWifiPhy()->GetPhyBand()))
                       .GetMicroSeconds() /
                   (10.24 * (1.0 - failAvg)));
    return metric;
}
} // namespace dot11s
} // namespace ns3
