/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_CLIENT_H
#define DHCP6_CLIENT_H

#include "dhcp6-duid.h"
#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/net-device-container.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/trickle-timer.h"

#include <optional>

namespace ns3
{

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Client
 * @brief Implements the DHCPv6 client.
 */
class Dhcp6Client : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Dhcp6Client();

    /**
     * @brief Get the DUID.
     * @return The DUID which identifies the client.
     */
    Duid GetSelfDuid() const;

    /// State of the DHCPv6 client.
    enum class State
    {
        WAIT_ADVERTISE,           // Waiting for an advertise message
        WAIT_REPLY,               // Waiting for a reply message
        RENEW,                    // Renewing the lease
        WAIT_REPLY_AFTER_DECLINE, // Waiting for a reply after sending a decline message
        WAIT_REPLY_AFTER_RELEASE, // Waiting for a reply after sending a release message
    };

    int64_t AssignStreams(int64_t stream) override;

    // protected:
    void DoDispose() override;

    //  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @ingroup dhcp6
     *
     * @class InterfaceConfig
     * @brief DHCPv6 client config details about each interface on the node.
     */
    class InterfaceConfig : public SimpleRefCount<InterfaceConfig>
    {
      public:
        /**
         * The default constructor.
         */
        InterfaceConfig();

        /**
         * Destructor.
         */
        ~InterfaceConfig();

        /// The Dhcp6Client on which the interface is present.
        Ptr<Dhcp6Client> m_client;

        /// The IPv6 interface index of this configuration.
        uint32_t m_interfaceIndex;

        /// The socket that has been opened for this interface.
        Ptr<Socket> m_socket;

        /// The DHCPv6 state of the client interface.
        State m_state;

        /// The server DUID.
        Duid m_serverDuid;

        /// The IAIDs associated with this DHCPv6 client interface.
        std::vector<uint32_t> m_iaids;

        TrickleTimer m_solicitTimer;  //!< TrickleTimer to schedule Solicit messages.
        Time m_msgStartTime;          //!< Time when message exchange starts.
        uint32_t m_transactId;        //!< Transaction ID of the client-initiated message.
        uint8_t m_nAcceptedAddresses; //!< Number of addresses accepted by client.

        /// List of addresses offered to the client.
        std::vector<Ipv6Address> m_offeredAddresses;

        /// List of addresses to be declined by the client.
        std::vector<Ipv6Address> m_declinedAddresses;

        Time m_renewTime;     //!< REN_MAX_RT, Time after which lease should be renewed.
        Time m_rebindTime;    //!< REB_MAX_RT, Time after which client should send a Rebind message.
        Time m_prefLifetime;  //!< Preferred lifetime of the address.
        Time m_validLifetime; //!< Valid lifetime of the address.
        EventId m_renewEvent; //!< Event ID for the Renew event.
        EventId m_rebindEvent; //!< Event ID for the rebind event

        /// Store all the Event IDs for the addresses being Released.
        std::vector<EventId> m_releaseEvent;

        /**
         * @brief Accept the DHCPv6 offer.
         * @param offeredAddress The IPv6 address that has been accepted.
         */
        void AcceptedAddress(const Ipv6Address& offeredAddress);

        /// Callback for the accepted addresses - needed for cleanup.
        std::optional<Callback<void, const Ipv6Address&>> m_acceptedAddressCb{std::nullopt};

        /**
         * @brief Add a declined address to the list maintained by the client.
         * @param offeredAddress The IPv6 address to be declined.
         */
        void DeclinedAddress(const Ipv6Address& offeredAddress);

        /// Callback for the declined addresses - needed for cleanup.
        std::optional<Callback<void, const Ipv6Address&>> m_declinedAddressCb{std::nullopt};

        /**
         * @brief Send a Decline message to the DHCPv6 server
         */
        void DeclineOffer();

        /**
         * @brief Cleanup the internal callbacks and timers.
         *
         * MUST be used before removing an interface to avoid circular references locks.
         */
        void Cleanup();
    };

    /**
     * @brief Verify the incoming advertise message.
     * @param header The DHCPv6 header received.
     * @param iDev The incoming NetDevice.
     * @return True if the Advertise is valid.
     */
    bool ValidateAdvertise(Dhcp6Header header, Ptr<NetDevice> iDev);

    /**
     * @brief Send a request to the DHCPv6 server.
     * @param iDev The outgoing NetDevice
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * @brief Send a request to the DHCPv6 server.
     * @param iDev The outgoing NetDevice
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void ProcessReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * @brief Check lease status after sending a Decline or Release message.
     * @param iDev The incoming NetDevice
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void CheckLeaseStatus(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server) const;

    /**
     * @brief Send a renew message to the DHCPv6 server.
     * @param interfaceIndex The interface whose leases are to be renewed.
     */
    void SendRenew(uint32_t interfaceIndex);

    /**
     * @brief Send a rebind message to the DHCPv6 server.
     * @param interfaceIndex The interface whose leases are to be rebound.
     */
    void SendRebind(uint32_t interfaceIndex);

    /**
     * @brief Send a Release message to the DHCPv6 server.
     * @param address The address whose lease is to be released.
     */
    void SendRelease(Ipv6Address address);

    /**
     * @brief Handles incoming packets from the network
     * @param socket incoming Socket
     */
    void NetHandler(Ptr<Socket> socket);

    /**
     * @brief Handle changes in the link state.
     * @param isUp Indicates whether the interface is up.
     * @param ifIndex The interface index.
     */
    void LinkStateHandler(bool isUp, int32_t ifIndex);

    /**
     * @brief Callback for when an M flag is received.
     *
     * The M flag is carried by Router Advertisements (RA), and it is a signal
     * that the DHCPv6 client must search for a DHCPv6 Server.
     *
     * @param recvInterface The interface on which the M flag was received.
     */
    void ReceiveMflag(uint32_t recvInterface);

    /**
     * @brief Used to send the Solicit message and start the DHCPv6 client.
     * @param device The client interface.
     */
    void Boot(Ptr<NetDevice> device);

    /**
     * @brief Retrieve all existing IAIDs.
     * @return A list of all IAIDs.
     */
    std::vector<uint32_t> GetIaids();

    /// Map each interface to its corresponding configuration details.
    std::unordered_map<uint32_t, Ptr<InterfaceConfig>> m_interfaces;

    Duid m_clientDuid;                             //!< The client DUID.
    TracedCallback<const Ipv6Address&> m_newLease; //!< Trace the new lease.

    /// Track the IPv6 Address - IAID association.
    std::unordered_map<Ipv6Address, uint32_t, Ipv6AddressHash> m_iaidMap;

    /// Random variable to set transaction ID
    Ptr<RandomVariableStream> m_transactionId;

    Time m_solicitInterval; //!< SOL_MAX_RT, time between solicitations.

    /**
     * Random jitter before sending the first Solicit. Equivalent to
     * SOL_MAX_DELAY (RFC 8415, Section 7.6)
     */
    Ptr<RandomVariableStream> m_solicitJitter;

    /// Random variable used to create the IAID.
    Ptr<RandomVariableStream> m_iaidStream;
};

/**
 * @brief Stream output operator
 * @param os output stream
 * @param h the Dhcp6Client
 * @return updated stream
 */
std::ostream& operator<<(std::ostream& os, const Dhcp6Client& h);

} // namespace ns3

#endif
