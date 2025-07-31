/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "coex-helper.h"

#include "ns3/device-coex-manager.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CoexHelper");

void
CoexHelper::Install(const NodeContainer& c) const
{
    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        Install(c.Get(i));
    }
}

void
CoexHelper::Install(Ptr<Node> node) const
{
    auto arbitrator = m_factory.Create();
    node->AggregateObject(arbitrator);
}

void
CoexHelper::EnableLogComponents(LogLevel logLevel)
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_NODE);

    LogComponentEnable("CoexArbitrator", logLevel);
    LogComponentEnable("DeviceCoexManager", logLevel);
    LogComponentEnable("MockEventGenerator", logLevel);
}

} // namespace ns3
