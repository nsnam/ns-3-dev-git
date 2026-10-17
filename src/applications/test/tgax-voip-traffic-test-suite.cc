/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/application-container.h"
#include "ns3/application-helper.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-socket-address.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/test.h"
#include "ns3/tgax-voip-traffic.h"
#include "ns3/traced-callback.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVoipTrafficTest");

namespace
{
const uint32_t voicePayloadSize = 33;        ///< payload size of voice packets in bytes
const uint32_t silencePayloadSize = 7;       ///< payload size of silence packets in bytes
const uint32_t compressedProtocolHeader = 3; ///< size of compressed protocol header (assumes IPv4)
const double tol = 0.1;                      ///< some tolerance for floating point comparisons
} // namespace

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * TGax voice-over-IP traffic test.
 */
class TgaxVoipTrafficTestCase : public TestCase
{
  public:
    /// Information about VoIP parameters
    struct VoipParams
    {
        Time meanActiveStateDuration;   //!< Mean duration of active/talking state
        Time meanInactiveStateDuration; //!< Mean duration of inactive/silence state
        double
            voiceToSilenceProbability; //!< Probability to transition from active to inactive state
        double
            silenceToVoiceProbability; //!< Probability to transition from inactive to active state
        Time scaleDelayJitter;         //!< Scale of laplacian distribution used for delay jitter
        Time boundDelayJitter;         //!< Bound of laplacian distribution used for delay jitter
    };

    /**
     * Constructor
     * @param name the name of the test to run
     * @param params the VoIP parameters to use for the test, default parameters are used if not
     * provided
     */
    TgaxVoipTrafficTestCase(const std::string& name, std::optional<VoipParams> params = {});

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Record a packets successfully sent
     * @param packet the transmitted packet
     * @param jitter the delay jitter applied to the packet
     */
    void PacketSent(Ptr<const Packet> packet, Time jitter);

    /**
     * Record a packet successfully received
     * @param context the context
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr);

    /**
     * Record a change in VoIP voice activity state
     * @param state the new voice activity state
     * @param duration the expected duration of the state
     */
    void StateUpdated(TgaxVoipTraffic::VoiceActivityState state, Time duration);

    /// Information about transmitted packet
    struct TxInfo
    {
        /**
         * Constructor
         * @param s the size of the packet in bytes
         * @param t the timestamp at which the packet is transmitted
         * @param j the delay jitter applied to the packet
         */
        TxInfo(uint32_t s, Time t, Time j)
            : size{s},
              tstamp{t},
              jitter{j}
        {
        }

        uint32_t size{}; //!< size of the packet in bytes
        Time tstamp{};   //!< timestamp at which the packet is transmitted
        Time jitter{};   //!< delay jitter applied to the packet
    };

    std::map<uint64_t, TxInfo> m_sent; //!< transmitted VoIP packets
    uint64_t m_received{0};            //!< number of bytes received
    std::vector<std::pair<TgaxVoipTraffic::VoiceActivityState, Time>>
        m_states; //!< Hold voice activity states and the time at which it started

    std::optional<VoipParams> m_params; //!< VoIP parameters
};

TgaxVoipTrafficTestCase::TgaxVoipTrafficTestCase(const std::string& name,
                                                 std::optional<VoipParams> params)
    : TestCase(name),
      m_params(params)
{
}

void
TgaxVoipTrafficTestCase::PacketSent(Ptr<const Packet> packet, Time jitter)
{
    NS_LOG_FUNCTION(this << packet << packet->GetSize() << packet->GetUid() << jitter);
    if (!m_sent.empty() && m_params && m_params->boundDelayJitter.IsZero())
    {
        NS_TEST_ASSERT_MSG_EQ(packet->GetUid(),
                              (m_sent.rbegin()->first + 1),
                              "Packets should arrive in order if there is no jitter");
    }
    m_sent.try_emplace(packet->GetUid(),
                       packet->GetSize() - compressedProtocolHeader,
                       Simulator::Now(),
                       jitter);
}

void
TgaxVoipTrafficTestCase::StateUpdated(TgaxVoipTraffic::VoiceActivityState state, Time duration)
{
    NS_LOG_FUNCTION(this << state << duration);
    m_states.emplace_back(state, Simulator::Now());
}

void
TgaxVoipTrafficTestCase::ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_FUNCTION(this << p << addr << p->GetSize());
    m_received += (p->GetSize() - compressedProtocolHeader);
}

void
TgaxVoipTrafficTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);

    const auto simulationTime{Seconds(300)};

    auto sender = CreateObject<Node>();
    auto receiver = CreateObject<Node>();

    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);

    SimpleNetDeviceHelper simpleHelper;
    auto devices = simpleHelper.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    PacketSocketAddress socketAddress;
    socketAddress.SetSingleDevice(devices.Get(0)->GetIfIndex());
    socketAddress.SetPhysicalAddress(devices.Get(1)->GetAddress());
    socketAddress.SetProtocol(1);

    ApplicationHelper sourceHelper(TgaxVoipTraffic::GetTypeId());
    sourceHelper.SetAttribute("Remote", AddressValue(socketAddress));
    sourceHelper.SetAttribute("ActivePacketPayloadSize",
                              UintegerValue(voicePayloadSize + compressedProtocolHeader));
    sourceHelper.SetAttribute("SilencePacketPayloadSize",
                              UintegerValue(silencePayloadSize + compressedProtocolHeader));
    if (m_params)
    {
        sourceHelper.SetAttribute("MeanActiveStateDuration",
                                  TimeValue(m_params->meanActiveStateDuration));
        sourceHelper.SetAttribute("MeanInactiveStateDuration",
                                  TimeValue(m_params->meanInactiveStateDuration));
        sourceHelper.SetAttribute("VoiceToSilenceProbability",
                                  DoubleValue(m_params->voiceToSilenceProbability));
        sourceHelper.SetAttribute("SilenceToVoiceProbability",
                                  DoubleValue(m_params->silenceToVoiceProbability));
        sourceHelper.SetAttribute("ScaleDelayJitter", TimeValue(m_params->scaleDelayJitter));
        sourceHelper.SetAttribute("BoundDelayJitter", TimeValue(m_params->boundDelayJitter));
    }
    auto sourceApp = sourceHelper.Install(sender);
    const auto startAppTime = Seconds(1.0);
    sourceApp.Start(startAppTime);
    sourceApp.Stop(startAppTime + simulationTime);
    m_states.emplace_back(TgaxVoipTraffic::VoiceActivityState::INACTIVE_SILENCE, startAppTime);

    PacketSinkHelper sinkHelper("ns3::PacketSocketFactory", socketAddress);
    auto sinkApp = sinkHelper.Install(receiver);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(2.0) + simulationTime);

    int64_t streamNumber = 10;
    sourceHelper.AssignStreams(nodes, streamNumber);

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::TgaxVoipTraffic/StateUpdate",
        MakeCallback(&TgaxVoipTrafficTestCase::StateUpdated, this));

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::TgaxVoipTraffic/TxWithJitter",
        MakeCallback(&TgaxVoipTrafficTestCase::PacketSent, this));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&TgaxVoipTrafficTestCase::ReceiveRx, this));
}

void
TgaxVoipTrafficTestCase::DoRun()
{
    Simulator::Run();
    Simulator::Destroy();

    const auto totalTx =
        std::accumulate(m_sent.cbegin(), m_sent.cend(), 0ULL, [](auto sum, const auto& elem) {
            return sum + elem.second.size;
        });
    NS_TEST_ASSERT_MSG_EQ(totalTx, m_received, "Did not receive all transmitted voip packets");

    const auto offset = m_params
                            ? m_params->boundDelayJitter
                            : MilliSeconds(80); // an offset is applied in the model to guarantee
                                                // positive time for scheduling packets
    std::optional<TxInfo> prevTx{};
    for (const auto& [id, info] : m_sent)
    {
        auto stateIt =
            std::upper_bound(m_states.begin(),
                             m_states.end(),
                             info.tstamp - info.jitter - offset,
                             [](Time value, const auto& state) { return value <= state.second; });
        stateIt--;
        auto expectedSize =
            (stateIt->first == TgaxVoipTraffic::VoiceActivityState::INACTIVE_SILENCE)
                ? silencePayloadSize
                : voicePayloadSize;
        NS_TEST_ASSERT_MSG_EQ(info.size, expectedSize, "Unexpected packet size");

        if (!prevTx)
        {
            // first TX, no delta with previous
            prevTx = info;
            continue;
        }

        const auto interval = info.tstamp - prevTx->tstamp;
        const auto jitterCorrection = info.jitter - prevTx->jitter;
        const auto expectedInterval =
            (stateIt->first == TgaxVoipTraffic::VoiceActivityState::INACTIVE_SILENCE)
                ? MilliSeconds(160)
                : MilliSeconds(20);
        NS_TEST_ASSERT_MSG_EQ(interval,
                              expectedInterval + jitterCorrection,
                              "Unexpected encoder frame interval");

        prevTx = info;
    }

    std::vector<Time> inactiveDurations;
    std::transform(m_states.cbegin(),
                   m_states.cend() - 1,
                   m_states.cbegin() + 1,
                   std::back_inserter(inactiveDurations),
                   [](const auto& lhs, const auto& rhs) {
                       return (lhs.first == TgaxVoipTraffic::VoiceActivityState::INACTIVE_SILENCE)
                                  ? (rhs.second - lhs.second)
                                  : Seconds(0);
                   });
    auto noZeroEnd = std::remove_if(inactiveDurations.begin(),
                                    inactiveDurations.end(),
                                    [&](const auto& t) { return t.IsZero(); });
    inactiveDurations.erase(noZeroEnd, inactiveDurations.end());
    const auto totalInactiveDuration =
        std::accumulate(inactiveDurations.cbegin(),
                        inactiveDurations.cend(),
                        Time(),
                        [](auto sum, const auto t) { return sum + t; });

    std::vector<Time> activeDurations;
    std::transform(m_states.cbegin(),
                   m_states.cend() - 1,
                   m_states.cbegin() + 1,
                   std::back_inserter(activeDurations),
                   [](const auto& lhs, const auto& rhs) {
                       return (lhs.first == TgaxVoipTraffic::VoiceActivityState::ACTIVE_TALKING)
                                  ? (rhs.second - lhs.second)
                                  : Seconds(0);
                   });
    noZeroEnd = std::remove_if(activeDurations.begin(), activeDurations.end(), [&](const auto& t) {
        return t.IsZero();
    });
    activeDurations.erase(noZeroEnd, activeDurations.end());
    const auto totalActiveDuration =
        std::accumulate(activeDurations.cbegin(),
                        activeDurations.cend(),
                        Time(),
                        [](auto sum, const auto t) { return sum + t; });

    const auto averageActiveDuration = totalActiveDuration / activeDurations.size();
    const auto expectedAverageActiveStateDurationMs =
        m_params ? m_params->meanActiveStateDuration.GetMilliSeconds() : 1250;
    NS_TEST_EXPECT_MSG_EQ_TOL(averageActiveDuration.GetMilliSeconds(),
                              expectedAverageActiveStateDurationMs,
                              tol * expectedAverageActiveStateDurationMs,
                              "Unexpected average active state duration");

    const auto averageInactiveDuration = totalInactiveDuration / inactiveDurations.size();
    const auto expectedAverageInactiveStateDurationMs =
        m_params ? m_params->meanInactiveStateDuration.GetMilliSeconds() : 1250;
    NS_TEST_EXPECT_MSG_EQ_TOL(averageInactiveDuration.GetMilliSeconds(),
                              expectedAverageInactiveStateDurationMs,
                              tol * expectedAverageInactiveStateDurationMs,
                              "Unexpected average inactive state duration");

    const auto totalDuration = totalInactiveDuration + totalActiveDuration;
    const auto voiceActivityFactor = static_cast<double>(totalActiveDuration.GetMicroSeconds()) /
                                     totalDuration.GetMicroSeconds();
    const auto expectedVoiceActivityFactor =
        m_params ? (m_params->silenceToVoiceProbability /
                    (m_params->silenceToVoiceProbability + m_params->voiceToSilenceProbability))
                 : 0.5; // default is 50%
    NS_TEST_EXPECT_MSG_EQ_TOL(voiceActivityFactor,
                              expectedVoiceActivityFactor,
                              tol,
                              "Unexpected voice activity factor");

    const double totalJitterUs =
        std::accumulate(m_sent.cbegin(), m_sent.cend(), 0.0, [](double sum, const auto& elem) {
            return sum + elem.second.jitter.GetMicroSeconds();
        });
    const auto avgJitterMs = (totalJitterUs / m_sent.size()) / 1000;
    NS_TEST_EXPECT_MSG_EQ_TOL(avgJitterMs, 0.0, tol, "Unexpected average jitter");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief TgaxVoipTraffic TestSuite
 */
class TgaxVoipTrafficTestSuite : public TestSuite
{
  public:
    TgaxVoipTrafficTestSuite();
};

TgaxVoipTrafficTestSuite::TgaxVoipTrafficTestSuite()
    : TestSuite("applications-tgax-voip-traffic", Type::UNIT)
{
    AddTestCase(new TgaxVoipTrafficTestCase("VoIP traffic with default parameters"),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVoipTrafficTestCase("VoIP traffic without jitter",
                                            TgaxVoipTrafficTestCase::VoipParams{MilliSeconds(1250),
                                                                                MilliSeconds(1250),
                                                                                0.016,
                                                                                0.016,
                                                                                Time(),
                                                                                Time()}),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVoipTrafficTestCase("VoIP traffic with custom parameters",
                                            TgaxVoipTrafficTestCase::VoipParams{MilliSeconds(1000),
                                                                                MilliSeconds(1500),
                                                                                0.0200,
                                                                                0.0133,
                                                                                MicroSeconds(5000),
                                                                                MilliSeconds(60)}),
                TestCase::Duration::QUICK);
}

static TgaxVoipTrafficTestSuite
    g_TgaxVoipTrafficTestSuite; //!< Static variable for test initialization
