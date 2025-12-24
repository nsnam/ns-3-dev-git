/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "tgax-voip-traffic.h"

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

NS_LOG_COMPONENT_DEFINE("TgaxVoipTraffic");

NS_OBJECT_ENSURE_REGISTERED(TgaxVoipTraffic);

TypeId
TgaxVoipTraffic::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TgaxVoipTraffic")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<TgaxVoipTraffic>()
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                TypeIdValue(TypeId::LookupByName("ns3::PacketSocketFactory")),
                MakeTypeIdAccessor(&TgaxVoipTraffic::m_protocolTid),
                MakeTypeIdChecker())
            .AddAttribute(
                "ActivePacketPayloadSize",
                "Size in bytes for payload of packets generated during periods of active talking.",
                UintegerValue(33),
                MakeUintegerAccessor(&TgaxVoipTraffic::m_activePacketSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "SilencePacketPayloadSize",
                "Size in bytes for payload of packets generated during periods of silence.",
                UintegerValue(7),
                MakeUintegerAccessor(&TgaxVoipTraffic::m_silencePacketSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("MeanActiveStateDuration",
                          "Mean duration of active/talking state.",
                          TimeValue(MilliSeconds(1250)),
                          MakeTimeAccessor(&TgaxVoipTraffic::SetActiveExponentialMean),
                          MakeTimeChecker())
            .AddAttribute("MeanInactiveStateDuration",
                          "Mean duration of inactive/silence state.",
                          TimeValue(MilliSeconds(1250)),
                          MakeTimeAccessor(&TgaxVoipTraffic::SetInactiveExponentialMean),
                          MakeTimeChecker())
            .AddAttribute("VoiceEncoderInterval",
                          "Interval between generation of voice packets.",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&TgaxVoipTraffic::m_voiceInterval),
                          MakeTimeChecker())
            .AddAttribute("SilenceEncoderInterval",
                          "Interval between generation of silence packets. "
                          "This implementation requires the value of this attribute to be a "
                          "multiple of VoiceEncoderInterval",
                          TimeValue(MilliSeconds(160)),
                          MakeTimeAccessor(&TgaxVoipTraffic::m_silenceInterval),
                          MakeTimeChecker())
            .AddAttribute(
                "VoiceToSilenceProbability",
                "The probability to transition from active talking state to silence state.",
                DoubleValue(0.016),
                MakeDoubleAccessor(&TgaxVoipTraffic::m_activeToInactive),
                MakeDoubleChecker<double>(0, 1))
            .AddAttribute(
                "SilenceToVoiceProbability",
                "The probability to transition from silence state to active talking state.",
                DoubleValue(0.016),
                MakeDoubleAccessor(&TgaxVoipTraffic::m_inactiveToActive),
                MakeDoubleChecker<double>(0, 1))
            .AddAttribute("ScaleDelayJitter",
                          "Scale of laplacian distribution used to calculate delay jitter for "
                          "generated packets.",
                          TimeValue(MicroSeconds(5110)),
                          MakeTimeAccessor(&TgaxVoipTraffic::m_delayJitterScale),
                          MakeTimeChecker())
            .AddAttribute("BoundDelayJitter",
                          "Bound of laplacian distribution used to calculate delay jitter for "
                          "generated packets. For no jitter, set this attribute to zero.",
                          TimeValue(MilliSeconds(80)),
                          MakeTimeAccessor(&TgaxVoipTraffic::m_delayJitterBound),
                          MakeTimeChecker())
            .AddTraceSource(
                "TxWithJitter",
                "A VoIP packet is sent, this trace also reports the jitter applied to the packet",
                MakeTraceSourceAccessor(&TgaxVoipTraffic::m_txJitterTrace),
                "ns3::TgaxVoipTraffic::TxTracedCallback")
            .AddTraceSource("StateUpdate",
                            "Voice activity state updated",
                            MakeTraceSourceAccessor(&TgaxVoipTraffic::m_stateUpdate),
                            "ns3::TgaxVoipTraffic::StateUpdatedCallback");
    return tid;
}

TgaxVoipTraffic::TgaxVoipTraffic()
    : m_inactiveExponential{CreateObject<ExponentialRandomVariable>()},
      m_activeExponential{CreateObject<ExponentialRandomVariable>()},
      m_inactiveUniform{CreateObject<UniformRandomVariable>()},
      m_activeUniform{CreateObject<UniformRandomVariable>()},
      m_delayJitterLaplacian{CreateObject<LaplacianRandomVariable>()}
{
    NS_LOG_FUNCTION(this);
}

TgaxVoipTraffic::~TgaxVoipTraffic()
{
    NS_LOG_FUNCTION(this);
}

int64_t
TgaxVoipTraffic::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_inactiveExponential->SetStream(currentStream++);
    m_activeExponential->SetStream(currentStream++);
    m_inactiveUniform->SetStream(currentStream++);
    m_activeUniform->SetStream(currentStream++);
    m_delayJitterLaplacian->SetStream(currentStream++);
    return (currentStream - stream);
}

void
TgaxVoipTraffic::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    SourceApplication::DoInitialize();
    m_delayJitterLaplacian->SetAttribute("Scale",
                                         DoubleValue(m_delayJitterScale.GetMicroSeconds()));
    m_delayJitterLaplacian->SetAttribute("Bound",
                                         DoubleValue(m_delayJitterBound.GetMicroSeconds()));
}

void
TgaxVoipTraffic::SetActiveExponentialMean(Time mean)
{
    NS_LOG_FUNCTION(this << mean);
    m_activeExponential->SetAttribute("Mean", DoubleValue(mean.GetMilliSeconds()));
}

void
TgaxVoipTraffic::SetInactiveExponentialMean(Time mean)
{
    NS_LOG_FUNCTION(this << mean);
    m_inactiveExponential->SetAttribute("Mean", DoubleValue(mean.GetMilliSeconds()));
}

void
TgaxVoipTraffic::DoStartApplication()
{
    NS_LOG_FUNCTION(this);

    m_socket->SetAllowBroadcast(true);
    m_socket->ShutdownRecv();

    if (m_connected)
    {
        ScheduleNext();
    }
}

void
TgaxVoipTraffic::CancelEvents()
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
TgaxVoipTraffic::GetEncoderFrameDuration() const
{
    return (m_currentState == VoiceActivityState::ACTIVE_TALKING) ? m_voiceInterval
                                                                  : m_silenceInterval;
}

Time
TgaxVoipTraffic::GetStateUpdateInterval() const
{
    NS_ABORT_MSG_IF(
        !(m_silenceInterval % m_voiceInterval).IsZero(),
        "Silence encoder frame duration should be a multiple of voice encoder frame duration");
    return m_voiceInterval;
}

void
TgaxVoipTraffic::ScheduleNext()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_stateUpdateEvent.IsPending());
    m_stateUpdateEvent =
        Simulator::Schedule(GetStateUpdateInterval(), &TgaxVoipTraffic::UpdateState, this);
}

void
TgaxVoipTraffic::SendPacket(uint64_t eventId, Ptr<Packet> packet, Time jitter)
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
TgaxVoipTraffic::UpdateState()
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
        if (!m_delayJitterBound.IsZero())
        {
            jitter = MicroSeconds(static_cast<int64x64_t>(m_delayJitterLaplacian->GetValue()));
            // Add the bound to always have a positive value as suggested in Robert Novak et al.,
            // "Downlink VoIP Packet Delay Jitter Model".
            delay += jitter + m_delayJitterBound;
        }
        const auto payloadSize = (m_currentState == VoiceActivityState::ACTIVE_TALKING)
                                     ? m_activePacketSize
                                     : m_silencePacketSize;
        auto packet = Create<Packet>(payloadSize);
        m_txPacketEvents[m_nextEventId] = Simulator::Schedule(delay,
                                                              &TgaxVoipTraffic::SendPacket,
                                                              this,
                                                              m_nextEventId,
                                                              packet,
                                                              jitter);
        ++m_nextEventId;
    }

    ScheduleNext();
}

void
TgaxVoipTraffic::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    ScheduleNext();
}

std::ostream&
operator<<(std::ostream& os, TgaxVoipTraffic::VoiceActivityState state)
{
    switch (state)
    {
    case TgaxVoipTraffic::VoiceActivityState::INACTIVE_SILENCE:
        os << "Silence";
        break;
    case TgaxVoipTraffic::VoiceActivityState::ACTIVE_TALKING:
        os << "Active talking";
        break;
    default:
        NS_FATAL_ERROR("Unknown voice activity state");
    }
    return os;
}

} // Namespace ns3
