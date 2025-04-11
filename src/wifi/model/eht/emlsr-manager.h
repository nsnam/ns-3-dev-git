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
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <utility>

class EmlsrCcaBusyTest;

namespace ns3
{

class EhtFrameExchangeManager;
class MgtEmlOmn;
class WifiMpdu;

/**
 * @ingroup wifi
 * Base struct for EMLSR Main PHY switch traces.
 */
struct EmlsrMainPhySwitchTrace
{
    virtual ~EmlsrMainPhySwitchTrace() = default;

    /// @return the name of this instance
    virtual std::string_view GetName() const = 0;

    /// @return a pointer to the clone of this object
    virtual std::shared_ptr<EmlsrMainPhySwitchTrace> Clone() const = 0;

    std::optional<uint8_t> fromLinkId; //!< ID of the link the main PHY is moving from (if any)
    uint8_t toLinkId{WIFI_LINKID_UNDEFINED}; //!< ID of the link the main PHY is moving to
};

/**
 * Implementation for the EMLSR Main PHY switch trace base struct. Child structs are inherited
 * from this implementation according to the CRTP idiom and must define a static string_view
 * member containing the name of the child.
 */
template <class T>
struct EmlsrMainPhySwitchTraceImpl : public EmlsrMainPhySwitchTrace
{
    /// @copydoc ns3::EmlsrMainPhySwitchTrace::GetName
    std::string_view GetName() const override
    {
        return T::m_name;
    }

    /// @copydoc ns3::EmlsrMainPhySwitchTrace::Clone
    std::shared_ptr<EmlsrMainPhySwitchTrace> Clone() const override
    {
        return std::shared_ptr<EmlsrMainPhySwitchTrace>(new T(static_cast<const T&>(*this)));
    }
};

/**
 * @ingroup wifi
 *
 * EmlsrManager is an abstract base class defining the API that EHT non-AP MLDs
 * with EMLSR activated can use to handle the operations on the EMLSR links
 */
class EmlsrManager : public Object
{
    /// Allow test cases to access private members
    friend class ::EmlsrCcaBusyTest;
    friend class WifiStaticSetupHelper;

  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    EmlsrManager();
    ~EmlsrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an EHT non-AP MLD.
     *
     * @param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Set the Transition Timeout advertised by the associated AP with EMLSR activated.
     *
     * @param timeout the advertised Transition Timeout
     */
    void SetTransitionTimeout(Time timeout);

    /**
     * @return the Transition Timeout, if advertised by the associated AP
     */
    std::optional<Time> GetTransitionTimeout() const;

    /**
     * Set the duration of the MediumSyncDelay timer.
     *
     * @param duration the duration of the MediumSyncDelay timer
     */
    void SetMediumSyncDuration(Time duration);

    /**
     * @return the duration of the MediumSyncDelay timer
     */
    Time GetMediumSyncDuration() const;

    /**
     * Set the Medium Synchronization OFDM ED threshold (dBm) to use while the MediumSyncDelay
     * timer is running.
     *
     * @param threshold the threshold in dBm (ranges from -72 to -62 dBm)
     */
    void SetMediumSyncOfdmEdThreshold(int8_t threshold);

    /**
     * @return the Medium Synchronization OFDM ED threshold (dBm) to use while the MediumSyncDelay
     * timer is running.
     */
    int8_t GetMediumSyncOfdmEdThreshold() const;

    /**
     * Set the maximum number of TXOPs a non-AP STA is allowed to attempt to initiate while
     * the MediumSyncDelay timer is running. No value indicates no limit.
     *
     * @param nTxops the maximum number of TXOPs a non-AP STA is allowed to attempt to
     *               initiate while the MediumSyncDelay timer is running
     */
    void SetMediumSyncMaxNTxops(std::optional<uint8_t> nTxops);

    /**
     * @return the maximum number of TXOPs a non-AP STA is allowed to attempt to initiate while
     * the MediumSyncDelay timer is running. No value indicates no limit.
     */
    std::optional<uint8_t> GetMediumSyncMaxNTxops() const;

    /**
     * @return the ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
     */
    uint8_t GetMainPhyId() const;

    /**
     * Take actions to enable EMLSR mode on the given set of links, if non-empty, or
     * disable EMLSR mode, otherwise.
     *
     * @param linkIds the IDs of the links on which EMLSR mode should be enabled
     *                (empty to disable EMLSR mode)
     */
    void SetEmlsrLinks(const std::set<uint8_t>& linkIds);

    /**
     * @return the set of links on which EMLSR mode is enabled
     */
    const std::set<uint8_t>& GetEmlsrLinks() const;

    /**
     * Set the member variable indicating whether the state of the CAM should be reset when
     * the main PHY switches channel and operates on the link associated with the CAM.
     *
     * @param enable whether the CAM state should be reset
     */
    void SetCamStateReset(bool enable);

    /**
     * @return the value of the member variable indicating whether the state of the CAM should be
     * reset when the main PHY switches channel and operates on the link associated with the CAM.
     */
    bool GetCamStateReset() const;

    /**
     * Notify that an UL TXOP is gained on the given link by the given AC. This method has to
     * determine whether to start the UL TXOP or release the channel.
     *
     * @param linkId the ID of the given link
     * @param aci the index of the given AC
     * @return a pair consisting of a boolean value indicating whether the UL TXOP can be started
     *         and a Time value indicating the delay after which the EMLSR client must restart
     *         channel access (if needed) in case the UL TXOP is not started
     */
    std::pair<bool, Time> GetDelayUntilAccessRequest(uint8_t linkId, AcIndex aci);

    /**
     * Set the member variable indicating whether Aux PHYs are capable of transmitting PPDUs.
     *
     * @param capable whether Aux PHYs are capable of transmitting PPDUs
     */
    void SetAuxPhyTxCapable(bool capable);

    /**
     * @return whether Aux PHYs are capable of transmitting PPDUs
     */
    bool GetAuxPhyTxCapable() const;

    /**
     * Set the member variable indicating whether in-device interference is such that a PHY cannot
     * decode anything and cannot decrease the backoff counter when another PHY is transmitting.
     *
     * @param enable whether in-device interference is such that a PHY cannot decode anything
     *               and cannot decrease the backoff counter when another PHY is transmitting
     */
    void SetInDeviceInterference(bool enable);

    /**
     * @return whether in-device interference is such that a PHY cannot decode anything and cannot
     *         decrease the backoff counter when another PHY is transmitting
     */
    bool GetInDeviceInterference() const;

    /**
     * Notify the reception of a management frame addressed to us.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the link over which the MPDU was received
     */
    void NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the start of a DL TXOP on the given link.
     *
     * @param linkId the ID of the given link
     */
    void NotifyDlTxopStart(uint8_t linkId);

    /**
     * Notify the start of an UL TXOP on the given link
     *
     * @param linkId the ID of the given link
     */
    void NotifyUlTxopStart(uint8_t linkId);

    /**
     * Notify that RTS transmission is starting on the given link.
     *
     * @param linkId the ID of the given link
     * @param rts the RTS being transmitted
     * @param txVector the TXVECTOR used to transmit the RTS
     */
    virtual void NotifyRtsSent(uint8_t linkId,
                               Ptr<const WifiPsdu> rts,
                               const WifiTxVector& txVector);

    /**
     * Notify that protection (if required) is completed and data frame exchange can start
     * on the given link.
     *
     * @param linkId the ID of the given link
     */
    void NotifyProtectionCompleted(uint8_t linkId);

    /**
     * Notify the end of a TXOP on the given link.
     *
     * @param linkId the ID of the given link
     * @param edca the EDCAF that carried out the TXOP, in case of UL TXOP, or a null pointer,
     *             in case of DL TXOP
     */
    void NotifyTxopEnd(uint8_t linkId, Ptr<QosTxop> edca = nullptr);

    /**
     * Notify that an STA affiliated with the EMLSR client is causing in-device interference
     * for the given amount of time.
     *
     * @param linkId the ID of the link on which the STA is operating
     * @param duration the duration of the in-device interference
     */
    virtual void NotifyInDeviceInterferenceStart(uint8_t linkId, Time duration);

    /**
     * Check whether the MediumSyncDelay timer is running for the STA operating on the given link.
     * If so, returns the time elapsed since the timer started.
     *
     * @param linkId the ID of the given link
     * @return the time elapsed since the MediumSyncDelay timer started, if this timer is running
     *         for the STA operating on the given link
     */
    std::optional<Time> GetElapsedMediumSyncDelayTimer(uint8_t linkId) const;

    /**
     * Cancel the MediumSyncDelay timer associated with the given link and take the appropriate
     * actions. This function must not be called when the MediumSyncDelay timer is not running
     * on the given link.
     *
     * @param linkId the ID of the link associated with the MediumSyncDelay timer to cancel
     */
    void CancelMediumSyncDelayTimer(uint8_t linkId);

    /**
     * Decrement the counter indicating the number of TXOP attempts left while the MediumSyncDelay
     * timer is running. This function must not be called when the MediumSyncDelay timer is not
     * running on the given link.
     *
     * @param linkId the ID of the link on which a new TXOP attempt may be carried out
     */
    void DecrementMediumSyncDelayNTxops(uint8_t linkId);

    /**
     * Reset the counter indicating the number of TXOP attempts left while the MediumSyncDelay
     * timer is running, so as to remove the limit on the number of attempts that can be made
     * while the MediumSyncDelay timer is running. This function is normally called when a TXOP
     * attempt is successful. This function must not be called when the MediumSyncDelay timer is
     * not running on the given link.
     *
     * @param linkId the ID of the link for which the counter of the TXOP attempts is reset
     */
    void ResetMediumSyncDelayNTxops(uint8_t linkId);

    /**
     * Return whether no more TXOP attempt is allowed on the given link. This function must not
     * be called when the MediumSyncDelay timer is not running on the given link.
     *
     * @param linkId the ID of the link on which a new TXOP attempt may be carried out
     * @return whether no more TXOP attempt on the given link is allowed
     */
    bool MediumSyncDelayNTxopsExceeded(uint8_t linkId);

    /**
     * Check whether a PPDU that may be an ICF is being received on the given link. If so, return
     * true along with the time to wait to know more information about the PPDU being received.
     * Otherwise, return false.
     *
     * @param linkId the ID of the given link
     * @return a pair indicating whether a PPDU that may be an ICF is being received on the link
     */
    std::pair<bool, Time> CheckPossiblyReceivingIcf(uint8_t linkId) const;

    /**
     * This method is called when an aux PHY has completed reception of an ICF to determine whether
     * there is any reason preventing the main PHY from switching link and be on the aux PHY link
     * by the time the ICF reception is completed to take over the DL TXOP.
     *
     * @param linkId the ID of the link on which ICF has been received
     * @return the reason, if any, preventing the main PHY from switching link
     */
    virtual std::optional<WifiIcfDrop> CheckMainPhyTakesOverDlTxop(uint8_t linkId) const;

  protected:
    void DoDispose() override;

    /**
     * Allow subclasses to take actions when the MAC is set.
     *
     * @param mac the wifi MAC
     */
    virtual void DoSetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * @return the MAC of the non-AP MLD managed by this EMLSR Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * @param linkId the ID of the given link
     * @return the EHT FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<EhtFrameExchangeManager> GetEhtFem(uint8_t linkId) const;

    /**
     * @return the ID of the link on which the EML Operating Mode Notification frame has to be sent
     */
    virtual uint8_t GetLinkToSendEmlOmn() = 0;

    /**
     * A previous EML Operating Mode Notification frame was dropped. Ask the subclass whether
     * the frame needs to be re-sent on the given link (if any).
     *
     * @param mpdu the dropped MPDU that includes the EML Operating Mode Notification frame
     * @return the ID of the link over which to re-send the frame, if needed
     */
    virtual std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) = 0;

    /**
     * @param linkId the ID of the given link
     * @return the operating channel the main PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForMainPhy(uint8_t linkId) const;

    /**
     * @param linkId the ID of the given link
     * @return the operating channel an aux PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForAuxPhy(uint8_t linkId) const;

    /**
     * Switch channel on the Main PHY so that it operates on the given link.
     *
     * @param linkId the ID of the link on which the main PHY has to operate
     * @param noSwitchDelay whether switching delay should be zero
     * @param requestAccess whether channel access should be requested on the link on which the
     *                      main PHY is moving onto
     * @param traceInfo information to pass to the main PHY switch traced callback (the fromLinkId
     *                  and toLinkId fields are set by this function)
     */
    void SwitchMainPhy(uint8_t linkId,
                       bool noSwitchDelay,
                       bool requestAccess,
                       EmlsrMainPhySwitchTrace&& traceInfo);

    static constexpr bool REQUEST_ACCESS = true; //!< request channel access when PHY switch ends
    static constexpr bool DONT_REQUEST_ACCESS =
        false; //!< do not request channel access when PHY switch ends

    /**
     * Switch channel on the Aux PHY operating on the given current link so that it operates
     * on the given next link.
     *
     * @param auxPhy the Aux PHY
     * @param currLinkId the ID of the link on which the aux PHY is currently operating
     * @param nextLinkId the ID of the link on which the aux PHY will be operating
     */
    void SwitchAuxPhy(Ptr<WifiPhy> auxPhy, uint8_t currLinkId, uint8_t nextLinkId);

    /**
     * Callback connected to the EmlsrLinkSwitch trace source of StaWifiMac.
     *
     * @param linkId the ID of the link which the PHY is connected to/disconnected from
     * @param phy a pointer to the PHY that is connected to/disconnected from the given link
     * @param connected true if the PHY is connected, false if the PHY is disconnected
     */
    virtual void EmlsrLinkSwitchCallback(uint8_t linkId, Ptr<WifiPhy> phy, bool connected);

    /**
     * Set the CCA ED threshold (if needed) on the given PHY that is switching channel to
     * operate on the given link.
     *
     * @param phy the given PHY
     * @param linkId the ID of the given link
     */
    void SetCcaEdThresholdOnLinkSwitch(Ptr<WifiPhy> phy, uint8_t linkId);

    /**
     * @return the EML Operating Mode Notification to send
     */
    MgtEmlOmn GetEmlOmn();

    /**
     * Subclasses have to provide an implementation for this method, that is called by the base
     * class when the EMLSR client gets channel access on the given link. This method has to
     * check possible reasons to give up the TXOP that apply to both main PHY and aux PHYs.
     *
     * @param linkId the ID of the given link
     * @return a pair consisting of a boolean value indicating whether the UL TXOP can be started
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
     * @param linkId the ID of the given link
     * @param aci the index of the given AC
     */
    virtual void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) = 0;

    /**
     * Subclasses have to provide an implementation for this method, that is called by the base
     * class when the EMLSR client gets channel access on the given link, on which an aux PHY that
     * is TX capable is operating. This method has to request the main PHY to switch to the
     * given link to take over the TXOP, if possible, or determine the delay after which the
     * EMLSR client restarts channel access on the given link, otherwise.
     *
     * @param linkId the ID of the given link
     * @return a pair consisting of a boolean value indicating whether the UL TXOP can be started
     *         and a Time value indicating the delay after which the EMLSR client must restart
     *         channel access (if needed) in case the UL TXOP is not started
     */
    virtual std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) = 0;

    /**
     * Set sleep state or awake state for all aux PHYs.
     *
     * @param sleep set sleep state, if true, or awake state, otherwise
     */
    void SetSleepStateForAllAuxPhys(bool sleep);

    /**
     * Cancel all pending events to put aux PHYs into sleep/awake state.
     */
    void CancelAllSleepEvents();

    /// Store information about a main PHY switch.
    struct MainPhySwitchInfo
    {
        Time start;               //!< start of channel switching
        bool disconnected{false}; //!< true if the main PHY is not connected to any link, i.e., it
                                  //!< is switching or waiting to be connected to a link
        uint8_t from{};           //!< ID of the link which the main PHY is/has been leaving
        uint8_t to{};             //!< ID of the link which the main PHY is moving to
        std::string reason;       //!< the reason for switching the main PHY
    };

    Time m_emlsrPaddingDelay;    //!< EMLSR Padding delay
    Time m_emlsrTransitionDelay; //!< EMLSR Transition delay
    uint8_t m_mainPhyId; //!< ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
    MHz_u m_auxPhyMaxWidth;                  //!< max channel width supported by aux PHYs
    WifiModulationClass m_auxPhyMaxModClass; //!< max modulation class supported by aux PHYs
    bool m_auxPhyTxCapable;                  //!< whether Aux PHYs are capable of transmitting PPDUs
    bool m_auxPhyToSleep;     //!< whether Aux PHYs should be put into sleep mode while the Main PHY
                              //!< is carrying out a (DL or UL) TXOP
    bool m_useNotifiedMacHdr; //!< whether to use the information about the MAC header of
                              //!< the MPDU being received (if notified by the PHY)
    std::map<uint8_t, EventId> m_auxPhyToSleepEvents; //!< PHY ID-indexed map of events scheduled to
                                                      //!< put an Aux PHY to sleep
    std::map<uint8_t, Time> m_startSleep; //!< PHY ID-indexed map of last time sleep mode started
    std::map<uint8_t, EventId> m_ulMainPhySwitch; //!< link ID-indexed map of timers started when
                                                  //!< an aux PHY gains an UL TXOP and schedules
                                                  //!< a channel switch for the main PHY
    MainPhySwitchInfo m_mainPhySwitchInfo;        //!< main PHY switch info

  private:
    /**
     * Set the ID of main PHY (position in the vector of PHYs held by WifiNetDevice). This
     * method cannot be called during or after initialization.
     *
     * @param mainPhyId the ID of the main PHY
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
     * Start the MediumSyncDelay timer and take the appropriate actions.
     *
     * @param linkId the ID of the link on which medium synchronization was lost
     */
    void StartMediumSyncDelayTimer(uint8_t linkId);

    /**
     * Take the appropriate actions when the MediumSyncDelay timer expires or is cancelled.
     *
     * @param linkId the ID of the link associated with the MediumSyncDelay timer to cancel
     */
    void MediumSyncDelayTimerExpired(uint8_t linkId);

    /**
     * Notify the subclass of the reception of a management frame addressed to us.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the link over which the MPDU was received
     */
    virtual void DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId) = 0;

    /**
     * Notify the subclass of the reception of an initial Control frame on the given link.
     *
     * @param linkId the ID of the link on which the initial Control frame was received
     */
    virtual void DoNotifyDlTxopStart(uint8_t linkId) = 0;

    /**
     * Notify the subclass of the start of an UL TXOP on the given link
     *
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyUlTxopStart(uint8_t linkId) = 0;

    /**
     * Notify the subclass that protection (if required) is completed and data frame exchange can
     * start on the given link.
     *
     * @param linkId the ID of the given link
     */
    virtual void DoNotifyProtectionCompleted(uint8_t linkId) = 0;

    /**
     * Notify the subclass of the end of a TXOP on the given link.
     *
     * @param linkId the ID of the given link
     * @param edca the EDCAF that carried out the TXOP, in case of UL TXOP, or a null pointer,
     *             in case of DL TXOP
     */
    virtual void DoNotifyTxopEnd(uint8_t linkId, Ptr<QosTxop> edca) = 0;

    /**
     * Notify the acknowledgment of the given MPDU.
     *
     * @param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);

    /**
     * Notify that the given MPDU has been discarded for the given reason.
     *
     * @param reason the reason why the MPDU was dropped
     * @param mpdu the dropped MPDU
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
     * @param currLinkId the ID of the link on which the main PHY is operating (if any)
     * @param nextLinkId the ID of the link on which the main PHY will be operating
     * @param auxPhy the aux PHY operating on the link on which the main PHY will be operating
     * @param duration the channel switch duration
     */
    virtual void NotifyMainPhySwitch(std::optional<uint8_t> currLinkId,
                                     uint8_t nextLinkId,
                                     Ptr<WifiPhy> auxPhy,
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

    /**
     * TracedCallback signature for main PHY switch events.
     *
     * @param info the information associated with the main PHY switch event
     */
    typedef void (*MainPhySwitchCallback)(const EmlsrMainPhySwitchTrace& info);

    /// TracedCallback for main PHY switch events typedef
    using MainPhySwitchTracedCallback = TracedCallback<const EmlsrMainPhySwitchTrace&>;

    MainPhySwitchTracedCallback m_mainPhySwitchTrace; //!< main PHY switch trace source

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
    std::map<uint8_t, Time>
        m_noPhySince; //!< link ID-indexed map of the time since no PHY is operating on the link
};

/**
 * Struct to trace that main PHY switched to start a DL TXOP after that an aux PHY received an ICF.
 */
struct EmlsrDlTxopIcfReceivedByAuxPhyTrace
    : public EmlsrMainPhySwitchTraceImpl<EmlsrDlTxopIcfReceivedByAuxPhyTrace>
{
    static constexpr std::string_view m_name = "DlTxopIcfReceivedByAuxPhy"; //!< trace name
};

/**
 * Struct to trace that main PHY switched to start an UL TXOP after that an aux PHY transmitted an
 * RTS.
 */
struct EmlsrUlTxopRtsSentByAuxPhyTrace
    : public EmlsrMainPhySwitchTraceImpl<EmlsrUlTxopRtsSentByAuxPhyTrace>
{
    static constexpr std::string_view m_name = "UlTxopRtsSentByAuxPhy"; //!< trace name
};

/**
 * Struct to trace that main PHY switched when a (DL or UL) TXOP ended.
 *
 * This trace is called when aux PHYs do not switch link and the main PHY switches back to the
 * preferred link when a TXOP carried out on another link ends.
 */
struct EmlsrTxopEndedTrace : public EmlsrMainPhySwitchTraceImpl<EmlsrTxopEndedTrace>
{
    static constexpr std::string_view m_name = "TxopEnded"; //!< trace name
};

/**
 * Struct to trace that main PHY started switching after a CTS timeout occurred on the link on
 * which an RTS was transmitted to start an UL TXOP. Provides the time elapsed since the
 * CTS timeout occurred.
 *
 *                                |-- main PHY switch --|
 *                                |----- to link 1 -----|
 *                                          ┌───────────┐
 *                                          │    CTS    │
 *  ────────────────────────┬───────────┬───┴X──────────┴─────────────────────────────
 *  [link 1]                │    RTS    │     │-elapsed-│
 *                          └───────────┘     │-- time--|-- main PHY switch --|
 *                                            │         |- to preferred link -|
 *                                        CTS timeout
 *
 * Normally, this trace is called when aux PHYs do not switch links, because the main PHY has to
 * return to the preferred link upon CTS timeout, because a TXOP did not start. In some cases, the
 * main PHY may be switching when CTS timeout occurs; this happens when an aux PHY that is TX
 * capable transmits an RTS and the main PHY starts switching to the aux PHY link (the start time
 * of the main PHY switch is computed such that the main PHY switch ends slightly after the
 * reception of the CTS). In such a case, the main PHY completes the current link switch and then
 * it starts switching to return back to the preferred link.
 *
 * Note that the Advanced EMLSR manager may allow a main PHY switch to be interrupted. If this
 * option is enabled and the main PHY is switching when CTS timeout occurs, the previous switch is
 * interrupted and the main PHY starts switching to the previous link (in this case, the time
 * elapsed since the CTS timeout occurred is zero). This holds true for both the case aux PHYs do
 * not switch link and the case aux PHYs switch link.
 */
struct EmlsrCtsAfterRtsTimeoutTrace
    : public EmlsrMainPhySwitchTraceImpl<EmlsrCtsAfterRtsTimeoutTrace>
{
    static constexpr std::string_view m_name = "CtsAfterRtsTimeout"; //!< trace name

    Time sinceCtsTimeout; //!< time elapsed since CTS timeout occurred

    /**
     * Constructor provided because this struct is not an aggregate (it has a base struct), hence
     * we cannot use designated initializers.
     *
     * @param elapsed the value for the sinceCtsTimeout field
     */
    EmlsrCtsAfterRtsTimeoutTrace(const Time& elapsed)
        : sinceCtsTimeout(elapsed)
    {
    }
};

/**
 * Struct to trace that main PHY switched to operate on a link on which an aux PHY that is not
 * TX capable has gained or is expected to shortly gain a TXOP.
 */
struct EmlsrUlTxopAuxPhyNotTxCapableTrace
    : public EmlsrMainPhySwitchTraceImpl<EmlsrUlTxopAuxPhyNotTxCapableTrace>
{
    static constexpr std::string_view m_name = "UlTxopAuxPhyNotTxCapable"; //!< trace name

    AcIndex acIndex; //!< Access category of TXOP on aux PHY
    Time remTime;    //!< Remaining time to complete backoff countdown on the aux PHY link
    Time remNav;     //!< the remaining NAV on main PHY link when main PHY is requested to switch

    /**
     * Constructor provided because this struct is not an aggregate (it has a base struct), hence
     * we cannot use designated initializers.
     *
     * @param aci the value for the acIndex field
     * @param delay the value for the remTime field
     * @param navLeft the value for the remNav field
     */
    EmlsrUlTxopAuxPhyNotTxCapableTrace(AcIndex aci, const Time& delay, const Time& navLeft)
        : acIndex(aci),
          remTime(delay),
          remNav(navLeft)
    {
    }
};

} // namespace ns3

#endif /* EMLSR_MANAGER_H */
