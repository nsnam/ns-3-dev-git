/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "energy-harvester.h"

#include "ns3/log.h"

namespace ns3
{
namespace energy
{

NS_LOG_COMPONENT_DEFINE("EnergyHarvester");
NS_OBJECT_ENSURE_REGISTERED(EnergyHarvester);

TypeId
EnergyHarvester::GetTypeId()
{
    static TypeId tid = TypeId("ns3::energy::EnergyHarvester")
                            .AddDeprecatedName("ns3::EnergyHarvester")
                            .SetParent<Object>()
                            .SetGroupName("Energy");
    return tid;
}

EnergyHarvester::EnergyHarvester()
{
    NS_LOG_FUNCTION(this);
}

EnergyHarvester::~EnergyHarvester()
{
    NS_LOG_FUNCTION(this);
}

void
EnergyHarvester::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(node);
    m_node = node;
}

Ptr<Node>
EnergyHarvester::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
EnergyHarvester::SetEnergySource(Ptr<EnergySource> source)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(source);
    m_energySource = source;
}

Ptr<EnergySource>
EnergyHarvester::GetEnergySource() const
{
    NS_LOG_FUNCTION(this);
    return m_energySource;
}

double
EnergyHarvester::GetPower() const
{
    NS_LOG_FUNCTION(this);
    return DoGetPower();
}

/*
 * Private function starts here.
 */

void
EnergyHarvester::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

double
EnergyHarvester::DoGetPower() const
{
    NS_LOG_FUNCTION(this);
    return 0.0;
}

} // namespace energy
} // namespace ns3
