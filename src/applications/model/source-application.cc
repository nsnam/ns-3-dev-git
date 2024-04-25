/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "source-application.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SourceApplication");

NS_OBJECT_ENSURE_REGISTERED(SourceApplication);

TypeId
SourceApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SourceApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddAttribute(
                "Remote",
                "The address of the destination, made of the remote IP address and the "
                "destination port",
                AddressValue(),
                MakeAddressAccessor(&SourceApplication::SetRemote, &SourceApplication::GetRemote),
                MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically when needed by the application.",
                          AddressValue(),
                          MakeAddressAccessor(&SourceApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&SourceApplication::m_tos),
                          MakeUintegerChecker<uint8_t>());
    return tid;
}

SourceApplication::SourceApplication()
    : m_peer{}
{
    NS_LOG_FUNCTION(this);
}

SourceApplication::~SourceApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
    }
}

Address
SourceApplication::GetRemote() const
{
    return m_peer;
}

} // Namespace ns3
