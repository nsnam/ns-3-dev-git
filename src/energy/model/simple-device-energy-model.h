/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#ifndef SIMPLE_DEVICE_ENERGY_MODEL_H
#define SIMPLE_DEVICE_ENERGY_MODEL_H

#include "device-energy-model.h"

#include "ns3/nstime.h"
#include "ns3/traced-value.h"

namespace ns3
{
namespace energy
{

/**
 * @ingroup energy
 *
 * A simple device energy model where current drain can be set by the user.
 *
 * It is supposed to be used as a testing model for energy sources.
 *
 */
class SimpleDeviceEnergyModel : public DeviceEnergyModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    SimpleDeviceEnergyModel();
    ~SimpleDeviceEnergyModel() override;

    /**
     * @brief Sets pointer to node.
     *
     * @param node Pointer to node.
     *
     * Implements DeviceEnergyModel::SetNode.
     */
    virtual void SetNode(Ptr<Node> node);

    /**
     * @brief Gets pointer to node.
     *
     * @returns Pointer to node.
     *
     * Implements DeviceEnergyModel::GetNode.
     */
    virtual Ptr<Node> GetNode() const;

    /**
     * @brief Sets pointer to EnergySource installed on node.
     *
     * @param source Pointer to EnergySource installed on node.
     *
     * Implements DeviceEnergyModel::SetEnergySource.
     */
    void SetEnergySource(Ptr<EnergySource> source) override;

    /**
     * @returns Total energy consumption of the vehicle.
     *
     * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
     */
    double GetTotalEnergyConsumption() const override;

    /**
     * @param newState New state the device is in.
     *
     * Not implemented
     */
    void ChangeState(int newState) override
    {
    }

    /**
     * @brief Handles energy depletion.
     *
     * Not implemented
     */
    void HandleEnergyDepletion() override
    {
    }

    /**
     * @brief Handles energy recharged.
     *
     * Not implemented
     */
    void HandleEnergyRecharged() override
    {
    }

    /**
     * @brief Handles energy changed.
     *
     * Not implemented
     */
    void HandleEnergyChanged() override
    {
    }

    /**
     * @param current the current draw of device.
     *
     * Set the actual current draw of the device.
     */
    void SetCurrentA(double current);

  private:
    void DoDispose() override;

    /**
     * @returns Current draw of device, at current state.
     *
     * Implements DeviceEnergyModel::GetCurrentA.
     */
    double DoGetCurrentA() const override;

    Time m_lastUpdateTime;                        //!< Last update time
    double m_actualCurrentA;                      //!< actual curred (in Ampere)
    Ptr<EnergySource> m_source;                   //!< Energy source
    Ptr<Node> m_node;                             //!< Node
    TracedValue<double> m_totalEnergyConsumption; //!< Total energy consumption trace
};

} // namespace energy
} // namespace ns3

#endif /* SIMPLE_DEVICE_ENERGY_MODEL_H */
