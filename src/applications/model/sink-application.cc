/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "sink-application.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SinkApplication");

NS_OBJECT_ENSURE_REGISTERED(SinkApplication);

TypeId
SinkApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SinkApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddAttribute(
                "Local",
                "The Address on which to Bind the rx socket. "
                "If it is not specified, it will listen to any address.",
                AddressValue(),
                MakeAddressAccessor(&SinkApplication::SetLocal, &SinkApplication::GetLocal),
                MakeAddressChecker())
            .AddAttribute(
                "Port",
                "Port on which the application listens for incoming packets.",
                UintegerValue(INVALID_PORT),
                MakeUintegerAccessor(&SinkApplication::SetPort, &SinkApplication::GetPort),
                MakeUintegerChecker<uint32_t>());
    return tid;
}

SinkApplication::SinkApplication(uint16_t defaultPort)
    : m_port{defaultPort}
{
    NS_LOG_FUNCTION(this << defaultPort);
}

SinkApplication::~SinkApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SinkApplication::SetLocal(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_local = addr;
}

Address
SinkApplication::GetLocal() const
{
    return m_local;
}

void
SinkApplication::SetPort(uint32_t port)
{
    NS_LOG_FUNCTION(this << port);
    if (port == INVALID_PORT)
    {
        return;
    }
    m_port = port;
}

uint32_t
SinkApplication::GetPort() const
{
    return m_port;
}

} // Namespace ns3
