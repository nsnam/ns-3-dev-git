/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "sixlowpan-simple-flooding.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanSimpleFlooding");
NS_OBJECT_ENSURE_REGISTERED(SixLowPanSimpleFlooding);

TypeId
SixLowPanSimpleFlooding::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanSimpleFlooding")
            .SetParent<SixLowPanMeshUnderRouting>()
            .SetGroupName("SixLowPan")
            .AddConstructor<SixLowPanSimpleFlooding>()
            .AddAttribute("MeshUnderJitter",
                          "The jitter (in ms) before each mesh-under rebroadcast. "
                          "Same name and default as the historical SixLowPanNetDevice "
                          "attribute, preserving backward-compatible behavior.",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
                          MakePointerAccessor(&SixLowPanSimpleFlooding::m_jitter),
                          MakePointerChecker<RandomVariableStream>());
    return tid;
}

SixLowPanSimpleFlooding::SixLowPanSimpleFlooding()
{
    NS_LOG_FUNCTION(this);
}

SixLowPanSimpleFlooding::~SixLowPanSimpleFlooding()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanSimpleFlooding::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_jitter = nullptr;
    SixLowPanMeshUnderRouting::DoDispose();
}

void
SixLowPanSimpleFlooding::OnPacketForward(Ptr<Packet> packet,
                                         const Address& originator,
                                         uint8_t seqNo,
                                         uint8_t hopsLeft,
                                         ForwardCallback forwardCb)
{
    NS_LOG_FUNCTION(this << packet << originator << +seqNo << +hopsLeft);
    Time jitter = MilliSeconds(m_jitter->GetValue());
    // Rebroadcast after the jitter delay. Simulator::Schedule does not
    // accept a Callback with unbound arguments, hence the small lambda.
    Simulator::Schedule(jitter, [forwardCb, packet]() { forwardCb(packet); });
}

int64_t
SixLowPanSimpleFlooding::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_jitter->SetStream(stream);
    return 1;
}

} // namespace ns3
