/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "basic-energy-source-helper.h"

#include "ns3/energy-source.h"

namespace ns3
{

BasicEnergySourceHelper::BasicEnergySourceHelper()
{
    m_basicEnergySource.SetTypeId("ns3::energy::BasicEnergySource");
}

BasicEnergySourceHelper::~BasicEnergySourceHelper()
{
}

void
BasicEnergySourceHelper::Set(std::string name, const AttributeValue& v)
{
    m_basicEnergySource.Set(name, v);
}

Ptr<energy::EnergySource>
BasicEnergySourceHelper::DoInstall(Ptr<Node> node) const
{
    NS_ASSERT(node);
    Ptr<energy::EnergySource> source = m_basicEnergySource.Create<energy::EnergySource>();
    NS_ASSERT(source);
    source->SetNode(node);
    return source;
}

} // namespace ns3
