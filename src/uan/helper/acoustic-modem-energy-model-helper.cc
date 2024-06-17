/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "acoustic-modem-energy-model-helper.h"

#include "ns3/basic-energy-source-helper.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/uan-net-device.h"
#include "ns3/uan-phy.h"

namespace ns3
{

AcousticModemEnergyModelHelper::AcousticModemEnergyModelHelper()
{
    m_modemEnergy.SetTypeId("ns3::AcousticModemEnergyModel");
    m_depletionCallback.Nullify();
}

AcousticModemEnergyModelHelper::~AcousticModemEnergyModelHelper()
{
}

void
AcousticModemEnergyModelHelper::Set(std::string name, const AttributeValue& v)
{
    m_modemEnergy.Set(name, v);
}

void
AcousticModemEnergyModelHelper::SetDepletionCallback(
    AcousticModemEnergyModel::AcousticModemEnergyDepletionCallback callback)
{
    m_depletionCallback = callback;
}

/*
 * Private function starts here.
 */

Ptr<energy::DeviceEnergyModel>
AcousticModemEnergyModelHelper::DoInstall(Ptr<NetDevice> device,
                                          Ptr<energy::EnergySource> source) const
{
    NS_ASSERT(device);
    NS_ASSERT(source);
    // check if device is UanNetDevice
    std::string deviceName = device->GetInstanceTypeId().GetName();
    if (deviceName != "ns3::UanNetDevice")
    {
        NS_FATAL_ERROR("NetDevice type is not UanNetDevice!");
    }
    Ptr<Node> node = device->GetNode();
    Ptr<AcousticModemEnergyModel> model = m_modemEnergy.Create<AcousticModemEnergyModel>();
    NS_ASSERT(model);
    // set node pointer
    model->SetNode(node);
    // set energy source pointer
    model->SetEnergySource(source);
    // get phy layer
    Ptr<UanNetDevice> uanDevice = DynamicCast<UanNetDevice>(device);
    Ptr<UanPhy> uanPhy = uanDevice->GetPhy();
    // set energy depletion callback
    model->SetEnergyDepletionCallback(m_depletionCallback);
    // add model to device model list in energy source
    source->AppendDeviceEnergyModel(model);
    // set node pointer
    source->SetNode(node);
    // create and install energy model callback
    energy::DeviceEnergyModel::ChangeStateCallback cb;
    cb = MakeCallback(&energy::DeviceEnergyModel::ChangeState, model);
    uanPhy->SetEnergyModelCallback(cb);

    return model;
}

} // namespace ns3
