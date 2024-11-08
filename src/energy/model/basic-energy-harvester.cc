/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "basic-energy-harvester.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("BasicEnergyHarvester");
NS_OBJECT_ENSURE_REGISTERED(BasicEnergyHarvester);

TypeId
BasicEnergyHarvester::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::energy::BasicEnergyHarvester")
            .AddDeprecatedName("ns3::BasicEnergyHarvester")
            .SetParent<EnergyHarvester>()
            .SetGroupName("Energy")
            .AddConstructor<BasicEnergyHarvester>()
            .AddAttribute("PeriodicHarvestedPowerUpdateInterval",
                          "Time between two consecutive periodic updates of the harvested power. "
                          "By default, the value is updated every 1 s",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&BasicEnergyHarvester::SetHarvestedPowerUpdateInterval,
                                           &BasicEnergyHarvester::GetHarvestedPowerUpdateInterval),
                          MakeTimeChecker())
            .AddAttribute("HarvestablePower",
                          "The harvestable power [Watts] that the energy harvester is allowed to "
                          "harvest. By default, the model will allow to harvest an amount of power "
                          "defined by a uniformly distributed random variable in 0 and 2.0 Watts",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=2.0]"),
                          MakePointerAccessor(&BasicEnergyHarvester::m_harvestablePower),
                          MakePointerChecker<RandomVariableStream>())
            .AddTraceSource("HarvestedPower",
                            "Harvested power by the BasicEnergyHarvester.",
                            MakeTraceSourceAccessor(&BasicEnergyHarvester::m_harvestedPower),
                            "ns3::TracedValueCallback::Double")
            .AddTraceSource("TotalEnergyHarvested",
                            "Total energy harvested by the harvester.",
                            MakeTraceSourceAccessor(&BasicEnergyHarvester::m_totalEnergyHarvestedJ),
                            "ns3::TracedValueCallback::Double");
    return tid;
}

BasicEnergyHarvester::BasicEnergyHarvester()
{
    NS_LOG_FUNCTION(this);
}

BasicEnergyHarvester::BasicEnergyHarvester(Time updateInterval)
{
    NS_LOG_FUNCTION(this << updateInterval);
    m_harvestedPowerUpdateInterval = updateInterval;
}

BasicEnergyHarvester::~BasicEnergyHarvester()
{
    NS_LOG_FUNCTION(this);
}

int64_t
BasicEnergyHarvester::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_harvestablePower->SetStream(stream);
    return 1;
}

void
BasicEnergyHarvester::SetHarvestedPowerUpdateInterval(Time updateInterval)
{
    NS_LOG_FUNCTION(this << updateInterval);
    m_harvestedPowerUpdateInterval = updateInterval;
}

Time
BasicEnergyHarvester::GetHarvestedPowerUpdateInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_harvestedPowerUpdateInterval;
}

/*
 * Private functions start here.
 */

void
BasicEnergyHarvester::UpdateHarvestedPower()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG(Simulator::Now().As(Time::S) << " BasicEnergyHarvester(" << GetNode()->GetId()
                                              << "): Updating harvesting power.");

    Time duration = Simulator::Now() - m_lastHarvestingUpdateTime;

    NS_ASSERT(duration.GetNanoSeconds() >= 0); // check if duration is valid

    double energyHarvested = 0.0;

    // do not update if simulation has finished
    if (Simulator::IsFinished())
    {
        NS_LOG_DEBUG("BasicEnergyHarvester: Simulation Finished.");
        return;
    }

    m_energyHarvestingUpdateEvent.Cancel();

    CalculateHarvestedPower();

    energyHarvested = duration.GetSeconds() * m_harvestedPower;

    // update total energy harvested
    m_totalEnergyHarvestedJ += energyHarvested;

    // notify energy source
    GetEnergySource()->UpdateEnergySource();

    // update last harvesting time stamp
    m_lastHarvestingUpdateTime = Simulator::Now();

    m_energyHarvestingUpdateEvent = Simulator::Schedule(m_harvestedPowerUpdateInterval,
                                                        &BasicEnergyHarvester::UpdateHarvestedPower,
                                                        this);
}

void
BasicEnergyHarvester::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    m_lastHarvestingUpdateTime = Simulator::Now();

    UpdateHarvestedPower(); // start periodic harvesting update
}

void
BasicEnergyHarvester::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
BasicEnergyHarvester::CalculateHarvestedPower()
{
    NS_LOG_FUNCTION(this);

    m_harvestedPower = m_harvestablePower->GetValue();

    NS_LOG_DEBUG(Simulator::Now().As(Time::S)
                 << " BasicEnergyHarvester:Harvested energy = " << m_harvestedPower);
}

double
BasicEnergyHarvester::DoGetPower() const
{
    NS_LOG_FUNCTION(this);
    return m_harvestedPower;
}

} // namespace energy
} // namespace ns3
