/*
 * Copyright (c) 2015, Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "mesh-stack-installer.h"

namespace ns3
{
NS_OBJECT_ENSURE_REGISTERED(MeshStack);

TypeId
MeshStack::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MeshStack").SetParent<Object>().SetGroupName("Mesh")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

} // namespace ns3
