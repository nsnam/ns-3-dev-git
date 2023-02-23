/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Authors: Sidharth Nabar <snabar@uw.edu>
 *          He Wu <mdzz@u.washington.edu>
 */

#ifndef WIFI_RADIO_ENERGY_MODEL_H
#define WIFI_RADIO_ENERGY_MODEL_H

#include "wifi-phy-listener.h"
#include "wifi-phy-state.h"

#include "ns3/device-energy-model.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"

namespace ns3
{

class WifiTxCurrentModel;

/**
 * \ingroup energy
 * A WifiPhy listener class for notifying the WifiRadioEnergyModel of Wifi radio
 * state change.
 *
 */
class WifiRadioEnergyModelPhyListener : public WifiPhyListener
{
  public:
    /**
     * Callback type for updating the transmit current based on the nominal TX power.
     */
    typedef Callback<void, double> UpdateTxCurrentCallback;

    WifiRadioEnergyModelPhyListener();
    ~WifiRadioEnergyModelPhyListener() override;

    /**
     * \brief Sets the change state callback. Used by helper class.
     *
     * \param callback Change state callback.
     */
    void SetChangeStateCallback(DeviceEnergyModel::ChangeStateCallback callback);

    /**
     * \brief Sets the update TX current callback.
     *
     * \param callback Update TX current callback.
     */
    void SetUpdateTxCurrentCallback(UpdateTxCurrentCallback callback);

    void NotifyRxStart(Time duration) override;
    void NotifyRxEndOk() override;
    void NotifyRxEndError() override;
    void NotifyTxStart(Time duration, double txPowerDbm) override;
    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& per20MhzDurations) override;
    void NotifySwitchingStart(Time duration) override;
    void NotifySleep() override;
    void NotifyOff() override;
    void NotifyWakeup() override;
    void NotifyOn() override;

  private:
    /**
     * A helper function that makes scheduling m_changeStateCallback possible.
     */
    void SwitchToIdle();

    /**
     * Change state callback used to notify the WifiRadioEnergyModel of a state
     * change.
     */
    DeviceEnergyModel::ChangeStateCallback m_changeStateCallback;

    /**
     * Callback used to update the TX current stored in WifiRadioEnergyModel based on
     * the nominal TX power used to transmit the current frame.
     */
    UpdateTxCurrentCallback m_updateTxCurrentCallback;

    EventId m_switchToIdleEvent; ///< switch to idle event
};

/**
 * \ingroup energy
 * \brief A WiFi radio energy model.
 *
 * 4 states are defined for the radio: TX, RX, IDLE, SLEEP. Default state is
 * IDLE.
 * The different types of transactions that are defined are:
 *  1. Tx: State goes from IDLE to TX, radio is in TX state for TX_duration,
 *     then state goes from TX to IDLE.
 *  2. Rx: State goes from IDLE to RX, radio is in RX state for RX_duration,
 *     then state goes from RX to IDLE.
 *  3. Go_to_Sleep: State goes from IDLE to SLEEP.
 *  4. End_of_Sleep: State goes from SLEEP to IDLE.
 * The class keeps track of what state the radio is currently in.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 * Default values for power consumption are based on measurements reported in:
 *
 * Daniel Halperin, Ben Greenstein, Anmol Sheth, David Wetherall,
 * "Demystifying 802.11n power consumption", Proceedings of HotPower'10
 *
 * Power consumption in Watts (single antenna):
 *
 * \f$ P_{tx} = 1.14 \f$ (transmit at 0dBm)
 *
 * \f$ P_{rx} = 0.94 \f$
 *
 * \f$ P_{idle} = 0.82 \f$
 *
 * \f$ P_{sleep} = 0.10 \f$
 *
 * Hence, considering the default supply voltage of 3.0 V for the basic energy
 * source, the default current values in Ampere are:
 *
 * \f$ I_{tx} = 0.380 \f$
 *
 * \f$ I_{rx} = 0.313 \f$
 *
 * \f$ I_{idle} = 0.273 \f$
 *
 * \f$ I_{sleep} = 0.033 \f$
 *
 * The dependence of the power consumption in transmission mode on the nominal
 * transmit power can also be achieved through a wifi TX current model.
 *
 */
class WifiRadioEnergyModel : public DeviceEnergyModel
{
  public:
    /**
     * Callback type for energy depletion handling.
     */
    typedef Callback<void> WifiRadioEnergyDepletionCallback;

    /**
     * Callback type for energy recharged handling.
     */
    typedef Callback<void> WifiRadioEnergyRechargedCallback;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    WifiRadioEnergyModel();
    ~WifiRadioEnergyModel() override;

    /**
     * \brief Sets pointer to EnergySource installed on node.
     *
     * \param source Pointer to EnergySource installed on node.
     *
     * Implements DeviceEnergyModel::SetEnergySource.
     */
    void SetEnergySource(const Ptr<EnergySource> source) override;

    /**
     * \returns Total energy consumption of the wifi device in watts.
     *
     * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
     */
    double GetTotalEnergyConsumption() const override;

    // Setter & getters for state power consumption.
    /**
     * \brief Gets idle current in Amperes.
     *
     * \returns idle current of the wifi device.
     */
    double GetIdleCurrentA() const;
    /**
     * \brief Sets idle current in Amperes.
     *
     * \param idleCurrentA the idle current
     */
    void SetIdleCurrentA(double idleCurrentA);
    /**
     * \brief Gets CCA busy current in Amperes.
     *
     * \returns CCA Busy current of the wifi device.
     */
    double GetCcaBusyCurrentA() const;
    /**
     * \brief Sets CCA busy current in Amperes.
     *
     * \param ccaBusyCurrentA the CCA busy current
     */
    void SetCcaBusyCurrentA(double ccaBusyCurrentA);
    /**
     * \brief Gets transmit current in Amperes.
     *
     * \returns transmit current of the wifi device.
     */
    double GetTxCurrentA() const;
    /**
     * \brief Sets transmit current in Amperes.
     *
     * \param txCurrentA the transmit current
     */
    void SetTxCurrentA(double txCurrentA);
    /**
     * \brief Gets receive current in Amperes.
     *
     * \returns receive current of the wifi device.
     */
    double GetRxCurrentA() const;
    /**
     * \brief Sets receive current in Amperes.
     *
     * \param rxCurrentA the receive current
     */
    void SetRxCurrentA(double rxCurrentA);
    /**
     * \brief Gets switching current in Amperes.
     *
     * \returns switching current of the wifi device.
     */
    double GetSwitchingCurrentA() const;
    /**
     * \brief Sets switching current in Amperes.
     *
     * \param switchingCurrentA the switching current
     */
    void SetSwitchingCurrentA(double switchingCurrentA);
    /**
     * \brief Gets sleep current in Amperes.
     *
     * \returns sleep current of the wifi device.
     */
    double GetSleepCurrentA() const;
    /**
     * \brief Sets sleep current in Amperes.
     *
     * \param sleepCurrentA the sleep current
     */
    void SetSleepCurrentA(double sleepCurrentA);

    /**
     * \returns Current state.
     */
    WifiPhyState GetCurrentState() const;

    /**
     * \param callback Callback function.
     *
     * Sets callback for energy depletion handling.
     */
    void SetEnergyDepletionCallback(WifiRadioEnergyDepletionCallback callback);

    /**
     * \param callback Callback function.
     *
     * Sets callback for energy recharged handling.
     */
    void SetEnergyRechargedCallback(WifiRadioEnergyRechargedCallback callback);

    /**
     * \param model the model used to compute the wifi TX current.
     */
    void SetTxCurrentModel(const Ptr<WifiTxCurrentModel> model);

    /**
     * \brief Calls the CalcTxCurrent method of the TX current model to
     *        compute the TX current based on such model
     *
     * \param txPowerDbm the nominal TX power in dBm
     */
    void SetTxCurrentFromModel(double txPowerDbm);

    /**
     * \brief Changes state of the WifiRadioEnergyMode.
     *
     * \param newState New state the wifi radio is in.
     *
     * Implements DeviceEnergyModel::ChangeState.
     */
    void ChangeState(int newState) override;

    /**
     * \param state the wifi state
     *
     * \returns the time the radio can stay in that state based on the remaining energy.
     */
    Time GetMaximumTimeInState(int state) const;

    /**
     * \brief Handles energy depletion.
     *
     * Implements DeviceEnergyModel::HandleEnergyDepletion
     */
    void HandleEnergyDepletion() override;

    /**
     * \brief Handles energy recharged.
     *
     * Implements DeviceEnergyModel::HandleEnergyRecharged
     */
    void HandleEnergyRecharged() override;

    /**
     * \brief Handles energy changed.
     *
     * Implements DeviceEnergyModel::HandleEnergyChanged
     */
    void HandleEnergyChanged() override;

    /**
     * \returns Pointer to the PHY listener.
     */
    WifiRadioEnergyModelPhyListener* GetPhyListener();

  private:
    void DoDispose() override;

    /**
     * \param state the wifi state
     * \returns draw of device in Amperes, at given state.
     */
    double GetStateA(int state) const;

    /**
     * \returns Current draw of device in Amperes, at current state.
     *
     * Implements DeviceEnergyModel::GetCurrentA.
     */
    double DoGetCurrentA() const override;

    /**
     * \param state New state the radio device is currently in.
     *
     * Sets current state. This function is private so that only the energy model
     * can change its own state.
     */
    void SetWifiRadioState(const WifiPhyState state);

    Ptr<EnergySource> m_source; ///< energy source

    // Member variables for current draw in different radio modes.
    double m_txCurrentA;                      ///< transmit current in Amperes
    double m_rxCurrentA;                      ///< receive current in Amperes
    double m_idleCurrentA;                    ///< idle current in Amperes
    double m_ccaBusyCurrentA;                 ///< CCA busy current in Amperes
    double m_switchingCurrentA;               ///< switching current in Amperes
    double m_sleepCurrentA;                   ///< sleep current in Amperes
    Ptr<WifiTxCurrentModel> m_txCurrentModel; ///< current model

    /// This variable keeps track of the total energy consumed by this model in watts.
    TracedValue<double> m_totalEnergyConsumption;

    // State variables.
    WifiPhyState m_currentState; ///< current state the radio is in
    Time m_lastUpdateTime;       ///< time stamp of previous energy update

    uint8_t m_nPendingChangeState; ///< pending state change

    /// Energy depletion callback
    WifiRadioEnergyDepletionCallback m_energyDepletionCallback;

    /// Energy recharged callback
    WifiRadioEnergyRechargedCallback m_energyRechargedCallback;

    /// WifiPhy listener
    WifiRadioEnergyModelPhyListener* m_listener;

    EventId m_switchToOffEvent; ///< switch to off event
};

} // namespace ns3

#endif /* WIFI_RADIO_ENERGY_MODEL_H */
