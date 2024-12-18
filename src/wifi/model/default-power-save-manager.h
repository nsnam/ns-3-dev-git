/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef DEFAULT_POWER_SAVE_MANAGER_H
#define DEFAULT_POWER_SAVE_MANAGER_H

#include "power-save-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * DefaultPowerSaveManager is the default power save manager.
 */
class DefaultPowerSaveManager : public PowerSaveManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultPowerSaveManager();
    ~DefaultPowerSaveManager() override;
};

} // namespace ns3

#endif /* DEFAULT_POWER_SAVE_MANAGER_H */
