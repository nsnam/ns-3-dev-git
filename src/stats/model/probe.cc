/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 */

#include "probe.h"

#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Probe");

NS_OBJECT_ENSURE_REGISTERED(Probe);

TypeId
Probe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Probe")
                            .SetParent<DataCollectionObject>()
                            .SetGroupName("Stats")
                            .AddAttribute("Start",
                                          "Time data collection starts",
                                          TimeValue(Seconds(0)),
                                          MakeTimeAccessor(&Probe::m_start),
                                          MakeTimeChecker())
                            .AddAttribute("Stop",
                                          "Time when data collection stops.  The special time "
                                          "value of 0 disables this attribute",
                                          TimeValue(Seconds(0)),
                                          MakeTimeAccessor(&Probe::m_stop),
                                          MakeTimeChecker());
    return tid;
}

Probe::Probe()
{
    NS_LOG_FUNCTION(this);
}

Probe::~Probe()
{
    NS_LOG_FUNCTION(this);
}

bool
Probe::IsEnabled() const
{
    return (DataCollectionObject::IsEnabled() && Simulator::Now() >= m_start &&
            (m_stop.IsZero() || Simulator::Now() < m_stop));
}

} // namespace ns3
