/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef EMLSR_MANAGER_H
#define EMLSR_MANAGER_H

#include "ns3/ctrl-headers.h"
#include "ns3/mac48-address.h"
#include "ns3/object.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-phy-operating-channel.h"

#include <map>
#include <optional>
#include <set>
#include <utility>

class EmlsrCcaBusyTest;

namespace ns3
{

class EhtFrameExchangeManager;
class MgtEmlOmn;
class WifiMpdu;

/**
 * \ingroup wifi
 *
 * EmlsrManager is an abstract base class defining the API that EHT non-AP MLDs
 * with EMLSR activated can use to handle the operations on the EMLSR links
 */
class EmlsrManager : public Object
{
    /// Allow test cases to access private members
    friend class ::EmlsrCcaBusyTest;

  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    EmlsrManager();
    ~EmlsrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an EHT non-AP MLD.
     *
     * \param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Set the Transition Timeout advertised by the associated AP with EMLSR activated.
     *
     * \param timeout the advertised Transition Timeout
     */
    void SetTransitionTimeout(Time timeout);

    /**
     * \return the Transition Timeout, if advertised by the associated AP
     */
    std::optional<Time> GetTransitionTimeout() const;

    /**
     * Set the duration of the MediumSyncDelay timer.
     *
     * \param duration the duration of the MediumSyncDelay timer
     */
    void SetMediumSyncDuration(Time duration);

    /**
     * \return the duration of the MediumSyncDelay timer
     */
    Time GetMediumSyncDuration() const;

    /**
     * Set the Medium Synchronization OFDM ED threshold (dBm) to use while the MediumSyncDelay
     * timer is running.
     *
     * \param threshold the threshold in dBm (ranges from -72 to -62 dBm)
     */
    void SetMediumSyncOfdmEdThreshold(int8_t threshold);

    /**
     * \return the Medium Synchronization OFDM ED threshold (dBm) to use while the MediumSyncDelay
     * timer is running.
     */
    int8_t GetMediumSyncOfdmEdThreshold() const;

    /**
     * Set the maximum number of TXOPs a non-AP STA is allowed to attempt to initiate while
     * the MediumSyncDelay timer is running. No value indicates no limit.
     *
     * \param nTxops the maximum number of TXOPs a non-AP STA is allowed to attempt to
     *               initiate while the MediumSyncDelay timer is running
     */
    void SetMediumSyncMaxNTxops(std::optional<uint8_t> nTxops);

    /**
     * \return the maximum number of TXOPs a non-AP STA is allowed to attempt to initiate while
     * the MediumSyncDelay timer is running. No value indicates no limit.
     */
    std::optional<uint8_t> GetMediumSyncMaxNTxops() const;

    /**
     * \return the ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
     */
    uint8_t GetMainPhyId() const;

    /**
     * Take actions to enable EMLSR mode on the given set of links, if non-empty, or
     * disable EMLSR mode, otherwise.
     *
     * \param linkIds the IDs of the links on which EMLSR mode should be enabled
     *                (empty to disable EMLSR mode)
     */
    void SetEmlsrLinks(const std::set<uint8_t>& linkIds);

    /**
     * \return the set of links on which EMLSR mode is enabled
     */
    const std::set<uint8_t>& GetEmlsrLinks() const;

    /**
     * Set the member variable indicating whether the state of the CAM should be reset when
     * the main PHY switches channel and operates on the link associated with the CAM.
     *
     * \param enable whether the CAM state should be reset
     */
    void SetCamStateReset(bool enable);

    /**
     * \return the value of the member variable indicating whether the state of the CAM should be
     * reset when the main PHY switches channel and operates on the link associated with the CAM.
     */
    bool GetCamStateReset() const;

    /**
     * Notify that an UL TXOP is gained on the given link by the given AC. This method has to
     * determine whether to start the UL TXOP or release the channel.
     *
     * \param linkId the ID of the given link
     * \param aci the index of the given AC
     * \return a pair consisting of a boolean value indicating whether the UL TXOP can be started
     *         and a Time value indicating the delay after which the EMLSR client must restart
     *         channel access (if needed) in case the UL TXOP is not started
     */
    std::pair<bool, Time> GetDelayUntilAccessRequest(uint8_t linkId, AcIndex aci);

    /**
     * Set the member variable indicating whether Aux PHYs are capable of transmitting PPDUs.
     *
     * \param capable whether Aux PHYs are capable of transmitting PPDUs
     */
    void SetAuxPhyTxCapable(bool capable);

    /**
     * \return whether Aux PHYs are capable of transmitting PPDUs
     */
    bool GetAuxPhyTxCapable() const;

    /**
     * Set the member variable indicating whether in-device interference is such that a PHY cannot
     * decode anything and cannot decrease the backoff counter when another PHY is transmitting.
     *
     * \param enable whether in-device interference is such that a PHY cannot decode anything
     *               and cannot decrease the backoff counter when another PHY is transmitting
     */
    void SetInDeviceInterference(bool enable);

    /**
     * \return whether in-device interference is such that a PHY cannot decode anything and cannot
     *         decrease the backoff counter when another PHY is transmitting
     */
    bool GetInDeviceInterference() const;

    /**
     * Notify the reception of a management frame addressed to us.
     *
     * \param mpdu the received MPDU
     * \param linkId the ID of the link over which the MPDU was received
     */
    void NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the reception of an initial Control frame on the given link.
     *
     * \param linkId the ID of the link on which the initial Control frame was received
     */
    void NotifyIcfReceived(uint8_t linkId);

    /**
     * Notify the start of an UL TXOP on the given link
     *
     * \param linkId the ID of the given link
     */
    void NotifyUlTxopStart(uint8_t linkId);

    /**
     * Notify the end of a TXOP on the given link.
     *
     * \param linkId the ID of the given link
     * \param ulTxopNotStarted whether this is a notification of the end of an UL TXOP that did
     *                      not even start (no frame transmitted)
     * \param ongoingDlTxop whether a DL TXOP is ongoing on the given link (if true, this is
     *                      a notification of the end of an UL TXOP)
     */
    void NotifyTxopEnd(uint8_t linkId, bool ulTxopNotStarted = false, bool ongoingDlTxop = false);

    /**
     * Check whether the MediumSyncDelay timer is running for the STA operating on the given link.
     * If so, returns the time elapsed since the timer started.
     *
     * \param linkId the ID of the given link
     * \return the time elapsed since the MediumSyncDelay timer started, if this timer is running
     *         for the STA operating on the given link
     */
    std::optional<Time> GetElapsedMediumSyncDelayTimer(uint8_t linkId) const;

    /**
     * Cancel the MediumSyncDelay timer associated with the given link and take the appropriate
     * actions. This function must not be called when the MediumSyncDelay timer is not running
     * on the given link.
     *
     * \param linkId the ID of the link associated with the MediumSyncDelay timer to cancel
     */
    void CancelMediumSyncDelayTimer(uint8_t linkId);

    /**
     * Decrement the counter indicating the number of TXOP attempts left while the MediumSyncDelay
     * timer is running. This function must not be called when the MediumSyncDelay timer is not
     * running on the given link.
     *
     * \param linkId the ID of the link on which a new TXOP attempt may be carried out
     */
    void DecrementMediumSyncDelayNTxops(uint8_t linkId);

    /**
     * Reset the counter indicating the number of TXOP attempts left while the MediumSyncDelay
     * timer is running, so as to remove the limit on the number of attempts that can be made
     * while the MediumSyncDelay timer is running. This function is normally called when a TXOP
     * attempt is successful. This function must not be called when the MediumSyncDelay timer is
     * not running on the given link.
     *
     * \param linkId the ID of the link for which the counter of the TXOP attempts is reset
     */
    void ResetMediumSyncDelayNTxops(uint8_t linkId);

    /**
     * Return whether no more TXOP attempt is allowed on the given link. This function must not
     * be called when the MediumSyncDelay timer is not running on the given link.
     *
     * \param linkId the ID of the link on which a new TXOP attempt may be carried out
     * \return whether no more TXOP attempt on the given link is allowed
     */
    bool MediumSyncDelayNTxopsExceeded(uint8_t linkId);

  protected:
    void DoDispose() override;

    /**
     * Allow subclasses to take actions when the MAC is set.
     *
     * \param mac the wifi MAC
     */
    virtual void DoSetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * \return the MAC of the non-AP MLD managed by this EMLSR Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * \param linkId the ID of the given link
     * \return the EHT FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<EhtFrameExchangeManager> GetEhtFem(uint8_t linkId) const;

    /**
     * \return the ID of the link on which the EML Operating Mode Notification frame has to be sent
     */
    virtual uint8_t GetLinkToSendEmlOmn() = 0;

    /**
     * A previous EML Operating Mode Notification frame was dropped. Ask the subclass whether
     * the frame needs to be re-sent on the given link (if any).
     *
     * \param mpdu the dropped MPDU that includes the EML Operating Mode Notification frame
     * \return the ID of the link over which to re-send the frame, if needed
     */
    virtual std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) = 0;

    /**
     * \param linkId the ID of the given link
     * \return the operating channel the main PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForMainPhy(uint8_t linkId) const;

    /**
     * \param linkId the ID of the given link
     * \return the operating channel an aux PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForAuxPhy(uint8_t linkId) const;

    /**
     * Switch channel on the Main PHY so that it operates on the given link.
     *
     * \param linkId the ID of the link on which the main PHY has to operate
     * \param noSwitchDelay whether switching delay should be zero
     * \param resetBackoff whether backoff should be reset on the link on which the main PHY
     *                     is operating
     * \param requestAccess whether channel access should be requested on the link on which the
     *                      main PHY is moving onto
     */
    void SwitchMainPhy(uint8_t linkId, bool noSwitchDelay, bool resetBackoff, bool requestAccess);

    static constexpr bool RESET_BACKOFF = true;       //!< reset backoff on main PHY switch
    static constexpr bool DONT_RESET_BACKOFF = false; //!< do not reset backoff on main PHY switch
    static constexpr bool REQUEST_ACCESS = true; //!< request channel access when PHY switch ends
    static constexpr bool DONT_REQUEST_ACCESS =
        false; //!< do not request channel access when PHY switch ends

    /**
     * Switch channel on the Aux PHY operating on the given current link so that it operates
     * on the given next link.
     *
     * \param auxPhy the Aux PHY
     * \param currLinkId the ID of the link on which the aux PHY is currently operating
     * \param nextLinkId the ID of the link on which the aux PHY will be operating
     */
    void SwitchAuxPhy(Ptr<WifiPhy> auxPhy, uint8_t currLinkId, uint8_t nextLinkId);

    /**
     * Set the CCA ED threshold (if needed) on the given PHY that is switching channel to
     * operate on the given link.
     *
     * \param phy the given PHY
     * \param linkId the ID of the given link
     */
    void SetCcaEdThresholdOnLinkSwitch(Ptr<WifiPhy> phy, uint8_t linkId);

    /**
     * \return the EML Operating Mode Notification to send
     */
    MgtEmlOmn GetEmlOmn();

    /**
     * Subclasses have to provide an implementation for this method, that is called by the base
     * class when the EMLSR client gets channel access on the given link. This method has to
     * check possible reasons to give up the TXOP that apply to both main PHY and aux PHYs.
     *
     * \param linkId the ID of the given link
     * \return a pair consisting of a boolean value indicating whether the UL TXOP can be started
     *         and a Time value indicating the delay after which the EMLSR client must restart
     *         channel access (if needed) in case the UL TXOP is not started
     */
    virtual std::pair<bool, Time> DoGetDelayUntilAccessRequest(uint8_t linkId) = 0;

    /**
     * Subclasses have to provide an implementation for this method, that is called by the base
     * class when the given AC of the EMLSR client gets channel access on the given link, on which
     * an aux PHY that is not TX capable is operating. This method has to request the main PHY to
     * switch to the given link to take over the TXOP, unless it is decided to give up the TXOP.
     *
     * \param linkId the ID of the given link
     * \param aci the index of the given AC
     */
    virtual void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) = 0;

    /**
     * Subclasses have to provide an implementation for this method, that is called by the base
     * class when the EMLSR client gets channel access on the given link, on which an aux PHY that
     * is TX capable is operating. This method has to request the main PHY to switch to the
     * given link to take over the TXOP, if possible, or determine the delay after which the
     * EMLSR client restarts channel access on the given link, otherwise.
     *
     * \param linkId the ID of the given link
     * \return a pair consisting of a boolean value indicating whether the UL TXOP can be started
     *         and a Time value indicating the delay after which the EMLSR client must restart
     *         channel access (if needed) in case the UL TXOP is not started
     */
    virtual std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) = 0;

    Time m_emlsrPaddingDelay;    //!< EMLSR Padding delay
    Time m_emlsrTransitionDelay; //!< EMLSR Transition delay
    uint8_t m_mainPhyId; //!< ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
    MHz_u m_auxPhyMaxWidth;                  //!< max channel width supported by aux PHYs
    WifiModulationClass m_auxPhyMaxModClass; //!< max modulation class supported by aux PHYs
    bool m_auxPhyTxCapable;                  //!< whether Aux PHYs are capable of transmitting PPDUs
    std::map<uint8_t, EventId> m_ulMainPhySwitch; //!< link ID-indexed map of timers started when
                                                  //!< an aux PHY gains an UL TXOP and schedules
                                                  //!< a channel switch for the main PHY

  private:
    /**
     * Set the ID of main PHY (position in the vector of PHYs held by WifiNetDevice). This
     * method cannot be called during or after initialization.
     *
     * \param mainPhyId the ID of the main PHY
     */
    void SetMainPhyId(uint8_t mainPhyId);

    /**
     * Compute the operating channels that the main PHY and the aux PHY(s) must switch to in order
     * to operate on each of the setup links. The operating channels may be different due to
     * limited channel width capabilities of the aux PHY(s). This method shall be called upon
     * completion of ML setup.
     */
    void ComputeOperatingChannels();

    /**
     * Send an EML Operating Mode Notification frame.
     */
    void SendEmlOmn();

    /**
     * Start the MediumSyncDelay timer and take the appropriate actions, if the timer is not
     * already running.
     *
     * \param linkId the ID of the link on which a TXOP was carried out that caused the STAs
     *               operating on other links to lose medium synchronization
     */
    void StartMediumSyncDelayTimer(uint8_t linkId);

    /**
     * Take the appropriate actions when the MediumSyncDelay timer expires or is cancelled.
     *
     * \param linkId the ID of the link associated with the MediumSyncDelay timer to cancel
     */
    void MediumSyncDelayTimerExpired(uint8_t linkId);

    /**
     * Notify the subclass of the reception of a management frame addressed to us.
     *
     * \param mpdu the received MPDU
     * \param linkId the ID of the link over which the MPDU was received
     */
    virtual void DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId) = 0;

    /**
     * Notify the subclass of the reception of an initial Control frame on the given link.
     *
     * \param linkId the ID of the link on which the initial Control frame was received
     */
    virtual void DoNotifyIcfReceived(uint8_t linkId) = 0;

    /**
     * Notify the subclass of the start of an UL TXOP on the given link
     *
     * \param linkId the ID of the given link
     */
    virtual void DoNotifyUlTxopStart(uint8_t linkId) = 0;

    /**
     * Notify the subclass of the end of a TXOP on the given link.
     *
     * \param linkId the ID of the given link
     */
    virtual void DoNotifyTxopEnd(uint8_t linkId) = 0;

    /**
     * Notify the acknowledgment of the given MPDU.
     *
     * \param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);

    /**
     * Notify that the given MPDU has been discarded for the given reason.
     *
     * \param reason the reason why the MPDU was dropped
     * \param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /**
     * This method is called to make an EMLSR mode change effective after the transition
     * delay has elapsed or a notification response has been received from the AP.
     */
    void ChangeEmlsrMode();

    /**
     * Adjust the operating channel of all the aux PHYs to meet the constraint on the maximum
     * channel width supported by aux PHYs.
     */
    void ApplyMaxChannelWidthAndModClassOnAuxPhys();

    /**
     * Notify subclass that EMLSR mode changed.
     */
    virtual void NotifyEmlsrModeChanged() = 0;

    /**
     * Notify subclass that the main PHY is switching channel to operate on another link.
     *
     * \param currLinkId the ID of the link on which the main PHY is operating (if any)
     * \param nextLinkId the ID of the link on which the main PHY will be operating
     * \param duration the channel switch duration
     */
    virtual void NotifyMainPhySwitch(std::optional<uint8_t> currLinkId,
                                     uint8_t nextLinkId,
                                     Time duration) = 0;

    /**
     * Information about the status of the MediumSyncDelay timer associated with a link.
     */
    struct MediumSyncDelayStatus
    {
        EventId timer;                        //!< the MediumSyncDelay timer
        std::optional<uint8_t> msdNTxopsLeft; //!< number of TXOP attempts left while the
                                              //!< MediumSyncDelay timer is running
    };

    Ptr<StaWifiMac> m_staMac;                     //!< the MAC of the managed non-AP MLD
    std::optional<Time> m_emlsrTransitionTimeout; /**< Transition timeout advertised by APs with
                                                       EMLSR activated */
    Time m_mediumSyncDuration;                    //!< duration of the MediumSyncDelay timer
    int8_t m_msdOfdmEdThreshold;                  //!< MediumSyncDelay OFDM ED threshold
    std::optional<uint8_t> m_msdMaxNTxops;        //!< MediumSyncDelay max number of TXOPs

    std::map<uint8_t, MediumSyncDelayStatus>
        m_mediumSyncDelayStatus; //!< the status of MediumSyncDelay timers (link ID-indexed)
    std::map<Ptr<WifiPhy>, dBm_u> m_prevCcaEdThreshold; //!< the CCA sensitivity threshold
                                                        //!< to restore once the MediumSyncDelay
                                                        //!< timer expires or the PHY moves to a
                                                        //!< link on which the timer is not running

    std::set<uint8_t> m_emlsrLinks; //!< ID of the EMLSR links (empty if EMLSR mode is disabled)
    std::optional<std::set<uint8_t>> m_nextEmlsrLinks; /**< ID of the links that will become the
                                                            EMLSR links when the pending
                                                            notification frame is acknowledged */
    Time m_lastAdvPaddingDelay;                        //!< last advertised padding delay
    Time m_lastAdvTransitionDelay;                     //!< last advertised transition delay
    EventId m_transitionTimeoutEvent; /**< Timer started after the successful transmission of an
                                           EML Operating Mode Notification frame */
    bool m_resetCamState; //!< whether to reset the state of CAM when main PHY switches channel
    bool m_inDeviceInterference; //!< whether in-device interference is such that a PHY cannot
                                 //!< decode anything  and cannot decrease the backoff counter
                                 //!< when another PHY is transmitting
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_mainPhyChannels; //!< link ID-indexed map of operating channels for the main PHY
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_auxPhyChannels; //!< link ID-indexed map of operating channels for the aux PHYs
};

} // namespace ns3

#endif /* EMLSR_MANAGER_H */
