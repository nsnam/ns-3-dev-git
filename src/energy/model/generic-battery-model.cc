/*
 * Copyright (c) 2010 Andrea Sacco: Li-Ion battery
 * Copyright (c) 2023 Tokushima University, Japan:
 * NiMh,NiCd,LeaAcid batteries and preset and multicell extensions.
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
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 *         Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "generic-battery-model.h"

#include <ns3/assert.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GenericBatteryModel");

NS_OBJECT_ENSURE_REGISTERED(GenericBatteryModel);

TypeId
GenericBatteryModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GenericBatteryModel")
            .SetParent<EnergySource>()
            .SetGroupName("Energy")
            .AddConstructor<GenericBatteryModel>()
            .AddAttribute("LowBatteryThreshold",
                          "Low battery threshold for generic battery model.",
                          DoubleValue(0.10), // 0.10 as a fraction of the initial energy
                          MakeDoubleAccessor(&GenericBatteryModel::m_lowBatteryTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("FullVoltage",
                          "(Q_full) The voltage of the cell when fully charged (V).",
                          DoubleValue(4.18),
                          MakeDoubleAccessor(&GenericBatteryModel::m_vFull),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxCapacity",
                          "(Q) The maximum capacity of the cell (Ah).",
                          DoubleValue(2.45),
                          MakeDoubleAccessor(&GenericBatteryModel::m_qMax),
                          MakeDoubleChecker<double>())
            .AddAttribute("NominalVoltage",
                          "(V_nom) Nominal voltage of the cell (V).",
                          DoubleValue(3.59),
                          MakeDoubleAccessor(&GenericBatteryModel::m_vNom),
                          MakeDoubleChecker<double>())
            .AddAttribute("NominalCapacity",
                          "(Q_nom) Cell capacity at the end of the nominal zone (Ah)",
                          DoubleValue(1.3),
                          MakeDoubleAccessor(&GenericBatteryModel::m_qNom),
                          MakeDoubleChecker<double>())
            .AddAttribute("ExponentialVoltage",
                          "(V_exp) Cell voltage at the end of the exponential zone (V).",
                          DoubleValue(3.75),
                          MakeDoubleAccessor(&GenericBatteryModel::m_vExp),
                          MakeDoubleChecker<double>())
            .AddAttribute("ExponentialCapacity",
                          "(Q_exp) Cell Capacity at the end of the exponential zone (Ah).",
                          DoubleValue(0.39),
                          MakeDoubleAccessor(&GenericBatteryModel::m_qExp),
                          MakeDoubleChecker<double>())
            .AddAttribute("InternalResistance",
                          "(R) Internal resistance of the cell (Ohms)",
                          DoubleValue(0.083),
                          MakeDoubleAccessor(&GenericBatteryModel::m_internalResistance),
                          MakeDoubleChecker<double>())
            .AddAttribute("TypicalDischargeCurrent",
                          "Typical discharge current used in manufacters datasheets (A)",
                          DoubleValue(2.33),
                          MakeDoubleAccessor(&GenericBatteryModel::m_typicalCurrent),
                          MakeDoubleChecker<double>())
            .AddAttribute("CutoffVoltage",
                          "The voltage where the battery is considered depleted (V).",
                          DoubleValue(3.3),
                          MakeDoubleAccessor(&GenericBatteryModel::m_cutoffVoltage),
                          MakeDoubleChecker<double>())
            .AddAttribute("PeriodicEnergyUpdateInterval",
                          "Time between two consecutive periodic energy updates.",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&GenericBatteryModel::SetEnergyUpdateInterval,
                                           &GenericBatteryModel::GetEnergyUpdateInterval),
                          MakeTimeChecker())
            .AddAttribute("BatteryType",
                          "Indicates the battery type used by the model",
                          EnumValue(LION_LIPO),
                          MakeEnumAccessor(&GenericBatteryModel::m_batteryType),
                          MakeEnumChecker(LION_LIPO,
                                          "LION_LIPO",
                                          NIMH_NICD,
                                          "NIMH_NICD",
                                          LEADACID,
                                          "LEADACID"))
            .AddTraceSource("RemainingEnergy",
                            "Remaining energy of generic battery",
                            MakeTraceSourceAccessor(&GenericBatteryModel::m_remainingEnergyJ),
                            "ns3::TracedValueCallback::Double");
    return tid;
}

GenericBatteryModel::GenericBatteryModel()
    : m_drainedCapacity(0),
      m_currentFiltered(0),
      m_entn(0),
      m_expZone(0),
      m_lastUpdateTime(Seconds(0.0))
{
    NS_LOG_FUNCTION(this);
}

GenericBatteryModel::~GenericBatteryModel()
{
    NS_LOG_FUNCTION(this);
}

double
GenericBatteryModel::GetInitialEnergy() const
{
    double initialEnergy = m_qMax * m_vFull * 3600;
    return initialEnergy;
}

double
GenericBatteryModel::GetSupplyVoltage() const
{
    return m_supplyVoltageV;
}

void
GenericBatteryModel::SetEnergyUpdateInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    m_energyUpdateInterval = interval;
}

void
GenericBatteryModel::SetDrainedCapacity(double drainedCapacity)
{
    NS_ASSERT(drainedCapacity >= 0 && drainedCapacity < m_qMax);
    m_drainedCapacity = drainedCapacity;
}

double
GenericBatteryModel::GetDrainedCapacity() const
{
    return m_drainedCapacity;
}

double
GenericBatteryModel::GetStateOfCharge() const
{
    double soc = 100 * (1 - m_drainedCapacity / m_qMax);
    return soc;
}

Time
GenericBatteryModel::GetEnergyUpdateInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_energyUpdateInterval;
}

double
GenericBatteryModel::GetRemainingEnergy()
{
    NS_LOG_FUNCTION(this);
    UpdateEnergySource();
    return m_remainingEnergyJ;
}

double
GenericBatteryModel::GetEnergyFraction()
{
    NS_LOG_FUNCTION(this);
    return GetStateOfCharge();
}

void
GenericBatteryModel::UpdateEnergySource()
{
    NS_LOG_FUNCTION(this);

    // do not update if simulation has finished
    if (Simulator::IsFinished())
    {
        return;
    }

    m_energyUpdateEvent.Cancel();

    CalculateRemainingEnergy();

    m_lastUpdateTime = Simulator::Now();

    if (m_supplyVoltageV <= m_cutoffVoltage)
    {
        // check if battery is depleted
        BatteryDepletedEvent();
    }
    else if (m_supplyVoltageV >= m_vFull)
    {
        // check if battery has reached full charge
        BatteryChargedEvent();
        // TODO: Should battery charging be stopped if full voltage is reached?
        //       or should it be allowed to continue charging (overcharge)?
    }

    m_energyUpdateEvent =
        Simulator::Schedule(m_energyUpdateInterval, &GenericBatteryModel::UpdateEnergySource, this);
}

void
GenericBatteryModel::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

void
GenericBatteryModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    BreakDeviceEnergyModelRefCycle();
}

void
GenericBatteryModel::BatteryDepletedEvent()
{
    NS_LOG_FUNCTION(this);
    // notify DeviceEnergyModel objects, all "usable" energy has been depleted
    // (cutoff voltage was reached)
    NotifyEnergyDrained();
}

void
GenericBatteryModel::BatteryChargedEvent()
{
    NS_LOG_FUNCTION(this);
    // notify DeviceEnergyModel objects, the battery has reached its full energy potential.
    // (full voltage was reached)
    NotifyEnergyRecharged();
}

void
GenericBatteryModel::CalculateRemainingEnergy()
{
    NS_LOG_FUNCTION(this);

    double totalCurrentA = CalculateTotalCurrent();

    m_energyUpdateLapseTime = Simulator::Now() - m_lastUpdateTime;

    NS_ASSERT(m_energyUpdateLapseTime.GetSeconds() >= 0);

    // Calculate i* (current step response)
    Time batteryResponseConstant = Seconds(30);

    double responseTime = (Simulator::Now() / batteryResponseConstant).GetDouble();
    m_currentFiltered = totalCurrentA * (1 - 1 / (std::exp(responseTime)));
    // TODO: the time in the responseTime should be a time taken since the last *battery current*
    // change, in the testing the  battery current is only changed at the beginning of the
    // simulation therefore, the simulation time can be used. However this must be changed to
    // a time counter to allow this value to reset in the middle of the simulation when the
    // battery current changes.

    m_drainedCapacity += (totalCurrentA * m_energyUpdateLapseTime).GetHours();

    if (totalCurrentA < 0)
    {
        // Charge current (Considered as "Negative" i)
        m_supplyVoltageV = GetChargeVoltage(totalCurrentA);
    }
    else
    {
        // Discharge current (Considered as "Positive" i)
        m_supplyVoltageV = GetVoltage(totalCurrentA);
    }
}

double
GenericBatteryModel::GetChargeVoltage(double i)
{
    // integral of i over time drained capacity in Ah
    double it = m_drainedCapacity;

    // empirical factors
    double A = m_vFull - m_vExp;
    double B = 3 / m_qExp;

    // voltage constant
    double E0 = m_vFull + m_internalResistance * m_typicalCurrent - A;

    // voltage of exponential zone when battery is fully charged
    double expZoneFull = A * std::exp(-B * m_qNom);

    // Obtain the voltage|resistance polarization constant
    double K = (E0 - m_vNom - (m_internalResistance * m_typicalCurrent) + expZoneFull) /
               (m_qMax / (m_qMax - m_qNom) * (m_qNom + m_typicalCurrent));

    double V = 0;
    double polResistance = 0;
    double polVoltage = 0;

    if (m_batteryType == LION_LIPO)
    { // For LiOn & LiPo batteries

        // Calculate exponential zone voltage
        m_expZone = A * std::exp(-B * it);

        polResistance = K * m_qMax / (it + 0.1 * m_qMax);
        polVoltage = K * m_qMax / (m_qMax - it);
        V = E0 - (m_internalResistance * i) - (polResistance * m_currentFiltered) -
            (polVoltage * it) + m_expZone;
    }
    else
    {
        // Calculate exponential zone voltage

        if (m_expZone == 0)
        {
            m_expZone = A * std::exp(-B * it);
        }
        double entnPrime = m_entn;
        double expZonePrime = m_expZone;
        m_entn = B * std::abs(i) * (-expZonePrime + A);
        m_expZone = expZonePrime + (m_energyUpdateLapseTime * entnPrime).GetHours();

        if (m_batteryType == NIMH_NICD)
        { // For NiMH and NiCd batteries
            polResistance = K * m_qMax / (std::abs(it) + 0.1 * m_qMax);
        }
        else if (m_batteryType == LEADACID)
        { // For Lead acid batteries
            polResistance = K * m_qMax / (it + 0.1 * m_qMax);
        }

        polVoltage = K * m_qMax / (m_qMax - it);

        V = E0 - (m_internalResistance * i) - (polResistance * m_currentFiltered) -
            (polVoltage * it) + m_expZone;
    }

    // Energy in Joules = RemainingCapacity * Voltage * Seconds in an Hour
    m_remainingEnergyJ = (m_qMax - it) * V * 3600;

    NS_LOG_DEBUG("* CHARGE *| " << Simulator::Now().As(Time::S) << "| i " << i << " | it " << it
                                << "| E0 " << E0 << " | polRes " << polResistance << " | polVol "
                                << polVoltage << "| B " << B << " | ExpZone " << m_expZone
                                << " | A " << A << "| K " << K << "| i* " << m_currentFiltered
                                << " | V " << V << " |  rmnEnergy " << m_remainingEnergyJ
                                << "J | SoC " << GetStateOfCharge() << "% ");

    return V;
}

double
GenericBatteryModel::GetVoltage(double i)
{
    NS_LOG_FUNCTION(this << i);

    // integral of i in dt, drained capacity in Ah
    double it = m_drainedCapacity;

    // empirical factors
    double A = m_vFull - m_vExp;

    double B = 3 / m_qExp;

    // constant voltage
    double E0 = m_vFull + m_internalResistance * m_typicalCurrent - A;

    // voltage of exponential zone when battery is fully charged
    double expZoneFull = A * std::exp(-B * m_qNom);

    // Obtain the voltage|resistance polarization constant
    double K = (E0 - m_vNom - (m_internalResistance * m_typicalCurrent) + expZoneFull) /
               (m_qMax / (m_qMax - m_qNom) * (m_qNom + m_typicalCurrent));

    double V = 0;
    double polResistance = K * (m_qMax / (m_qMax - it));
    double polVoltage = polResistance;

    // Calculate exponential zone voltage according to the battery type
    if (m_batteryType == LION_LIPO)
    {
        m_expZone = A * exp(-B * it);
    }
    else
    {
        NS_ASSERT(m_batteryType == NIMH_NICD || m_batteryType == LEADACID);
        if (m_expZone == 0)
        {
            m_expZone = A * exp(-B * it);
        }

        double entnPrime = m_entn;
        double expZonePrime = m_expZone;

        m_entn = B * std::abs(i) * (-expZonePrime);
        m_expZone = expZonePrime + (m_energyUpdateLapseTime * entnPrime).GetHours();
    }

    V = E0 - (m_internalResistance * i) - (polResistance * m_currentFiltered) - (polVoltage * it) +
        m_expZone;

    // EnergyJ = RemainingCapacity * Voltage * Seconds in an Hour
    m_remainingEnergyJ = (m_qMax - it) * V * 3600;

    NS_LOG_DEBUG("* DISCHARGE *| " << Simulator::Now().As(Time::S) << "| i " << i << " | it " << it
                                   << " | A " << A << " | B " << B << " | ExpZone " << m_expZone
                                   << " | V " << V << " | rmnEnergy " << m_remainingEnergyJ
                                   << "J | SoC " << GetStateOfCharge() << "% "
                                   << "\n"
                                   << "             | K " << K << " | E0 " << E0);

    return V;
}

} // namespace ns3
