/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Raj Bhattacharjea <raj.b@gatech.edu>
 */
#include "tcp-socket-factory-impl.h"

#include "tcp-l4-protocol.h"

#include "ns3/assert.h"
#include "ns3/socket.h"

namespace ns3
{

TcpSocketFactoryImpl::TcpSocketFactoryImpl()
    : m_tcp(nullptr)
{
}

TcpSocketFactoryImpl::~TcpSocketFactoryImpl()
{
    NS_ASSERT(!m_tcp);
}

void
TcpSocketFactoryImpl::SetTcp(Ptr<TcpL4Protocol> tcp)
{
    m_tcp = tcp;
}

Ptr<Socket>
TcpSocketFactoryImpl::CreateSocket()
{
    return m_tcp->CreateSocket();
}

void
TcpSocketFactoryImpl::DoDispose()
{
    m_tcp = nullptr;
    TcpSocketFactory::DoDispose();
}

} // namespace ns3
