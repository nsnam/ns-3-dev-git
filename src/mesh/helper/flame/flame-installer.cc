/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "flame-installer.h"

#include "ns3/flame-protocol.h"
#include "ns3/mesh-wifi-interface-mac.h"

namespace ns3
{
using namespace flame;
NS_OBJECT_ENSURE_REGISTERED(FlameStack);

TypeId
FlameStack::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FlameStack")
                            .SetParent<MeshStack>()
                            .SetGroupName("Mesh")
                            .AddConstructor<FlameStack>();
    return tid;
}

FlameStack::FlameStack()
{
}

FlameStack::~FlameStack()
{
}

void
FlameStack::DoDispose()
{
}

bool
FlameStack::InstallStack(Ptr<MeshPointDevice> mp)
{
    Ptr<FlameProtocol> flame = CreateObject<FlameProtocol>();
    return flame->Install(mp);
}

void
FlameStack::Report(const Ptr<MeshPointDevice> mp, std::ostream& os)
{
    mp->Report(os);
    /// @todo report flame counters
    Ptr<FlameProtocol> flame = mp->GetObject<FlameProtocol>();
    NS_ASSERT(flame);
    flame->Report(os);
}

void
FlameStack::ResetStats(const Ptr<MeshPointDevice> mp)
{
    mp->ResetStats();
    /// @todo reset flame counters
    Ptr<FlameProtocol> flame = mp->GetObject<FlameProtocol>();
    NS_ASSERT(flame);

    flame->ResetStats();
}
} // namespace ns3
