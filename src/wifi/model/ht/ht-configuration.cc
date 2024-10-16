/*
 * Copyright (c) 2018  Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ht-configuration.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HtConfiguration");

NS_OBJECT_ENSURE_REGISTERED(HtConfiguration);

HtConfiguration::HtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

HtConfiguration::~HtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
HtConfiguration::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::HtConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<HtConfiguration>()
            .AddAttribute("ShortGuardIntervalSupported",
                          "Whether or not short guard interval is supported.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&HtConfiguration::m_sgiSupported),
                          MakeBooleanChecker())
            .AddAttribute("LdpcSupported",
                          "Whether or not LDPC coding is supported.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&HtConfiguration::m_ldpcSupported),
                          MakeBooleanChecker())
            .AddAttribute("Support40MHzOperation",
                          "Whether or not 40 MHz operation is to be supported.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&HtConfiguration::m_40MHzSupported),
                          MakeBooleanChecker());
    return tid;
}

void
HtConfiguration::SetShortGuardIntervalSupported(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_sgiSupported = enable;
}

bool
HtConfiguration::GetShortGuardIntervalSupported() const
{
    return m_sgiSupported;
}

void
HtConfiguration::SetLdpcSupported(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_ldpcSupported = enable;
}

bool
HtConfiguration::GetLdpcSupported() const
{
    return m_ldpcSupported;
}

void
HtConfiguration::Set40MHzOperationSupported(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_40MHzSupported = enable;
}

bool
HtConfiguration::Get40MHzOperationSupported() const
{
    return m_40MHzSupported;
}

} // namespace ns3
