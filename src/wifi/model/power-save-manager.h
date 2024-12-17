/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef POWER_SAVE_MANAGER_H
#define POWER_SAVE_MANAGER_H

#include "ns3/object.h"

namespace ns3
{

class StaWifiMac;

/**
 * @ingroup wifi
 *
 * PowerSaveManager is an abstract base class. Each subclass defines a logic
 * to switch a STA in powersave mode between active state and doze state.
 */
class PowerSaveManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ~PowerSaveManager() override;

    /**
     * Set the MAC which is using this Power Save Manager
     *
     * @param mac a pointer to the MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

  protected:
    void DoDispose() override;

    /**
     * @return the MAC of the non-AP MLD managed by this Power Save Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

  private:
    Ptr<StaWifiMac> m_staMac; //!< MAC which is using this Power Save Manager
};

} // namespace ns3

#endif /* POWER_SAVE_MANAGER_H */
