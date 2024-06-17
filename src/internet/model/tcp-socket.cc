/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#define __STDC_LIMIT_MACROS

#include "tcp-socket.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpSocket");

NS_OBJECT_ENSURE_REGISTERED(TcpSocket);

const char* const TcpSocket::TcpStateName[TcpSocket::LAST_STATE] = {
    "CLOSED",
    "LISTEN",
    "SYN_SENT",
    "SYN_RCVD",
    "ESTABLISHED",
    "CLOSE_WAIT",
    "LAST_ACK",
    "FIN_WAIT_1",
    "FIN_WAIT_2",
    "CLOSING",
    "TIME_WAIT",
};

TypeId
TcpSocket::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpSocket")
            .SetParent<Socket>()
            .SetGroupName("Internet")
            .AddAttribute(
                "SndBufSize",
                "TcpSocket maximum transmit buffer size (bytes)",
                UintegerValue(131072), // 128k
                MakeUintegerAccessor(&TcpSocket::GetSndBufSize, &TcpSocket::SetSndBufSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "RcvBufSize",
                "TcpSocket maximum receive buffer size (bytes)",
                UintegerValue(131072),
                MakeUintegerAccessor(&TcpSocket::GetRcvBufSize, &TcpSocket::SetRcvBufSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "SegmentSize",
                "TCP maximum segment size in bytes (may be adjusted based on MTU discovery)",
                UintegerValue(536),
                MakeUintegerAccessor(&TcpSocket::GetSegSize, &TcpSocket::SetSegSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("InitialSlowStartThreshold",
                          "TCP initial slow start threshold (bytes)",
                          UintegerValue(UINT32_MAX),
                          MakeUintegerAccessor(&TcpSocket::GetInitialSSThresh,
                                               &TcpSocket::SetInitialSSThresh),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "InitialCwnd",
                "TCP initial congestion window size (segments)",
                UintegerValue(10),
                MakeUintegerAccessor(&TcpSocket::GetInitialCwnd, &TcpSocket::SetInitialCwnd),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("ConnTimeout",
                          "TCP retransmission timeout when opening connection (seconds)",
                          TimeValue(Seconds(3)),
                          MakeTimeAccessor(&TcpSocket::GetConnTimeout, &TcpSocket::SetConnTimeout),
                          MakeTimeChecker())
            .AddAttribute(
                "ConnCount",
                "Number of connection attempts (SYN retransmissions) before "
                "returning failure",
                UintegerValue(6),
                MakeUintegerAccessor(&TcpSocket::GetSynRetries, &TcpSocket::SetSynRetries),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "DataRetries",
                "Number of data retransmission attempts",
                UintegerValue(6),
                MakeUintegerAccessor(&TcpSocket::GetDataRetries, &TcpSocket::SetDataRetries),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "DelAckTimeout",
                "Timeout value for TCP delayed acks, in seconds",
                TimeValue(Seconds(0.2)),
                MakeTimeAccessor(&TcpSocket::GetDelAckTimeout, &TcpSocket::SetDelAckTimeout),
                MakeTimeChecker())
            .AddAttribute(
                "DelAckCount",
                "Number of packets to wait before sending a TCP ack",
                UintegerValue(2),
                MakeUintegerAccessor(&TcpSocket::GetDelAckMaxCount, &TcpSocket::SetDelAckMaxCount),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("TcpNoDelay",
                          "Set to true to disable Nagle's algorithm",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpSocket::GetTcpNoDelay, &TcpSocket::SetTcpNoDelay),
                          MakeBooleanChecker())
            .AddAttribute(
                "PersistTimeout",
                "Persist timeout to probe for rx window",
                TimeValue(Seconds(6)),
                MakeTimeAccessor(&TcpSocket::GetPersistTimeout, &TcpSocket::SetPersistTimeout),
                MakeTimeChecker());
    return tid;
}

TcpSocket::TcpSocket()
{
    NS_LOG_FUNCTION(this);
}

TcpSocket::~TcpSocket()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
