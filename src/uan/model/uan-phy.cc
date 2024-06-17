/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-phy.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanPhyCalcSinr);

TypeId
UanPhyCalcSinr::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanPhyCalcSinr").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

void
UanPhyCalcSinr::Clear()
{
}

void
UanPhyCalcSinr::DoDispose()
{
    Clear();
    Object::DoDispose();
}

NS_OBJECT_ENSURE_REGISTERED(UanPhyPer);

TypeId
UanPhyPer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanPhyPer").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

void
UanPhyPer::Clear()
{
}

void
UanPhyPer::DoDispose()
{
    Clear();
    Object::DoDispose();
}

NS_OBJECT_ENSURE_REGISTERED(UanPhy);

TypeId
UanPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UanPhy")
            .SetParent<Object>()
            .SetGroupName("Uan")
            .AddTraceSource("PhyTxBegin",
                            "Trace source indicating a packet has "
                            "begun transmitting over the channel medium.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyTxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxEnd",
                            "Trace source indicating a packet has "
                            "been completely transmitted over the channel.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyTxDrop",
                            "Trace source indicating a packet has "
                            "been dropped by the device during transmission.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyTxDropTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxBegin",
                            "Trace source indicating a packet has "
                            "begun being received from the channel medium by the device.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyRxBeginTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxEnd",
                            "Trace source indicating a packet has "
                            "been completely received from the channel medium by the device.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyRxEndTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PhyRxDrop",
                            "Trace source indicating a packet has "
                            "been dropped by the device during reception.",
                            MakeTraceSourceAccessor(&UanPhy::m_phyRxDropTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

void
UanPhy::NotifyTxBegin(Ptr<const Packet> packet)
{
    m_phyTxBeginTrace(packet);
}

void
UanPhy::NotifyTxEnd(Ptr<const Packet> packet)
{
    m_phyTxEndTrace(packet);
}

void
UanPhy::NotifyTxDrop(Ptr<const Packet> packet)
{
    m_phyTxDropTrace(packet);
}

void
UanPhy::NotifyRxBegin(Ptr<const Packet> packet)
{
    m_phyRxBeginTrace(packet);
}

void
UanPhy::NotifyRxEnd(Ptr<const Packet> packet)
{
    m_phyRxEndTrace(packet);
}

void
UanPhy::NotifyRxDrop(Ptr<const Packet> packet)
{
    m_phyRxDropTrace(packet);
}

} // namespace ns3
