/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADVANCED_EMLSR_MANAGER_H
#define ADVANCED_EMLSR_MANAGER_H

#include "default-emlsr-manager.h"

#include "ns3/channel-access-manager.h"

#include <memory>

class EmlsrSwitchMainPhyBackTest;

namespace ns3
{

class WifiPhyListener;

/**
 * @ingroup wifi
 *
 * AdvancedEmlsrManager is an advanced EMLSR manager.
 */
class AdvancedEmlsrManager : public DefaultEmlsrManager
{
    /// Allow test cases to access private members
    friend class ::EmlsrSwitchMainPhyBackTest;

  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    AdvancedEmlsrManager();
    ~AdvancedEmlsrManager() override;

    std::optional<WifiIcfDrop> CheckMainPhyTakesOverDlTxop(uint8_t linkId) const override;

    /**
     * This method is called by the PHY listener attached to the main PHY when a switch main PHY
     * back timer is started to notify of events that may delay the channel access for the main
     * PHY on the current link. If the expected channel access is beyond the end of the switch
     * main PHY timer expiration plus a channel switch delay, the timer is stopped immediately.
     */
    void InterruptSwitchMainPhyBackTimerIfNeeded();

  protected:
    void DoDispose() override;
    void DoSetWifiMac(Ptr<StaWifiMac> mac) override;
    std::pair<bool, Time> DoGetDelayUntilAccessRequest(uint8_t linkId) override;
    std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) override;
    void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) override;
    void NotifyEmlsrModeChanged() override;
    void SwitchMainPhyBackToPreferredLink(uint8_t linkId,
                                          EmlsrMainPhySwitchTrace&& traceInfo) override;

    /**
     * Possibly take actions when notified of the MAC header of the MPDU being received by the
     * given PHY.
     *
     * @param phy the given PHY
     * @param macHdr the MAC header of the MPDU being received
     * @param txVector the TXVECTOR used to transmit the PSDU
     * @param psduDuration the remaining duration of the PSDU
     */
    void ReceivedMacHdr(Ptr<WifiPhy> phy,
                        const WifiMacHeader& macHdr,
                        const WifiTxVector& txVector,
                        Time psduDuration);

    /**
     * Use information from NAV and CCA performed by the given PHY on the given link in the last
     * PIFS interval to determine whether the given EDCAF can start a TXOP. This function is
     * intended to be used when the main PHY switches channel to start an UL TXOP on a link where
     * channel access was obtained by a non-TX capable aux PHY.
     *
     * @param phy the PHY that performed CCA in the last PIFS interval
     * @param linkId the ID of the given link
     * @param edca the given EDCAF
     */
    void CheckNavAndCcaLastPifs(Ptr<WifiPhy> phy, uint8_t linkId, Ptr<QosTxop> edca);

    /**
     * Determine whether the main PHY shall be requested to switch to the link of an aux PHY that
     * is expected to gain channel access through the given AC in the given delay but it is not
     * TX capable.
     *
     * @param linkId the ID of the link on which the aux PHY is operating
     * @param aci the index of the given AC
     * @param delay the delay after which the given AC is expected to gain channel access. Zero
     *              indicates that channel access has been actually gained
     * @return whether the main PHY shall be requested to switch to the link of the aux PHY
     */
    bool RequestMainPhyToSwitch(uint8_t linkId, AcIndex aci, const Time& delay);

    /**
     * This method is called when the given AC of the EMLSR client is expected to get channel
     * access in the given delay on the given link, on which an aux PHY that is not TX capable
     * is operating. This method has to decide whether to request the main PHY to switch to the
     * given link to try to start a TXOP.
     *
     * @param linkId the ID of the given link
     * @param aci the index of the given AC
     * @param delay the delay after which the given AC is expected to gain channel access
     */
    void SwitchMainPhyIfTxopToBeGainedByAuxPhy(uint8_t linkId, AcIndex aci, const Time& delay);

    /**
     * This method is called when the switch main PHY back timer (which is started when the main PHY
     * switches to the link of an aux PHY that does not switch and is not TX capable) expires and
     * decides whether to delay the request to switch the main PHY back to the preferred link or to
     * execute it immediately. This method can also be called to terminate a running switch main PHY
     * back timer, in case it is determined that channel access is not expected to be gained before
     * the expiration of the timer plus the channel switch delay.
     *
     * @param linkId the ID of the link that the main PHY is leaving
     * @param stopReason the reason for terminating the switch main PHY back timer before expiration
     */
    void SwitchMainPhyBackDelayExpired(uint8_t linkId,
                                       std::optional<WifiExpectedAccessReason> stopReason);

    /**
     * Register a PHY listener so that this EMLSR Manager is notified of PHY events generated by
     * the given PHY.
     *
     * @param phy the PHY which a listener is connected to
     */
    void RegisterListener(Ptr<WifiPhy> phy);

    /// Disconnect the PHY listener from the PHY it is connected to (if any)
    void UnregisterListener();

  private:
    void DoNotifyTxopEnd(uint8_t linkId, Ptr<QosTxop> edca) override;
    void DoNotifyDlTxopStart(uint8_t linkId) override;
    void DoNotifyUlTxopStart(uint8_t linkId) override;

    bool m_allowUlTxopInRx;        //!< whether a (main or aux) PHY is allowed to start an UL
                                   //!< TXOP if another PHY is receiving a PPDU
    bool m_interruptSwitching;     //!< whether a main PHY switching can be interrupted to start
                                   //!< switching to another link
    bool m_useAuxPhyCca;           //!< whether the CCA performed in the last PIFS interval by a
                                   //!< non-TX capable aux PHY should be used when the main PHY
                                   //!< ends switching to the aux PHY's link to determine whether
                                   //!< TX can start or not
    Time m_switchMainPhyBackDelay; //!< duration of the timer started in case of non-TX capable aux
                                   //!< PHY when medium is sensed busy during the PIFS interval
                                   //!< preceding/following the main PHY switch end
    bool m_keepMainPhyAfterDlTxop; //!< whether the main PHY must stay, for a switch main PHY back
                                   //!< delay, on an aux PHY link after a DL TXOP, in case aux PHYs
                                   //!< are not TX capable and do not switch
    bool m_checkAccessOnMainPhyLink; //!< in case aux PHYs are not TX capable and an Access
                                     //!< Category, say it AC X, is about to gain channel access on
                                     //!< an aux PHY link, determine whether the time the ACs with
                                     //!< priority higher than or equal to AC X and with frames to
                                     //!< send on the main PHY link are expected to gain access on
                                     //!< the main PHY link should be taken into account when taking
                                     //!< the decision to switch the main PHY to the aux PHY link
    AcIndex m_minAcToSkipCheckAccess; //!< if m_checkAccessOnMainPhyLink is set to false, indicate
                                      //!< the minimum priority AC for which it is allowed to skip
                                      //!< the check related to the expected channel access time on
                                      //!< the main PHY link
    EventId m_ccaLastPifs;            //!< event scheduled in case of non-TX capable aux PHY to
                                      //!< determine whether TX can be started based on whether
                                      //!< the medium has been idle during the last PIFS interval
    EventId m_switchMainPhyBackEvent; //!< event scheduled in case of non-TX capable aux PHY when
                                      //!< medium is sensed busy during the PIFS interval
                                      //!< preceding/following the main PHY switch end
    std::shared_ptr<WifiPhyListener>
        m_phyListener; //!< PHY listener connected to an aux PHY (that is not TX capable and does
                       //!< not switch link) while the main PHY attempts to gain access on the aux
                       //!< PHY link
    Ptr<WifiPhy> m_auxPhyWithListener; //!< aux PHY which a PHY listener is connected to
};

/**
 * Struct to trace that main PHY switched to leave a link on which an aux PHY was expected to gain
 * a TXOP but the main PHY did not manage to gain a TXOP in the pre-configured amount of time.
 */
struct EmlsrSwitchMainPhyBackTrace : public EmlsrMainPhySwitchTraceImpl<EmlsrSwitchMainPhyBackTrace>
{
    static constexpr std::string_view m_name = "TxopNotGainedOnAuxPhyLink"; //!< trace name

    Time elapsed; //!< the time elapsed since the switch main PHY back timer started
    std::optional<WifiExpectedAccessReason>
        earlySwitchReason; //!< the reason why the main PHY switches back before the expiration of
                           //!< the switch main PHY back timer
    bool isSwitching; //!< whether the main PHY is switching while it is requested to switch back

    /**
     * Constructor provided because this struct is not an aggregate (it has a base struct), hence
     * we cannot use designated initializers.
     *
     * @param time the value for the elapsed field
     * @param reason the value for the earlySwitchReason field
     * @param switching the value for the isSwitching field
     */
    EmlsrSwitchMainPhyBackTrace(Time time,
                                std::optional<WifiExpectedAccessReason> reason,
                                bool switching)
        : elapsed(time),
          earlySwitchReason(reason),
          isSwitching(switching)
    {
    }
};

} // namespace ns3

#endif /* ADVANCED_EMLSR_MANAGER_H */
