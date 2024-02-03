/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef VIDEO_TRAFFIC_H
#define VIDEO_TRAFFIC_H

#include "source-application.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-callback.h"

#include <deque>
#include <map>
#include <optional>

namespace ns3
{

class Socket;
class WeibullRandomVariable;
class GammaRandomVariable;

/**
 * @ingroup applications
 * @brief Generate video traffic.
 *
 * This video traffic generator follows requirements from IEEE 802.11-14/0571r12 - 11ax Evaluation
 * Methodology.
 */
class VideoTraffic : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    VideoTraffic();
    ~VideoTraffic() override;

    /// TrafficModelClassIdentifier enumeration
    enum TrafficModelClassIdentifier
    {
        CUSTOM,
        BUFFERED_VIDEO_1,
        BUFFERED_VIDEO_2,
        BUFFERED_VIDEO_3,
        BUFFERED_VIDEO_4,
        BUFFERED_VIDEO_5,
        BUFFERED_VIDEO_6,
        MULTICAST_VIDEO_1,
        MULTICAST_VIDEO_2
    };

    /// List of parameters for a given traffic model
    struct TrafficModelParameters
    {
        DataRate bitRate;    //!< video bit rate
        double weibullScale; //!< scale parameter for the Weibull distribution (corresponds to
                             //!< lambda parameter in table 5 from IEEE 802.11-14/0571r12 - 11ax
                             //!< Evaluation Methodology)
        double weibullShape; //!< shape parameter for the Weibull distribution (corresponds to k
                             //!< parameter in table 5 from IEEE 802.11-14/0571r12 - 11ax Evaluation
                             //!< Methodology)
    };

    /// Parameters for each traffic model
    using TrafficModels = std::map<TrafficModelClassIdentifier, TrafficModelParameters>;

    static const TrafficModels m_trafficModels; //!< Traffic models as defined in Table 5 from IEEE
                                                //!< 802.11-14/0571r12 - 11ax Evaluation Methodology

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * Cancel all pending events.
     */
    void CancelEvents();

    /**
     * Schedule send of a packet with a random latency
     */
    void SendWithLatency();

    /**
     * Effectively send a packet once the latency has elapsed
     * @param eventId the event ID
     * @param size the size of the packet to send (in bytes)
     * @param latency the latency applied to that packet
     */
    void Send(uint64_t eventId, uint32_t size, Time latency);

    /**
     * Schedule the next frame generation
     */
    void ScheduleNextFrame();

    /**
     * Generate new video frame
     */
    void GenerateVideoFrame();

    /**
     * @return the payload size of the next packet to transmit (in bytes)
     */
    uint32_t GetNextPayloadSize();

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

    /**
     * Handle a Data Sent event
     *
     * @param socket the connected socket
     * @param size the amount of transmitted bytes
     */
    void TxDone(Ptr<Socket> socket, uint32_t size);

    /**
     * Handle a Send event
     *
     * @param socket the connected socket
     * @param available the amount of available bytes for transmission
     */
    void TxAvailable(Ptr<Socket> socket, uint32_t available);

    TrafficModelClassIdentifier m_trafficModelClassId; //!< The Traffic Model Class Identifier
    TypeId m_tid;                                      //!< Type of the socket used
    DataRate m_bitRate;                                //!< Video bit rate (if model is custom)
    double m_weibullScale; //!< Scale parameter for the Weibull distribution (if model is custom)
    double m_weibullShape; //!< Shape parameter for the Weibull distribution (if model is custom)
    double m_gammaScale;   //!< Scale parameter for the Gamma distribution
    double m_gammaShape;   //!< Shape parameter for the Gamma distribution

    Ptr<WeibullRandomVariable>
        m_weibull; //!< Weibull random variable to generate size of video frames (in bytes)
    Ptr<GammaRandomVariable>
        m_gamma; //!< Gamma random variable to generate latency (in milliseconds)

    Ptr<Socket> m_socket;              //!< Associated socket
    std::optional<uint32_t> m_maxSize; //!< Limit on the number of bytes that can be sent at once
                                       //!< over the network, hence we limit at application level to
                                       //!< apply the latency to each transmitted packet
    bool m_connected;                  //!< True if connected
    uint32_t m_remainingSize; //!< Number of bytes to send directly to the socket because current
                              //!< video frame is too large to be sent at once
    Time m_interArrival;      //!< Calculated inter arrival duration between two generated packets

    EventId m_generateFrameEvent;           //!< Event ID of pending frame generation event
    std::deque<uint32_t> m_generatedFrames; //!< Hold size of generated video frames

    std::map<uint64_t, EventId> m_sendEvents; //!< Event IDs of pending TX events
    uint64_t m_nextEventId;                   //!< The next event ID

    /// Structure to store information about packets that are not successfully transmitted
    struct UnsentPacketInfo
    {
        uint64_t id;        //!< the associated TX event ID
        Ptr<Packet> packet; //!< the packet to transmit
        Time latency;       //!< the networking latency applied to the first transmission attempt
    };

    std::deque<UnsentPacketInfo> m_unsentPackets; //!< Hold unsent packet for later attempt

    /**
     * TracedCallback signature for packet and latency.
     *
     * @param [in] packet The packet.
     * @param [in] latency The networking latency.
     */
    typedef void (*TxTracedCallback)(Ptr<const Packet> packet, Time latency);

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /// Traced Callback: transmitted packets and their latencies.
    TracedCallback<Ptr<const Packet>, Time> m_txLatencyTrace;

    /// Traced Callback: generated frames (amount of payload bytes).
    TracedCallback<uint32_t> m_frameGeneratedTrace;
};

} // namespace ns3

#endif /* VIDEO_TRAFFIC_H */
