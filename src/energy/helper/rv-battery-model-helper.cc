/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "rv-battery-model-helper.h"

#include "ns3/energy-source.h"

namespace ns3
{

RvBatteryModelHelper::RvBatteryModelHelper()
{
    m_rvBatteryModel.SetTypeId("ns3::energy::RvBatteryModel");
}

RvBatteryModelHelper::~RvBatteryModelHelper()
{
}

void
RvBatteryModelHelper::Set(std::string name, const AttributeValue& v)
{
    m_rvBatteryModel.Set(name, v);
}

Ptr<energy::EnergySource>
RvBatteryModelHelper::DoInstall(Ptr<Node> node) const
{
    NS_ASSERT(node);
    Ptr<energy::EnergySource> source = m_rvBatteryModel.Create<energy::EnergySource>();
    NS_ASSERT(source);
    source->SetNode(node);
    return source;
}

} // namespace ns3
