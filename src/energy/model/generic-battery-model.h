/*
 * Copyright (c) 2010 Andrea Sacco: Li-Ion battery
 * Copyright (c) 2023 Tokushima University, Japan:
 * NiMh,NiCd,LeaAcid batteries and preset and multi-cell extensions.
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

#ifndef GENERIC_BATTERY_MODEL_H
#define GENERIC_BATTERY_MODEL_H

#include "energy-source.h"

#include <ns3/event-id.h>
#include <ns3/nstime.h>
#include <ns3/traced-value.h>

namespace ns3
{

/**
 * \ingroup energy
 *
 *  Battery types.
 *  These are grouped according to their chemical characteristics
 *  present during a charge/discharge curve.
 */
enum GenericBatteryType
{
    LION_LIPO = 0, //!< Lithium-ion and Lithium-polymer batteries
    NIMH_NICD = 1, //!< Nickel-metal hydride and Nickel cadmium batteries
    LEADACID = 2   //!< Lead Acid Batteries
};

/**
 * \ingroup energy
 *
 *  Battery models that described the parameters of the the battery presets.
 */
enum BatteryModel
{
    PANASONIC_HHR650D_NIMH = 0,    //!< Panasonic HHR650D NiMh battery
    CSB_GP1272_LEADACID = 1,       //!< CSB GP1272 Lead acid battery
    PANASONIC_CGR18650DA_LION = 2, //!< Panasonic CGR18650DA Li-Ion battery
    RSPRO_LGP12100_LEADACID = 3,   //!< RS Pro LGP12100 Lead acid battery
    PANASONIC_N700AAC_NICD = 4     //!< Panasonic N700AAC NiCd battery
};

/**
 * \ingroup energy
 *
 *  The structure containing the the parameter values that describe a
 *  battery preset.
 */
struct BatteryPresets
{
    GenericBatteryType batteryType; //!< The type of battery used in the preset.
    std::string description;        //!< Additional information about the battery.
    double vFull;                   //!< Initial voltage of the battery, in Volts
    double qMax;                    //!< The maximum capacity of the battery, in Ah
    double vNom;                    //!< Nominal voltage of the battery, in Volts
    double qNom;                    //!< Battery capacity at the end of the nominal zone, in Ah
    double vExp;               //!< Battery voltage at the end of the exponential zone, in Volts
    double qExp;               //!< Capacity value at the end of the exponential zone, in Ah
    double internalResistance; //!< Internal resistance of the battery, in Ohms
    double typicalCurrent;     //!< Typical discharge current used to fit the curves
    double cuttoffVoltage;     //!< The threshold voltage where the battery is considered depleted
};

/**
 * \ingroup energy
 *
 *  Contains the values that form the battery presents available in this module.
 */
static BatteryPresets g_batteryPreset[] = {{NIMH_NICD,
                                            "Panasonic HHR650D | NiMH | 1.2V 6.5Ah | Size: D",
                                            1.39,
                                            7.0,
                                            1.18,
                                            6.25,
                                            1.28,
                                            1.3,
                                            0.0046,
                                            1.3,
                                            1.0},
                                           {LEADACID,
                                            "CSB GP1272 | Lead Acid | 12V 7.2Ah",
                                            12.8,
                                            7.2,
                                            11.5,
                                            4.5,
                                            12.5,
                                            2,
                                            0.056,
                                            0.36,
                                            8.0},
                                           {LION_LIPO,
                                            "Panasonic CGR18650DA | Li-Ion | 3.6V 2.45Ah | Size: A",
                                            4.17,
                                            2.33,
                                            3.57,
                                            2.14,
                                            3.714,
                                            1.74,
                                            0.0830,
                                            0.466,
                                            3.0},
                                           {LEADACID,
                                            "Rs PRO LGP12100 | Lead Acid | 12V 100Ah",
                                            12.60,
                                            130,
                                            12.44,
                                            12.3,
                                            12.52,
                                            12,
                                            0.00069,
                                            5,
                                            11},
                                           {NIMH_NICD,
                                            "PANASONIC N-700AAC | NiCd | 1.2V 700mAh | Size: AA",
                                            1.38,
                                            0.790,
                                            1.17,
                                            0.60,
                                            1.25,
                                            0.24,
                                            0.016,
                                            0.7,
                                            0.8}};

/**
 * \ingroup energy
 * \brief A generic battery model for  Li-Ion, NiCd, NiMh and Lead acid batteries
 *
 * The generic battery model can be used to describe the discharge behavior of
 * the battery chemestries supported by the model.
 */
class GenericBatteryModel : public EnergySource
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    GenericBatteryModel();

    ~GenericBatteryModel() override;

    /**
     * Implements GetInitialEnergy. It returns the amount of energy in Joules stored in the
     * battery when fully charged. This energy is different to the total amount of usable energy
     * in the battery. This is because the battery cannot be used until Voltage = 0, only until
     * it reaches the cutoff voltage.
     *
     * \return The initial energy stored in the fully charged battery, in Joules.
     */
    double GetInitialEnergy() const override;

    /**
     * Implements GetSupplyVoltage.
     *
     * \return Supply voltage at the energy source.
     */
    double GetSupplyVoltage() const override;

    /**
     * Implements GetRemainingEnergy.
     *
     * \return Remaining energy in energy source, in Joules
     */
    double GetRemainingEnergy() override;

    /**
     * Implements GetEnergyFraction. For the generic battery model, energy fraction
     * is equivalent to the remaining usable capacity (i.e. The SoC).
     *
     * \return Energy fraction.
     */
    double GetEnergyFraction() override;

    /**
     * Implements UpdateEnergySource.
     */
    void UpdateEnergySource() override;

    /**
     * This function sets the interval between each energy update.
     *
     * \param interval Energy update interval.
     */
    void SetEnergyUpdateInterval(Time interval);

    /**
     *  This function is used to change the initial capacity in the battery.
     *  A value of 0 means that the battery is fully charged. The value cannot
     *  be set to a value bigger than the rated capacity (fully discharged) or
     *  less than 0 (fully charged).
     *
     *  \param drainedCapacity The capacity drained so far in the battery.
     */
    void SetDrainedCapacity(double drainedCapacity);

    /**
     * Obtain the amount of drained capacity from the battery based on the
     * integral of the current over time (Coulomb counting method).
     *
     * \return The drainedCapacity (Ah)
     */
    double GetDrainedCapacity() const;

    /**
     *  Calculates an estimate of the State of Charge (SoC).
     *  In essence, the amount of usable capacity remaining in the battery (%).
     *
     *  \return The percentage of usable capacity remaining in the battery.
     */
    double GetStateOfCharge() const;

    /**
     * \return The interval between each energy update.
     */
    Time GetEnergyUpdateInterval() const;

  private:
    void DoInitialize() override;
    void DoDispose() override;

    /**
     * Handles the battery reaching its cutoff voltage. This function notifies
     * all the energy models aggregated to the node about the usable energy in the
     * battery has being depleted. Each energy model is then responsible for its own handler.
     */
    void BatteryDepletedEvent();

    /**
     * Handles the battery reaching its full voltage. This function notifies
     * all the energy models aggregated to the node about the battery reaching its
     * full energy charge.
     */
    void BatteryChargedEvent();

    /**
     * Calculates remaining energy. This function uses the total current from all
     * device models to calculate the amount of energy to decrease. The energy to
     * decrease is given by:
     *    energy to decrease = total current * supply voltage * time duration
     * This function subtracts the calculated energy to decrease from remaining
     * energy.
     */
    void CalculateRemainingEnergy();

    /**
     *  Get the battery voltage in function of the discharge current.
     *  It consider different discharge curves for different discharge currents
     *  and the remaining energy of the battery.
     *
     *  \param current The actual discharge current value (+i).
     *  \return The voltage of the battery.
     */
    double GetVoltage(double current);

    /**
     *  Obtain the battery voltage as a result of a charge current.
     *
     *  \param current The actual charge current value (-i).
     *  \return The voltage of the battery.
     */
    double GetChargeVoltage(double current);

  private:
    TracedValue<double> m_remainingEnergyJ; //!< Remaining energy, in Joules
    double m_drainedCapacity;               //!< Capacity drained from the battery, in Ah
    double m_currentFiltered;               //!< The step response (a.k.a. low pass filter)
    double m_entn;                          //!< The previous value of the exponential zone
                                            //!< in NiMh,NiCd and LeadAcid.
    double m_expZone;                       //!< Voltage value of the exponential zone
    Time m_energyUpdateLapseTime; //!< The lapse of time between the last battery energy update and
                                  //!< the current time.
    double m_supplyVoltageV;      //!< Actual voltage of the battery
    double m_lowBatteryTh;        //!< Low battery threshold, as a fraction of the initial energy
    EventId m_energyUpdateEvent;  //!< Energy update event
    Time m_lastUpdateTime;        //!< Last update time
    Time m_energyUpdateInterval;  //!< Energy update interval
    double m_vFull;               //!< Initial voltage of the battery, in Volts
    double m_vNom;                //!< Nominal voltage of the battery, in Volts
    double m_vExp;                //!< Battery voltage at the end of the exponential zone, in Volts
    double m_internalResistance;  //!< Internal resistance of the battery, in Ohms
    double m_qMax;                //!< The maximum capacity of the battery, in Ah
    double m_qNom;                //!< Battery capacity at the end of the nominal zone, in Ah
    double m_qExp;                //!< Capacity value at the end of the exponential zone, in Ah
    double m_typicalCurrent;      //!< Typical discharge current used to fit the curves
    double m_cutoffVoltage; //!< The threshold voltage where the battery is considered depleted
    GenericBatteryType m_batteryType; //!< Indicates the battery type used by the model
};

} // namespace ns3

#endif /* GENERIC_BATTERY_MODEL_H */
