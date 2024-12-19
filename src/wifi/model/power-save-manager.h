/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef POWER_SAVE_MANAGER_H
#define POWER_SAVE_MANAGER_H

#include "ns3/nstime.h"
#include "ns3/object.h"

#include <map>

namespace ns3
{

class StaWifiMac;
class Txop;
class MgtBeaconHeader;
class WifiMpdu;
enum WifiMacDropReason : uint8_t;
enum WifiPowerManagementMode : uint8_t;

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

    /**
     * Enable or disable Power Save mode on a given set of links. If this object is not initialized
     * yet, the settings are recorded and notified to the STA wifi MAC upon initialization.
     *
     * @param linkIdEnableMap a link ID-indexed map indicating whether to enable or not power save
     *                        mode on the link with the given ID
     */
    void SetPowerSaveMode(const std::map<uint8_t, bool>& linkIdEnableMap);

    /// @return the Listen interval
    uint32_t GetListenInterval() const;

    /**
     * Notify that the non-AP STA/MLD has completed association with an AP.
     */
    void NotifyAssocCompleted();

    /**
     * Notify that the non-AP STA/MLD has disassociated.
     */
    void NotifyDisassociation();

    /**
     * Notify that the Power Management mode of the non-AP STA operating on the given link
     * has changed.
     *
     * @param pmMode the new PM mode
     * @param linkId the ID of the given link
     */
    void NotifyPmModeChanged(WifiPowerManagementMode pmMode, uint8_t linkId);

    /**
     * Notify that a Beacon frame has been received from the associated AP on the given link.
     *
     * @param mpdu the MPDU carrying the Beacon frame
     * @param linkId the ID of the given link
     */
    void NotifyReceivedBeacon(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the reception of a frame in response to a PS-Poll frame on the given link.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the given link
     */
    void NotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the reception of a groupcast frame (possibly after a DTIM) on the given link.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the given link
     */
    void NotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify that the given TXOP is requesting channel access on the given link.
     *
     * @param txop the DCF/EDCAF requesting channel access
     * @param linkId the ID of the given link
     */
    void NotifyRequestAccess(Ptr<Txop> txop, uint8_t linkId);

    /**
     * Notify that the given MPDU has been discarded for the given reason.
     *
     * @param reason the reason why the MPDU was dropped
     * @param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

  protected:
    void DoInitialize() override;
    void DoDispose() override;

    /**
     * Information about each STA operating on a given link.
     */
    struct StaInfo
    {
        Time beaconInterval{0};       ///< Beacon interval advertised by the AP
        Time lastBeaconTimestamp{0};  ///< last time a Beacon was received from the AP
        bool pendingUnicast{false};   ///< AP has buffered unicast frame(s) (set from last TIM and
                                      ///< updated as frames are received from the AP)
        bool pendingGroupcast{false}; ///< AP has buffered groupcast frame(s) (set from last TIM
                                      ///< and updated as frames are received from the AP)
    };

    /**
     * @return the MAC of the non-AP MLD managed by this Power Save Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * Get the information about the STA operating on the given link.
     *
     * @param linkId the ID of the given link
     * @return the information about the STA operating on the given link
     */
    StaInfo& GetStaInfo(uint8_t linkId);

    /**
     * Get whether the STA has frames to send on the given link.
     *
     * @param linkId the ID of the given link
     * @return whether the STA has frames to send on the given link
     */
    bool HasFramesToSend(uint8_t linkId) const;

    /**
     * Notify subclasses that the non-AP STA/MLD has completed association with an AP.
     */
    virtual void DoNotifyAssocCompleted() = 0;

    /**
     * Notify subclasses that the non-AP STA/MLD has disassociated.
     */
    virtual void DoNotifyDisassociation() = 0;

    /**
     * Notify subclasses that a Beacon frame has been received from the associated AP on the given
     * link.
     *
     * @param beacon the Beacon frame
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyReceivedBeacon(const MgtBeaconHeader& beacon, uint8_t linkId) = 0;

    /**
     * Notify subclasses of the reception of a frame in response to a PS-Poll frame on the given
     * link. The notification is sent a SIFS after the reception of the frame.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, uint8_t linkId) = 0;

    /**
     * Notify subclasses of the reception of a groupcast frame (possibly after a DTIM) on the given
     * link.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, uint8_t linkId) = 0;

    /**
     * Notify subclasses that the given TXOP is requesting channel access on the given link.
     *
     * @param txop the DCF/EDCAF requesting channel access
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyRequestAccess(Ptr<Txop> txop, uint8_t linkId) = 0;

  private:
    Ptr<StaWifiMac> m_staMac;             //!< MAC which is using this Power Save Manager
    std::map<uint8_t, StaInfo> m_staInfo; ///< link ID-indexed map of STA infos
    std::map<uint8_t, bool>
        m_linkIdEnableMap; ///< a link ID-indexed map indicating whether to enable or not power save
                           ///< mode on the link with the given ID (used before initialization)
    uint32_t m_listenInterval; ///< beacon listen interval
};

} // namespace ns3

#endif /* POWER_SAVE_MANAGER_H */
