/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "wraparound-model.h"

#include "ns3/log.h"
#include "ns3/mobility-building-info.h"
#include "ns3/node.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WraparoundModel");
NS_OBJECT_ENSURE_REGISTERED(WraparoundModel);

TypeId
WraparoundModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WraparoundModel")
                            .SetParent<Object>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<WraparoundModel>();
    return tid;
}

Ptr<MobilityModel>
WraparoundModel::GetVirtualMobilityModel(Ptr<const MobilityModel> tx,
                                         Ptr<const MobilityModel> rx) const
{
    NS_LOG_DEBUG("Transmitter using virtual mobility model. Real position "
                 << tx->GetPosition() << ", receiver position " << rx->GetPosition()
                 << ", wrapped position "
                 << GetVirtualPosition(tx->GetPosition(), rx->GetPosition()) << ".");
    auto virtualMm = tx->Copy();
    // Set the transmitter to its virtual position respective to receiver
    virtualMm->SetPosition(GetVirtualPosition(tx->GetPosition(), rx->GetPosition()));

    // Unidirectionally aggregate NodeId to it, so it can be fetched later by
    // propagation models
    auto node = tx->GetObject<Node>();
    if (node)
    {
        virtualMm->UnidirectionalAggregateObject(node);
    }

    // Some mobility models access building info related to mobility model
    auto mbi = tx->GetObject<MobilityBuildingInfo>();
    if (mbi)
    {
        virtualMm->UnidirectionalAggregateObject(mbi);
    }
    return virtualMm;
}

Vector3D
WraparoundModel::GetVirtualPosition(const Vector3D tx, const Vector3D rx) const
{
    return tx; // Placeholder, you are supposed to implement whatever wraparound model you want
}
