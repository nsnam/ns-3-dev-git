/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "basic-energy-source.h"

#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("BasicEnergySource");
NS_OBJECT_ENSURE_REGISTERED(BasicEnergySource);

TypeId
BasicEnergySource::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::energy::BasicEnergySource")
            .AddDeprecatedName("ns3::BasicEnergySource")
            .SetParent<EnergySource>()
            .SetGroupName("Energy")
            .AddConstructor<BasicEnergySource>()
            .AddAttribute("BasicEnergySourceInitialEnergyJ",
                          "Initial energy stored in basic energy source.",
                          DoubleValue(10), // in Joules
                          MakeDoubleAccessor(&BasicEnergySource::SetInitialEnergy,
                                             &BasicEnergySource::GetInitialEnergy),
                          MakeDoubleChecker<double>())
            .AddAttribute("BasicEnergySupplyVoltageV",
                          "Initial supply voltage for basic energy source.",
                          DoubleValue(3.0), // in Volts
                          MakeDoubleAccessor(&BasicEnergySource::SetSupplyVoltage,
                                             &BasicEnergySource::GetSupplyVoltage),
                          MakeDoubleChecker<double>())
            .AddAttribute("BasicEnergyLowBatteryThreshold",
                          "Low battery threshold for basic energy source.",
                          DoubleValue(0.10), // as a fraction of the initial energy
                          MakeDoubleAccessor(&BasicEnergySource::m_lowBatteryTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("BasicEnergyHighBatteryThreshold",
                          "High battery threshold for basic energy source.",
                          DoubleValue(0.15), // as a fraction of the initial energy
                          MakeDoubleAccessor(&BasicEnergySource::m_highBatteryTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("PeriodicEnergyUpdateInterval",
                          "Time between two consecutive periodic energy updates.",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&BasicEnergySource::SetEnergyUpdateInterval,
                                           &BasicEnergySource::GetEnergyUpdateInterval),
                          MakeTimeChecker())
            .AddTraceSource("RemainingEnergy",
                            "Remaining energy at BasicEnergySource.",
                            MakeTraceSourceAccessor(&BasicEnergySource::m_remainingEnergyJ),
                            "ns3::TracedValueCallback::Double");
    return tid;
}

BasicEnergySource::BasicEnergySource()
{
    NS_LOG_FUNCTION(this);
    m_lastUpdateTime = Seconds(0);
    m_depleted = false;
}

BasicEnergySource::~BasicEnergySource()
{
    NS_LOG_FUNCTION(this);
}

void
BasicEnergySource::SetInitialEnergy(double initialEnergyJ)
{
    NS_LOG_FUNCTION(this << initialEnergyJ);
    NS_ASSERT(initialEnergyJ >= 0);
    m_initialEnergyJ = initialEnergyJ;
    m_remainingEnergyJ = m_initialEnergyJ;
}

void
BasicEnergySource::SetSupplyVoltage(double supplyVoltageV)
{
    NS_LOG_FUNCTION(this << supplyVoltageV);
    m_supplyVoltageV = supplyVoltageV;
}

void
BasicEnergySource::SetEnergyUpdateInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    m_energyUpdateInterval = interval;
}

Time
BasicEnergySource::GetEnergyUpdateInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_energyUpdateInterval;
}

double
BasicEnergySource::GetSupplyVoltage() const
{
    NS_LOG_FUNCTION(this);
    return m_supplyVoltageV;
}

double
BasicEnergySource::GetInitialEnergy() const
{
    NS_LOG_FUNCTION(this);
    return m_initialEnergyJ;
}

double
BasicEnergySource::GetRemainingEnergy()
{
    NS_LOG_FUNCTION(this);
    // update energy source to get the latest remaining energy.
    UpdateEnergySource();
    return m_remainingEnergyJ;
}

double
BasicEnergySource::GetEnergyFraction()
{
    NS_LOG_FUNCTION(this);
    // update energy source to get the latest remaining energy.
    UpdateEnergySource();
    return m_remainingEnergyJ / m_initialEnergyJ;
}

void
BasicEnergySource::UpdateEnergySource()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("BasicEnergySource:Updating remaining energy.");

    double remainingEnergy = m_remainingEnergyJ;
    CalculateRemainingEnergy();

    m_lastUpdateTime = Simulator::Now();

    if (!m_depleted && m_remainingEnergyJ <= m_lowBatteryTh * m_initialEnergyJ)
    {
        m_depleted = true;
        HandleEnergyDrainedEvent();
    }
    else if (m_depleted && m_remainingEnergyJ > m_highBatteryTh * m_initialEnergyJ)
    {
        m_depleted = false;
        HandleEnergyRechargedEvent();
    }
    else if (m_remainingEnergyJ != remainingEnergy)
    {
        NotifyEnergyChanged();
    }

    if (m_energyUpdateEvent.IsExpired())
    {
        m_energyUpdateEvent = Simulator::Schedule(m_energyUpdateInterval,
                                                  &BasicEnergySource::UpdateEnergySource,
                                                  this);
    }
}

/*
 * Private functions start here.
 */

void
BasicEnergySource::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    UpdateEnergySource(); // start periodic update
}

void
BasicEnergySource::DoDispose()
{
    NS_LOG_FUNCTION(this);
    BreakDeviceEnergyModelRefCycle(); // break reference cycle
}

void
BasicEnergySource::HandleEnergyDrainedEvent()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("BasicEnergySource:Energy depleted!");
    NotifyEnergyDrained(); // notify DeviceEnergyModel objects
}

void
BasicEnergySource::HandleEnergyRechargedEvent()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("BasicEnergySource:Energy recharged!");
    NotifyEnergyRecharged(); // notify DeviceEnergyModel objects
}

void
BasicEnergySource::CalculateRemainingEnergy()
{
    NS_LOG_FUNCTION(this);
    double totalCurrentA = CalculateTotalCurrent();
    Time duration = Simulator::Now() - m_lastUpdateTime;
    NS_ASSERT(duration.IsPositive());
    // energy = current * voltage * time
    double energyToDecreaseJ = (totalCurrentA * m_supplyVoltageV * duration).GetSeconds();
    NS_ASSERT(m_remainingEnergyJ >= energyToDecreaseJ);
    m_remainingEnergyJ -= energyToDecreaseJ;
    NS_LOG_DEBUG("BasicEnergySource:Remaining energy = " << m_remainingEnergyJ);
}

} // namespace energy
} // namespace ns3
