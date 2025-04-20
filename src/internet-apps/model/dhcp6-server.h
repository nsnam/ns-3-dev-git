/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_SERVER_H
#define DHCP6_SERVER_H

#include "dhcp6-duid.h"
#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/ipv6-address.h"
#include "ns3/net-device-container.h"
#include "ns3/pair.h"
#include "ns3/ptr.h"

#include <map>

namespace ns3
{

class Inet6SocketAddress;
class Socket;
class Packet;

/**
 * @ingroup dhcp6
 *
 * @class LeaseInfo
 * @brief Includes information about available subnets and corresponding leases.
 */
class LeaseInfo
{
  public:
    /**
     * Constructor.
     * @param addressPool Address pool
     * @param prefix Prefix of the address pool
     * @param minAddress Minimum address in the pool
     * @param maxAddress Maximum address in the pool
     */
    LeaseInfo(Ipv6Address addressPool,
              Ipv6Prefix prefix,
              Ipv6Address minAddress,
              Ipv6Address maxAddress);

    friend class Dhcp6Server;

  private:
    /**
     * @brief Get the address pool.
     * @return The address pool
     */
    Ipv6Address GetAddressPool() const;

    /**
     * @brief Get the prefix of the address pool.
     * @return The prefix of the address pool
     */
    Ipv6Prefix GetPrefix() const;

    /**
     * @brief Get the minimum address in the pool.
     * @return The minimum address in the pool
     */
    Ipv6Address GetMinAddress() const;

    /**
     * @brief Get the maximum address in the pool.
     * @return The maximum address in the pool
     */
    Ipv6Address GetMaxAddress() const;

    /**
     * @brief Get the number of addresses leased.
     * @return The number of addresses leased
     */
    uint32_t GetNumAddresses() const;

    /**
     * @brief Expired Addresses (Section 6.2 of RFC 8415)
     * Expired time / Ipv6Address
     */
    typedef std::multimap<Time, std::pair<Duid, Ipv6Address>> ExpiredAddresses;

    /**
     * @brief Leased Addresses
     * Client DUID + Ipv6Address / Lease time
     */
    typedef std::unordered_multimap<Duid, std::pair<Ipv6Address, Time>, Duid::DuidHash>
        LeasedAddresses;

    /**
     * @brief Declined Addresses
     * Ipv6Address + Client DUID
     */
    typedef std::unordered_map<Ipv6Address, Duid, Ipv6AddressHash> DeclinedAddresses;

    LeasedAddresses m_leasedAddresses;     //!< Leased addresses
    ExpiredAddresses m_expiredAddresses;   //!< Expired addresses
    DeclinedAddresses m_declinedAddresses; //!< Declined addresses
    Ipv6Address m_maxOfferedAddress;       //!< Maximum address offered so far.

    Ipv6Address m_addressPool; //!< Address pool
    Ipv6Prefix m_prefix;       //!< Prefix of the address pool
    Ipv6Address m_minAddress;  //!< Minimum address in the pool
    Ipv6Address m_maxAddress;  //!< Maximum address in the pool
    uint32_t m_numAddresses;   //!< Number of addresses leased.
};

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Server
 * @brief Implements the DHCPv6 server.
 */
class Dhcp6Server : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor.
     */
    Dhcp6Server();

    /**
     * @brief Set the list of net devices that the DHCPv6 server will use.
     * @param netDevices The net devices that the server will listen on.
     */
    void SetDhcp6ServerNetDevice(NetDeviceContainer netDevices);

    /**
     * @brief Add a managed address pool.
     * @param pool The address pool to be managed by the server.
     * @param prefix The prefix of the address pool.
     * @param minAddress The minimum address in the pool.
     * @param maxAddress The maximum address in the pool.
     */
    void AddSubnet(Ipv6Address pool,
                   Ipv6Prefix prefix,
                   Ipv6Address minAddress,
                   Ipv6Address maxAddress);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Handles incoming packets from the network
     * @param socket Socket bound to port 547 of the DHCP server
     */
    void NetHandler(Ptr<Socket> socket);

    /**
     * @brief Sends DHCPv6 Advertise after receiving DHCPv6 Solicit.
     * @param iDev incoming NetDevice
     * @param header DHCPv6 header of the received message
     * @param client Address of the DHCPv6 client
     */
    void ProcessSolicit(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * @brief Sends DHCPv6 Advertise after receiving DHCPv6 Solicit.
     * @param iDev incoming NetDevice
     * @param header DHCPv6 header of the received message
     * @param client Address of the DHCPv6 client
     */
    void SendAdvertise(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * @brief Sends Reply after receiving Request
     * @param iDev incoming NetDevice
     * @param header DHCPv6 header of the received message
     * @param client Address of the DHCP client
     */
    void SendReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * @brief Sends Reply after receiving Request
     * @param iDev incoming NetDevice
     * @param header DHCPv6 header of the received message
     * @param client Address of the DHCP client
     */
    void RenewRebindLeases(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * @brief Sends Reply after receiving Request
     * @param iDev incoming NetDevice
     * @param header DHCPv6 header of the received message
     * @param client Address of the DHCP client
     */
    void UpdateBindings(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * @brief Modifies the remaining lease time of addresses
     */
    void TimerHandler();

    /**
     * @brief Callback for when an M flag is received.
     * @param recvInterface The interface on which the M flag was received.
     */
    void ReceiveMflag(uint32_t recvInterface);

    /**
     * @brief Clean up stale lease info.
     */
    void CleanLeases();

    Ptr<Socket> m_recvSocket;            //!< Socket bound to port 547.
    Duid m_serverDuid;                   //!< Server DUID
    std::vector<LeaseInfo> m_subnets;    //!< List of managed subnets.
    Time m_leaseCleanup = Seconds(10.0); //!< Lease cleanup time
    EventId m_leaseCleanupEvent;         //!< Event ID for lease cleanup

    /// Map of NetDevice - Corresponding socket used to send packets.
    std::unordered_map<uint32_t, Ptr<Socket>> m_sendSockets;

    /// Store IA bindings. Map of DUID + IA Type / IAID
    std::multimap<Duid, std::pair<Options::OptionType, uint32_t>> m_iaBindings;

    /**
     * @brief Default preferred lifetime for an address.
     * According to ISC's Kea guide, the default preferred lifetime is 3000
     * seconds.
     */
    Time m_prefLifetime;

    /**
     * @brief Default valid lifetime.
     * According to ISC's Kea guide, the default valid lifetime is 4000 seconds.
     */
    Time m_validLifetime;

    /**
     * @brief The default renew timer.
     * This defines the T1 timer. According to ISC's Kea guide, the default
     * renew timer is 1000 seconds.
     * Maximum value is REN_MAX_RT (RFC 8415, Section 7.6).
     */
    Time m_renew;

    /**
     * @brief The default rebind timer.
     * This defines the T2 timer. According to ISC's Kea guide, the default
     * rebind timer is 2000 seconds.
     * Maximum value is REB_MAX_RT (RFC 8415, Section 7.6).
     */
    Time m_rebind;
};
} // namespace ns3

#endif
