/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef UDP_SOCKET_IMPL_H
#define UDP_SOCKET_IMPL_H

#include "icmpv4.h"
#include "ipv4-interface.h"
#include "udp-socket.h"

#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"

#include <queue>
#include <stdint.h>

namespace ns3
{

class Ipv4EndPoint;
class Ipv6EndPoint;
class Node;
class Packet;
class UdpL4Protocol;
class Ipv6Header;
class Ipv6Interface;

/**
 * @ingroup socket
 * @ingroup udp
 *
 * @brief A sockets interface to UDP
 *
 * This class subclasses ns3::UdpSocket, and provides a socket interface
 * to ns3's implementation of UDP.
 *
 * For IPv4 packets, the TOS is set according to the following rules:
 * - if the socket is connected, the TOS set for the socket is used
 * - if the socket is not connected, the TOS specified in the destination address
 *   passed to SendTo is used, while the TOS set for the socket is ignored
 * In both cases, a SocketIpTos tag is only added to the packet if the resulting
 * TOS is non-null. The Bind and Connect operations set the TOS for the
 * socket to the value specified in the provided address.
 * If the TOS determined for a packet (as described above) is not null, the
 * packet is assigned a priority based on that TOS value (according to the
 * Socket::IpTos2Priority function). Otherwise, the priority set for the
 * socket is assigned to the packet. Setting a TOS for a socket also sets a
 * priority for the socket (according to the Socket::IpTos2Priority function).
 * A SocketPriority tag is only added to the packet if the resulting priority
 * is non-null.
 */

class UdpSocketImpl : public UdpSocket
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Create an unbound udp socket.
     */
    UdpSocketImpl();
    ~UdpSocketImpl() override;

    /**
     * @brief Set the associated node.
     * @param node the node
     */
    void SetNode(Ptr<Node> node);
    /**
     * @brief Set the associated UDP L4 protocol.
     * @param udp the UDP L4 protocol
     */
    void SetUdp(Ptr<UdpL4Protocol> udp);

    SocketErrno GetErrno() const override;
    SocketType GetSocketType() const override;
    Ptr<Node> GetNode() const override;
    int Bind() override;
    int Bind6() override;
    int Bind(const Address& address) override;
    int Close() override;
    int ShutdownSend() override;
    int ShutdownRecv() override;
    int Connect(const Address& address) override;
    int Listen() override;
    uint32_t GetTxAvailable() const override;
    int Send(Ptr<Packet> p, uint32_t flags) override;
    int SendTo(Ptr<Packet> p, uint32_t flags, const Address& address) override;
    uint32_t GetRxAvailable() const override;
    Ptr<Packet> Recv(uint32_t maxSize, uint32_t flags) override;
    Ptr<Packet> RecvFrom(uint32_t maxSize, uint32_t flags, Address& fromAddress) override;
    int GetSockName(Address& address) const override;
    int GetPeerName(Address& address) const override;
    int MulticastJoinGroup(uint32_t interfaceIndex, const Address& groupAddress) override;
    int MulticastLeaveGroup(uint32_t interfaceIndex, const Address& groupAddress) override;
    void BindToNetDevice(Ptr<NetDevice> netdevice) override;
    bool SetAllowBroadcast(bool allowBroadcast) override;
    bool GetAllowBroadcast() const override;
    void Ipv6JoinGroup(Ipv6Address address,
                       Socket::Ipv6MulticastFilterMode filterMode,
                       std::vector<Ipv6Address> sourceAddresses) override;

  private:
    // Attributes set through UdpSocket base class
    void SetRcvBufSize(uint32_t size) override;
    uint32_t GetRcvBufSize() const override;
    void SetIpMulticastTtl(uint8_t ipTtl) override;
    uint8_t GetIpMulticastTtl() const override;
    void SetIpMulticastIf(int32_t ipIf) override;
    int32_t GetIpMulticastIf() const override;
    void SetIpMulticastLoop(bool loop) override;
    bool GetIpMulticastLoop() const override;
    void SetMtuDiscover(bool discover) override;
    bool GetMtuDiscover() const override;

    /**
     * @brief UdpSocketFactory friend class.
     * @relates UdpSocketFactory
     */
    friend class UdpSocketFactory;
    // invoked by Udp class

    /**
     * Finish the binding process
     * @returns 0 on success, -1 on failure
     */
    int FinishBind();

    /**
     * @brief Called by the L3 protocol when it received a packet to pass on to TCP.
     *
     * @param packet the incoming packet
     * @param header the packet's IPv4 header
     * @param port the remote port
     * @param incomingInterface the incoming interface
     */
    void ForwardUp(Ptr<Packet> packet,
                   Ipv4Header header,
                   uint16_t port,
                   Ptr<Ipv4Interface> incomingInterface);

    /**
     * @brief Called by the L3 protocol when it received a packet to pass on to TCP.
     *
     * @param packet the incoming packet
     * @param header the packet's IPv6 header
     * @param port the remote port
     * @param incomingInterface the incoming interface
     */
    void ForwardUp6(Ptr<Packet> packet,
                    Ipv6Header header,
                    uint16_t port,
                    Ptr<Ipv6Interface> incomingInterface);

    /**
     * @brief Kill this socket by zeroing its attributes (IPv4)
     *
     * This is a callback function configured to m_endpoint in
     * SetupCallback(), invoked when the endpoint is destroyed.
     */
    void Destroy();

    /**
     * @brief Kill this socket by zeroing its attributes (IPv6)
     *
     * This is a callback function configured to m_endpoint in
     * SetupCallback(), invoked when the endpoint is destroyed.
     */
    void Destroy6();

    /**
     * @brief Deallocate m_endPoint and m_endPoint6
     */
    void DeallocateEndPoint();

    /**
     * @brief Send a packet
     * @param p packet
     * @returns 0 on success, -1 on failure
     */
    int DoSend(Ptr<Packet> p);
    /**
     * @brief Send a packet to a specific destination and port (IPv4)
     * @param p packet
     * @param daddr destination address
     * @param dport destination port
     * @param tos ToS
     * @returns 0 on success, -1 on failure
     */
    int DoSendTo(Ptr<Packet> p, Ipv4Address daddr, uint16_t dport, uint8_t tos);
    /**
     * @brief Send a packet to a specific destination and port (IPv6)
     * @param p packet
     * @param daddr destination address
     * @param dport destination port
     * @returns 0 on success, -1 on failure
     */
    int DoSendTo(Ptr<Packet> p, Ipv6Address daddr, uint16_t dport);

    /**
     * @brief Called by the L3 protocol when it received an ICMP packet to pass on to TCP.
     *
     * @param icmpSource the ICMP source address
     * @param icmpTtl the ICMP Time to Live
     * @param icmpType the ICMP Type
     * @param icmpCode the ICMP Code
     * @param icmpInfo the ICMP Info
     */
    void ForwardIcmp(Ipv4Address icmpSource,
                     uint8_t icmpTtl,
                     uint8_t icmpType,
                     uint8_t icmpCode,
                     uint32_t icmpInfo);

    /**
     * @brief Called by the L3 protocol when it received an ICMPv6 packet to pass on to TCP.
     *
     * @param icmpSource the ICMP source address
     * @param icmpTtl the ICMP Time to Live
     * @param icmpType the ICMP Type
     * @param icmpCode the ICMP Code
     * @param icmpInfo the ICMP Info
     */
    void ForwardIcmp6(Ipv6Address icmpSource,
                      uint8_t icmpTtl,
                      uint8_t icmpType,
                      uint8_t icmpCode,
                      uint32_t icmpInfo);

    // Connections to other layers of TCP/IP
    Ipv4EndPoint* m_endPoint;  //!< the IPv4 endpoint
    Ipv6EndPoint* m_endPoint6; //!< the IPv6 endpoint
    Ptr<Node> m_node;          //!< the associated node
    Ptr<UdpL4Protocol> m_udp;  //!< the associated UDP L4 protocol
    Callback<void, Ipv4Address, uint8_t, uint8_t, uint8_t, uint32_t>
        m_icmpCallback; //!< ICMP callback
    Callback<void, Ipv6Address, uint8_t, uint8_t, uint8_t, uint32_t>
        m_icmpCallback6; //!< ICMPv6 callback

    Address m_defaultAddress;                      //!< Default address
    uint16_t m_defaultPort;                        //!< Default port
    TracedCallback<Ptr<const Packet>> m_dropTrace; //!< Trace for dropped packets

    mutable SocketErrno m_errno; //!< Socket error code
    bool m_shutdownSend;         //!< Send no longer allowed
    bool m_shutdownRecv;         //!< Receive no longer allowed
    bool m_connected;            //!< Connection established
    bool m_allowBroadcast;       //!< Allow send broadcast packets

    std::queue<std::pair<Ptr<Packet>, Address>> m_deliveryQueue; //!< Queue for incoming packets
    uint32_t m_rxAvailable; //!< Number of available bytes to be received

    // Socket attributes
    uint32_t m_rcvBufSize;    //!< Receive buffer size
    uint8_t m_ipMulticastTtl; //!< Multicast TTL
    int32_t m_ipMulticastIf;  //!< Multicast Interface
    bool m_ipMulticastLoop;   //!< Allow multicast loop
    bool m_mtuDiscover;       //!< Allow MTU discovery
};

} // namespace ns3

#endif /* UDP_SOCKET_IMPL_H */
