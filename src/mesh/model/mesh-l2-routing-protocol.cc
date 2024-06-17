/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "mesh-l2-routing-protocol.h"

#include "mesh-point-device.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MeshL2RoutingProtocol");

NS_OBJECT_ENSURE_REGISTERED(MeshL2RoutingProtocol);

TypeId
MeshL2RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MeshL2RoutingProtocol").SetParent<Object>().SetGroupName("Mesh");
    return tid;
}

MeshL2RoutingProtocol::~MeshL2RoutingProtocol()
{
    m_mp = nullptr;
}

void
MeshL2RoutingProtocol::SetMeshPoint(Ptr<MeshPointDevice> mp)
{
    m_mp = mp;
}

Ptr<MeshPointDevice>
MeshL2RoutingProtocol::GetMeshPoint() const
{
    return m_mp;
}

} // namespace ns3
