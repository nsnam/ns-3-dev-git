/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "device-coex-manager.h"

#include "coex-arbitrator.h"

#include "ns3/log.h"

/**
 * @file
 * @ingroup coex
 * ns3::coex::DeviceManager implementation.
 */

namespace ns3
{
namespace coex
{

NS_LOG_COMPONENT_DEFINE("DeviceCoexManager");

NS_OBJECT_ENSURE_REGISTERED(DeviceManager);

TypeId
DeviceManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::coex::DeviceManager").SetParent<Object>().SetGroupName("Coex");
    return tid;
}

DeviceManager::~DeviceManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DeviceManager::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_coexArbitrator = nullptr;
    Object::DoDispose();
}

void
DeviceManager::SetCoexArbitrator(Ptr<Arbitrator> coexArbitrator)
{
    NS_LOG_FUNCTION(this << coexArbitrator);
    m_coexArbitrator = coexArbitrator;
}

Ptr<Arbitrator>
DeviceManager::GetCoexArbitrator() const
{
    return m_coexArbitrator;
}

} // namespace coex
} // namespace ns3
