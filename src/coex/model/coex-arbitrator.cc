/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "coex-arbitrator.h"

#include "device-coex-manager.h"

#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"

/**
 * @file
 * @ingroup coex
 * ns3::coex::Arbitrator implementation.
 */

namespace ns3
{
namespace coex
{

NS_LOG_COMPONENT_DEFINE("CoexArbitrator");

NS_OBJECT_ENSURE_REGISTERED(Arbitrator);

TypeId
Arbitrator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::coex::Arbitrator")
                            .SetParent<Object>()
                            .SetGroupName("Coex")
                            .AddConstructor<Arbitrator>();
    return tid;
}

Arbitrator::Arbitrator()
{
    NS_LOG_FUNCTION_NOARGS();
}

Arbitrator::~Arbitrator()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Arbitrator::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_node = nullptr;
    for (const auto& [rat, coexManager] : m_devCoexManagers)
    {
        coexManager->Dispose();
    }
    m_devCoexManagers.clear();
    Object::DoDispose();
}

void
Arbitrator::NotifyNewAggregate()
{
    NS_LOG_FUNCTION(this);
    if (!m_node)
    {
        // verify that a Node has been aggregated
        if (auto node = this->GetObject<Node>())
        {
            m_node = node;
        }
    }
    Object::NotifyNewAggregate();
}

void
Arbitrator::SetDeviceCoexManager(Rat rat, Ptr<DeviceManager> coexManager)
{
    NS_LOG_FUNCTION(this << rat << coexManager);

    m_devCoexManagers[rat] = coexManager;
    coexManager->SetCoexArbitrator(this);
}

Ptr<DeviceManager>
Arbitrator::GetDeviceCoexManager(Rat rat) const
{
    if (const auto it = m_devCoexManagers.find(rat); it != m_devCoexManagers.cend())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace coex
} // namespace ns3
