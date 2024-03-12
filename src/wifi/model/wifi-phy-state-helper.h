/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef WIFI_PHY_STATE_HELPER_H
#define WIFI_PHY_STATE_HELPER_H

#include "phy-entity.h"
#include "wifi-phy-common.h"
#include "wifi-phy-state.h"
#include "wifi-ppdu.h"

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <algorithm>
#include <list>
#include <memory>
#include <vector>

namespace ns3
{

class WifiPhyListener;
class WifiTxVector;
class WifiMode;
class Packet;
class WifiPsdu;
struct RxSignalInfo;

/**
 * Callback if PSDU successfully received (i.e. if aggregate,
 * it means that at least one MPDU of the A-MPDU was received,
 * considering that the per-MPDU reception status is also provided).
 *
 * arg1: PSDU received successfully
 * arg2: info on the received signal (\see RxSignalInfo)
 * arg3: TXVECTOR of PSDU
 * arg4: vector of per-MPDU status of reception.
 */
typedef Callback<void, Ptr<const WifiPsdu>, RxSignalInfo, WifiTxVector, std::vector<bool>>
    RxOkCallback;
/**
 * Callback if PSDU unsuccessfuly received
 *
 * arg1: PSDU received unsuccessfuly
 */
typedef Callback<void, Ptr<const WifiPsdu>> RxErrorCallback;

/**
 * \ingroup wifi
 *
 * This objects implements the PHY state machine of the Wifi device.
 */
class WifiPhyStateHelper : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    WifiPhyStateHelper();

    /**
     * Set a callback for a successful reception.
     *
     * \param callback the RxOkCallback to set
     */
    void SetReceiveOkCallback(RxOkCallback callback);
    /**
     * Set a callback for a failed reception.
     *
     * \param callback the RxErrorCallback to set
     */
    void SetReceiveErrorCallback(RxErrorCallback callback);
    /**
     * Register WifiPhyListener to this WifiPhyStateHelper.
     *
     * \param listener the WifiPhyListener to register
     */
    void RegisterListener(const std::shared_ptr<WifiPhyListener>& listener);
    /**
     * Remove WifiPhyListener from this WifiPhyStateHelper.
     *
     * \param listener the WifiPhyListener to unregister
     */
    void UnregisterListener(const std::shared_ptr<WifiPhyListener>& listener);
    /**
     * Return the current state of WifiPhy.
     *
     * \return the current state of WifiPhy
     */
    WifiPhyState GetState() const;
    /**
     * Check whether the current state is CCA busy.
     *
     * \return true if the current state is CCA busy, false otherwise
     */
    bool IsStateCcaBusy() const;
    /**
     * Check whether the current state is IDLE.
     *
     * \return true if the current state is IDLE, false otherwise
     */
    bool IsStateIdle() const;
    /**
     * Check whether the current state is RX.
     *
     * \return true if the current state is RX, false otherwise
     */
    bool IsStateRx() const;
    /**
     * Check whether the current state is TX.
     *
     * \return true if the current state is TX, false otherwise
     */
    bool IsStateTx() const;
    /**
     * Check whether the current state is SWITCHING.
     *
     * \return true if the current state is SWITCHING, false otherwise
     */
    bool IsStateSwitching() const;
    /**
     * Check whether the current state is SLEEP.
     *
     * \return true if the current state is SLEEP, false otherwise
     */
    bool IsStateSleep() const;
    /**
     * Check whether the current state is OFF.
     *
     * \return true if the current state is OFF, false otherwise
     */
    bool IsStateOff() const;
    /**
     * Return the time before the state is back to IDLE.
     *
     * \return the delay before the state is back to IDLE
     */
    Time GetDelayUntilIdle() const;
    /**
     * Return the time the last RX start.
     *
     * \return the time the last RX start.
     */
    Time GetLastRxStartTime() const;
    /**
     * Return the time the last RX end.
     *
     * \return the time the last RX end.
     */
    Time GetLastRxEndTime() const;

    /**
     * \param states a set of PHY states
     * \return the last time the PHY has been in any of the given states
     */
    Time GetLastTime(std::initializer_list<WifiPhyState> states) const;

    /**
     * Switch state to TX for the given duration.
     *
     * \param txDuration the duration of the PPDU to transmit
     * \param psdus the PSDUs in the transmitted PPDU (only one unless it is a MU PPDU)
     * \param txPowerDbm the nominal TX power in dBm
     * \param txVector the TX vector for the transmission
     */
    void SwitchToTx(Time txDuration,
                    WifiConstPsduMap psdus,
                    double txPowerDbm,
                    const WifiTxVector& txVector);
    /**
     * Switch state to RX for the given duration.
     *
     * \param rxDuration the duration of the RX
     */
    void SwitchToRx(Time rxDuration);
    /**
     * Switch state to channel switching for the given duration.
     *
     * \param switchingDuration the duration of required to switch the channel
     */
    void SwitchToChannelSwitching(Time switchingDuration);
    /**
     * Notify the reception of an MPDU included in an A-MPDU.
     *
     * \param psdu the successfully received PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector TXVECTOR of the PSDU
     */
    void NotifyRxMpdu(Ptr<const WifiPsdu> psdu,
                      RxSignalInfo rxSignalInfo,
                      const WifiTxVector& txVector);
    /**
     * Handle the successful reception of a PSDU.
     *
     * \param psdu the successfully received PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector TXVECTOR of the PSDU
     * \param staId the station ID of the PSDU (only used for MU)
     * \param statusPerMpdu reception status per MPDU
     */
    void NotifyRxPsduSucceeded(Ptr<const WifiPsdu> psdu,
                               RxSignalInfo rxSignalInfo,
                               const WifiTxVector& txVector,
                               uint16_t staId,
                               const std::vector<bool>& statusPerMpdu);
    /**
     * Handle the unsuccessful reception of a PSDU.
     *
     * \param psdu the PSDU that we failed to received
     * \param snr the SNR of the received PSDU in linear scale
     */
    void NotifyRxPsduFailed(Ptr<const WifiPsdu> psdu, double snr);

    /**
     * Handle the outcome of a reception of a PPDU.
     *
     * \param ppdu the received PPDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector TXVECTOR of the PSDU
     * \param staId the station ID of the PSDU (only used for MU)
     * \param statusPerMpdu reception status per MPDU
     */
    void NotifyRxPpduOutcome(Ptr<const WifiPpdu> ppdu,
                             RxSignalInfo rxSignalInfo,
                             const WifiTxVector& txVector,
                             uint16_t staId,
                             const std::vector<bool>& statusPerMpdu);
    /**
     * Switch from RX after the reception was successful.
     */
    void SwitchFromRxEndOk();
    /**
     * Switch from RX after the reception failed.
     */
    void SwitchFromRxEndError();
    /**
     * Abort current reception following a CCA reset request.
     * \param operatingWidth the channel width the PHY is operating on (in MHz)
     */
    void SwitchFromRxAbort(uint16_t operatingWidth);
    /**
     * Switch to CCA busy.
     *
     * \param duration the duration of the CCA state
     * \param channelType the channel type for which the CCA busy state is reported.
     * \param per20MhzDurations vector that indicates for how long each 20 MHz subchannel
     *        (corresponding to the index of the element in the vector) is busy and where a zero
     * duration indicates that the subchannel is idle. The vector is non-empty if the PHY supports
     * 802.11ax or later and if the operational channel width is larger than 20 MHz.
     */
    void SwitchMaybeToCcaBusy(Time duration,
                              WifiChannelListType channelType,
                              const std::vector<Time>& per20MhzDurations);
    /**
     * Switch to sleep mode.
     */
    void SwitchToSleep();
    /**
     * Switch from sleep mode.
     */
    void SwitchFromSleep();
    /**
     * Switch to off mode.
     */
    void SwitchToOff();
    /**
     * Switch from off mode.
     */
    void SwitchFromOff();

    /**
     * TracedCallback signature for state changes.
     *
     * \param [in] start Time when the \pname{state} started.
     * \param [in] duration Amount of time we've been in (or will be in)
     *             the \pname{state}.
     * \param [in] state The state.
     */
    typedef void (*StateTracedCallback)(Time start, Time duration, WifiPhyState state);

    /**
     * TracedCallback signature for receive end OK event.
     *
     * \param [in] packet The received packet.
     * \param [in] snr    The SNR of the received packet in linear scale.
     * \param [in] mode   The transmission mode of the packet.
     * \param [in] preamble The preamble of the packet.
     */
    typedef void (*RxOkTracedCallback)(Ptr<const Packet> packet,
                                       double snr,
                                       WifiMode mode,
                                       WifiPreamble preamble);

    /**
     * TracedCallback signature for the outcome of a received packet.
     *
     * \param [in] psdu The received PSDU (Physical Layer Service Data Unit).
     * \param [in] signalInfo Information about the received signal, including its power and other
     * characteristics.
     * \param [in] txVector The transmission vector used for the packet, detailing
     * the transmission parameters.
     * \param [in] outcomes A vector of boolean values indicating the
     * success or failure of receiving individual MPDUs within the PSDU.
     */
    typedef void (*RxOutcomeTracedCallback)(Ptr<const WifiPsdu> psdu,
                                            RxSignalInfo signalInfo,
                                            const WifiTxVector& txVector,
                                            const std::vector<bool>& outcomes);

    /**
     * TracedCallback signature for receive end error event.
     *
     * \param [in] packet       The received packet.
     * \param [in] snr          The SNR of the received packet in linear scale.
     */
    typedef void (*RxEndErrorTracedCallback)(Ptr<const Packet> packet, double snr);

    /**
     * TracedCallback signature for transmit event.
     *
     * \param [in] packet The received packet.
     * \param [in] mode   The transmission mode of the packet.
     * \param [in] preamble The preamble of the packet.
     * \param [in] power  The transmit power level.
     */
    typedef void (*TxTracedCallback)(Ptr<const Packet> packet,
                                     WifiMode mode,
                                     WifiPreamble preamble,
                                     uint8_t power);

    /**
     * Notify all WifiPhyListener objects of the given PHY event.
     *
     * \tparam FUNC \deduced Member function type
     * \tparam Ts \deduced Function argument types
     * \param f the member function to invoke
     * \param args arguments to pass to the member function
     */
    template <typename FUNC, typename... Ts>
    void NotifyListeners(FUNC f, Ts&&... args);

  private:
    /**
     * typedef for a list of WifiPhyListeners. We use weak pointers so that unregistering a
     * listener is not necessary to delete a listener (reference count is not incremented by
     * weak pointers).
     */
    typedef std::list<std::weak_ptr<WifiPhyListener>> Listeners;

    /**
     * Log the idle and CCA busy states.
     */
    void LogPreviousIdleAndCcaBusyStates();

    /**
     * Switch the state from RX.
     */
    void DoSwitchFromRx();

    /**
     * The trace source fired when state is changed.
     */
    TracedCallback<Time, Time, WifiPhyState> m_stateLogger;

    NS_LOG_TEMPLATE_DECLARE;        //!< the log component
    bool m_sleeping;                ///< sleeping
    bool m_isOff;                   ///< switched off
    Time m_endTx;                   ///< end transmit
    Time m_endRx;                   ///< end receive
    Time m_endCcaBusy;              ///< end CCA busy
    Time m_endSwitching;            ///< end switching
    Time m_endSleep;                ///< end sleep
    Time m_endOff;                  ///< end off
    Time m_endIdle;                 ///< end idle
    Time m_startTx;                 ///< start transmit
    Time m_startRx;                 ///< start receive
    Time m_startCcaBusy;            ///< start CCA busy
    Time m_startSwitching;          ///< start switching
    Time m_startSleep;              ///< start sleep
    Time m_startOff;                ///< start off
    Time m_previousStateChangeTime; ///< previous state change time

    Listeners m_listeners; ///< listeners
    TracedCallback<Ptr<const Packet>, double, WifiMode, WifiPreamble>
        m_rxOkTrace; ///< receive OK trace callback
    TracedCallback<Ptr<const WifiPpdu>, RxSignalInfo, const WifiTxVector&, const std::vector<bool>&>
        m_rxOutcomeTrace;                                     ///< receive OK trace callback
    TracedCallback<Ptr<const Packet>, double> m_rxErrorTrace; ///< receive error trace callback
    TracedCallback<Ptr<const Packet>, WifiMode, WifiPreamble, uint8_t>
        m_txTrace;                     ///< transmit trace callback
    RxOkCallback m_rxOkCallback;       ///< receive OK callback
    RxErrorCallback m_rxErrorCallback; ///< receive error callback
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <typename FUNC, typename... Ts>
void
WifiPhyStateHelper::NotifyListeners(FUNC f, Ts&&... args)
{
    NS_LOG_FUNCTION(this);
    // In some cases (e.g., when notifying an EMLSR client of a link switch), a notification
    // to a PHY listener involves the addition and/or removal of a PHY listener, thus modifying
    // the list we are iterating over. This is dangerous, so ensure that we iterate over a copy
    // of the list of PHY listeners. The copied list contains shared pointers to the PHY listeners
    // to prevent them from being deleted.
    std::list<std::shared_ptr<WifiPhyListener>> listeners;
    std::transform(m_listeners.cbegin(),
                   m_listeners.cend(),
                   std::back_inserter(listeners),
                   [](auto&& listener) { return listener.lock(); });

    for (const auto& listener : listeners)
    {
        if (listener)
        {
            std::invoke(f, listener, std::forward<Ts>(args)...);
        }
    }
}

} // namespace ns3

#endif /* WIFI_PHY_STATE_HELPER_H */
