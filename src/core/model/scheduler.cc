/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "scheduler.h"

#include "assert.h"
#include "log.h"

/**
 * @file
 * @ingroup scheduler
 * ns3::Scheduler implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Scheduler");

NS_OBJECT_ENSURE_REGISTERED(Scheduler);

Scheduler::~Scheduler()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Scheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Scheduler").SetParent<Object>().SetGroupName("Core");
    return tid;
}

} // namespace ns3
