/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_DEFAULT_GCR_MANAGER_H
#define WIFI_DEFAULT_GCR_MANAGER_H

#include "gcr-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * WifiDefaultGcrManager is the default implementation for groupcast with retries GCR, as defined in
 * 802.11aa. Since the standard does not describe how to map GCR-capable STAs to a given GCR group,
 * the default implementation assumes all GCR-capable STAs are part of all GCR groups. Also, it is
 * left left open to implementation which individual address to use to use while protecting a GCR
 * transmission. The default implementation decides to pick the address of the first associated
 * GCR-capable STA.
 */
class WifiDefaultGcrManager : public GcrManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    WifiDefaultGcrManager();
    ~WifiDefaultGcrManager() override;

    Mac48Address GetIndividuallyAddressedRecipient(
        const Mac48Address& groupcastAddress) const override;
};

} // namespace ns3

#endif /* WIFI_DEFAULT_GCR_MANAGER_H */
