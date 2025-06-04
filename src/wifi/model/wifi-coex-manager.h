/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef WIFI_COEX_MANAGER_H
#define WIFI_COEX_MANAGER_H

#include "ns3/device-coex-manager.h"
#include "ns3/event-id.h"

namespace ns3
{

class WifiNetDevice;

/**
 * @ingroup wifi
 *
 * Coex Manager for Wi-Fi netdevices. Provides the interface to receive notifications from a
 * Coex Arbitrator.
 */
class WifiCoexManager : public coex::DeviceManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WifiCoexManager();
    ~WifiCoexManager() override;

    void ResourceBusyStart(const coex::Event& coexEvent) override;

    /**
     * Set the managed WifiNetDevice.
     *
     * @param device the managed WifiNetDevice
     */
    void SetWifiNetDevice(Ptr<WifiNetDevice> device);

  protected:
    void DoDispose() override;

    Ptr<WifiNetDevice> m_device; ///< the managed WifiNetDevice
    EventId m_resourceBusyEvent; ///< end of resource busy period event
};

} // namespace ns3

#endif /* WIFI_COEX_MANAGER_H */
