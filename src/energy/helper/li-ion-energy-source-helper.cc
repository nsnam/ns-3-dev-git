/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Hoda Ayatollahi <hayatoll@ur.rochester.edu>
 *          Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#include "li-ion-energy-source-helper.h"

#include "ns3/energy-source.h"

namespace ns3
{

LiIonEnergySourceHelper::LiIonEnergySourceHelper()
{
    m_liIonEnergySource.SetTypeId("ns3::energy::LiIonEnergySource");
}

LiIonEnergySourceHelper::~LiIonEnergySourceHelper()
{
}

void
LiIonEnergySourceHelper::Set(std::string name, const AttributeValue& v)
{
    m_liIonEnergySource.Set(name, v);
}

Ptr<energy::EnergySource>
LiIonEnergySourceHelper::DoInstall(Ptr<Node> node) const
{
    NS_ASSERT(node);
    Ptr<energy::EnergySource> source = m_liIonEnergySource.Create<energy::EnergySource>();
    NS_ASSERT(source);
    source->SetNode(node);
    return source;
}

} // namespace ns3
