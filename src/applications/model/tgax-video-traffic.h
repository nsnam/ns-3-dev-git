/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef TGAX_VIDEO_TRAFFIC_H
#define TGAX_VIDEO_TRAFFIC_H

#include "source-application.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-callback.h"

#include <deque>
#include <map>
#include <optional>

namespace ns3
{

class WeibullRandomVariable;
class GammaRandomVariable;

/**
 * @ingroup applications
 * @brief Generate video traffic.
 *
 * This video traffic generator implements the Buffered Video Steaming model from IEEE
 * 802.11-14/0571r12 - 11ax Evaluation Methodology (see applications documentation for
 * full citation).
 */
class TgaxVideoTraffic : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TgaxVideoTraffic();
    ~TgaxVideoTraffic() override;

    /// TrafficModelClassIdentifier enumeration
    enum class TrafficModelClassIdentifier : uint8_t
    {
        CUSTOM = 0, //< Custom traffic model (by default, load parameters of Buffered Video Model 1)
        BUFFERED_VIDEO_1,  //< Buffered Video Model 1
        BUFFERED_VIDEO_2,  //< Buffered Video Model 2
        BUFFERED_VIDEO_3,  //< Buffered Video Model 3
        BUFFERED_VIDEO_4,  //< Buffered Video Model 4
        BUFFERED_VIDEO_5,  //< Buffered Video Model 5
        BUFFERED_VIDEO_6,  //< Buffered Video Model 6
        MULTICAST_VIDEO_1, //< Multicast Video Model 1
        MULTICAST_VIDEO_2  //< Multicast Video Model 2
    };

    /// List of parameters for a given traffic model
    struct TrafficModelParameters
    {
        DataRate bitRate;           //!< video bit rate
        double frameSizeBytesScale; //!< scale parameter for the Weibull distribution used to
                                    //!< generate size of video frames in bytes (corresponds to
                                    //!< lambda parameter in table 5 from IEEE 802.11-14/0571r12 -
                                    //!< 11ax Evaluation Methodology)
        double frameSizeBytesShape; //!< shape parameter for the Weibull distribution used to
                                    //!< generate size of video frames in bytes (corresponds to k
                                    //!< parameter in table 5 from IEEE 802.11-14/0571r12 - 11ax
                                    //!< Evaluation Methodology)
    };

    /// Parameters for each traffic model
    using TrafficModels = std::map<TrafficModelClassIdentifier, TrafficModelParameters>;

    static const TrafficModels m_trafficModels; //!< Traffic models as defined in Table 5 from IEEE
                                                //!< 802.11-14/0571r12 - 11ax Evaluation Methodology

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoInitialize() override;

  private:
    void DoStartApplication() override;
    void DoConnectionSucceeded(Ptr<Socket> socket) override;
    void CancelEvents() override;

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
    DataRate m_bitRate;                                //!< Video bit rate (if model is custom)
    double m_frameSizeBytesScale; //!< Scale parameter for the Weibull distribution used to generate
                                  //!< size of video frames (if model is custom)
    double m_frameSizeBytesShape; //!< Shape parameter for the Weibull distribution used to generate
                                  //!< size of video frames (if model is custom)
    double
        m_latencyMsScale; //!< Scale parameter for the Gamma distribution used to generate latency
    double
        m_latencyMsShape; //!< Shape parameter for the Gamma distribution used to generate latency

    Ptr<WeibullRandomVariable>
        m_frameSizeBytes; //!< Weibull random variable to generate size of video frames (in bytes)
    Ptr<GammaRandomVariable>
        m_latencyMs; //!< Gamma random variable to generate latency (in milliseconds)

    std::optional<uint32_t> m_maxSize; //!< Limit on the number of bytes that can be sent at once
                                       //!< over the network, hence we limit at application level to
                                       //!< apply the latency to each transmitted packet
    uint32_t m_remainingSize{0}; //!< Number of bytes to send directly to the socket because current
                                 //!< video frame is too large to be sent at once
    Time m_interArrival; //!< Calculated inter arrival duration between two generated packets

    EventId m_generateFrameEvent;           //!< Event ID of pending frame generation event
    std::deque<uint32_t> m_generatedFrames; //!< Hold size of generated video frames

    std::map<uint64_t, EventId> m_sendEvents; //!< Event IDs of pending TX events
    uint64_t m_nextEventId{0};                //!< The next event ID

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

    /// Traced Callback: transmitted packets and their latencies.
    TracedCallback<Ptr<const Packet>, Time> m_txLatencyTrace;

    /// Traced Callback: generated frames (amount of payload bytes).
    TracedCallback<uint32_t> m_frameGeneratedTrace;
};

} // namespace ns3

#endif /* TGAX_VIDEO_TRAFFIC_H */
