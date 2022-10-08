/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * \ingroup wifi
 *
 * Default wifi Association Manager.
 */
class WifiDefaultAssocManager : public WifiAssocManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
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
     * \param linkId the ID of the given link
     */
    void ChannelSwitchTimeout(uint8_t linkId);

    EventId m_waitBeaconEvent;   ///< wait beacon event
    EventId m_probeRequestEvent; ///< probe request event
    Time m_channelSwitchTimeout; ///< maximum delay for channel switching

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
