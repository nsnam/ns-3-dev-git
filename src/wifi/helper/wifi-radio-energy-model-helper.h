/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#ifndef WIFI_RADIO_ENERGY_MODEL_HELPER_H
#define WIFI_RADIO_ENERGY_MODEL_HELPER_H

#include "ns3/energy-model-helper.h"
#include "ns3/wifi-radio-energy-model.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Assign WifiRadioEnergyModel to wifi devices.
 *
 * This installer installs WifiRadioEnergyModel for only WifiNetDevice objects.
 *
 */
class WifiRadioEnergyModelHelper : public DeviceEnergyModelHelper
{
  public:
    /**
     * Construct a helper which is used to add a radio energy model to a node
     */
    WifiRadioEnergyModelHelper();

    /**
     * Destroy a RadioEnergy Helper
     */
    ~WifiRadioEnergyModelHelper() override;

    /**
     * @param name the name of the attribute to set
     * @param v the value of the attribute
     *
     * Sets an attribute of the underlying PHY object.
     */
    void Set(std::string name, const AttributeValue& v) override;

    /**
     * @param callback Callback function for energy depletion handling.
     *
     * Sets the callback to be invoked when energy is depleted.
     */
    void SetDepletionCallback(WifiRadioEnergyModel::WifiRadioEnergyDepletionCallback callback);

    /**
     * @param callback Callback function for energy recharged handling.
     *
     * Sets the callback to be invoked when energy is recharged.
     */
    void SetRechargedCallback(WifiRadioEnergyModel::WifiRadioEnergyRechargedCallback callback);

    /**
     * @tparam Ts \deduced Argument types
     * @param name the name of the model to set
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * Configure a Transmission Current model for this EnergySource.
     */
    template <typename... Ts>
    void SetTxCurrentModel(std::string name, Ts&&... args);

  private:
    /**
     * @param device Pointer to the NetDevice to install DeviceEnergyModel.
     * @param source Pointer to EnergySource to install.
     * @returns Ptr<DeviceEnergyModel>
     *
     * Implements DeviceEnergyModel::Install.
     */
    Ptr<energy::DeviceEnergyModel> DoInstall(Ptr<NetDevice> device,
                                             Ptr<energy::EnergySource> source) const override;

  private:
    ObjectFactory m_radioEnergy; ///< radio energy
    WifiRadioEnergyModel::WifiRadioEnergyDepletionCallback
        m_depletionCallback; ///< radio energy depletion callback
    WifiRadioEnergyModel::WifiRadioEnergyRechargedCallback
        m_rechargedCallback;        ///< radio energy recharged callback
    ObjectFactory m_txCurrentModel; ///< transmit current model
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
WifiRadioEnergyModelHelper::SetTxCurrentModel(std::string name, Ts&&... args)
{
    m_txCurrentModel = ObjectFactory(name, std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* WIFI_RADIO_ENERGY_MODEL_HELPER_H */
