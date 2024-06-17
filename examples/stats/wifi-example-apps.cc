/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 *
 * These applications are used in the WiFi Distance Test experiment,
 * described and implemented in test02.cc.  That file should be in the
 * same place as this file.  The applications have two very simple
 * jobs, they just generate and receive packets.  We could use the
 * standard Application classes included in the NS-3 distribution.
 * These have been written just to change the behavior a little, and
 * provide more examples.
 *
 */

#include "wifi-example-apps.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/stats-module.h"

#include <ostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WiFiDistanceApps");

// ==============================================
// SENDER
// ==============================================

TypeId
Sender::GetTypeId()
{
    static TypeId tid = TypeId("Sender")
                            .SetParent<Application>()
                            .AddConstructor<Sender>()
                            .AddAttribute("PacketSize",
                                          "The size of packets transmitted.",
                                          UintegerValue(64),
                                          MakeUintegerAccessor(&Sender::m_packetSize),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("Destination",
                                          "Target host address.",
                                          Ipv4AddressValue("255.255.255.255"),
                                          MakeIpv4AddressAccessor(&Sender::m_destAddr),
                                          MakeIpv4AddressChecker())
                            .AddAttribute("Port",
                                          "Destination app port.",
                                          UintegerValue(1603),
                                          MakeUintegerAccessor(&Sender::m_destPort),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("NumPackets",
                                          "Total number of packets to send.",
                                          UintegerValue(30),
                                          MakeUintegerAccessor(&Sender::m_nPackets),
                                          MakeUintegerChecker<uint32_t>(1))
                            .AddAttribute("Interval",
                                          "Delay between transmissions.",
                                          StringValue("ns3::ConstantRandomVariable[Constant=0.5]"),
                                          MakePointerAccessor(&Sender::m_interval),
                                          MakePointerChecker<RandomVariableStream>())
                            .AddTraceSource("Tx",
                                            "A new packet is created and is sent",
                                            MakeTraceSourceAccessor(&Sender::m_txTrace),
                                            "ns3::Packet::TracedCallback");
    return tid;
}

Sender::Sender()
{
    NS_LOG_FUNCTION_NOARGS();
    m_interval = CreateObject<ConstantRandomVariable>();
}

Sender::~Sender()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Sender::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();

    m_socket = nullptr;
    // chain up
    Application::DoDispose();
}

void
Sender::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS();

    if (!m_socket)
    {
        Ptr<SocketFactory> socketFactory =
            GetNode()->GetObject<SocketFactory>(UdpSocketFactory::GetTypeId());
        m_socket = socketFactory->CreateSocket();
        m_socket->Bind();
    }

    m_count = 0;

    Simulator::Cancel(m_sendEvent);
    m_sendEvent = Simulator::ScheduleNow(&Sender::SendPacket, this);
}

void
Sender::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    Simulator::Cancel(m_sendEvent);
}

void
Sender::SendPacket()
{
    // NS_LOG_FUNCTION_NOARGS();
    NS_LOG_INFO("Sending packet at " << Simulator::Now() << " to " << m_destAddr);

    Ptr<Packet> packet = Create<Packet>(m_packetSize);

    TimestampTag timestamp;
    timestamp.SetTimestamp(Simulator::Now());
    packet->AddByteTag(timestamp);

    // Could connect the socket since the address never changes; using SendTo
    // here simply because all of the standard apps do not.
    m_socket->SendTo(packet, 0, InetSocketAddress(m_destAddr, m_destPort));

    // Report the event to the trace.
    m_txTrace(packet);

    if (++m_count < m_nPackets)
    {
        m_sendEvent =
            Simulator::Schedule(Seconds(m_interval->GetValue()), &Sender::SendPacket, this);
    }
}

// ==============================================
// RECEIVER
// ==============================================

TypeId
Receiver::GetTypeId()
{
    static TypeId tid = TypeId("Receiver")
                            .SetParent<Application>()
                            .AddConstructor<Receiver>()
                            .AddAttribute("Port",
                                          "Listening port.",
                                          UintegerValue(1603),
                                          MakeUintegerAccessor(&Receiver::m_port),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

Receiver::Receiver()
{
    NS_LOG_FUNCTION_NOARGS();
}

Receiver::~Receiver()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Receiver::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();

    m_socket = nullptr;
    // chain up
    Application::DoDispose();
}

void
Receiver::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS();

    if (!m_socket)
    {
        Ptr<SocketFactory> socketFactory =
            GetNode()->GetObject<SocketFactory>(UdpSocketFactory::GetTypeId());
        m_socket = socketFactory->CreateSocket();
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        m_socket->Bind(local);
    }

    m_socket->SetRecvCallback(MakeCallback(&Receiver::Receive, this));
}

void
Receiver::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
Receiver::SetCounter(Ptr<CounterCalculator<>> calc)
{
    m_calc = calc;
}

void
Receiver::SetDelayTracker(Ptr<TimeMinMaxAvgTotalCalculator> delay)
{
    m_delay = delay;
}

void
Receiver::Receive(Ptr<Socket> socket)
{
    // NS_LOG_FUNCTION (this << socket << packet << from);

    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        if (InetSocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("Received " << packet->GetSize() << " bytes from "
                                    << InetSocketAddress::ConvertFrom(from).GetIpv4());
        }

        TimestampTag timestamp;
        // Should never not be found since the sender is adding it, but
        // you never know.
        if (packet->FindFirstMatchingByteTag(timestamp))
        {
            Time tx = timestamp.GetTimestamp();

            if (m_delay)
            {
                m_delay->Update(Simulator::Now() - tx);
            }
        }

        if (m_calc)
        {
            m_calc->Update();
        }
    }
}
