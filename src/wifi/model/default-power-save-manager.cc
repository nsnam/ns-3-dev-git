/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-power-save-manager.h"

#include "sta-wifi-mac.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultPowerSaveManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultPowerSaveManager);

TypeId
DefaultPowerSaveManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DefaultPowerSaveManager")
                            .SetParent<PowerSaveManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultPowerSaveManager>();
    return tid;
}

DefaultPowerSaveManager::DefaultPowerSaveManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultPowerSaveManager::~DefaultPowerSaveManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

} // namespace ns3
