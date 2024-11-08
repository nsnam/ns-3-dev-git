/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_PROTECTION_MANAGER_H
#define WIFI_PROTECTION_MANAGER_H

#include "wifi-protection.h"

#include "ns3/object.h"

#include <memory>

namespace ns3
{

class WifiTxParameters;
class WifiMpdu;
class WifiMac;
class WifiRemoteStationManager;

/**
 * @ingroup wifi
 *
 * WifiProtectionManager is an abstract base class. Each subclass defines a logic
 * to select the protection method for a given frame.
 */
class WifiProtectionManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WifiProtectionManager();
    ~WifiProtectionManager() override;

    /**
     * Set the MAC which is using this Protection Manager
     *
     * @param mac a pointer to the MAC
     */
    void SetWifiMac(Ptr<WifiMac> mac);
    /**
     * Set the ID of the link this Protection Manager is associated with.
     *
     * @param linkId the ID of the link this Protection Manager is associated with
     */
    void SetLinkId(uint8_t linkId);

    /**
     * Determine the protection method to use if the given MPDU is added to the current
     * frame. Return a null pointer if the protection method is unchanged or the new
     * protection method otherwise.
     *
     * @param mpdu the MPDU to be added to the current frame
     * @param txParams the current TX parameters for the current frame
     * @return a null pointer if the protection method is unchanged or the new
     *         protection method otherwise
     */
    virtual std::unique_ptr<WifiProtection> TryAddMpdu(Ptr<const WifiMpdu> mpdu,
                                                       const WifiTxParameters& txParams) = 0;

    /**
     * Determine the protection method to use if the given MSDU is aggregated to the
     * current frame. Return a null pointer if the protection method is unchanged or
     * the new protection method otherwise.
     *
     * @param msdu the MSDU to be aggregated to the current frame
     * @param txParams the current TX parameters for the current frame
     * @return a null pointer if the protection method is unchanged or the new
     *         protection method otherwise
     */
    virtual std::unique_ptr<WifiProtection> TryAggregateMsdu(Ptr<const WifiMpdu> msdu,
                                                             const WifiTxParameters& txParams) = 0;

  protected:
    void DoDispose() override;

    /**
     * @return the remote station manager operating on our link
     */
    Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager() const;

    /**
     * Add a User Info field to the given MU-RTS Trigger Frame to solicit a CTS from
     * the station with the given MAC address. The MU-RTS is intended to protect a data
     * frame having the given TX width. The TX width of the solicited CTS is the minimum
     * between the TX width of the protected data frame and the maximum width supported
     * by the solicited station.
     *
     * @param muRts the MU-RTS Trigger Frame
     * @param txWidth the TX width of the protected data frame
     * @param receiver the MAC address of the solicited station
     */
    void AddUserInfoToMuRts(CtrlTriggerHeader& muRts,
                            MHz_u txWidth,
                            const Mac48Address& receiver) const;

    Ptr<WifiMac> m_mac; //!< MAC which is using this Protection Manager
    uint8_t m_linkId;   //!< ID of the link this Protection Manager is operating on
};

} // namespace ns3

#endif /* WIFI_PROTECTION_MANAGER_H */
