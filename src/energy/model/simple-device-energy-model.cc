/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "simple-device-energy-model.h"

#include "energy-source.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("SimpleDeviceEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(SimpleDeviceEnergyModel);

TypeId
SimpleDeviceEnergyModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::energy::SimpleDeviceEnergyModel")
                            .AddDeprecatedName("ns3::SimpleDeviceEnergyModel")
                            .SetParent<DeviceEnergyModel>()
                            .SetGroupName("Energy")
                            .AddConstructor<SimpleDeviceEnergyModel>()
                            .AddTraceSource("TotalEnergyConsumption",
                                            "Total energy consumption of the radio device.",
                                            MakeTraceSourceAccessor(
                                                &SimpleDeviceEnergyModel::m_totalEnergyConsumption),
                                            "ns3::TracedValueCallback::Double");
    return tid;
}

SimpleDeviceEnergyModel::SimpleDeviceEnergyModel()
{
    NS_LOG_FUNCTION(this);
    m_lastUpdateTime = Seconds(0);
    m_actualCurrentA = 0.0;
    m_source = nullptr;
}

SimpleDeviceEnergyModel::~SimpleDeviceEnergyModel()
{
    NS_LOG_FUNCTION(this);
}

void
SimpleDeviceEnergyModel::SetEnergySource(Ptr<EnergySource> source)
{
    NS_LOG_FUNCTION(this << source);
    NS_ASSERT(source);
    m_source = source;
}

void
SimpleDeviceEnergyModel::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    NS_ASSERT(node);
    m_node = node;
}

Ptr<Node>
SimpleDeviceEnergyModel::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

double
SimpleDeviceEnergyModel::GetTotalEnergyConsumption() const
{
    NS_LOG_FUNCTION(this);
    Time duration = Simulator::Now() - m_lastUpdateTime;

    double energyToDecrease = 0.0;
    double supplyVoltage = m_source->GetSupplyVoltage();
    energyToDecrease = duration.GetSeconds() * m_actualCurrentA * supplyVoltage;

    m_source->UpdateEnergySource();

    return m_totalEnergyConsumption + energyToDecrease;
}

void
SimpleDeviceEnergyModel::SetCurrentA(double current)
{
    NS_LOG_FUNCTION(this << current);
    Time duration = Simulator::Now() - m_lastUpdateTime;

    double energyToDecrease = 0.0;
    double supplyVoltage = m_source->GetSupplyVoltage();
    energyToDecrease = duration.GetSeconds() * m_actualCurrentA * supplyVoltage;

    // update total energy consumption
    m_totalEnergyConsumption += energyToDecrease;
    // update last update time stamp
    m_lastUpdateTime = Simulator::Now();
    // update the current drain
    m_actualCurrentA = current;
    // notify energy source
    m_source->UpdateEnergySource();
}

void
SimpleDeviceEnergyModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_source = nullptr;
}

double
SimpleDeviceEnergyModel::DoGetCurrentA() const
{
    NS_LOG_FUNCTION(this);
    return m_actualCurrentA;
}

} // namespace energy
} // namespace ns3
