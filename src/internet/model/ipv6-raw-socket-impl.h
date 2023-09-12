/*
 * Copyright (c) 2007-2009 Strasbourg University
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#ifndef IPV6_RAW_SOCKET_IMPL_H
#define IPV6_RAW_SOCKET_IMPL_H

#include "ipv6-header.h"

#include "ns3/ipv6-address.h"
#include "ns3/socket.h"

#include <list>

namespace ns3
{

class NetDevice;
class Node;

/**
 * \ingroup socket
 * \ingroup ipv6
 *
 * \brief IPv6 raw socket.
 *
 * A RAW Socket typically is used to access specific IP layers not usually
 * available through L4 sockets, e.g., ICMP. The implementer should take
 * particular care to define the Ipv6RawSocketImpl Attributes, and in
 * particular the Protocol attribute.
 * Not setting it will result in a zero protocol at IP level (corresponding
 * to the HopByHop IPv6 Extension header, i.e., Ipv6ExtensionHopByHopHeader)
 * when sending data through the socket, which is probably not the intended
 * behavior.
 *
 * A correct example is (from src/applications/model/radvd.cc):
 * \code
   if (!m_socket)
     {
       TypeId tid = TypeId::LookupByName ("ns3::Ipv6RawSocketFactory");
       m_socket = Socket::CreateSocket (GetNode (), tid);

       NS_ASSERT (m_socket);

       m_socket->SetAttribute ("Protocol", UintegerValue(Ipv6Header::IPV6_ICMPV6));
       m_socket->SetRecvCallback (MakeCallback (&Radvd::HandleRead, this));
     }
 * \endcode
 *
 */
class Ipv6RawSocketImpl : public Socket
{
  public:
    /**
     * \brief Get the type ID of this class.
     * \return type ID
     */
    static TypeId GetTypeId();

    Ipv6RawSocketImpl();
    ~Ipv6RawSocketImpl() override;

    /**
     * \brief Set the node associated with this socket.
     * \param node node to set
     */
    void SetNode(Ptr<Node> node);

    Socket::SocketErrno GetErrno() const override;

    /**
     * \brief Get socket type (NS3_SOCK_RAW)
     * \return socket type
     */
    Socket::SocketType GetSocketType() const override;

    Ptr<Node> GetNode() const override;

    int Bind(const Address& address) override;
    int Bind() override;
    int Bind6() override;

    int GetSockName(Address& address) const override;
    int GetPeerName(Address& address) const override;

    int Close() override;
    int ShutdownSend() override;
    int ShutdownRecv() override;
    int Connect(const Address& address) override;
    int Listen() override;
    uint32_t GetTxAvailable() const override;
    uint32_t GetRxAvailable() const override;
    int Send(Ptr<Packet> p, uint32_t flags) override;
    int SendTo(Ptr<Packet> p, uint32_t flags, const Address& toAddress) override;
    Ptr<Packet> Recv(uint32_t maxSize, uint32_t flags) override;
    Ptr<Packet> RecvFrom(uint32_t maxSize, uint32_t flags, Address& fromAddress) override;
    void Ipv6JoinGroup(Ipv6Address address,
                       Socket::Ipv6MulticastFilterMode filterMode,
                       std::vector<Ipv6Address> sourceAddresses) override;

    /**
     * \brief Set protocol field.
     * \param protocol protocol to set
     */
    void SetProtocol(uint16_t protocol);

    /**
     * \brief Forward up to receive method.
     * \param p packet
     * \param hdr IPv6 header
     * \param device device
     * \return true if forwarded, false otherwise
     */
    bool ForwardUp(Ptr<const Packet> p, Ipv6Header hdr, Ptr<NetDevice> device);

    bool SetAllowBroadcast(bool allowBroadcast) override;
    bool GetAllowBroadcast() const override;

    /**
     * \brief Clean the ICMPv6 filter structure
     */
    void Icmpv6FilterSetPassAll();

    /**
     * \brief Set the filter to block all the ICMPv6 types
     */
    void Icmpv6FilterSetBlockAll();

    /**
     * \brief Set the filter to pass one ICMPv6 type
     * \param type the ICMPv6 type to pass
     */
    void Icmpv6FilterSetPass(uint8_t type);

    /**
     * \brief Set the filter to block one ICMPv6 type
     * \param type the ICMPv6 type to block
     */
    void Icmpv6FilterSetBlock(uint8_t type);

    /**
     * \brief Ask the filter about the status of one ICMPv6 type
     * \param type the ICMPv6 type
     * \return true if the ICMP type is passing through
     */
    bool Icmpv6FilterWillPass(uint8_t type);

    /**
     * \brief Ask the filter about the status of one ICMPv6 type
     * \param type the ICMPv6 type
     * \return true if the ICMP type is being blocked
     */
    bool Icmpv6FilterWillBlock(uint8_t type);

  private:
    /**
     * \brief IPv6 raw data and additional information.
     */
    struct Data
    {
        Ptr<Packet> packet;    /**< Packet data */
        Ipv6Address fromIp;    /**< Source address */
        uint16_t fromProtocol; /**< Protocol used */
    };

    /**
     * \brief Dispose object.
     */
    void DoDispose() override;

    /**
     * \brief Last error number.
     */
    mutable Socket::SocketErrno m_err;

    /**
     * \brief Node.
     */
    Ptr<Node> m_node;

    /**
     * \brief Source address.
     */
    Ipv6Address m_src;

    /**
     * \brief Destination address.
     */
    Ipv6Address m_dst;

    /**
     * \brief Protocol.
     */
    uint16_t m_protocol;

    /**
     * \brief Packet waiting to be processed.
     */
    std::list<Data> m_data;

    /**
     * \brief Flag to shutdown send capability.
     */
    bool m_shutdownSend;

    /**
     * \brief Flag to shutdown receive capability.
     */
    bool m_shutdownRecv;

    /**
     * \brief Struct to hold the ICMPv6 filter
     */
    struct Icmpv6Filter
    {
        uint32_t icmpv6Filt[8]; //!< ICMPv6 filter specification
    };

    /**
     * \brief ICMPv6 filter.
     */
    Icmpv6Filter m_icmpFilter;
};

} /* namespace ns3 */

#endif /* IPV6_RAW_SOCKET_IMPL_H */
