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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#ifndef BASIC_ENERGY_SOURCE_H
#define BASIC_ENERGY_SOURCE_H

#include "energy-source.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"

namespace ns3
{
namespace energy
{

/**
 * \ingroup energy
 * BasicEnergySource decreases/increases remaining energy stored in itself in
 * linearly.
 *
 */
class BasicEnergySource : public EnergySource
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();
    BasicEnergySource();
    ~BasicEnergySource() override;

    /**
     * \return Initial energy stored in energy source, in Joules.
     *
     * Implements GetInitialEnergy.
     */
    double GetInitialEnergy() const override;

    /**
     * \returns Supply voltage at the energy source.
     *
     * Implements GetSupplyVoltage.
     */
    double GetSupplyVoltage() const override;

    /**
     * \return Remaining energy in energy source, in Joules
     *
     * Implements GetRemainingEnergy.
     */
    double GetRemainingEnergy() override;

    /**
     * \returns Energy fraction.
     *
     * Implements GetEnergyFraction.
     */
    double GetEnergyFraction() override;

    /**
     * Implements UpdateEnergySource.
     */
    void UpdateEnergySource() override;

    /**
     * \param initialEnergyJ Initial energy, in Joules
     *
     * Sets initial energy stored in the energy source. Note that initial energy
     * is assumed to be set before simulation starts and is set only once per
     * simulation.
     */
    void SetInitialEnergy(double initialEnergyJ);

    /**
     * \param supplyVoltageV Supply voltage at the energy source, in Volts.
     *
     * Sets supply voltage of the energy source.
     */
    void SetSupplyVoltage(double supplyVoltageV);

    /**
     * \param interval Energy update interval.
     *
     * This function sets the interval between each energy update.
     */
    void SetEnergyUpdateInterval(Time interval);

    /**
     * \returns The interval between each energy update.
     */
    Time GetEnergyUpdateInterval() const;

  private:
    /// Defined in ns3::Object
    void DoInitialize() override;

    /// Defined in ns3::Object
    void DoDispose() override;

    /**
     * Handles the remaining energy going to zero event. This function notifies
     * all the energy models aggregated to the node about the energy being
     * depleted. Each energy model is then responsible for its own handler.
     */
    void HandleEnergyDrainedEvent();

    /**
     * Handles the remaining energy exceeding the high threshold after it went
     * below the low threshold. This function notifies all the energy models
     * aggregated to the node about the energy being recharged. Each energy model
     * is then responsible for its own handler.
     */
    void HandleEnergyRechargedEvent();

    /**
     * Calculates remaining energy. This function uses the total current from all
     * device models to calculate the amount of energy to decrease. The energy to
     * decrease is given by:
     *    energy to decrease = total current * supply voltage * time duration
     * This function subtracts the calculated energy to decrease from remaining
     * energy.
     */
    void CalculateRemainingEnergy();

  private:
    double m_initialEnergyJ; //!< initial energy, in Joules
    double m_supplyVoltageV; //!< supply voltage, in Volts
    double m_lowBatteryTh;   //!< low battery threshold, as a fraction of the initial energy
    double m_highBatteryTh;  //!< high battery threshold, as a fraction of the initial energy
    /**
     * set to true when the remaining energy goes below the low threshold,
     * set to false again when the remaining energy exceeds the high threshold
     */
    bool m_depleted;
    TracedValue<double> m_remainingEnergyJ; //!< remaining energy, in Joules
    EventId m_energyUpdateEvent;            //!< energy update event
    Time m_lastUpdateTime;                  //!< last update time
    Time m_energyUpdateInterval;            //!< energy update interval
};

} // namespace energy
} // namespace ns3

#endif /* BASIC_ENERGY_SOURCE_H */
