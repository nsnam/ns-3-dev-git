/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef MOBILE_GAMING_H
#define MOBILE_GAMING_H

#include "source-application.h"

#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"

namespace ns3
{

class UniformRandomVariable;
class LargestExtremeValueRandomVariable;

/**
 * @ingroup applications
 * @brief Generate RT mobile gaming traffic.
 *
 * This RT mobile gaming traffic generator follows requirements from IEEE 802.11 Real Time
 * Applications TIG Report (Section 4.1.4: Traffic model).
 *
 * RT mobile gaming traffic typically consists in small packets (between 30 and 500 Bytes) for both
 * uplink and downlink, where usually downlink packets are bigger than uplink ones. Packets are
 * generated on average every 30-60ms for uplink and downlink, usually downlink packet interval is
 * larger than uplink one. The bandwidth for RT mobile gaming traffic is between 100kbps and 1Mbps.
 */
class MobileGaming : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    MobileGaming();
    ~MobileGaming() override;

    int64_t AssignStreams(int64_t stream) override;

    /**
     * Traffic model stages
     */
    enum class TrafficModelStage : uint8_t
    {
        INITIAL = 0,
        GAMING = 1,
        ENDING = 2
    };

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Cancel all pending events.
     */
    void CancelEvents();

    /**
     * Schedule the next packet transmission
     */
    void ScheduleNext();

    /**
     * Transmit one initial, gaming or ending packet
     */
    void SendPacket();

    /**
     * Handle a Connection Succeed event
     *
     * @param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);

    /**
     * Handle a Connection Failed event
     *
     * @param socket the not connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);

    TypeId m_tid;            //!< Type of the socket used
    bool m_connected{false}; //!< True if connected

    Ptr<UniformRandomVariable>
        m_initialSizeUniform; //!< Uniform random variable to generate the initial packet size
    Ptr<UniformRandomVariable>
        m_endSizeUniform; //!< Uniform random variable to generate the end packet size

    Ptr<LargestExtremeValueRandomVariable>
        m_levArrivals; //!< Largest extreme value random variable to generate packet arrival times
    Ptr<LargestExtremeValueRandomVariable>
        m_levSizes; //!< Largest extreme value random variable to generate packet sizes

    TrafficModelStage m_currentStage{TrafficModelStage::INITIAL}; //!< Hold the current stage

    EventId m_txEvent; //!< Event ID of pending TX event scheduling

    /**
     * TracedCallback signature for packet and stage.
     *
     * @param [in] packet The packet.
     * @param [in] stage The gaming traffic model stage corresponding to the packet.
     */
    typedef void (*TxTracedCallback)(Ptr<const Packet> packet, TrafficModelStage stage);

    /// Traced Callback: transmitted packets and their stage.
    TracedCallback<Ptr<const Packet>, TrafficModelStage> m_txStageTrace;
};

/**
 * Serialize gaming traffic model stage to ostream in a human-readable form.
 *
 * @param os std::ostream
 * @param stage the gaming traffic model stage
 * @return std::ostream
 */
std::ostream& operator<<(std::ostream& os, MobileGaming::TrafficModelStage stage);

} // namespace ns3

#endif /* MOBILE_GAMING_H */
