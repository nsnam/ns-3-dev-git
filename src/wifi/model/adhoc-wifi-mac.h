/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef ADHOC_WIFI_MAC_H
#define ADHOC_WIFI_MAC_H

#include "wifi-mac.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * @brief Wifi MAC high model for an ad-hoc Wifi MAC
 */
class AdhocWifiMac : public WifiMac
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    AdhocWifiMac();
    ~AdhocWifiMac() override;

    void SetLinkUpCallback(Callback<void> linkUp) override;
    bool CanForwardPacketsTo(Mac48Address to) const override;

  private:
    void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void DoCompleteConfig() override;
    void Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from) override;
};

} // namespace ns3

#endif /* ADHOC_WIFI_MAC_H */
