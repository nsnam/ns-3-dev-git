/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef BULK_SEND_APPLICATION_H
#define BULK_SEND_APPLICATION_H

#include "seq-ts-size-header.h"
#include "source-application.h"

#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Socket;
class TcpHeader;
class TcpSocketBase;

/**
 * @ingroup applications
 * @defgroup bulksend BulkSendApplication
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM
 * and SOCK_SEQPACKET sockets are supported.
 * For example, TCP sockets can be used, but
 * UDP sockets can not be used.
 */

/**
 * @ingroup bulksend
 *
 * @brief Send as much traffic as possible, trying to fill the bandwidth.
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the application is stopped (if MaxBytes is
 * zero). Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM
 * and SOCK_SEQPACKET sockets are supported.
 * For example, TCP sockets can be used, but
 * UDP sockets can not be used.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting
 * statistics from this header have been added to \c ns3::PacketSink
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.
 */
class BulkSendApplication : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    BulkSendApplication();
    ~BulkSendApplication() override;

    /**
     * @brief Set the upper bound for the total number of bytes to send.
     *
     * Once this bound is reached, no more application bytes are sent. If the
     * application is stopped during the simulation and restarted, the
     * total number of bytes sent is not reset; however, the maxBytes
     * bound is still effective and the application will continue sending
     * up to maxBytes. The value zero for maxBytes means that
     * there is no upper bound; i.e. data is sent until the application
     * or simulation is stopped.
     *
     * @param maxBytes the upper bound of bytes to send
     */
    void SetMaxBytes(uint64_t maxBytes);

    /**
     * @brief Get the socket this application is attached to.
     * @return pointer to associated socket
     */
    Ptr<Socket> GetSocket() const;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Send data until the L4 transmission buffer is full.
     * @param from From address
     * @param to To address
     */
    void SendData(const Address& from, const Address& to);

    Ptr<Socket> m_socket;                //!< Associated socket
    bool m_connected;                    //!< True if connected
    uint32_t m_sendSize;                 //!< Size of data to send each time
    uint64_t m_maxBytes;                 //!< Limit total number of bytes sent
    uint64_t m_totBytes;                 //!< Total bytes sent so far
    TypeId m_tid;                        //!< The type of protocol to use.
    uint32_t m_seq{0};                   //!< Sequence
    Ptr<Packet> m_unsentPacket;          //!< Variable to cache unsent packet
    bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the SeqTsSizeHeader

    /// Traced Callback: retransmitted packets
    TracedCallback<Ptr<const Packet>,
                   const TcpHeader&,
                   const Address&,
                   const Address&,
                   Ptr<const TcpSocketBase>>
        m_retransmissionTrace;

    /// Callback for tracing the packet Tx events, includes source, destination,  the packet sent,
    /// and header
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_txTraceWithSeqTsSize;

  private:
    /**
     * @brief Connection Succeeded (called by Socket through a callback)
     * @param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);
    /**
     * @brief Connection Failed (called by Socket through a callback)
     * @param socket the connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);
    /**
     * @brief Send more data as soon as some has been transmitted.
     *
     * Used in socket's SetSendCallback - params are forced by it.
     *
     * @param socket socket to use
     * @param unused actually unused
     */
    void DataSend(Ptr<Socket> socket, uint32_t unused);

    /**
     *  @brief Packet retransmitted (called by TcpSocketBase sockets via callback)
     *  @param p the retransmitted packet
     *  @param header the TCP header
     *  @param localAddr the local address
     *  @param peerAddr the peer address
     *  @param socket the socket that retransmitted the packet
     */
    void PacketRetransmitted(Ptr<const Packet> p,
                             const TcpHeader& header,
                             const Address& localAddr,
                             const Address& peerAddr,
                             Ptr<const TcpSocketBase> socket);
};

} // namespace ns3

#endif /* BULK_SEND_APPLICATION_H */
