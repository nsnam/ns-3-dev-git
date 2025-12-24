/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef TGAX_VOIP_TRAFFIC_H
#define TGAX_VOIP_TRAFFIC_H

#include "source-application.h"

#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <map>

namespace ns3
{

class ExponentialRandomVariable;
class UniformRandomVariable;
class LaplacianRandomVariable;

/**
 * @ingroup applications
 * @brief Generate VoIP traffic.
 *
 * This voip traffic generator follows requirements from IEEE 802.11-14/0571r12 - 11ax Evaluation
 * Methodology.
 *
 * The VoIP traffic alternates between periods of active talking and silence,
 * with given probabilities to transition from one state to another.
 * These state updates are assumed to be done at the speech encoder frame rate.
 *
 * Fixed-size VoIP packets are generated at every encoder frame interval,
 * plus a random network packet arrival delay jitter (if BoundDelayJitter is non-zero).
 * The size of these packets also depend on the current state.
 *
 * The VoIP model from the reference relies on UDP with compressed protocol headers.
 * Since compressed protocol headers are not supported in the simulator, a packet socket
 * is used instead, allowing user to tune payload sizes by adding up the size of the compressed
 * headers.
 *
 * This model can also be used with usual UDP or TCP sockets. For the later, user should be warned
 * that the model does not provide any mechanism when TX buffer is full.
 */
class TgaxVoipTraffic : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TgaxVoipTraffic();
    ~TgaxVoipTraffic() override;

    /**
     * Voice activity states
     */
    enum class VoiceActivityState : uint8_t
    {
        INACTIVE_SILENCE = 0, //< Inactive/silence state
        ACTIVE_TALKING = 1,   //< Active/talking state
    };

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoInitialize() override;

  private:
    void DoStartApplication() override;
    void DoConnectionSucceeded(Ptr<Socket> socket) override;
    void CancelEvents() override;

    /**
     * Set the mean of the exponential distribution used to calculate durations of active/talking
     * state
     * @param mean the mean value
     */
    void SetActiveExponentialMean(Time mean);

    /**
     * Set the mean of the exponential distribution used to calculate durations of inactive/silent
     * state
     * @param mean the mean value
     */
    void SetInactiveExponentialMean(Time mean);

    /**
     * Update voice activity state
     */
    void UpdateState();

    /**
     * Schedule the next state update
     */
    void ScheduleNext();

    /**
     * Transmit one VoIP packet
     * @param eventId the event ID
     * @param packet the packet to transmit
     * @param jitter the delay jitter applied to that packet
     */
    void SendPacket(uint64_t eventId, Ptr<Packet> packet, Time jitter);

    /**
     * Get the duration to encode a frame based on the current state
     *
     * @return the duration to encode a frame
     */
    Time GetEncoderFrameDuration() const;

    /**
     * Get the interval between two state updates
     *
     * @return the interval between two state updates
     */
    Time GetStateUpdateInterval() const;

    uint32_t m_activePacketSize;  //!< Size in bytes for payload of active packets
    uint32_t m_silencePacketSize; //!< Size in bytes for payload of silence packets
    Time m_voiceInterval;         //!< Interval between generation of voice packets
    Time m_silenceInterval;       //!< Interval between generation of silence packets
    double m_activeToInactive;    //!< Probability to transition from active/talking state to
                                  //!< inactive/silence state
    double m_inactiveToActive;    //!< Probability to transition from inactive/silence state to
                                  //!< active/talking state
    Time m_delayJitterScale; //!< Scale of laplacian distribution used to calculate delay jitter
    Time m_delayJitterBound; //!< Bound of laplacian distribution used to calculate delay jitter

    Ptr<ExponentialRandomVariable>
        m_inactiveExponential; //!< Exponential random variable to generate inactive/silent state
                               //!< durations
    Ptr<ExponentialRandomVariable> m_activeExponential; //!< Exponential random variable to generate
                                                        //!< active/talking state durations
    Ptr<UniformRandomVariable> m_inactiveUniform; //!< Uniform random variable to generate state
                                                  //!< transitions from inactive state
    Ptr<UniformRandomVariable> m_activeUniform;   //!< Uniform random variable to generate state
                                                  //!< transitions from active state
    Ptr<LaplacianRandomVariable>
        m_delayJitterLaplacian; //!< Laplacian random variable to generate delay jitter

    VoiceActivityState m_currentState{
        VoiceActivityState::INACTIVE_SILENCE}; //!< Hold the current voice activity state
    bool m_pendingStateTransition{false}; //!< Flag whether a state transition should once duration
                                          //!< of current state has elapsed
    Time m_remainingStateDuration;        //!< The remaining duration in the current state
    Time m_remainingEncodingDuration;     //!< The remaining duration to encode the current frame

    EventId m_stateUpdateEvent; //!< Event ID of pending state update event scheduling

    std::map<uint64_t, EventId> m_txPacketEvents; //!< Event IDs of pending TX events
    uint64_t m_nextEventId{0};                    //!< The next event ID

    /**
     * TracedCallback signature for packet and jitter.
     *
     * @param [in] packet The packet.
     * @param [in] jitter The networking jitter.
     */
    typedef void (*TxTracedCallback)(Ptr<const Packet> packet, Time jitter);

    /**
     * TracedCallback signature for state change.
     *
     * @param [in] state The new state.
     * @param [in] duration The duration of the state.
     */
    typedef void (*StateUpdatedCallback)(VoiceActivityState state, Time duration);

    /// Traced Callback: transmitted packets and their jitters.
    TracedCallback<Ptr<const Packet>, Time> m_txJitterTrace;

    /// Traced Callback: voice activity state updated.
    TracedCallback<VoiceActivityState, Time> m_stateUpdate;
};

/**
 * Serialize voice activity state to ostream in a human-readable form.
 *
 * @param os std::ostream
 * @param state the voice activity state
 * @return std::ostream
 */
std::ostream& operator<<(std::ostream& os, TgaxVoipTraffic::VoiceActivityState state);

} // namespace ns3

#endif /* TGAX_VOIP_TRAFFIC_H */
