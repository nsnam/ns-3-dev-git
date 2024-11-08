/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "wifi-radio-energy-model.h"

#include "wifi-tx-current-model.h"

#include "ns3/energy-source.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED(WifiRadioEnergyModel);

TypeId
WifiRadioEnergyModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiRadioEnergyModel")
            .SetParent<DeviceEnergyModel>()
            .SetGroupName("Energy")
            .AddConstructor<WifiRadioEnergyModel>()
            .AddAttribute("IdleCurrentA",
                          "The default radio Idle current in Ampere.",
                          DoubleValue(0.273), // idle mode = 273mA
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetIdleCurrentA,
                                             &WifiRadioEnergyModel::GetIdleCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("CcaBusyCurrentA",
                          "The default radio CCA Busy State current in Ampere.",
                          DoubleValue(0.273), // default to be the same as idle mode
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetCcaBusyCurrentA,
                                             &WifiRadioEnergyModel::GetCcaBusyCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("TxCurrentA",
                          "The radio TX current in Ampere.",
                          DoubleValue(0.380), // transmit at 0dBm = 380mA
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetTxCurrentA,
                                             &WifiRadioEnergyModel::GetTxCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("RxCurrentA",
                          "The radio RX current in Ampere.",
                          DoubleValue(0.313), // receive mode = 313mA
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetRxCurrentA,
                                             &WifiRadioEnergyModel::GetRxCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("SwitchingCurrentA",
                          "The default radio Channel Switch current in Ampere.",
                          DoubleValue(0.273), // default to be the same as idle mode
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetSwitchingCurrentA,
                                             &WifiRadioEnergyModel::GetSwitchingCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("SleepCurrentA",
                          "The radio Sleep current in Ampere.",
                          DoubleValue(0.033), // sleep mode = 33mA
                          MakeDoubleAccessor(&WifiRadioEnergyModel::SetSleepCurrentA,
                                             &WifiRadioEnergyModel::GetSleepCurrentA),
                          MakeDoubleChecker<ampere_u>())
            .AddAttribute("TxCurrentModel",
                          "A pointer to the attached TX current model.",
                          PointerValue(),
                          MakePointerAccessor(&WifiRadioEnergyModel::m_txCurrentModel),
                          MakePointerChecker<WifiTxCurrentModel>())
            .AddTraceSource(
                "TotalEnergyConsumption",
                "Total energy consumption of the radio device.",
                MakeTraceSourceAccessor(&WifiRadioEnergyModel::m_totalEnergyConsumption),
                "ns3::TracedValueCallback::Double");
    return tid;
}

WifiRadioEnergyModel::WifiRadioEnergyModel()
    : m_source(nullptr),
      m_currentState(WifiPhyState::IDLE),
      m_lastUpdateTime(),
      m_nPendingChangeState(0)
{
    NS_LOG_FUNCTION(this);
    m_energyDepletionCallback.Nullify();
    // set callback for WifiPhy listener
    m_listener = std::make_shared<WifiRadioEnergyModelPhyListener>();
    m_listener->SetChangeStateCallback(MakeCallback(&DeviceEnergyModel::ChangeState, this));
    // set callback for updating the TX current
    m_listener->SetUpdateTxCurrentCallback(
        MakeCallback(&WifiRadioEnergyModel::SetTxCurrentFromModel, this));
}

WifiRadioEnergyModel::~WifiRadioEnergyModel()
{
    NS_LOG_FUNCTION(this);
    m_txCurrentModel = nullptr;
    m_listener.reset();
}

void
WifiRadioEnergyModel::SetEnergySource(const Ptr<energy::EnergySource> source)
{
    NS_LOG_FUNCTION(this << source);
    NS_ASSERT(source);
    m_source = source;
    m_switchToOffEvent.Cancel();
    const auto durationToOff = GetMaximumTimeInState(m_currentState);
    m_switchToOffEvent = Simulator::Schedule(durationToOff,
                                             &WifiRadioEnergyModel::ChangeState,
                                             this,
                                             static_cast<int>(WifiPhyState::OFF));
}

Watt_u
WifiRadioEnergyModel::GetTotalEnergyConsumption() const
{
    NS_LOG_FUNCTION(this);

    const auto duration = Simulator::Now() - m_lastUpdateTime;
    NS_ASSERT(duration.IsPositive()); // check if duration is valid

    // energy to decrease = current * voltage * time
    const auto supplyVoltage = m_source->GetSupplyVoltage();
    const auto energyToDecrease = duration.GetSeconds() * GetStateA(m_currentState) * supplyVoltage;

    // notify energy source
    m_source->UpdateEnergySource();

    return m_totalEnergyConsumption + energyToDecrease;
}

ampere_u
WifiRadioEnergyModel::GetIdleCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_idleCurrent;
}

void
WifiRadioEnergyModel::SetIdleCurrentA(ampere_u idleCurrent)
{
    NS_LOG_FUNCTION(this << idleCurrent);
    m_idleCurrent = idleCurrent;
}

ampere_u
WifiRadioEnergyModel::GetCcaBusyCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_ccaBusyCurrent;
}

void
WifiRadioEnergyModel::SetCcaBusyCurrentA(ampere_u ccaBusyCurrent)
{
    NS_LOG_FUNCTION(this << ccaBusyCurrent);
    m_ccaBusyCurrent = ccaBusyCurrent;
}

ampere_u
WifiRadioEnergyModel::GetTxCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_txCurrent;
}

void
WifiRadioEnergyModel::SetTxCurrentA(ampere_u txCurrent)
{
    NS_LOG_FUNCTION(this << txCurrent);
    m_txCurrent = txCurrent;
}

ampere_u
WifiRadioEnergyModel::GetRxCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_rxCurrent;
}

void
WifiRadioEnergyModel::SetRxCurrentA(ampere_u rxCurrent)
{
    NS_LOG_FUNCTION(this << rxCurrent);
    m_rxCurrent = rxCurrent;
}

ampere_u
WifiRadioEnergyModel::GetSwitchingCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_switchingCurrent;
}

void
WifiRadioEnergyModel::SetSwitchingCurrentA(ampere_u switchingCurrent)
{
    NS_LOG_FUNCTION(this << switchingCurrent);
    m_switchingCurrent = switchingCurrent;
}

ampere_u
WifiRadioEnergyModel::GetSleepCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_sleepCurrent;
}

void
WifiRadioEnergyModel::SetSleepCurrentA(ampere_u sleepCurrent)
{
    NS_LOG_FUNCTION(this << sleepCurrent);
    m_sleepCurrent = sleepCurrent;
}

WifiPhyState
WifiRadioEnergyModel::GetCurrentState() const
{
    NS_LOG_FUNCTION(this);
    return m_currentState;
}

void
WifiRadioEnergyModel::SetEnergyDepletionCallback(WifiRadioEnergyDepletionCallback callback)
{
    NS_LOG_FUNCTION(this);
    if (callback.IsNull())
    {
        NS_LOG_DEBUG("WifiRadioEnergyModel:Setting NULL energy depletion callback!");
    }
    m_energyDepletionCallback = callback;
}

void
WifiRadioEnergyModel::SetEnergyRechargedCallback(WifiRadioEnergyRechargedCallback callback)
{
    NS_LOG_FUNCTION(this);
    if (callback.IsNull())
    {
        NS_LOG_DEBUG("WifiRadioEnergyModel:Setting NULL energy recharged callback!");
    }
    m_energyRechargedCallback = callback;
}

void
WifiRadioEnergyModel::SetTxCurrentModel(const Ptr<WifiTxCurrentModel> model)
{
    m_txCurrentModel = model;
}

void
WifiRadioEnergyModel::SetTxCurrentFromModel(dBm_u txPower)
{
    if (m_txCurrentModel)
    {
        m_txCurrent = m_txCurrentModel->CalcTxCurrent(txPower);
    }
}

Time
WifiRadioEnergyModel::GetMaximumTimeInState(WifiPhyState state) const
{
    if (state == WifiPhyState::OFF)
    {
        NS_FATAL_ERROR("Requested maximum remaining time for OFF state");
    }
    const auto remainingEnergy = m_source->GetRemainingEnergy();
    const auto supplyVoltage = m_source->GetSupplyVoltage();
    const auto current = GetStateA(state);
    return Seconds(remainingEnergy / (current * supplyVoltage));
}

void
WifiRadioEnergyModel::ChangeState(int newState)
{
    WifiPhyState newPhyState{newState};
    NS_LOG_FUNCTION(this << newPhyState);

    m_nPendingChangeState++;

    if (m_nPendingChangeState > 1 && newPhyState == WifiPhyState::OFF)
    {
        SetWifiRadioState(newPhyState);
        m_nPendingChangeState--;
        return;
    }

    if (newPhyState != WifiPhyState::OFF)
    {
        m_switchToOffEvent.Cancel();
        const auto durationToOff = GetMaximumTimeInState(newPhyState);
        m_switchToOffEvent = Simulator::Schedule(durationToOff,
                                                 &WifiRadioEnergyModel::ChangeState,
                                                 this,
                                                 static_cast<int>(WifiPhyState::OFF));
    }

    const auto duration = Simulator::Now() - m_lastUpdateTime;
    NS_ASSERT(duration.IsPositive()); // check if duration is valid

    // energy to decrease = current * voltage * time
    const auto supplyVoltage = m_source->GetSupplyVoltage();
    const auto energyToDecrease = duration.GetSeconds() * GetStateA(m_currentState) * supplyVoltage;

    // update total energy consumption
    m_totalEnergyConsumption += energyToDecrease;
    NS_ASSERT(m_totalEnergyConsumption <= m_source->GetInitialEnergy());

    // update last update time stamp
    m_lastUpdateTime = Simulator::Now();

    // notify energy source
    m_source->UpdateEnergySource();

    // in case the energy source is found to be depleted during the last update, a callback might be
    // invoked that might cause a change in the Wifi PHY state (e.g., the PHY is put into SLEEP
    // mode). This in turn causes a new call to this member function, with the consequence that the
    // previous instance is resumed after the termination of the new instance. In particular, the
    // state set by the previous instance is erroneously the final state stored in m_currentState.
    // The check below ensures that previous instances do not change m_currentState.

    if (m_nPendingChangeState <= 1 && m_currentState != WifiPhyState::OFF)
    {
        // update current state & last update time stamp
        SetWifiRadioState(newPhyState);

        // some debug message
        NS_LOG_DEBUG("WifiRadioEnergyModel:Total energy consumption is " << m_totalEnergyConsumption
                                                                         << "J");
    }

    m_nPendingChangeState--;
}

void
WifiRadioEnergyModel::HandleEnergyDepletion()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("WifiRadioEnergyModel:Energy is depleted!");
    // invoke energy depletion callback, if set.
    if (!m_energyDepletionCallback.IsNull())
    {
        m_energyDepletionCallback();
    }
}

void
WifiRadioEnergyModel::HandleEnergyRecharged()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("WifiRadioEnergyModel:Energy is recharged!");
    // invoke energy recharged callback, if set.
    if (!m_energyRechargedCallback.IsNull())
    {
        m_energyRechargedCallback();
    }
}

void
WifiRadioEnergyModel::HandleEnergyChanged()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("WifiRadioEnergyModel:Energy is changed!");
    if (m_currentState != WifiPhyState::OFF)
    {
        m_switchToOffEvent.Cancel();
        const auto durationToOff = GetMaximumTimeInState(m_currentState);
        m_switchToOffEvent = Simulator::Schedule(durationToOff,
                                                 &WifiRadioEnergyModel::ChangeState,
                                                 this,
                                                 static_cast<int>(WifiPhyState::OFF));
    }
}

std::shared_ptr<WifiRadioEnergyModelPhyListener>
WifiRadioEnergyModel::GetPhyListener()
{
    NS_LOG_FUNCTION(this);
    return m_listener;
}

/*
 * Private functions start here.
 */

void
WifiRadioEnergyModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_source = nullptr;
    m_energyDepletionCallback.Nullify();
}

ampere_u
WifiRadioEnergyModel::GetStateA(WifiPhyState state) const
{
    switch (state)
    {
    case WifiPhyState::IDLE:
        return m_idleCurrent;
    case WifiPhyState::CCA_BUSY:
        return m_ccaBusyCurrent;
    case WifiPhyState::TX:
        return m_txCurrent;
    case WifiPhyState::RX:
        return m_rxCurrent;
    case WifiPhyState::SWITCHING:
        return m_switchingCurrent;
    case WifiPhyState::SLEEP:
        return m_sleepCurrent;
    case WifiPhyState::OFF:
        return 0.0;
    }
    NS_FATAL_ERROR("WifiRadioEnergyModel: undefined radio state " << state);
}

ampere_u
WifiRadioEnergyModel::DoGetCurrentA() const
{
    return GetStateA(m_currentState);
}

void
WifiRadioEnergyModel::SetWifiRadioState(const WifiPhyState state)
{
    NS_LOG_FUNCTION(this << state);
    m_currentState = state;
    std::string stateName;
    switch (state)
    {
    case WifiPhyState::IDLE:
        stateName = "IDLE";
        break;
    case WifiPhyState::CCA_BUSY:
        stateName = "CCA_BUSY";
        break;
    case WifiPhyState::TX:
        stateName = "TX";
        break;
    case WifiPhyState::RX:
        stateName = "RX";
        break;
    case WifiPhyState::SWITCHING:
        stateName = "SWITCHING";
        break;
    case WifiPhyState::SLEEP:
        stateName = "SLEEP";
        break;
    case WifiPhyState::OFF:
        stateName = "OFF";
        break;
    }
    NS_LOG_DEBUG("WifiRadioEnergyModel:Switching to state: " << stateName
                                                             << " at time = " << Simulator::Now());
}

// -------------------------------------------------------------------------- //

WifiRadioEnergyModelPhyListener::WifiRadioEnergyModelPhyListener()
{
    NS_LOG_FUNCTION(this);
    m_changeStateCallback.Nullify();
    m_updateTxCurrentCallback.Nullify();
}

WifiRadioEnergyModelPhyListener::~WifiRadioEnergyModelPhyListener()
{
    NS_LOG_FUNCTION(this);
}

void
WifiRadioEnergyModelPhyListener::SetChangeStateCallback(
    energy::DeviceEnergyModel::ChangeStateCallback callback)
{
    NS_LOG_FUNCTION(this << &callback);
    NS_ASSERT(!callback.IsNull());
    m_changeStateCallback = callback;
}

void
WifiRadioEnergyModelPhyListener::SetUpdateTxCurrentCallback(UpdateTxCurrentCallback callback)
{
    NS_LOG_FUNCTION(this << &callback);
    NS_ASSERT(!callback.IsNull());
    m_updateTxCurrentCallback = callback;
}

void
WifiRadioEnergyModelPhyListener::NotifyRxStart(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::RX));
    m_switchToIdleEvent.Cancel();
}

void
WifiRadioEnergyModelPhyListener::NotifyRxEndOk()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::IDLE));
}

void
WifiRadioEnergyModelPhyListener::NotifyRxEndError()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::IDLE));
}

void
WifiRadioEnergyModelPhyListener::NotifyTxStart(Time duration, dBm_u txPower)
{
    NS_LOG_FUNCTION(this << duration << txPower);
    if (m_updateTxCurrentCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Update tx current callback not set!");
    }
    m_updateTxCurrentCallback(txPower);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::TX));
    // schedule changing state back to IDLE after TX duration
    m_switchToIdleEvent.Cancel();
    m_switchToIdleEvent =
        Simulator::Schedule(duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifyCcaBusyStart(Time duration,
                                                    WifiChannelListType channelType,
                                                    const std::vector<Time>& /*per20MhzDurations*/)
{
    NS_LOG_FUNCTION(this << duration << channelType);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::CCA_BUSY));
    // schedule changing state back to IDLE after CCA_BUSY duration
    m_switchToIdleEvent.Cancel();
    m_switchToIdleEvent =
        Simulator::Schedule(duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifySwitchingStart(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::SWITCHING));
    // schedule changing state back to IDLE after CCA_BUSY duration
    m_switchToIdleEvent.Cancel();
    m_switchToIdleEvent =
        Simulator::Schedule(duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifySleep()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::SLEEP));
    m_switchToIdleEvent.Cancel();
}

void
WifiRadioEnergyModelPhyListener::NotifyWakeup()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::IDLE));
}

void
WifiRadioEnergyModelPhyListener::NotifyOff()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::OFF));
    m_switchToIdleEvent.Cancel();
}

void
WifiRadioEnergyModelPhyListener::NotifyOn()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::IDLE));
}

void
WifiRadioEnergyModelPhyListener::SwitchToIdle()
{
    NS_LOG_FUNCTION(this);
    if (m_changeStateCallback.IsNull())
    {
        NS_FATAL_ERROR("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
    m_changeStateCallback(static_cast<int>(WifiPhyState::IDLE));
}

} // namespace ns3
