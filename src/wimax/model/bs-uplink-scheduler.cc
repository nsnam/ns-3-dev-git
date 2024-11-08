/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "bs-uplink-scheduler.h"

#include "bandwidth-manager.h"
#include "bs-link-manager.h"
#include "bs-net-device.h"
#include "burst-profile-manager.h"
#include "cid.h"
#include "service-flow-record.h"
#include "service-flow.h"
#include "ss-manager.h"
#include "ss-record.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UplinkScheduler");

NS_OBJECT_ENSURE_REGISTERED(UplinkScheduler);

UplinkScheduler::UplinkScheduler()
    : m_bs(nullptr),
      m_timeStampIrInterval(),
      m_nrIrOppsAllocated(0),
      m_isIrIntrvlAllocated(false),
      m_isInvIrIntrvlAllocated(false),
      m_dcdTimeStamp(Simulator::Now()),
      m_ucdTimeStamp(Simulator::Now())
{
}

UplinkScheduler::UplinkScheduler(Ptr<BaseStationNetDevice> bs)
    : m_bs(bs),
      m_timeStampIrInterval(),
      m_nrIrOppsAllocated(0),
      m_isIrIntrvlAllocated(false),
      m_isInvIrIntrvlAllocated(false),
      m_dcdTimeStamp(Simulator::Now()),
      m_ucdTimeStamp(Simulator::Now())
{
}

UplinkScheduler::~UplinkScheduler()
{
    m_bs = nullptr;
    m_uplinkAllocations.clear();
}

void
UplinkScheduler::InitOnce()
{
}

TypeId
UplinkScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UplinkScheduler").SetParent<Object>().SetGroupName("Wimax");
    return tid;
}

uint8_t
UplinkScheduler::GetNrIrOppsAllocated() const
{
    return m_nrIrOppsAllocated;
}

void
UplinkScheduler::SetNrIrOppsAllocated(uint8_t nrIrOppsAllocated)
{
    m_nrIrOppsAllocated = nrIrOppsAllocated;
}

bool
UplinkScheduler::GetIsIrIntrvlAllocated() const
{
    return m_isIrIntrvlAllocated;
}

void
UplinkScheduler::SetIsIrIntrvlAllocated(bool isIrIntrvlAllocated)
{
    m_isIrIntrvlAllocated = isIrIntrvlAllocated;
}

bool
UplinkScheduler::GetIsInvIrIntrvlAllocated() const
{
    return m_isInvIrIntrvlAllocated;
}

void
UplinkScheduler::SetIsInvIrIntrvlAllocated(bool isInvIrIntrvlAllocated)
{
    m_isInvIrIntrvlAllocated = isInvIrIntrvlAllocated;
}

Time
UplinkScheduler::GetDcdTimeStamp() const
{
    return m_dcdTimeStamp;
}

void
UplinkScheduler::SetDcdTimeStamp(Time dcdTimeStamp)
{
    m_dcdTimeStamp = dcdTimeStamp;
}

Time
UplinkScheduler::GetUcdTimeStamp() const
{
    return m_ucdTimeStamp;
}

void
UplinkScheduler::SetUcdTimeStamp(Time ucdTimeStamp)
{
    m_ucdTimeStamp = ucdTimeStamp;
}

std::list<OfdmUlMapIe>
UplinkScheduler::GetUplinkAllocations() const
{
    return m_uplinkAllocations;
}

Time
UplinkScheduler::GetTimeStampIrInterval()
{
    return m_timeStampIrInterval;
}

void
UplinkScheduler::SetTimeStampIrInterval(Time timeStampIrInterval)
{
    m_timeStampIrInterval = timeStampIrInterval;
}

Ptr<BaseStationNetDevice>
UplinkScheduler::GetBs()
{
    return m_bs;
}

void
UplinkScheduler::SetBs(Ptr<BaseStationNetDevice> bs)
{
    m_bs = bs;
}
} // namespace ns3
