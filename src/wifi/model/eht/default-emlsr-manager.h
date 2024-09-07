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
 * @ingroup wifi
 *
 * DefaultEmlsrManager is the default EMLSR manager.
 */
class DefaultEmlsrManager : public EmlsrManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultEmlsrManager();
    ~DefaultEmlsrManager() override;

    void NotifyRtsSent(uint8_t linkId,
                       Ptr<const WifiPsdu> rts,
                       const WifiTxVector& txVector) override;

  protected:
    uint8_t GetLinkToSendEmlOmn() override;
    std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) override;
    std::pair<bool, Time> DoGetDelayUntilAccessRequest(uint8_t linkId) override;
    void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) override;
    std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) override;
    void NotifyEmlsrModeChanged() override;

    /**
     * This function is intended to be called when an aux PHY is about to transmit an RTS on
     * the given link to calculate the time remaining to the end of the CTS reception.
     *
     * @param linkId the ID of the given link
     * @return the time remaining to the end of the CTS reception
     */
    Time GetTimeToCtsEnd(uint8_t linkId) const;

    /**
     * This function is intended to be called when an aux PHY is about to transmit an RTS on
     * the given link to calculate the time remaining to the end of the CTS reception.
     *
     * @param linkId the ID of the given link
     * @param rtsTxVector the TXVECTOR used to transmit the RTS
     * @return the time remaining to the end of the CTS reception
     */
    Time GetTimeToCtsEnd(uint8_t linkId, const WifiTxVector& rtsTxVector) const;

    /**
     * This method can only be called when aux PHYs do not switch link. Switch the main PHY back
     * to the preferred link and reconnect the aux PHY that was operating on the link left by the
     * main PHY.
     *
     * @param linkId the ID of the link that the main PHY is leaving
     * @param traceInfo information to pass to the main PHY switch traced callback (the fromLinkId
     *                  and toLinkId fields are set by SwitchMainPhy)
     */
    void SwitchMainPhyBackToPreferredLink(uint8_t linkId, EmlsrMainPhySwitchTrace&& traceInfo);

    /// Store information about a main PHY switch.
    struct MainPhySwitchInfo
    {
        Time end;     //!< end of channel switching
        uint8_t from; //!< ID of the link which the main PHY is/has been leaving
        uint8_t to;   //!< ID of the link which the main PHY is moving to
    };

    bool m_switchAuxPhy; /**< whether Aux PHY should switch channel to operate on the link on which
                              the Main PHY was operating before moving to the link of the Aux PHY */
    Ptr<WifiPhy> m_auxPhyToReconnect; //!< Aux PHY the ChannelAccessManager of the link on which
                                      //!< the main PHY is operating has to connect a listener to
                                      //!< when the main PHY is back operating on its previous link
    EventId m_auxPhySwitchEvent;      //!< event scheduled for an aux PHY to switch link
    MainPhySwitchInfo m_mainPhySwitchInfo;          //!< main PHY switch info
    std::map<uint8_t, Time> m_switchMainPhyOnRtsTx; //!< link ID-indexed map of the time when an RTS
                                                    //!< that requires the main PHY to switch link
                                                    //!< is expected to be transmitted on the link

  private:
    void DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void NotifyMainPhySwitch(std::optional<uint8_t> currLinkId,
                             uint8_t nextLinkId,
                             Ptr<WifiPhy> auxPhy,
                             Time duration) override;
    void DoNotifyIcfReceived(uint8_t linkId) override;
    void DoNotifyUlTxopStart(uint8_t linkId) override;
    void DoNotifyTxopEnd(uint8_t linkId) override;
};

} // namespace ns3

#endif /* DEFAULT_EMLSR_MANAGER_H */
