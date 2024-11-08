/*
 * Copyright (c) 2010 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "simulator-impl.h"

#include "log.h"

/**
 * @file
 * @ingroup simulator
 * ns3::SimulatorImpl implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED(SimulatorImpl);

TypeId
SimulatorImpl::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SimulatorImpl").SetParent<Object>().SetGroupName("Core");
    return tid;
}

} // namespace ns3
