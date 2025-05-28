/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "wifi-coex-arbitrator.h"

#include "ns3/log.h"

namespace ns3
{
namespace coex
{

NS_LOG_COMPONENT_DEFINE("WifiCoexArbitrator");

NS_OBJECT_ENSURE_REGISTERED(WifiArbitrator);

TypeId
WifiArbitrator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::coex::WifiArbitrator")
                            .SetParent<Arbitrator>()
                            .SetGroupName("Coex")
                            .AddConstructor<WifiArbitrator>();
    return tid;
}

WifiArbitrator::WifiArbitrator()
{
    NS_LOG_FUNCTION(this);
}

WifiArbitrator::~WifiArbitrator()
{
    NS_LOG_FUNCTION_NOARGS();
}

} // namespace coex
} // namespace ns3
