/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef WIFI_COEX_ARBITRATOR_H
#define WIFI_COEX_ARBITRATOR_H

#include "coex-arbitrator.h"

namespace ns3
{
namespace coex
{

/**
 * @ingroup coex
 * @brief Wifi Coex Arbitrator
 *
 * Coex arbitrator selecting unavailability policy for wifi Netdevices.
 */
class WifiArbitrator : public Arbitrator
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WifiArbitrator();
    ~WifiArbitrator() override;

  protected:
    bool IsRequestAccepted(const Event& coexEvent) override;
};

} // namespace coex
} // namespace ns3

#endif /* WIFI_COEX_ARBITRATOR_H */
