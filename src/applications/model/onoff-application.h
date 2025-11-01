//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#ifndef ONOFF_APPLICATION_H
#define ONOFF_APPLICATION_H

#include "seq-ts-size-header.h"
#include "source-application.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"

namespace ns3
{

class RandomVariableStream;

/**
 * @ingroup applications
 * @defgroup onoff OnOffApplication
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
 * @ingroup onoff
 *
 * @brief Generate traffic to a single destination according to an
 *        OnOff pattern.
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 *
 * Note:  When an application is started, the first packet transmission
 * occurs _after_ a delay equal to (packet size/bit rate).  Note also,
 * when an application transitions into an off state in between packet
 * transmissions, the remaining time until when the next transmission
 * would have occurred is cached and is used when the application starts
 * up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
 * If the application is started at time 3 seconds, the first packet
 * transmission will be scheduled for time 5 seconds (3 + 1000/500)
 * and subsequent transmissions at 2 second intervals.  If the above
 * application were instead stopped at time 4 seconds, and restarted at
 * time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
 * because when it was stopped at 4 seconds, there was only 1 second remaining
 * until the originally scheduled transmission, and this time remaining
 * information is cached and used to schedule the next transmission
 * upon restarting.
 *
 * If the underlying socket type supports broadcast, this application
 * will automatically enable the SetAllowBroadcast(true) socket option.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting
 * statistics from this header have been added to \c ns3::PacketSink
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.  Note that the continuity of the sequence
 * number may be disrupted across On/Off cycles.
 */
class OnOffApplication : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    OnOffApplication();
    ~OnOffApplication() override;

    /**
     * @brief Set the total number of bytes to send.
     *
     * Once these bytes are sent, no packet is sent again, even in on state.
     * The value zero means that there is no limit.
     *
     * @param maxBytes the total number of bytes to send
     */
    void SetMaxBytes(uint64_t maxBytes);

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    // helpers
    /**
     * @brief Cancel all pending events.
     */
    void CancelEvents();

    // Event handlers
    /**
     * @brief Start an On period
     */
    void StartSending();
    /**
     * @brief Start an Off period
     */
    void StopSending();
    /**
     * @brief Send a packet
     */
    void SendPacket();

    bool m_connected{false};             //!< True if connected
    Ptr<RandomVariableStream> m_onTime;  //!< rng for On Time
    Ptr<RandomVariableStream> m_offTime; //!< rng for Off Time
    DataRate m_cbrRate;                  //!< Rate that data is generated
    DataRate m_cbrRateFailSafe;          //!< Rate that data is generated (check copy)
    uint32_t m_pktSize;                  //!< Size of packets
    uint32_t m_residualBits{0};          //!< Number of generated, but not sent, bits
    Time m_lastStartTime;                //!< Time last packet sent
    uint64_t m_maxBytes;                 //!< Limit total number of bytes sent
    uint64_t m_totBytes{0};              //!< Total bytes sent so far
    EventId m_startStopEvent;            //!< Event id for next start or stop event
    EventId m_sendEvent;                 //!< Event id of pending "send packet" event
    TypeId m_tid;                        //!< Type of the socket used
    uint32_t m_seq{0};                   //!< Sequence
    Ptr<Packet> m_unsentPacket;          //!< Unsent packet cached for future attempt
    bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the use of SeqTsSizeHeader

    /// Callbacks for tracing the packet Tx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

    /// Callback for tracing the packet Tx events, includes source, destination, the packet sent,
    /// and header
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_txTraceWithSeqTsSize;

  private:
    /**
     * @brief Schedule the next packet transmission
     */
    void ScheduleNextTx();
    /**
     * @brief Schedule the next On period start
     */
    void ScheduleStartEvent();
    /**
     * @brief Schedule the next Off period start
     */
    void ScheduleStopEvent();
    /**
     * @brief Handle a Connection Succeed event
     * @param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);
    /**
     * @brief Handle a Connection Failed event
     * @param socket the not connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);

    TracedValue<bool> m_state; //!< State of application (0-OFF, 1-ON)
};

} // namespace ns3

#endif /* ONOFF_APPLICATION_H */
