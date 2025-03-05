/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "default-dso-manager.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultDsoManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultDsoManager);

TypeId
DefaultDsoManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DefaultDsoManager")
            .SetParent<DsoManager>()
            .SetGroupName("Wifi")
            .AddConstructor<DefaultDsoManager>()
            .AddAttribute("ChSwitchToDsoBandDelay",
                          "The delay to switch the channel to the DSO subband.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&DefaultDsoManager::m_chSwitchToDsoBandDelay),
                          MakeTimeChecker(Time(), Time())); // only a zero delay is allowed for now
    return tid;
}

DefaultDsoManager::DefaultDsoManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultDsoManager::~DefaultDsoManager()
{
    NS_LOG_FUNCTION(this);
}

Time
DefaultDsoManager::GetSwitchingDelayToDsoSubband() const
{
    return m_chSwitchToDsoBandDelay;
}

} // namespace ns3
