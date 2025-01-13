/*
 * Copyright (c) 2012 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "buildings-helper.h"

#include "ns3/abort.h"
#include "ns3/building-list.h"
#include "ns3/building.h"
#include "ns3/log.h"
#include "ns3/mobility-building-info.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BuildingsHelper");

void
BuildingsHelper::Install(NodeContainer c)
{
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Install(*i);
    }
}

void
BuildingsHelper::Install(Ptr<Node> node)
{
    Ptr<Object> object = node;
    Ptr<MobilityModel> model = object->GetObject<MobilityModel>();

    NS_ABORT_MSG_UNLESS(model, "node " << node->GetId() << " does not have a MobilityModel");

    Ptr<MobilityBuildingInfo> buildingInfo = CreateObject<MobilityBuildingInfo>();
    model->AggregateObject(buildingInfo);
}

} // namespace ns3
