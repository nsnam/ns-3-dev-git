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

#include "ns3/wifi-export.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * DefaultPowerSaveManager is the default power save manager.
 */
class WIFI_EXPORT DefaultPowerSaveManager : public PowerSaveManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultPowerSaveManager();
    ~DefaultPowerSaveManager() override;

  protected:
    /**
     * Put the PHY operating on the given link to sleep, if no reason to stay awake.
     *
     * @param linkId the ID of the given link
     */
    void GoToSleepIfPossible(linkId_t linkId);

    void DoNotifyAssocCompleted() override;
    void DoNotifyDisassociation() override;
    void DoNotifyPmModeChanged(WifiPowerManagementMode pmMode, linkId_t linkId) override;
    void DoNotifyReceivedBeacon(const MgtBeaconHeader& beacon, linkId_t linkId) override;
    void DoNotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, linkId_t linkId) override;
    void DoNotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, linkId_t linkId) override;
    void DoNotifyRequestAccess(Ptr<Txop> txop, linkId_t linkId) override;
    void DoNotifyChannelReleased(Ptr<Txop> txop, linkId_t linkId) override;
    void DoTxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu) override;

    std::map<linkId_t, EventId> m_wakeUpEvents; ///< events scheduled to wake up PHYs
    std::map<linkId_t, EventId> m_sleepEvents;  ///< events scheduled to set PHYs to sleep
    Time m_psmTimeout;    ///< the extra time during which the PHY is kept in active state before
                          ///< being put to sleep state
    Time m_listenAdvance; ///< the amount of time the STA wakes up in advance prior to the TBTT
};

} // namespace ns3

#endif /* DEFAULT_POWER_SAVE_MANAGER_H */
