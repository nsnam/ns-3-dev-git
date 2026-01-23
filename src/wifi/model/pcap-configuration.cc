/*
 * Copyright (c) 2026 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "pcap-configuration.h"

#include "ns3/boolean.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PcapConfiguration");

NS_OBJECT_ENSURE_REGISTERED(PcapConfiguration);

PcapConfiguration::PcapConfiguration()
{
    NS_LOG_FUNCTION(this);
}

PcapConfiguration::~PcapConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
PcapConfiguration::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::PcapConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<PcapConfiguration>()
            .AddAttribute("RadiotapTs",
                          "Whether Radiotap timestamp option is activated",
                          BooleanValue(true),
                          MakeBooleanAccessor(&PcapConfiguration::m_radiotapTs),
                          MakeBooleanChecker());
    return tid;
}

bool
PcapConfiguration::GetRadiotapTs() const
{
    return m_radiotapTs;
}

} // namespace ns3
