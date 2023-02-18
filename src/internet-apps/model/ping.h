/*
 * Copyright (c) 2022 Chandrakant Jena
 * Copyright (c) 2007-2009 Strasbourg University (original Ping6 code)
 * Copyright (c) 2008 INRIA (original v4Ping code)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Chandrakant Jena <chandrakant.barcelona@gmail.com>
 *
 * Derived from original v4Ping application (author: Mathieu Lacage)
 * Derived from original ping6 application (author: Sebastien Vincent)
 */

#ifndef PING_H
#define PING_H

#include "ns3/application.h"
#include "ns3/average.h"
#include "ns3/traced-callback.h"

#include <map>

namespace ns3
{

class Socket;

/**
 * \ingroup internet-apps
 * \defgroup ping Ping
 */

/**
 * \ingroup ping
 *
 * This application behaves similarly to the Unix ping application, although
 * with fewer options supported.  The application can be used to send
 * ICMP echo requests to unicast IPv4 and IPv6 addresses.
 * The application can produce a verbose output similar to the real
 * application, and can also export statistics via a trace source.
 * The ping packet count, packet size, and interval between pings can
 * be controlled via attributes of this class.
 */
class Ping : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \enum VerboseMode
     * \brief Encode three possible levels of verbose output
     */
    enum VerboseMode
    {
        VERBOSE = 0, //!< Verbose output (similar to real ping output)
        QUIET = 1,   //!< Quiet output (similar to real 'ping -q' output)
        SILENT = 2,  //!< Silent output (no terminal output at all)
    };

    /**
     * \enum DropReason
     * \brief Reason why a ping was dropped
     */
    enum DropReason
    {
        DROP_TIMEOUT = 0,      //!< Response timed out
        DROP_HOST_UNREACHABLE, //!< Received ICMP Destination Host Unreachable
        DROP_NET_UNREACHABLE,  //!< Received ICMP Destination Network Unreachable
    };

    /**
     * A ping report provides all of the data that is typically output to the
     * terminal when the application stops, including number sent and received
     * and the RTT statistics.
     */
    struct PingReport
    {
        uint32_t m_transmitted{0}; //!< Number of echo requests sent
        uint32_t m_received{0};    //!< Number of echo replies received
        uint16_t m_loss{0};        //!< Percentage of lost packets (decimal value 0-100)
        Time m_duration{0};        //!< Duration of the application
        double m_rttMin{0};        //!< rtt min value
        double m_rttAvg{0};        //!< rtt avg value
        double m_rttMax{0};        //!< rtt max value
        double m_rttMdev{0};       //!< rtt mdev value
    };

    /**
     * Constructor
     */
    Ping();

    /**
     * Destructor
     */
    ~Ping() override;

    /**
     * \brief Set routers for IPv6 routing type 0 (loose routing).
     * \param routers routers addresses
     */
    void SetRouters(const std::vector<Ipv6Address>& routers);

    /**
     * TracedCallback signature for Rtt trace
     *
     * \param [in] seq The ICMP sequence number
     * \param [in] p The ICMP echo request packet (including ICMP header)
     */
    typedef void (*TxTrace)(uint16_t seq, Ptr<const Packet> p);

    /**
     * TracedCallback signature for Rtt trace
     *
     * \param [in] seq The ICMP sequence number
     * \param [in] rtt The reported RTT
     */
    typedef void (*RttTrace)(uint16_t seq, Time rtt);

    /**
     * TracedCallback signature for Drop trace
     *
     * \param [in] seq The ICMP sequence number
     * \param [in] reason The reason for the reported drop
     */
    typedef void (*DropTrace)(uint16_t seq, DropReason reason);

    /**
     * TracedCallback signature for Report trace
     *
     * \param [in] report The report information
     */
    typedef void (*ReportTrace)(const struct PingReport& report);

  private:
    /**
     * \brief Writes data to buffer in little-endian format.
     *
     * Least significant byte of data is at lowest buffer address
     *
     * \param[out] buffer the buffer to write to
     * \param[in] data the data to write
     */
    void Write64(uint8_t* buffer, const uint64_t data);

    /**
     * \brief Writes data from a little-endian formatted buffer to data.
     *
     * \param buffer the buffer to read from
     * \return the read data
     */
    uint64_t Read64(const uint8_t* buffer);

    // inherited from Application base class.
    void StartApplication() override;
    void StopApplication() override;
    void DoDispose() override;

    /**
     * \brief Return the application signatiure.
     * \returns the application signature.
     *
     * The application signature is the NodeId concatenated with the
     * application index in the node.
     */
    uint64_t GetApplicationSignature() const;

    /**
     * \brief Receive an ICMPv4 or an ICMPv6 Echo reply
     * \param socket the receiving socket
     *
     * This function is called by lower layers through a callback.
     */
    void Receive(Ptr<Socket> socket);

    /**
     * \brief Send one Ping (ICMPv4 ECHO or ICMPv6 ECHO) to the destination
     */
    void Send();

    /**
     * Print the report
     */
    void PrintReport();

    /// Sender Local Address
    Address m_interfaceAddress;
    /// Remote address
    Address m_destination;
    /// Wait interval between ECHO requests
    Time m_interval{Seconds(1)};

    /**
     * Specifies the number of data bytes to be sent.
     * The default is 56, which translates into 64 ICMP data bytes when combined with the 8 bytes of
     * ICMP header data.
     */
    uint32_t m_size{56};
    /// The socket we send packets from
    Ptr<Socket> m_socket;
    /// ICMP ECHO sequence number
    uint16_t m_seq{0};
    /// Callbacks for tracing the packet Tx events
    TracedCallback<uint16_t, Ptr<Packet>> m_txTrace;
    /// TracedCallback for RTT samples
    TracedCallback<uint16_t, Time> m_rttTrace;
    /// TracedCallback for drop events
    TracedCallback<uint16_t, DropReason> m_dropTrace;
    /// TracedCallback for final ping report
    TracedCallback<const struct PingReport&> m_reportTrace;
    /// Variable to stor verbose mode
    VerboseMode m_verbose{VerboseMode::VERBOSE};
    /// Received packets counter
    uint32_t m_recv{0};
    /// Duplicate packets counter
    uint32_t m_duplicate{0};
    /// Start time to report total ping time
    Time m_started;
    /// Average rtt is ms
    Average<double> m_avgRtt;
    /// Next packet will be sent
    EventId m_next;

    /**
     * \brief Sent echo request data.
     */
    class EchoRequestData
    {
      public:
        /**
         * Constructor.
         * \param txTimePar Echo request Tx time.
         * \param ackedPar True if the Echo request has been acknowledged at least once.
         */
        EchoRequestData(Time txTimePar, bool ackedPar)
            : txTime(txTimePar),
              acked(ackedPar)
        {
        }

        Time txTime;       //!< Tx time
        bool acked{false}; //!< True if packet has been acknowledged
    };

    /// All sent but not answered packets. Map icmp seqno -> when sent, acked at least once.
    std::vector<EchoRequestData> m_sent;
    /// Number of packets to be sent.
    uint32_t m_count{0};
    /// Time to wait for a response, in seconds. The option affects only timeout in absence of any
    /// responses, otherwise ping waits for two RTTs
    Time m_timeout{Seconds(1)};
    /// True if the report has been printed already.
    bool m_reportPrinted{false};
    /// Use IPv4 (false) or IPv6 (true)
    bool m_useIpv6{false};
    /// Destination is Broadcast or Multicast
    bool m_multipleDestinations{false};

    /// Routers addresses for IPv6 routing type 0.
    std::vector<Ipv6Address> m_routers;

    /// App signature: ID of the node where the app is installed || ID of the Application.
    uint64_t m_appSignature{0};
};

} // namespace ns3

#endif /* PING_H */
