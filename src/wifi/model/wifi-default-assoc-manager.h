/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_DEFAULT_ASSOC_MANAGER_H
#define WIFI_DEFAULT_ASSOC_MANAGER_H

#include "wifi-assoc-manager.h"

namespace ns3
{

class StaWifiMac;

/**
 * @ingroup wifi
 *
 * Default wifi Association Manager.
 */
class WifiDefaultAssocManager : public WifiAssocManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WifiDefaultAssocManager();
    ~WifiDefaultAssocManager() override;

    void NotifyChannelSwitched(uint8_t linkId) override;
    bool Compare(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const override;

  protected:
    void DoDispose() override;
    bool CanBeInserted(const StaWifiMac::ApInfo& apInfo) const override;
    bool CanBeReturned(const StaWifiMac::ApInfo& apInfo) const override;

    /**
     * Perform operations to do at the end of a scanning procedure, such as
     * identifying the links to setup in case of 11be MLD devices.
     */
    void EndScanning();

  private:
    void DoStartScanning() override;

    /**
     * Take action upon the expiration of the timer set when requesting channel
     * switch on the given link.
     *
     * @param linkId the ID of the given link
     */
    void ChannelSwitchTimeout(uint8_t linkId);

    EventId m_waitBeaconEvent;                ///< wait beacon event
    EventId m_probeRequestEvent;              ///< probe request event
    Time m_channelSwitchTimeout;              ///< maximum delay for channel switching
    bool m_skipAssocIncompatibleChannelWidth; ///< flag whether to skip APs with incompatible
                                              ///< channel width

    /** Channel switch info */
    struct ChannelSwitchInfo
    {
        EventId timer;              ///< timer
        Mac48Address apLinkAddress; ///< AP link address
        Mac48Address apMldAddress;  ///< AP MLD address
    };

    std::vector<ChannelSwitchInfo> m_channelSwitchInfo; ///< per-link channel switch info
};

} // namespace ns3

#endif /* WIFI_DEFAULT_ASSOC_MANAGER_H */
