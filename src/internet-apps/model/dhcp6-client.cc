/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#include "dhcp6-client.h"

#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device-container.h"
#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/trickle-timer.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Client");

TypeId
Dhcp6Client::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Dhcp6Client")
            .SetParent<Application>()
            .AddConstructor<Dhcp6Client>()
            .SetGroupName("InternetApps")
            .AddAttribute("Transactions",
                          "A value to be used as the transaction ID.",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000000.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_transactionId),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("SolicitJitter",
                          "The jitter in ms that a node waits before sending any solicitation. By "
                          "default, the model will wait for a duration in ms defined by a uniform "
                          "random-variable between 0 and SolicitJitter. This is equivalent to"
                          "SOL_MAX_DELAY (RFC 8415, Section 7.6).",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_solicitJitter),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("IaidValue",
                          "The identifier for a new IA created by a client.",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000000.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_iaidStream),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("SolicitInterval",
                          "Time after which the client resends the Solicit."
                          "Equivalent to SOL_MAX_RT (RFC 8415, Section 7.6)",
                          TimeValue(Seconds(100)),
                          MakeTimeAccessor(&Dhcp6Client::m_solicitInterval),
                          MakeTimeChecker())
            .AddTraceSource("NewLease",
                            "The client has obtained a lease",
                            MakeTraceSourceAccessor(&Dhcp6Client::m_newLease),
                            "ns3::Ipv6Address::TracedCallback");
    return tid;
}

Dhcp6Client::Dhcp6Client()
{
    NS_LOG_FUNCTION(this);
}

void
Dhcp6Client::DoDispose()
{
    NS_LOG_FUNCTION(this);

    for (auto& itr : m_interfaces)
    {
        itr.second->Cleanup();
        itr.second = nullptr;
    }
    m_interfaces.clear();
    m_iaidMap.clear();

    Application::DoDispose();
}

int64_t
Dhcp6Client::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_solicitJitter->SetStream(stream);
    m_transactionId->SetStream(stream + 1);
    m_iaidStream->SetStream(stream + 2);
    return 3;
}

bool
Dhcp6Client::ValidateAdvertise(Dhcp6Header header, Ptr<NetDevice> iDev)
{
    Ptr<Packet> packet = Create<Packet>();
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(iDev);
    uint32_t clientTransactId = m_interfaces[ifIndex]->m_transactId;
    uint32_t receivedTransactId = header.GetTransactId();

    if (clientTransactId != receivedTransactId)
    {
        return false;
    }

    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    NS_ASSERT_MSG(clientDuid == m_clientDuid, "Client DUID mismatch.");

    m_interfaces[ifIndex]->m_serverDuid = header.GetServerIdentifier().GetDuid();
    return true;
}

void
Dhcp6Client::SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_FUNCTION(this << iDev << header << server);

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header requestHeader;
    requestHeader.ResetOptions();
    requestHeader.SetMessageType(Dhcp6Header::MessageType::REQUEST);

    // TODO: Use min, max for GetValue
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(iDev);
    const auto& dhcpInterface = m_interfaces[ifIndex];
    dhcpInterface->m_transactId = static_cast<uint32_t>(m_transactionId->GetValue());
    requestHeader.SetTransactId(dhcpInterface->m_transactId);

    // Add Client Identifier Option.
    requestHeader.AddClientIdentifier(m_clientDuid);

    // Add Server Identifier Option, copied from the received header.
    Duid serverDuid = header.GetServerIdentifier().GetDuid();
    requestHeader.AddServerIdentifier(serverDuid);

    // Add Elapsed Time Option.
    uint32_t actualElapsedTime =
        (Simulator::Now() - dhcpInterface->m_msgStartTime).GetMilliSeconds() / 10;
    uint16_t elapsed = actualElapsedTime > 65535 ? 65535 : actualElapsedTime;
    requestHeader.AddElapsedTime(elapsed);

    // Add IA_NA option.
    // Request all addresses from the Advertise message.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();

    for (const auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        for (const auto& iaAddrOpt : iaOpt.m_iaAddressOption)
        {
            requestHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
            requestHeader.AddAddress(iaOpt.GetIaid(),
                                     iaAddrOpt.GetIaAddress(),
                                     iaAddrOpt.GetPreferredLifetime(),
                                     iaAddrOpt.GetValidLifetime());

            NS_LOG_DEBUG("Requesting " << iaAddrOpt.GetIaAddress());
        }
    }

    // Add Option Request.
    header.AddOptionRequest(Options::OptionType::OPTION_SOL_MAX_RT);

    packet->AddHeader(requestHeader);

    // TODO: Handle server unicast option.

    // Send the request message.
    dhcpInterface->m_state = State::WAIT_REPLY;
    if (dhcpInterface->m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Header::SERVER_PORT)) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Request sent.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Request.");
    }
}

Dhcp6Client::InterfaceConfig::InterfaceConfig()
{
    m_renewTime = Seconds(1000);
    m_rebindTime = Seconds(2000);
    m_prefLifetime = Seconds(3000);
    m_validLifetime = Seconds(4000);
    m_nAcceptedAddresses = 0;
}

Dhcp6Client::InterfaceConfig::~InterfaceConfig()
{
    std::cout << "InterfaceConfig::~InterfaceConfig" << std::endl;

    m_socket = nullptr;
    m_client = nullptr;

    m_iaids.clear();
    m_solicitTimer.Stop();
    m_declinedAddresses.clear();

    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();

    for (auto& itr : m_releaseEvent)
    {
        itr.Cancel();
    }
}

void
Dhcp6Client::InterfaceConfig::AcceptedAddress(const Ipv6Address& offeredAddress)
{
    NS_LOG_DEBUG("Accepting address " << offeredAddress);

    // Check that the offered address is from DHCPv6.
    bool found = false;
    for (auto& addr : m_offeredAddresses)
    {
        if (addr == offeredAddress)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        m_nAcceptedAddresses += 1;

        // Notify the new lease.
        m_client->m_newLease(offeredAddress);
        std::cerr << "* got a new lease " << offeredAddress << std::endl;
    }
}

void
Dhcp6Client::InterfaceConfig::DeclinedAddress(const Ipv6Address& offeredAddress)
{
    NS_LOG_DEBUG("Address to be declined " << offeredAddress);

    // Check that the offered address is from DHCPv6.
    bool found = false;
    for (auto& addr : m_offeredAddresses)
    {
        if (addr == offeredAddress)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        m_declinedAddresses.emplace_back(offeredAddress);
        if (m_declinedAddresses.size() + m_nAcceptedAddresses == m_offeredAddresses.size())
        {
            DeclineOffer();
        }
    }
}

void
Dhcp6Client::InterfaceConfig::DeclineOffer()
{
    if (m_declinedAddresses.empty())
    {
        return;
    }

    // Cancel all scheduled Release, Renew, Rebind events.
    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();
    for (auto itr : m_releaseEvent)
    {
        itr.Cancel();
    }

    Dhcp6Header declineHeader;
    Ptr<Packet> packet = Create<Packet>();

    // Remove address associations.
    for (const auto& offer : m_declinedAddresses)
    {
        uint32_t iaid = m_client->m_iaidMap[offer];

        // IA_NA option, IA address option
        declineHeader.AddIanaOption(iaid, m_renewTime.GetSeconds(), m_rebindTime.GetSeconds());
        declineHeader.AddAddress(iaid,
                                 offer,
                                 m_prefLifetime.GetSeconds(),
                                 m_validLifetime.GetSeconds());
        NS_LOG_DEBUG("Declining address " << offer);
    }

    m_transactId = static_cast<uint32_t>(m_client->m_transactionId->GetValue());
    declineHeader.SetTransactId(m_transactId);
    declineHeader.SetMessageType(Dhcp6Header::MessageType::DECLINE);

    // Add client identifier option
    declineHeader.AddClientIdentifier(m_client->m_clientDuid);

    // Add server identifier option
    declineHeader.AddServerIdentifier(m_serverDuid);

    m_msgStartTime = Simulator::Now();
    declineHeader.AddElapsedTime(0);

    packet->AddHeader(declineHeader);
    if ((m_socket->SendTo(packet,
                          0,
                          Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                             Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Decline sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Decline");
    }

    m_state = State::WAIT_REPLY_AFTER_DECLINE;
}

void
Dhcp6Client::InterfaceConfig::Cleanup()
{
    m_solicitTimer.Stop();

    for (auto& releaseEvent : m_releaseEvent)
    {
        releaseEvent.Cancel();
    }

    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();

    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    m_socket->Close();

    Ptr<Ipv6> ipv6 = m_client->GetNode()->GetObject<Ipv6>();

    // Remove the offered IPv6 addresses from the interface.
    for (uint32_t i = 0; i < ipv6->GetNAddresses(m_interfaceIndex); i++)
    {
        for (const auto& addr : m_offeredAddresses)
        {
            if (ipv6->GetAddress(m_interfaceIndex, i) == addr)
            {
                ipv6->RemoveAddress(m_interfaceIndex, i);
                break;
            }
        }
    }
    m_offeredAddresses.clear();

    Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
        ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), m_interfaceIndex));

    icmpv6->TraceDisconnectWithoutContext("DadSuccess", m_acceptedAddressCb.value());
    m_acceptedAddressCb = MakeNullCallback<void, const Ipv6Address&>();
    icmpv6->TraceDisconnectWithoutContext("DadFailure", m_declinedAddressCb.value());
    m_declinedAddressCb = MakeNullCallback<void, const Ipv6Address&>();
}

void
Dhcp6Client::CheckLeaseStatus(Ptr<NetDevice> iDev,
                              Dhcp6Header header,
                              Inet6SocketAddress server) const
{
    // Read Status Code option.
    Options::StatusCodeValues statusCode = header.GetStatusCodeOption().GetStatusCode();

    NS_LOG_DEBUG("Received status " << (uint16_t)statusCode << " from DHCPv6 server");
    if (statusCode == Options::StatusCodeValues::Success)
    {
        NS_LOG_INFO("DHCPv6 client: Server bindings updated successfully.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Server bindings update failed.");
    }
}

void
Dhcp6Client::ProcessReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_FUNCTION(this << iDev << header << server);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(iDev);

    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[ifIndex];

    // Read IA_NA options.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();

    dhcpInterface->m_declinedAddresses.clear();

    Time earliestRebind{Time::Max()};
    Time earliestRenew{Time::Max()};
    std::vector<uint32_t> iaidList;

    for (const auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        for (const auto& iaAddrOpt : iaOpt.m_iaAddressOption)
        {
            Ipv6Address offeredAddress = iaAddrOpt.GetIaAddress();

            // TODO: In Linux, all leased addresses seem to be /128. Double-check this.
            Ipv6InterfaceAddress addr(offeredAddress, 128);
            ipv6->AddAddress(ifIndex, addr);
            ipv6->SetUp(ifIndex);

            // Set the preferred and valid lifetimes.
            dhcpInterface->m_prefLifetime = Seconds(iaAddrOpt.GetPreferredLifetime());
            dhcpInterface->m_validLifetime = Seconds(iaAddrOpt.GetValidLifetime());

            // Add the IPv6 address - IAID association.
            m_iaidMap[offeredAddress] = iaOpt.GetIaid();

            // TODO: Check whether Release event happens for each address.
            dhcpInterface->m_releaseEvent.emplace_back(
                Simulator::Schedule(dhcpInterface->m_validLifetime,
                                    &Dhcp6Client::SendRelease,
                                    this,
                                    offeredAddress));

            dhcpInterface->m_offeredAddresses.push_back(offeredAddress);
        }

        earliestRenew = std::min(earliestRenew, Seconds(iaOpt.GetT1()));
        earliestRebind = std::min(earliestRebind, Seconds(iaOpt.GetT2()));
        iaidList.emplace_back(iaOpt.GetIaid());
    }

    // The renew and rebind events are scheduled for the earliest time across
    // all IA_NA options. RFC 8415, Section 18.2.4.
    dhcpInterface->m_renewTime = earliestRenew;
    dhcpInterface->m_renewEvent.Cancel();
    dhcpInterface->m_renewEvent =
        Simulator::Schedule(dhcpInterface->m_renewTime, &Dhcp6Client::SendRenew, this, ifIndex);

    // Set the rebind timer and schedule the event.
    dhcpInterface->m_rebindTime = earliestRebind;
    dhcpInterface->m_rebindEvent.Cancel();
    dhcpInterface->m_rebindEvent =
        Simulator::Schedule(dhcpInterface->m_rebindTime, &Dhcp6Client::SendRebind, this, ifIndex);

    int32_t interfaceId = ipv6->GetInterfaceForDevice(iDev);
    Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
        ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), interfaceId));

    // If DAD fails, the offer is declined.

    if (!dhcpInterface->m_acceptedAddressCb.has_value())
    {
        dhcpInterface->m_acceptedAddressCb =
            MakeCallback(&Dhcp6Client::InterfaceConfig::AcceptedAddress, dhcpInterface);
        icmpv6->TraceConnectWithoutContext("DadSuccess",
                                           dhcpInterface->m_acceptedAddressCb.value());
    }

    if (!dhcpInterface->m_declinedAddressCb.has_value())
    {
        dhcpInterface->m_declinedAddressCb =
            MakeCallback(&Dhcp6Client::InterfaceConfig::DeclinedAddress, dhcpInterface);
        icmpv6->TraceConnectWithoutContext("DadFailure",
                                           dhcpInterface->m_declinedAddressCb.value());
    }
}

void
Dhcp6Client::SendRenew(uint32_t dhcpInterfaceIndex)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    m_interfaces[dhcpInterfaceIndex]->m_transactId =
        static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(m_interfaces[dhcpInterfaceIndex]->m_transactId);
    header.SetMessageType(Dhcp6Header::MessageType::RENEW);

    // Add client identifier option
    header.AddClientIdentifier(m_clientDuid);

    // Add server identifier option
    header.AddServerIdentifier(m_interfaces[dhcpInterfaceIndex]->m_serverDuid);

    m_interfaces[dhcpInterfaceIndex]->m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (const auto& iaidRenew : m_interfaces[dhcpInterfaceIndex]->m_iaids)
    {
        header.AddIanaOption(iaidRenew,
                             m_interfaces[dhcpInterfaceIndex]->m_renewTime.GetSeconds(),
                             m_interfaces[dhcpInterfaceIndex]->m_rebindTime.GetSeconds());

        // Iterate through the IPv6Address - IAID map, and add all addresses
        // that match the IAID to be renewed.
        for (const auto& itr : m_iaidMap)
        {
            Ipv6Address address = itr.first;
            uint32_t iaid = itr.second;
            if (iaid == iaidRenew)
            {
                header.AddAddress(iaidRenew,
                                  address,
                                  m_interfaces[dhcpInterfaceIndex]->m_prefLifetime.GetSeconds(),
                                  m_interfaces[dhcpInterfaceIndex]->m_validLifetime.GetSeconds());
            }
        }

        NS_LOG_DEBUG("Renewing addresses in IAID " << iaidRenew);
    }

    // Add Option Request option.
    header.AddOptionRequest(Options::OptionType::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_interfaces[dhcpInterfaceIndex]->m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Header::SERVER_PORT))) >=
        0)
    {
        NS_LOG_INFO("DHCPv6 client: Renew sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Renew");
    }

    m_interfaces[dhcpInterfaceIndex]->m_state = State::WAIT_REPLY;
}

void
Dhcp6Client::SendRebind(uint32_t dhcpInterfaceIndex)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    m_interfaces[dhcpInterfaceIndex]->m_transactId =
        static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(m_interfaces[dhcpInterfaceIndex]->m_transactId);
    header.SetMessageType(Dhcp6Header::MessageType::REBIND);

    // Add client identifier option
    header.AddClientIdentifier(m_clientDuid);

    m_interfaces[dhcpInterfaceIndex]->m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (const auto& iaid : m_interfaces[dhcpInterfaceIndex]->m_iaids)
    {
        header.AddIanaOption(iaid,
                             m_interfaces[dhcpInterfaceIndex]->m_renewTime.GetSeconds(),
                             m_interfaces[dhcpInterfaceIndex]->m_rebindTime.GetSeconds());

        NS_LOG_DEBUG("Rebinding addresses in IAID " << iaid);
    }

    // Add Option Request option.
    header.AddOptionRequest(Options::OptionType::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_interfaces[dhcpInterfaceIndex]->m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Header::SERVER_PORT))) >=
        0)
    {
        NS_LOG_INFO("DHCPv6 client: Rebind sent.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Rebind");
    }

    m_interfaces[dhcpInterfaceIndex]->m_state = State::WAIT_REPLY;
}

void
Dhcp6Client::SendRelease(Ipv6Address address)
{
    NS_LOG_FUNCTION(this);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    for (const auto& itr : m_interfaces)
    {
        uint32_t ifIndex = itr.first;
        Ptr<InterfaceConfig> dhcpInterface = itr.second;

        dhcpInterface->m_transactId = static_cast<uint32_t>(m_transactionId->GetValue());
        bool removed = ipv6->RemoveAddress(ifIndex, address);

        if (!removed)
        {
            continue;
        }

        header.SetTransactId(dhcpInterface->m_transactId);
        header.SetMessageType(Dhcp6Header::MessageType::RELEASE);

        // Add client identifier option
        header.AddClientIdentifier(m_clientDuid);

        // Add server identifier option
        header.AddServerIdentifier(dhcpInterface->m_serverDuid);

        dhcpInterface->m_msgStartTime = Simulator::Now();
        header.AddElapsedTime(0);

        // IA_NA option, IA address option
        uint32_t iaid = m_iaidMap[address];
        header.AddIanaOption(iaid,
                             dhcpInterface->m_renewTime.GetSeconds(),
                             dhcpInterface->m_rebindTime.GetSeconds());
        header.AddAddress(iaid,
                          address,
                          dhcpInterface->m_prefLifetime.GetSeconds(),
                          dhcpInterface->m_validLifetime.GetSeconds());

        NS_LOG_DEBUG("Releasing address " << address);

        packet->AddHeader(header);
        if ((dhcpInterface->m_socket->SendTo(packet,
                                             0,
                                             Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                                Dhcp6Header::SERVER_PORT))) >= 0)
        {
            NS_LOG_INFO("DHCPv6 client: Release sent.");
        }
        else
        {
            NS_LOG_INFO("DHCPv6 client: Error while sending Release");
        }

        dhcpInterface->m_state = State::WAIT_REPLY_AFTER_RELEASE;
    }
}

void
Dhcp6Client::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    Dhcp6Header header;

    Inet6SocketAddress senderAddr = Inet6SocketAddress::ConvertFrom(from);

    Ipv6PacketInfoTag interfaceInfo;
    NS_ASSERT_MSG(packet->RemovePacketTag(interfaceInfo),
                  "No incoming interface on DHCPv6 message.");

    uint32_t incomingIf = interfaceInfo.GetRecvIf();
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    Ptr<NetDevice> iDev = GetNode()->GetDevice(incomingIf);
    uint32_t iIf = ipv6->GetInterfaceForDevice(iDev);
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[iIf];

    if (packet->RemoveHeader(header) == 0 || !dhcpInterface)
    {
        return;
    }
    if (dhcpInterface->m_state == State::WAIT_ADVERTISE &&
        header.GetMessageType() == Dhcp6Header::MessageType::ADVERTISE)
    {
        NS_LOG_INFO("DHCPv6 client: Received Advertise.");
        dhcpInterface->m_solicitTimer.Stop();
        bool check = ValidateAdvertise(header, iDev);
        if (check)
        {
            SendRequest(iDev, header, senderAddr);
        }
    }
    if (dhcpInterface->m_state == State::WAIT_REPLY &&
        header.GetMessageType() == Dhcp6Header::MessageType::REPLY)
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");

        dhcpInterface->m_renewEvent.Cancel();
        dhcpInterface->m_rebindEvent.Cancel();
        for (auto itr : dhcpInterface->m_releaseEvent)
        {
            itr.Cancel();
        }

        ProcessReply(iDev, header, senderAddr);
    }
    if ((dhcpInterface->m_state == State::WAIT_REPLY_AFTER_DECLINE ||
         dhcpInterface->m_state == State::WAIT_REPLY_AFTER_RELEASE) &&
        (header.GetMessageType() == Dhcp6Header::MessageType::REPLY))
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");
        CheckLeaseStatus(iDev, header, senderAddr);
    }
}

void
Dhcp6Client::LinkStateHandler(bool isUp, int32_t ifIndex)
{
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[ifIndex];
    if (isUp)
    {
        NS_LOG_DEBUG("DHCPv6 client: Link up at " << Simulator::Now().As(Time::S));
        StartApplication();
    }
    else
    {
        dhcpInterface->Cleanup();
        m_interfaces[ifIndex] = nullptr;
        NS_LOG_DEBUG("DHCPv6 client: Link down at " << Simulator::Now().As(Time::S));
    }
}

Duid
Dhcp6Client::GetSelfDuid() const
{
    return m_clientDuid;
}

void
Dhcp6Client::StartApplication()
{
    Ptr<Node> node = GetNode();

    NS_ASSERT_MSG(node, "Dhcp6Client::StartApplication: cannot get the node from the device.");

    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ipv6, "Dhcp6Client::StartApplication: node does not have IPv6.");

    NS_LOG_DEBUG("Starting DHCPv6 application on node " << node->GetId());

    // Set DHCPv6 callback for each interface of the node.
    uint32_t nInterfaces = ipv6->GetNInterfaces();

    // We skip interface 0 because it's the Loopback.
    for (uint32_t ifIndex = 1; ifIndex < nInterfaces; ifIndex++)
    {
        Ptr<NetDevice> device = ipv6->GetNetDevice(ifIndex);
        Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
            ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), ifIndex));

        // If the RA message contains an M flag, the client starts sending Solicits.
        icmpv6->SetDhcpv6Callback(MakeCallback(&Dhcp6Client::ReceiveMflag, this));
    }
}

void
Dhcp6Client::ReceiveMflag(uint32_t recvInterface)
{
    NS_LOG_FUNCTION(this);

    Ptr<Node> node = GetNode();
    Ptr<Ipv6L3Protocol> ipv6l3 = node->GetObject<Ipv6L3Protocol>();

    if (!m_interfaces[recvInterface])
    {
        // Create config object if M flag is received for the first time.
        Ptr<InterfaceConfig> dhcpInterface = Create<InterfaceConfig>();
        dhcpInterface->m_client = this;
        dhcpInterface->m_interfaceIndex = recvInterface;
        dhcpInterface->m_socket = nullptr;
        m_interfaces[recvInterface] = dhcpInterface;

        // Add an IAID to the client interface.
        // Note: There may be multiple IAIDs per interface. We use only one.
        std::vector<uint32_t> existingIaNaIds;
        while (true)
        {
            uint32_t iaid = m_iaidStream->GetInteger();
            if (std::find(existingIaNaIds.begin(), existingIaNaIds.end(), iaid) ==
                existingIaNaIds.end())
            {
                dhcpInterface->m_iaids.push_back(iaid);
                existingIaNaIds.emplace_back(iaid);
                break;
            }
        }

        Ptr<Ipv6Interface> ipv6Interface =
            node->GetObject<Ipv6L3Protocol>()->GetInterface(recvInterface);
        ipv6Interface->TraceConnectWithoutContext(
            "InterfaceStatus",
            MakeCallback(&Dhcp6Client::LinkStateHandler, this));
    }

    for (const auto& itr : m_interfaces)
    {
        uint32_t interface = itr.first;
        Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
        Ptr<NetDevice> device = ipv6->GetNetDevice(interface);
        Ptr<InterfaceConfig> dhcpInterface = itr.second;

        // Check that RA was received on this interface.
        if (interface == recvInterface && m_interfaces[interface])
        {
            if (!m_interfaces[interface]->m_socket)
            {
                Ipv6Address linkLocal =
                    ipv6l3->GetInterface(interface)->GetLinkLocalAddress().GetAddress();
                TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

                Ptr<Socket> socket = Socket::CreateSocket(node, tid);
                socket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Header::CLIENT_PORT));
                socket->BindToNetDevice(device);
                socket->SetRecvPktInfo(true);
                socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));

                m_interfaces[interface]->m_socket = socket;

                // Introduce a random delay before sending the Solicit message.
                Simulator::Schedule(Time(MilliSeconds(m_solicitJitter->GetValue())),
                                    &Dhcp6Client::Boot,
                                    this,
                                    device);

                uint32_t minInterval = m_solicitInterval.GetSeconds() / 2;
                dhcpInterface->m_solicitTimer = TrickleTimer(Seconds(minInterval), 4, 1);
                dhcpInterface->m_solicitTimer.SetFunction(&Dhcp6Client::Boot, this);
                dhcpInterface->m_solicitTimer.Enable();
                break;
            }
        }
    }
}

void
Dhcp6Client::Boot(Ptr<NetDevice> device)
{
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(device);
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[ifIndex];

    Ptr<Node> node = GetNode();
    if (m_clientDuid.IsInvalid())
    {
        m_clientDuid.Initialize(node);
    }

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    // Create a unique transaction ID.
    dhcpInterface->m_transactId = static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(dhcpInterface->m_transactId);
    header.SetMessageType(Dhcp6Header::MessageType::SOLICIT);

    // Store start time of the message exchange.
    dhcpInterface->m_msgStartTime = Simulator::Now();

    header.AddElapsedTime(0);
    header.AddClientIdentifier(m_clientDuid);
    header.AddOptionRequest(Options::OptionType::OPTION_SOL_MAX_RT);

    // Add IA_NA option.

    for (auto iaid : dhcpInterface->m_iaids)
    {
        header.AddIanaOption(iaid,
                             dhcpInterface->m_renewTime.GetSeconds(),
                             dhcpInterface->m_rebindTime.GetSeconds());
    }

    packet->AddHeader(header);

    if ((dhcpInterface->m_socket->SendTo(packet,
                                         0,
                                         Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                            Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Solicit sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Solicit");
    }

    dhcpInterface->m_state = State::WAIT_ADVERTISE;
}

void
Dhcp6Client::StopApplication()
{
    NS_LOG_FUNCTION(this);

    for (auto& itr : m_interfaces)
    {
        // Close sockets.
        if (!itr.second)
        {
            continue;
        }
        itr.second->Cleanup();
    }

    m_interfaces.clear();
    m_iaidMap.clear();
}

} // namespace ns3
