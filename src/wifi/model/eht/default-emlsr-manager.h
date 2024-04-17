/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef DEFAULT_EMLSR_MANAGER_H
#define DEFAULT_EMLSR_MANAGER_H

#include "emlsr-manager.h"

#include <optional>

namespace ns3
{

/**
 * \ingroup wifi
 *
 * DefaultEmlsrManager is the default EMLSR manager.
 */
class DefaultEmlsrManager : public EmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultEmlsrManager();
    ~DefaultEmlsrManager() override;

  protected:
    uint8_t GetLinkToSendEmlOmn() override;
    std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) override;
    std::pair<bool, Time> DoGetDelayUntilAccessRequest(uint8_t linkId) override;
    void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) override;
    std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) override;

    /**
     * This function is intended to be called when an aux PHY is about to transmit an RTS on
     * the given link to calculate the time remaining to the end of the CTS reception.
     *
     * \param linkId the ID of the given link
     * \return the time remaining to the end of the CTS reception
     */
    Time GetTimeToCtsEnd(uint8_t linkId) const;

    /**
     * This method can only be called when aux PHYs do not switch link. Switch the main PHY back
     * to the primary link and reconnect the aux PHY that was operating on the link left by the
     * main PHY.
     *
     * \param linkId the ID of the link that the main PHY is leaving
     */
    void SwitchMainPhyBackToPrimaryLink(uint8_t linkId);

    /// Store information about a main PHY switch.
    struct MainPhySwitchInfo
    {
        Time end;     //!< end of channel switching
        uint8_t from; //!< ID of the link which the main PHY is/has been leaving
    };

    bool m_switchAuxPhy;  /**< whether Aux PHY should switch channel to operate on the link on which
                               the Main PHY was operating before moving to the link of the Aux PHY */
    bool m_auxPhyToSleep; //!< whether Aux PHY should be put into sleep mode while the Main PHY
                          //!< is operating on the same link as the Aux PHY
    EventId m_auxPhyToSleepEvent;     //!< the event scheduled to put an Aux PHY into sleep mode
    Ptr<WifiPhy> m_auxPhyToReconnect; //!< Aux PHY the ChannelAccessManager of the link on which
                                      //!< the main PHY is operating has to connect a listener to
                                      //!< when the main PHY is back operating on its previous link
    EventId m_auxPhySwitchEvent;      //!< event scheduled for an aux PHY to switch link
    MainPhySwitchInfo m_mainPhySwitchInfo; //!< main PHY switch info

  private:
    void DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void NotifyEmlsrModeChanged() override;
    void NotifyMainPhySwitch(std::optional<uint8_t> currLinkId,
                             uint8_t nextLinkId,
                             Time duration) override;
    void DoNotifyIcfReceived(uint8_t linkId) override;
    void DoNotifyUlTxopStart(uint8_t linkId) override;
    void DoNotifyTxopEnd(uint8_t linkId) override;
};

} // namespace ns3

#endif /* DEFAULT_EMLSR_MANAGER_H */
