/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#ifndef ACOUSTIC_MODEM_ENERGY_MODEL_HELPER_H
#define ACOUSTIC_MODEM_ENERGY_MODEL_HELPER_H

#include "ns3/acoustic-modem-energy-model.h"
#include "ns3/energy-model-helper.h"

namespace ns3
{

/**
 * @ingroup uan
 *
 * Assign AcousticModemEnergyModel to uan devices.
 *
 * This installer installs AcousticModemEnergyModel for only
 * UanNetDevice objects.
 */
class AcousticModemEnergyModelHelper : public DeviceEnergyModelHelper
{
  public:
    /**
     * Construct a helper which is used to add a radio energy model to a node.
     */
    AcousticModemEnergyModelHelper();

    /**
     * Destroy an AcousticModemEnergy Helper.
     */
    ~AcousticModemEnergyModelHelper() override;

    /**
     * Sets an attribute of the underlying energy model object.
     *
     * @param name The name of the attribute to set.
     * @param v The value of the attribute.
     */
    void Set(std::string name, const AttributeValue& v) override;

    /**
     * Sets the callback to be invoked when energy is depleted.
     *
     * @param callback Callback function for energy depletion handling.
     */
    void SetDepletionCallback(
        AcousticModemEnergyModel::AcousticModemEnergyDepletionCallback callback);

  private:
    /**
     * Implements DeviceEnergyModel::Install.
     *
     * @param device Pointer to the NetDevice to install DeviceEnergyModel.
     * @param source Pointer to EnergySource installed on node.
     * @return The energy model.
     */
    Ptr<energy::DeviceEnergyModel> DoInstall(Ptr<NetDevice> device,
                                             Ptr<energy::EnergySource> source) const override;

  private:
    /** Energy model factory. */
    ObjectFactory m_modemEnergy;

    /** Callback for energy depletion. */
    AcousticModemEnergyModel::AcousticModemEnergyDepletionCallback m_depletionCallback;
};

} // namespace ns3

#endif /* ACOUSTIC_MODEM_ENERGY_MODEL_HELPER_H */
