/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "voip-traffic.h"

#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VoipTraffic");

NS_OBJECT_ENSURE_REGISTERED(VoipTraffic);

TypeId
VoipTraffic::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VoipTraffic")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<VoipTraffic>()
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                TypeIdValue(TypeId::LookupByName("ns3::PacketSocketFactory")),
                MakeTypeIdAccessor(&VoipTraffic::m_tid),
                MakeTypeIdChecker())
            .AddAttribute(
                "ActivePacketPayloadSize",
                "Size in bytes for payload of packets generated during periods of active talking.",
                UintegerValue(33),
                MakeUintegerAccessor(&VoipTraffic::m_activePacketSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "SilencePacketPayloadSize",
                "Size in bytes for payload of packets generated during periods of silence.",
                UintegerValue(7),
                MakeUintegerAccessor(&VoipTraffic::m_silencePacketSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("MeanActiveStateDuration",
                          "Mean duration of active/talking state.",
                          TimeValue(MilliSeconds(1250)),
                          MakeTimeAccessor(&VoipTraffic::SetActiveExponentialMean),
                          MakeTimeChecker())
            .AddAttribute("MeanInactiveStateDuration",
                          "Mean duration of inactive/silence state.",
                          TimeValue(MilliSeconds(1250)),
                          MakeTimeAccessor(&VoipTraffic::SetInactiveExponentialMean),
                          MakeTimeChecker())
            .AddAttribute("VoiceEncoderInterval",
                          "Interval between generation of voice packets.",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&VoipTraffic::m_voiceInterval),
                          MakeTimeChecker())
            .AddAttribute(
                "SilenceEncoderInterval",
                "Interval between generation of silence packets. "
                "This should be a multiple of the interval between generation of voice packets",
                TimeValue(MilliSeconds(160)),
                MakeTimeAccessor(&VoipTraffic::m_silenceInterval),
                MakeTimeChecker())
            .AddAttribute(
                "VoiceToSilenceProbability",
                "The probability to transition from active talking state to silence state.",
                DoubleValue(0.016),
                MakeDoubleAccessor(&VoipTraffic::m_activeToInactive),
                MakeDoubleChecker<double>(0, 1))
            .AddAttribute(
                "SilenceToVoiceProbability",
                "The probability to transition from silence state to active talking state.",
                DoubleValue(0.016),
                MakeDoubleAccessor(&VoipTraffic::m_inactiveToActive),
                MakeDoubleChecker<double>(0, 1))
            .AddAttribute("MeanDelayJitter",
                          "Mean of laplacian distribution used to calculate delay jitter for "
                          "generated packets.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&VoipTraffic::m_laplacianLocation),
                          MakeTimeChecker())
            .AddAttribute("ScaleDelayJitter",
                          "Scale of laplacian distribution used to calculate delay jitter for "
                          "generated packets.",
                          TimeValue(MicroSeconds(5110)),
                          MakeTimeAccessor(&VoipTraffic::m_laplacianScale),
                          MakeTimeChecker())
            .AddAttribute("BoundDelayJitter",
                          "Bound of laplacian distribution used to calculate delay jitter for "
                          "generated packets.",
                          TimeValue(MilliSeconds(80)),
                          MakeTimeAccessor(&VoipTraffic::m_laplacianBound),
                          MakeTimeChecker())
            .AddTraceSource(
                "TxWithJitter",
                "A VoIP packet is sent, this trace also reports the jitter applied to the packet",
                MakeTraceSourceAccessor(&VoipTraffic::m_txJitterTrace),
                "ns3::VoipTraffic::TxTracedCallback")
            .AddTraceSource("StateUpdate",
                            "Voice activity state updated",
                            MakeTraceSourceAccessor(&VoipTraffic::m_stateUpdate),
                            "ns3::VoipTraffic::StateUpdatedCallback");
    return tid;
}

VoipTraffic::VoipTraffic()
    : m_socket{nullptr},
      m_connected{false},
      m_inactiveExponential{CreateObject<ExponentialRandomVariable>()},
      m_activeExponential{CreateObject<ExponentialRandomVariable>()},
      m_inactiveUniform{CreateObject<UniformRandomVariable>()},
      m_activeUniform{CreateObject<UniformRandomVariable>()},
      m_laplacian{CreateObject<LaplacianRandomVariable>()},
      m_currentState{VoiceActivityState::INACTIVE_SILENCE},
      m_pendingStateTransition{false},
      m_remainingStateDuration{Seconds(0)},
      m_remainingEncodingDuration{Seconds(0)},
      m_nextEventId{0}
{
    NS_LOG_FUNCTION(this);
}

VoipTraffic::~VoipTraffic()
{
    NS_LOG_FUNCTION(this);
}

int64_t
VoipTraffic::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_inactiveExponential->SetStream(currentStream++);
    m_activeExponential->SetStream(currentStream++);
    m_inactiveUniform->SetStream(currentStream++);
    m_activeUniform->SetStream(currentStream++);
    m_laplacian->SetStream(currentStream++);
    return (currentStream - stream);
}

void
VoipTraffic::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    Application::DoDispose();
}

void
VoipTraffic::SetActiveExponentialMean(Time mean)
{
    NS_LOG_FUNCTION(this << mean);
    m_activeExponential->SetAttribute("Mean", DoubleValue(mean.GetMilliSeconds()));
}

void
VoipTraffic::SetInactiveExponentialMean(Time mean)
{
    NS_LOG_FUNCTION(this << mean);
    m_inactiveExponential->SetAttribute("Mean", DoubleValue(mean.GetMilliSeconds()));
}

void
VoipTraffic::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);

        int ret = -1;
        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            ret = m_socket->Bind(m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind6();
            }
            else if (InetSocketAddress::IsMatchingType(m_peer) ||
                     PacketSocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_socket->SetConnectCallback(MakeCallback(&VoipTraffic::ConnectionSucceeded, this),
                                     MakeCallback(&VoipTraffic::ConnectionFailed, this));

        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
        }

        m_socket->Connect(m_peer);
        m_socket->SetAllowBroadcast(true);
        m_socket->ShutdownRecv();
    }

    CancelEvents();

    if (m_connected)
    {
        ScheduleNext();
    }
}

void
VoipTraffic::StopApplication()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
VoipTraffic::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    for (auto& event : m_txPacketEvents)
    {
        event.second.Cancel();
    }
    m_txPacketEvents.clear();
    m_stateUpdateEvent.Cancel();
    m_remainingEncodingDuration = GetEncoderFrameDuration();
}

Time
VoipTraffic::GetEncoderFrameDuration() const
{
    return (m_currentState == VoiceActivityState::ACTIVE_TALKING) ? m_voiceInterval
                                                                  : m_silenceInterval;
}

Time
VoipTraffic::GetStateUpdateInterval() const
{
    NS_ABORT_MSG_IF(
        !(m_silenceInterval % m_voiceInterval).IsZero(),
        "Silence encoder frame duration should be a multiple of voice encoder frame duration");
    return m_voiceInterval;
}

void
VoipTraffic::ScheduleNext()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_stateUpdateEvent.IsPending());
    m_stateUpdateEvent =
        Simulator::Schedule(GetStateUpdateInterval(), &VoipTraffic::UpdateState, this);
}

void
VoipTraffic::SendPacket(uint64_t eventId, Ptr<Packet> packet, Time jitter)
{
    const auto size = packet->GetSize();
    NS_LOG_FUNCTION(this << eventId << size << jitter);

    auto it = m_txPacketEvents.find(eventId);
    NS_ASSERT(it != m_txPacketEvents.end());
    NS_ASSERT(it->second.IsExpired());

    const auto actualSize = static_cast<uint32_t>(m_socket->Send(packet));
    NS_ABORT_IF(actualSize != size);
    m_txTrace(packet);
    m_txJitterTrace(packet, jitter);

    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " voip traffic source sent "
                               << size << " bytes to "
                               << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                               << InetSocketAddress::ConvertFrom(m_peer).GetPort());
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " voip traffic source sent "
                               << size << " bytes to "
                               << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                               << Inet6SocketAddress::ConvertFrom(m_peer).GetPort());
    }

    m_txPacketEvents.erase(it);
}

void
VoipTraffic::UpdateState()
{
    NS_LOG_FUNCTION(this);

    m_remainingStateDuration -= GetStateUpdateInterval();
    m_remainingEncodingDuration -= GetStateUpdateInterval();
    NS_ASSERT(!m_remainingEncodingDuration.IsStrictlyNegative());

    auto newState = m_currentState;
    if (m_currentState == VoiceActivityState::INACTIVE_SILENCE)
    {
        if (m_inactiveUniform->GetValue() >= (1 - m_inactiveToActive))
        {
            newState = VoiceActivityState::ACTIVE_TALKING;
        }
    }
    else
    {
        if (m_activeUniform->GetValue() >= (1 - m_activeToInactive))
        {
            newState = VoiceActivityState::INACTIVE_SILENCE;
        }
    }
    m_pendingStateTransition = m_pendingStateTransition || (newState != m_currentState);

    if (!m_remainingStateDuration.IsStrictlyPositive() && m_remainingEncodingDuration.IsZero())
    {
        if (m_pendingStateTransition)
        {
            newState = (m_currentState == VoiceActivityState::ACTIVE_TALKING)
                           ? VoiceActivityState::INACTIVE_SILENCE
                           : VoiceActivityState::ACTIVE_TALKING;
        }
        m_remainingStateDuration = (newState == VoiceActivityState::ACTIVE_TALKING)
                                       ? MilliSeconds(m_activeExponential->GetValue())
                                       : MilliSeconds(m_inactiveExponential->GetValue());
        m_stateUpdate(newState, m_remainingStateDuration);
        if ((newState != m_currentState))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                                   << " voip voice activity state changed from " << m_currentState
                                   << " to " << newState << " for "
                                   << m_remainingStateDuration.As(Time::MS));
            m_currentState = newState;
        }
        else
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                                   << " voip voice activity state unchanged to " << m_currentState
                                   << " for " << m_remainingStateDuration.As(Time::MS));
        }
        m_pendingStateTransition = false;
    }

    if (m_remainingEncodingDuration.IsZero())
    {
        m_remainingEncodingDuration = GetEncoderFrameDuration();
        auto delay = GetEncoderFrameDuration();
        Time jitter{};
        if (!m_laplacianBound.IsZero())
        {
            jitter = MicroSeconds(
                static_cast<int64x64_t>(m_laplacian->GetValue(m_laplacianLocation.GetMicroSeconds(),
                                                              m_laplacianScale.GetMicroSeconds(),
                                                              m_laplacianBound.GetMicroSeconds())));
            // Add the bound to always have a positive value as suggested in Robert Novak et al.,
            // "Downlink VoIP Packet Delay Jitter Model".
            delay += jitter + m_laplacianBound;
        }
        const auto payloadSize = (m_currentState == VoiceActivityState::ACTIVE_TALKING)
                                     ? m_activePacketSize
                                     : m_silencePacketSize;
        auto packet = Create<Packet>(payloadSize);
        m_txPacketEvents[m_nextEventId] = Simulator::Schedule(delay,
                                                              &VoipTraffic::SendPacket,
                                                              this,
                                                              m_nextEventId,
                                                              packet,
                                                              jitter);
        ++m_nextEventId;
    }

    ScheduleNext();
}

void
VoipTraffic::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    ScheduleNext();
}

void
VoipTraffic::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

std::ostream&
operator<<(std::ostream& os, VoipTraffic::VoiceActivityState state)
{
    switch (state)
    {
    case VoipTraffic::VoiceActivityState::INACTIVE_SILENCE:
        os << "Silence";
        break;
    case VoipTraffic::VoiceActivityState::ACTIVE_TALKING:
        os << "Active talking";
        break;
    default:
        NS_FATAL_ERROR("Unknown voice activity state");
    }
    return os;
}

} // Namespace ns3
