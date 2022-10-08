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

#include "wifi-radio-energy-model-helper.h"

#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-tx-current-model.h"

namespace ns3
{

WifiRadioEnergyModelHelper::WifiRadioEnergyModelHelper()
{
    m_radioEnergy.SetTypeId("ns3::WifiRadioEnergyModel");
    m_depletionCallback.Nullify();
    m_rechargedCallback.Nullify();
}

WifiRadioEnergyModelHelper::~WifiRadioEnergyModelHelper()
{
}

void
WifiRadioEnergyModelHelper::Set(std::string name, const AttributeValue& v)
{
    m_radioEnergy.Set(name, v);
}

void
WifiRadioEnergyModelHelper::SetDepletionCallback(
    WifiRadioEnergyModel::WifiRadioEnergyDepletionCallback callback)
{
    m_depletionCallback = callback;
}

void
WifiRadioEnergyModelHelper::SetRechargedCallback(
    WifiRadioEnergyModel::WifiRadioEnergyRechargedCallback callback)
{
    m_rechargedCallback = callback;
}

/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
WifiRadioEnergyModelHelper::DoInstall(Ptr<NetDevice> device, Ptr<EnergySource> source) const
{
    NS_ASSERT(device);
    NS_ASSERT(source);
    // check if device is WifiNetDevice
    std::string deviceName = device->GetInstanceTypeId().GetName();
    if (deviceName != "ns3::WifiNetDevice")
    {
        NS_FATAL_ERROR("NetDevice type is not WifiNetDevice!");
    }
    Ptr<Node> node = device->GetNode();
    Ptr<WifiRadioEnergyModel> model = m_radioEnergy.Create()->GetObject<WifiRadioEnergyModel>();
    NS_ASSERT(model);

    // set energy depletion callback
    // if none is specified, make a callback to WifiPhy::SetOffMode
    Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(device);
    Ptr<WifiPhy> wifiPhy = wifiDevice->GetPhy();
    wifiPhy->SetWifiRadioEnergyModel(model);
    if (m_depletionCallback.IsNull())
    {
        model->SetEnergyDepletionCallback(MakeCallback(&WifiPhy::SetOffMode, wifiPhy));
    }
    else
    {
        model->SetEnergyDepletionCallback(m_depletionCallback);
    }
    // set energy recharged callback
    // if none is specified, make a callback to WifiPhy::ResumeFromOff
    if (m_rechargedCallback.IsNull())
    {
        model->SetEnergyRechargedCallback(MakeCallback(&WifiPhy::ResumeFromOff, wifiPhy));
    }
    else
    {
        model->SetEnergyRechargedCallback(m_rechargedCallback);
    }
    // add model to device model list in energy source
    source->AppendDeviceEnergyModel(model);
    // set energy source pointer
    model->SetEnergySource(source);
    // create and register energy model PHY listener
    wifiPhy->RegisterListener(model->GetPhyListener());
    //
    if (m_txCurrentModel.GetTypeId().GetUid())
    {
        Ptr<WifiTxCurrentModel> txcurrent = m_txCurrentModel.Create<WifiTxCurrentModel>();
        model->SetTxCurrentModel(txcurrent);
    }
    return model;
}

} // namespace ns3
