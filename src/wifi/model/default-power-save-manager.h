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

  protected:
    /**
     * Put the PHY operating on the given link to sleep, if no reason to stay awake.
     *
     * @param linkId the ID of the given link
     */
    void GoToSleepIfPossible(uint8_t linkId);

    void DoNotifyAssocCompleted() override;
    void DoNotifyDisassociation() override;
    void DoNotifyReceivedBeacon(const MgtBeaconHeader& beacon, uint8_t linkId) override;
    void DoNotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void DoNotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void DoNotifyRequestAccess(Ptr<Txop> txop, uint8_t linkId) override;
    void DoTxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu) override;

    std::map<uint8_t, EventId> m_wakeUpEvents; ///< events scheduled to wake up PHYs
};

} // namespace ns3

#endif /* DEFAULT_POWER_SAVE_MANAGER_H */
