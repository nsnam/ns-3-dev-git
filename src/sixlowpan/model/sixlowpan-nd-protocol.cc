/*
 * Copyright (c) 2020 Università di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Adnan Rashid <adnanrashidpk@gmail.com>
 *         Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#include "sixlowpan-nd-protocol.h"

#include "sixlowpan-header.h"
#include "sixlowpan-nd-context.h"
#include "sixlowpan-nd-header.h"
#include "sixlowpan-nd-prefix.h"
#include "sixlowpan-net-device.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/iana-ieee802-numbers.h"
#include "ns3/integer.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/mac16-address.h"
#include "ns3/mac48-address.h"
#include "ns3/mac64-address.h"
#include "ns3/ndisc-cache.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanNdProtocol");

NS_OBJECT_ENSURE_REGISTERED(SixLowPanNdProtocol);

SixLowPanNdProtocol::SixLowPanNdProtocol()
    : Icmpv6L4Protocol()
{
    NS_LOG_FUNCTION(this);

    m_nodeRole = SixLowPanNode;
}

SixLowPanNdProtocol::~SixLowPanNdProtocol()
{
    NS_LOG_FUNCTION(this);
}

TypeId
SixLowPanNdProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanNdProtocol")
            .SetParent<Icmpv6L4Protocol>()
            .SetGroupName("Internet")
            .AddConstructor<SixLowPanNdProtocol>()
            .AddAttribute("AddressRegistrationJitter",
                          "The jitter in ms a node is allowed to wait before sending any address "
                          "registration. Some jitter aims to prevent collisions. By default, the "
                          "model will wait for a duration in ms defined by a uniform "
                          "random variable between 0 and AddressRegistrationJitter",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
                          MakePointerAccessor(&SixLowPanNdProtocol::m_addressRegistrationJitter),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("RegistrationLifeTime",
                          "The amount of time (units of 60 seconds) that the router should retain "
                          "the NCE for the node.",
                          UintegerValue(65535),
                          MakeUintegerAccessor(&SixLowPanNdProtocol::m_regTime),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("AdvanceTime",
                          "How many seconds before registration expiry to begin re-registration.",
                          UintegerValue(5),
                          MakeUintegerAccessor(&SixLowPanNdProtocol::m_advance),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("DefaultRouterLifeTime",
                          "The default router lifetime.",
                          TimeValue(Minutes(60)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_routerLifeTime),
                          MakeTimeChecker(Time(0), Seconds(0xffff)))
            .AddAttribute("DefaultPrefixInformationPreferredLifeTime",
                          "The default Prefix Information preferred lifetime.",
                          TimeValue(Minutes(10)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_pioPreferredLifeTime),
                          MakeTimeChecker())
            .AddAttribute("DefaultPrefixInformationValidLifeTime",
                          "The default Prefix Information valid lifetime.",
                          TimeValue(Minutes(10)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_pioValidLifeTime),
                          MakeTimeChecker())
            .AddAttribute("DefaultContextValidLifeTime",
                          "The default Context valid lifetime.",
                          TimeValue(Minutes(10)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_contextValidLifeTime),
                          MakeTimeChecker())
            .AddAttribute("DefaultAbroValidLifeTime",
                          "The default ABRO Valid lifetime.",
                          TimeValue(Minutes(10)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_abroValidLifeTime),
                          MakeTimeChecker())
            .AddAttribute("MaxRtrSolicitationInterval",
                          "Maximum Time between two RS (after the backoff).",
                          TimeValue(Seconds(60)),
                          MakeTimeAccessor(&SixLowPanNdProtocol::m_maxRtrSolicitationInterval),
                          MakeTimeChecker())
            .AddTraceSource(
                "AddressRegistrationResult",
                "Trace fired when an address registration succeeds or fails",
                MakeTraceSourceAccessor(&SixLowPanNdProtocol::m_addressRegistrationResultTrace),
                "ns3::SixLowPanNdProtocol::AddressRegistrationCallback")
            .AddTraceSource("MulticastRS",
                            "Trace fired when a multicast RS is sent",
                            MakeTraceSourceAccessor(&SixLowPanNdProtocol::m_multicastRsTrace),
                            "ns3::SixLowPanNdProtocol::MulticastRsCallback")
            .AddTraceSource("NaRx",
                            "Trace fired when a NA packet is received",
                            MakeTraceSourceAccessor(&SixLowPanNdProtocol::m_naRxTrace),
                            "ns3::SixLowPanNdProtocol::NaRxCallback");

    return tid;
}

int64_t
SixLowPanNdProtocol::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_addressRegistrationJitter->SetStream(stream);
    return 2;
}

void
SixLowPanNdProtocol::DoInitialize()
{
    if (!m_raEntries.empty())
    {
        m_nodeRole = SixLowPanBorderRouter;
    }

    m_addrPendingReg.isValid = false;

    Icmpv6L4Protocol::DoInitialize();
}

void
SixLowPanNdProtocol::NotifyNewAggregate()
{
    NS_LOG_FUNCTION(this);
    if (!m_node)
    {
        Ptr<Node> node = this->GetObject<Node>();
        if (node)
        {
            Ptr<Ipv6> ipv6 = this->GetObject<Ipv6>();
            if (ipv6 && m_downTarget.IsNull())
            {
                SetNode(node);
                // We must NOT insert the protocol as a default protocol.
                // This protocol will be inserted later for specific NetDevices.
                // ipv6->Insert (this);
                SetDownTarget6(MakeCallback(&Ipv6::Send, ipv6));
            }
        }
    }
    IpL4Protocol::NotifyNewAggregate();
}

void
SixLowPanNdProtocol::SendSixLowPanNsWithEaro(Ipv6Address addrToRegister,
                                             Ipv6Address dst,
                                             Address dstMac,
                                             uint16_t time,
                                             const std::vector<uint8_t>& rovr,
                                             Ptr<NetDevice> sixLowPanNetDevice)
{
    NS_LOG_FUNCTION(this << addrToRegister << dst << dstMac << time << rovr << sixLowPanNetDevice);

    NS_ASSERT_MSG(!dst.IsMulticast(),
                  "Destination address must not be a multicast address in EARO messages.");

    // Build NS Header
    Icmpv6NS nsHdr(addrToRegister);

    // Build EARO option
    // EARO (request) + SLLAO + TLLAO (SLLAO and TLLAO must be identical, RFC 8505, section 5.6)
    Icmpv6OptionSixLowPanExtendedAddressRegistration earo(time, rovr, 0);
    Icmpv6OptionLinkLayerAddress tlla(false, sixLowPanNetDevice->GetAddress());
    Icmpv6OptionLinkLayerAddress slla(true, sixLowPanNetDevice->GetAddress());

    Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol>();
    Ipv6Address src =
        ipv6->GetAddress(ipv6->GetInterfaceForDevice(sixLowPanNetDevice), 0).GetAddress();

    // Build NS EARO Packet
    Ptr<Packet> p = MakeNsEaroPacket(src, dst, nsHdr, slla, tlla, earo);

    // Build ipv6 header manually as neighbour cache is probably empty
    Ipv6Header hdr;
    hdr.SetSource(src);
    hdr.SetDestination(dst);
    hdr.SetNextHeader(Icmpv6L4Protocol::PROT_NUMBER);
    hdr.SetPayloadLength(p->GetSize());
    hdr.SetHopLimit(255);

    Ptr<Packet> pkt = p->Copy();
    pkt->AddHeader(hdr);

    sixLowPanNetDevice->Send(pkt, dstMac, iana::Ieee802Numbers::IPV6);
}

void
SixLowPanNdProtocol::SendSixLowPanNaWithEaro(Ipv6Address src,
                                             Ipv6Address dst,
                                             Ipv6Address target,
                                             uint16_t time,
                                             const std::vector<uint8_t>& rovr,
                                             Ptr<NetDevice> sixLowPanNetDevice,
                                             uint8_t status)
{
    NS_LOG_FUNCTION(this << src << dst << target << time << rovr << sixLowPanNetDevice << status);

    // Build the NA Header
    Icmpv6NA naHdr;
    naHdr.SetIpv6Target(target);
    naHdr.SetFlagO(false);
    naHdr.SetFlagS(true);
    naHdr.SetFlagR(true);

    // Build EARO Option
    Icmpv6OptionSixLowPanExtendedAddressRegistration earo(status, time, rovr, 0);

    // Build NA EARO Packet
    Ptr<Packet> p = MakeNaEaroPacket(src, dst, naHdr, earo);

    SendMessage(p, src, dst, 255);
}

void
SixLowPanNdProtocol::SendSixLowPanMulticastRS(Ipv6Address src, Address hardwareAddress)
{
    NS_LOG_FUNCTION(this << src << hardwareAddress);

    m_multicastRsTrace(src);

    Ptr<Packet> p = Create<Packet>();
    Icmpv6RS rs;

    Icmpv6OptionLinkLayerAddress slla(true, hardwareAddress);
    Icmpv6OptionSixLowPanCapabilityIndication cio;
    p->AddHeader(slla);
    p->AddHeader(cio);

    Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol>();
    if (ipv6->GetInterfaceForAddress(src) == -1)
    {
        NS_LOG_LOGIC("Preventing RS from being sent or rescheduled because the source address "
                     << src << " has been removed");
        return;
    }

    NS_LOG_LOGIC("Send RS (from " << src << " to AllRouters multicast address)");

    rs.CalculatePseudoHeaderChecksum(src,
                                     Ipv6Address::GetAllRoutersMulticast(),
                                     p->GetSize() + rs.GetSerializedSize(),
                                     PROT_NUMBER);
    p->AddHeader(rs);

    m_rsRetransmissionCount++;
    Time delay = Seconds(10);
    if (m_rsRetransmissionCount >= m_rsMaxRetransmissionCount)
    {
        if (m_rsRetransmissionCount >= 5)
        {
            delay = Seconds(60);
        }
        else
        {
            delay =
                std::min(delay * pow(2, 1 + m_rsRetransmissionCount - m_rsMaxRetransmissionCount),
                         m_maxRtrSolicitationInterval);
        }
    }

    Simulator::Schedule(MilliSeconds(m_rsRetransmissionJitter->GetValue()),
                        &Icmpv6L4Protocol::DelayedSendMessage,
                        this,
                        p,
                        src,
                        Ipv6Address::GetAllRoutersMulticast(),
                        255);

    m_handleRsTimeoutEvent =
        Simulator::Schedule(delay + MilliSeconds(m_rsRetransmissionJitter->GetValue()),
                            &SixLowPanNdProtocol::SendSixLowPanMulticastRS,
                            this,
                            src,
                            hardwareAddress);
}

void
SixLowPanNdProtocol::SendSixLowPanRA(Ipv6Address src, Ipv6Address dst, Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << src << dst << interface);

    Ptr<SixLowPanNetDevice> sixLowPanNetDevice =
        DynamicCast<SixLowPanNetDevice>(interface->GetDevice());
    NS_ABORT_MSG_IF(m_nodeRole == SixLowPanBorderRouter &&
                        m_raEntries.find(sixLowPanNetDevice) == m_raEntries.end(),
                    "6LBR not configured on the interface");

    // if the node is a 6LBR, send out the RA entry for the interface
    auto it = m_raEntries.find(sixLowPanNetDevice);
    if (it != m_raEntries.end())
    {
        Ptr<SixLowPanRaEntry> raEntry = it->second;

        // Build SLLA Option
        Icmpv6OptionLinkLayerAddress slla(true, interface->GetDevice()->GetAddress());

        // Build 6CIO Option
        Icmpv6OptionSixLowPanCapabilityIndication cio;
        // cio.SetOption(Icmpv6OptionSixLowPanCapabilityIndication::D); // no EDAR EDAC support yet
        cio.SetOption(Icmpv6OptionSixLowPanCapabilityIndication::B);
        cio.SetOption(Icmpv6OptionSixLowPanCapabilityIndication::E);

        // Build RA Packet
        Ptr<Packet> p = MakeRaPacket(src, dst, slla, cio, raEntry);

        // Build Ipv6 Header manually
        Ipv6Header ipHeader;
        ipHeader.SetSource(src);
        ipHeader.SetDestination(dst);
        ipHeader.SetNextHeader(PROT_NUMBER);
        ipHeader.SetPayloadLength(p->GetSize());
        ipHeader.SetHopLimit(255);

        // send RA
        NS_LOG_LOGIC("Send RA to " << dst);

        interface->Send(p, ipHeader, dst);
    }
}

enum IpL4Protocol::RxStatus
SixLowPanNdProtocol::Receive(Ptr<Packet> packet,
                             const Ipv6Header& header,
                             Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << *packet << header << interface);

    uint8_t type;
    packet->CopyData(&type, sizeof(type));

    switch (type)
    {
    case Icmpv6Header::ICMPV6_ND_ROUTER_SOLICITATION:
        HandleSixLowPanRS(packet, header.GetSource(), header.GetDestination(), interface);
        break;
    case Icmpv6Header::ICMPV6_ND_ROUTER_ADVERTISEMENT:
        HandleSixLowPanRA(packet, header.GetSource(), header.GetDestination(), interface);
        break;
    case Icmpv6Header::ICMPV6_ND_NEIGHBOR_SOLICITATION:
        HandleSixLowPanNS(packet, header.GetSource(), header.GetDestination(), interface);
        break;
    case Icmpv6Header::ICMPV6_ND_NEIGHBOR_ADVERTISEMENT:
        HandleSixLowPanNA(packet, header.GetSource(), header.GetDestination(), interface);
        break;
    default:
        return Icmpv6L4Protocol::Receive(packet, header, interface);
        break;
    }
    return IpL4Protocol::RX_OK;
}

void
SixLowPanNdProtocol::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_handleRsTimeoutEvent.Cancel();
    m_addressRegistrationTimeoutEvent.Cancel();
    m_raEntries.clear();
    m_bindingTableList.clear();
    m_pendingRas.clear();
    m_registeredAddresses.clear();
    m_addrPendingReg.interface = nullptr;
    m_addressRegistrationJitter = nullptr;
    Icmpv6L4Protocol::DoDispose();
}

void
SixLowPanNdProtocol::HandleSixLowPanNS(Ptr<Packet> pkt,
                                       const Ipv6Address& src,
                                       const Ipv6Address& dst,
                                       Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << pkt << src << dst << interface);

    Ptr<SixLowPanNetDevice> sixLowPanNetDevice =
        DynamicCast<SixLowPanNetDevice>(interface->GetDevice());
    if (!sixLowPanNetDevice)
    {
        HandleNS(pkt, src, dst, interface);
        return;
    }

    // NS (EARO) — only a 6LBR maintains a binding table and handles registration.
    if (m_nodeRole != SixLowPanBorderRouter)
    {
        NS_LOG_LOGIC("Discarding NS(EARO): not a 6LBR");
        return;
    }

    if (src == Ipv6Address::GetAny())
    {
        NS_LOG_LOGIC("Discarding a NS from unspecified source address");
        return;
    }

    if (dst.IsMulticast())
    {
        NS_LOG_LOGIC("Discarding a NS sent to a multicast destination");
        return;
    }

    Ptr<Packet> packet = pkt->Copy();

    Icmpv6NS nsHdr;
    Icmpv6OptionLinkLayerAddress sllaoHdr(true);
    Icmpv6OptionLinkLayerAddress tllaoHdr(false);
    Icmpv6OptionSixLowPanExtendedAddressRegistration earoHdr;
    bool hasEaro = false;

    if (!ParseAndValidateNsEaroPacket(packet, nsHdr, sllaoHdr, tllaoHdr, earoHdr, hasEaro))
    {
        NS_LOG_LOGIC("Discarding invalid NS(EARO)");
        return;
    }
    Ipv6Address target = nsHdr.GetIpv6Target();

    // Check if hasEaro
    if (!hasEaro)
    {
        // Let the "normal" Icmpv6L4Protocol handle it.
        HandleNS(pkt, src, dst, interface);
        return;
    }

    Ptr<SixLowPanNdBindingTable> bindingTable = FindBindingTable(interface);
    NS_ASSERT_MSG(bindingTable, "Can not find a SixLowPanNdBindingTable");

    Ptr<NdiscCache> cache = DynamicCast<NdiscCache>(FindCache(sixLowPanNetDevice));
    NS_ASSERT_MSG(cache, "Can not find a NdiscCache");

    // Validate: reject if a reachable binding exists for this target with a different ROVR.
    auto btEntry = bindingTable->Lookup(target);
    if (btEntry && btEntry->IsReachable() && btEntry->GetRovr() != earoHdr.GetRovr())
    {
        NS_LOG_LOGIC("NS EARO: ROVR mismatch, discarding");
        SendSixLowPanNaWithEaro(dst,
                                src,
                                target,
                                earoHdr.GetRegTime(),
                                earoHdr.GetRovr(),
                                sixLowPanNetDevice,
                                DUPLICATE_ADDRESS);
        return;
    }

    // Update or create Binding Table entry.
    if (!btEntry)
    {
        btEntry = bindingTable->Add(target);
        btEntry->SetRovr(earoHdr.GetRovr());
    }
    btEntry->MarkReachable(earoHdr.GetRegTime());

    // Update or create Neighbor Cache entry.
    NdiscCache::Entry* ncEntry = cache->Lookup(target);
    if (!ncEntry)
    {
        ncEntry = cache->Add(target);
    }
    ncEntry->SetRouter(false);
    ncEntry->SetMacAddress(sllaoHdr.GetAddress());
    ncEntry->MarkReachable();
    ncEntry->StartReachableTimer();

    // Install a /128 host route for non-link-local registered addresses.
    if (!target.IsLinkLocal())
    {
        Ptr<Ipv6L3Protocol> ipv6l3Protocol = m_node->GetObject<Ipv6L3Protocol>();
        ipv6l3Protocol->GetRoutingProtocol()->NotifyAddRoute(
            target,
            Ipv6Prefix(128),
            src,
            ipv6l3Protocol->GetInterfaceForDevice(interface->GetDevice()));
    }

    SendSixLowPanNaWithEaro(dst,
                            src,
                            target,
                            earoHdr.GetRegTime(),
                            earoHdr.GetRovr(),
                            sixLowPanNetDevice,
                            0);
}

void
SixLowPanNdProtocol::HandleSixLowPanNA(Ptr<Packet> packet,
                                       const Ipv6Address& src,
                                       const Ipv6Address& dst,
                                       Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << *packet << src << dst << interface);

    if (src == Ipv6Address::GetAny())
    {
        NS_LOG_LOGIC("Discarding a NA from unspecified source address (" << Ipv6Address::GetAny()
                                                                         << ")");
        return;
    }

    m_naRxTrace(packet->Copy());

    Ptr<Packet> p = packet->Copy();

    Icmpv6NA naHdr;
    Icmpv6OptionLinkLayerAddress tlla(false);
    Icmpv6OptionSixLowPanExtendedAddressRegistration earo;

    bool hasEaro = false;
    if (!ParseAndValidateNaEaroPacket(packet, naHdr, tlla, earo, hasEaro))
    {
        return; // note that currently it will always return valid
    }

    // NA without EARO: pass to the standard handler for address resolution.
    if (!hasEaro)
    {
        HandleNA(p, naHdr.GetIpv6Target(), dst, interface);
        return;
    }

    // NA (EARO) — validate before processing the registration result.
    if (earo.GetRovr() != m_rovr)
    {
        NS_LOG_LOGIC("NA EARO: ROVR mismatch, discarding");
        return;
    }

    if (!m_addrPendingReg.isValid ||
        m_addrPendingReg.addressPendingRegistration != naHdr.GetIpv6Target())
    {
        NS_LOG_LOGIC("NA EARO: target does not match pending registration, discarding");
        return;
    }

    // Process registration result.
    bool registrationSucceeded = (earo.GetStatus() == SUCCESS);
    m_addressRegistrationResultTrace(m_addrPendingReg.addressPendingRegistration,
                                     registrationSucceeded,
                                     earo.GetStatus());
    if (!registrationSucceeded)
    {
        // Let the retransmit timeout handle retry up to the maximum count.
        return;
    }
    AddressRegistrationSuccess(src);
}

void
SixLowPanNdProtocol::HandleSixLowPanRS(Ptr<Packet> packet,
                                       const Ipv6Address& src,
                                       const Ipv6Address& dst,
                                       Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << packet << src << dst << interface);

    Ptr<SixLowPanNetDevice> sixLowPanNetDevice =
        DynamicCast<SixLowPanNetDevice>(interface->GetDevice());
    if (!sixLowPanNetDevice)
    {
        HandleRS(packet, src, dst, interface);
        return;
    }

    // Validate: only a 6LBR responds to RS.
    if (m_nodeRole == SixLowPanNode || m_nodeRole == SixLowPanNodeOnly)
    {
        NS_LOG_LOGIC("Discarding an RS because this node is not a router");
        return;
    }

    if (src == Ipv6Address::GetAny())
    {
        NS_LOG_LOGIC("Discarding a RS from unspecified source address (" << Ipv6Address::GetAny()
                                                                         << ")");
        return;
    }

    Icmpv6RS rsHdr;
    Icmpv6OptionLinkLayerAddress slla(true);
    Icmpv6OptionSixLowPanCapabilityIndication cio;

    if (!ParseAndValidateRsPacket(packet, rsHdr, slla, cio))
    {
        return;
    }

    // Update or create Neighbor Cache entry for the soliciting node.
    Ptr<NdiscCache> neighbourCache = DynamicCast<NdiscCache>(FindCache(sixLowPanNetDevice));
    NS_ASSERT_MSG(neighbourCache, "Can not find a NdiscCache");

    auto ncEntry = neighbourCache->Lookup(src);
    if (!ncEntry)
    {
        ncEntry = neighbourCache->Add(src);
        ncEntry->SetRouter(false);
    }
    if (ncEntry->GetMacAddress() != slla.GetAddress())
    {
        ncEntry->MarkStale(slla.GetAddress());
    }

    // Send RA in response.
    SendSixLowPanRA(interface->GetLinkLocalAddress().GetAddress(), src, interface);
}

void
SixLowPanNdProtocol::HandleSixLowPanRA(Ptr<Packet> packet,
                                       const Ipv6Address& src,
                                       const Ipv6Address& dst,
                                       Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << packet << src << dst << interface);

    Ptr<SixLowPanNetDevice> sixLowPanNetDevice =
        DynamicCast<SixLowPanNetDevice>(interface->GetDevice());
    if (!sixLowPanNetDevice)
    {
        HandleRA(packet, src, dst, interface);
        return;
    }

    if (src == Ipv6Address::GetAny())
    {
        NS_LOG_LOGIC("Discarding a RA from unspecified source address (" << Ipv6Address::GetAny()
                                                                         << ")");
        return;
    }

    // Stop RS retransmissions: receiving any RA from a valid source indicates
    // a 6LBR is reachable, regardless of whether the RA passes validation below.
    if (m_handleRsTimeoutEvent.IsPending())
    {
        m_handleRsTimeoutEvent.Cancel();
        m_rsRetransmissionCount = 0;
    }

    Icmpv6RA raHdr;
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter abro;
    Icmpv6OptionLinkLayerAddress slla(true);
    Icmpv6OptionSixLowPanCapabilityIndication cio;
    std::list<Icmpv6OptionPrefixInformation> pios;
    std::list<Icmpv6OptionSixLowPanContext> contexts;

    if (!ParseAndValidateRaPacket(packet, raHdr, pios, abro, slla, cio, contexts))
    {
        return;
    }

    // Deduplicate: only process the first RA from each 6LBR.
    // Subsequent RAs are dropped until ABRO version management is implemented.
    if (m_raCache.count(abro.GetRouterAddress()))
    {
        return;
    }
    m_raCache.insert(abro.GetRouterAddress());

    // Build the set of addresses to register: link-local first (no PIO for
    // link-local), then one autoconfigured global address per PIO.
    SixLowPanPendingRa pending;
    pending.source = src;
    pending.interface = interface;
    pending.llaHdr = slla;
    // Add the link-local address first (no PIO for link-local addresses)
    pending.addressesToBeRegistered.emplace_back(interface->GetLinkLocalAddress().GetAddress(),
                                                 Icmpv6OptionPrefixInformation{});
    for (const auto& pio : pios)
    {
        Ipv6Address gaddr = Ipv6Address::MakeAutoconfiguredAddress(sixLowPanNetDevice->GetAddress(),
                                                                   pio.GetPrefix());
        pending.addressesToBeRegistered.emplace_back(gaddr, pio);
    }
    m_pendingRas.push_back(pending);

    AddressRegistration();
}

bool
SixLowPanNdProtocol::Lookup(Ptr<Packet> p,
                            const Ipv6Header& ipHeader,
                            Ipv6Address dst,
                            Ptr<NetDevice> device,
                            Ptr<NdiscCache> cache,
                            Address* hardwareDestination)
{
    if (!cache)
    {
        cache = FindCache(device);
    }
    if (!cache)
    {
        return false;
    }

    NdiscCache::Entry* ncEntry = cache->Lookup(dst);
    if (!ncEntry)
    {
        // RFC 8505 prohibits multicast neighbor discovery in 6LoWPAN networks.
        // The base class would send a multicast NS when an entry is not found,
        // but in 6LoWPAN ND, address resolution is handled via ARO/EARO registration
        // rather than multicast NS. Return false to let the caller handle the
        // unresolved address without triggering multicast NS.
        return false;
    }
    return Icmpv6L4Protocol::Lookup(p, ipHeader, dst, device, cache, hardwareDestination);
}

void
SixLowPanNdProtocol::CreateBindingTable(Ptr<NetDevice> device, Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << device << interface);

    Ptr<SixLowPanNdBindingTable> table = CreateObject<SixLowPanNdBindingTable>();
    table->SetDevice(device, interface, this);

    m_bindingTableList.push_back(table);
}

Ptr<SixLowPanNdBindingTable>
SixLowPanNdProtocol::FindBindingTable(Ptr<Ipv6Interface> interface)
{
    NS_LOG_FUNCTION(this << interface);

    Ptr<NetDevice> device = interface->GetDevice();
    for (const auto& table : m_bindingTableList)
    {
        if (table->GetDevice() == device)
        {
            return table;
        }
    }
    return nullptr;
}

void
SixLowPanNdProtocol::FunctionDadTimeout(Ipv6Interface* interface, Ipv6Address addr)
{
    NS_LOG_FUNCTION(this << interface << addr);
}

void
SixLowPanNdProtocol::SetRovr(std::vector<uint8_t> rovr)
{
    NS_LOG_FUNCTION(this);
    m_rovr = rovr;
}

void
SixLowPanNdProtocol::AddressRegistration()
{
    NS_LOG_FUNCTION(this);

    if (m_addressRegistrationTimeoutEvent.IsPending() || m_addressRegistrationEvent.IsPending())
    {
        return;
    }

    // Populate m_addrPendingReg if no registration is currently in progress.
    Time additionalDelay = Seconds(0);
    if (!m_addrPendingReg.isValid)
    {
        if (!m_pendingRas.empty())
        {
            // New registration triggered by a received RA.
            m_addrPendingReg.isValid = true;
            m_addrPendingReg.addressPendingRegistration =
                m_pendingRas.front().addressesToBeRegistered.front().first;
            m_addrPendingReg.registrar = m_pendingRas.front().source;
            m_addrPendingReg.newRegistration = true;
            m_addrPendingReg.llaHdr = m_pendingRas.front().llaHdr;
            m_addrPendingReg.interface = m_pendingRas.front().interface;
            m_addrPendingReg.pioHdr = m_pendingRas.front().addressesToBeRegistered.front().second;
        }
        else if (!m_registeredAddresses.empty())
        {
            // Renewal of an already-registered address.
            m_addrPendingReg.isValid = true;
            m_addrPendingReg.addressPendingRegistration =
                m_registeredAddresses.front().registeredAddr;
            m_addrPendingReg.registrar = m_registeredAddresses.front().registrar;
            m_addrPendingReg.newRegistration = false;
            m_addrPendingReg.llaHdr = m_registeredAddresses.front().llaHdr;
            m_addrPendingReg.interface = m_registeredAddresses.front().interface;
            m_addrPendingReg.pioHdr = m_registeredAddresses.front().pioHdr;

            Time now = Simulator::Now();
            if (m_registeredAddresses.front().registrationTimeout > now)
            {
                additionalDelay =
                    m_registeredAddresses.front().registrationTimeout - now - Seconds(m_advance);
            }
        }
        else
        {
            // No addresses to register — send multicast RS on each interface to solicit an RA.
            Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol>();
            NS_ASSERT(ipv6);

            for (uint32_t i = 0; i < ipv6->GetNInterfaces(); ++i)
            {
                Ptr<Ipv6Interface> iface = ipv6->GetInterface(i);
                if (!iface->IsUp())
                {
                    continue;
                }

                Ipv6InterfaceAddress ifaddr = iface->GetAddress(0); // typically link-local
                Ipv6Address lla = ifaddr.GetAddress();

                Simulator::Schedule(MilliSeconds(m_addressRegistrationJitter->GetValue()),
                                    &SixLowPanNdProtocol::SendSixLowPanMulticastRS,
                                    this,
                                    lla,
                                    iface->GetDevice()->GetAddress());
            }
            return;
        }
    }

    // Schedule NS(EARO) and registration timeout.
    m_addressRegistrationCounter++;

    Simulator::Schedule(additionalDelay + MilliSeconds(m_addressRegistrationJitter->GetValue()),
                        &SixLowPanNdProtocol::SendSixLowPanNsWithEaro,
                        this,
                        m_addrPendingReg.addressPendingRegistration,
                        m_addrPendingReg.registrar,
                        m_addrPendingReg.llaHdr.GetAddress(),
                        m_regTime,
                        m_rovr,
                        m_addrPendingReg.interface->GetDevice());

    m_addressRegistrationTimeoutEvent =
        Simulator::Schedule(additionalDelay + m_retransmissionTime +
                                MilliSeconds(m_addressRegistrationJitter->GetValue()),
                            &SixLowPanNdProtocol::AddressRegistrationTimeout,
                            this);
}

void
SixLowPanNdProtocol::AddressRegistrationSuccess(Ipv6Address registrar)
{
    NS_LOG_FUNCTION(this << registrar);
    NS_ABORT_MSG_IF(registrar != m_addrPendingReg.registrar,
                    "AddressRegistrationSuccess, mismatch between sender and expected sender "
                        << registrar << "  vs expected " << m_addrPendingReg.registrar);

    if (m_addressRegistrationTimeoutEvent.IsPending())
    {
        m_addressRegistrationTimeoutEvent.Cancel();
    }

    m_addressRegistrationCounter = 0;

    // Update the registered address list.
    if (!m_addrPendingReg.newRegistration)
    {
        // Renewal: refresh the timeout and rotate to the back of the queue.
        SixLowPanRegisteredAddress regAddr = m_registeredAddresses.front();
        regAddr.registrationTimeout = Now() + Minutes(m_regTime);
        m_registeredAddresses.pop_front();
        m_registeredAddresses.push_back(regAddr);
    }
    else
    {
        // New registration: record it and consume the address from the pending RA queue.
        NS_ABORT_MSG_IF(m_pendingRas.empty(),
                        "AddressRegistrationSuccess, expected to register an address from the "
                        "pending RA list, but it's empty");
        NS_ABORT_MSG_IF(
            m_pendingRas.front().addressesToBeRegistered.empty(),
            "AddressRegistrationSuccess, expected to register an address from the pending RA list "
                << "but the pending registration address list is empty");

        SixLowPanRegisteredAddress newRegisteredAddr;
        newRegisteredAddr.registrationTimeout = Now() + Minutes(m_regTime);
        newRegisteredAddr.registeredAddr = m_addrPendingReg.addressPendingRegistration;
        newRegisteredAddr.registrar = m_addrPendingReg.registrar;
        newRegisteredAddr.llaHdr = m_addrPendingReg.llaHdr;
        newRegisteredAddr.interface = m_pendingRas.front().interface;
        newRegisteredAddr.pioHdr = m_addrPendingReg.pioHdr;
        m_registeredAddresses.push_back(newRegisteredAddr);

        // Drop the front pending RA once all its addresses have been registered.
        m_pendingRas.front().addressesToBeRegistered.pop_front();
        if (m_pendingRas.front().addressesToBeRegistered.empty())
        {
            m_pendingRas.pop_front();
        }
    }

    // Notify the IPv6 stack to configure the registered address.
    if (m_addrPendingReg.addressPendingRegistration.IsLinkLocal())
    {
        ReceiveLLA(m_addrPendingReg.llaHdr,
                   m_addrPendingReg.registrar,
                   Ipv6Address::GetAny(),
                   m_addrPendingReg.interface);
    }
    else
    {
        Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol>();
        Icmpv6OptionPrefixInformation prefixHdr = m_addrPendingReg.pioHdr;
        ipv6->AddAutoconfiguredAddress(
            ipv6->GetInterfaceForDevice(m_addrPendingReg.interface->GetDevice()),
            prefixHdr.GetPrefix(),
            prefixHdr.GetPrefixLength(),
            prefixHdr.GetFlags(),
            prefixHdr.GetValidTime(),
            prefixHdr.GetPreferredTime(),
            registrar);
    }

    // Clear the pending registration and schedule the next one.
    m_addrPendingReg.isValid = false; // invalidate before scheduling the next registration
    m_addressRegistrationEvent =
        Simulator::Schedule(MilliSeconds(m_addressRegistrationJitter->GetValue()),
                            &SixLowPanNdProtocol::AddressRegistration,
                            this);
}

void
SixLowPanNdProtocol::AddressRegistrationTimeout()
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_IF(
        !m_addrPendingReg.isValid,
        "Address Registration Timeout but there is no valid address pending registration. "
            << "Node ID=" << m_node->GetId());

    if (m_addressRegistrationCounter < m_maxUnicastSolicit)
    {
        AddressRegistration();
    }
    else
    {
        NS_LOG_INFO("Address registration failed for node "
                    << m_node->GetId()
                    << ", address: " << m_addrPendingReg.addressPendingRegistration
                    << ", registrar: " << m_addrPendingReg.registrar
                    << ", retries: " << static_cast<int>(m_addressRegistrationCounter));

        // todo
        // Add code to remove next hop from the reliable neighbors.
        // If the re-registration failed (for all of the candidate next hops), remove the address.
        // If we don't have any address anymore, start sending RS (again).
        // For now since we only have 1 6LBR we are registering with, we just stop trying to
        // register with it
    }
}

void
SixLowPanNdProtocol::SetInterfaceAs6lbr(Ptr<SixLowPanNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);

    if (m_raEntries.find(device) != m_raEntries.end())
    {
        NS_LOG_LOGIC("Not going to reconfigure an interface");
        return;
    }

    Ptr<SixLowPanRaEntry> newRa = Create<SixLowPanRaEntry>();
    newRa->SetManagedFlag(false);
    newRa->SetHomeAgentFlag(false);
    newRa->SetOtherConfigFlag(false);
    newRa->SetCurHopLimit(0);  // unspecified by this router
    newRa->SetRetransTimer(0); // unspecified by this router

    newRa->SetReachableTime(0); // unspecified by this router

    uint64_t routerLifetime = std::ceil(m_routerLifeTime.GetMinutes());
    if (routerLifetime > 0xffff)
    {
        routerLifetime = 0xffff;
    }

    newRa->SetRouterLifeTime(routerLifetime);

    Ptr<Ipv6L3Protocol> ipv6 = GetNode()->GetObject<Ipv6L3Protocol>();
    int32_t interfaceId = ipv6->GetInterfaceForDevice(device);
    Ipv6Address borderAddress = Ipv6Address::GetAny();
    for (uint32_t i = 0; i < ipv6->GetNAddresses(interfaceId); ++i)
    {
        if (ipv6->GetAddress(interfaceId, i).GetScope() == Ipv6InterfaceAddress::GLOBAL)
        {
            borderAddress = ipv6->GetAddress(interfaceId, i).GetAddress();
            continue;
        }
    }
    NS_ABORT_MSG_IF(
        borderAddress == Ipv6Address::GetAny(),
        "Can not set a 6LBR because I can't find a global address associated with the interface");
    newRa->SetAbroBorderRouterAddress(borderAddress);
    newRa->SetAbroVersion(0x66); // placeholder value for testing purposes
    newRa->SetAbroValidLifeTime(m_abroValidLifeTime.GetSeconds());

    m_raEntries[device] = newRa;
}

void
SixLowPanNdProtocol::SetAdvertisedPrefix(Ptr<SixLowPanNetDevice> device, Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this << device << prefix);

    if (m_raEntries.find(device) == m_raEntries.end())
    {
        NS_LOG_LOGIC("Not adding a prefix to an unconfigured interface");
        return;
    }

    Ptr<SixLowPanNdPrefix> newPrefix = Create<SixLowPanNdPrefix>(prefix.ConvertToIpv6Address(),
                                                                 prefix.GetPrefixLength(),
                                                                 m_pioPreferredLifeTime,
                                                                 m_pioValidLifeTime);

    m_raEntries[device]->AddPrefix(newPrefix);
}

void
SixLowPanNdProtocol::AddAdvertisedContext(Ptr<SixLowPanNetDevice> device, Ipv6Prefix context)
{
    NS_LOG_FUNCTION(this << device << context);

    if (m_raEntries.find(device) == m_raEntries.end())
    {
        NS_LOG_LOGIC("Not adding a context to an unconfigured interface");
        return;
    }
    auto contextMap = m_raEntries[device]->GetContexts();

    bool found = std::any_of(contextMap.begin(), contextMap.end(), [&context](const auto& entry) {
        return entry.second->GetContextPrefix() == context;
    });
    if (found)
    {
        NS_LOG_WARN("Not adding an already existing context - remove the old one first "
                    << context);
        return;
    }

    uint8_t unusedCid;
    for (unusedCid = 0; unusedCid < 16; ++unusedCid)
    {
        if (contextMap.count(unusedCid) == 0)
        {
            break;
        }
    }

    Ptr<SixLowPanNdContext> newContext =
        Create<SixLowPanNdContext>(true, unusedCid, m_contextValidLifeTime, context);
    newContext->SetLastUpdateTime(Simulator::Now());

    m_raEntries[device]->AddContext(newContext);
}

void
SixLowPanNdProtocol::RemoveAdvertisedContext(Ptr<SixLowPanNetDevice> device, Ipv6Prefix context)
{
    NS_LOG_FUNCTION(this << device << context);

    if (m_raEntries.find(device) == m_raEntries.end())
    {
        NS_LOG_LOGIC("Not removing a context from an unconfigured interface");
        return;
    }

    auto contextMap = m_raEntries[device]->GetContexts();

    for (const auto& [cid, ctx] : contextMap)
    {
        if (ctx->GetContextPrefix() == context)
        {
            m_raEntries[device]->RemoveContext(ctx);
            return;
        }
    }
    NS_LOG_WARN("Not removing a non-existing context " << context);
}

bool
SixLowPanNdProtocol::IsBorderRouterOnInterface(Ptr<SixLowPanNetDevice> device) const
{
    NS_LOG_FUNCTION(this << device);

    return m_raEntries.find(device) != m_raEntries.end();
}

//
// SixLowPanRaEntry class
//

// NS_LOG_COMPONENT_DEFINE ("SixLowPanRaEntry");

SixLowPanNdProtocol::SixLowPanRaEntry::SixLowPanRaEntry()
{
    NS_LOG_FUNCTION(this);
}

SixLowPanNdProtocol::SixLowPanRaEntry::SixLowPanRaEntry(
    Icmpv6RA raHeader,
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter abroHdr,
    std::list<Icmpv6OptionSixLowPanContext> contextList,
    std::list<Icmpv6OptionPrefixInformation> prefixList)
{
    NS_LOG_FUNCTION(this << abroHdr << &prefixList << &contextList);

    SetManagedFlag(raHeader.GetFlagM());
    SetOtherConfigFlag(raHeader.GetFlagO());
    SetHomeAgentFlag(raHeader.GetFlagH());
    SetReachableTime(raHeader.GetReachableTime());
    SetRouterLifeTime(raHeader.GetLifeTime());
    SetRetransTimer(raHeader.GetRetransmissionTime());
    SetCurHopLimit(raHeader.GetCurHopLimit());
    ParseAbro(abroHdr);

    for (const auto& ctxOpt : contextList)
    {
        Ptr<SixLowPanNdContext> context = Create<SixLowPanNdContext>();
        context->SetCid(ctxOpt.GetCid());
        context->SetFlagC(ctxOpt.IsFlagC());
        context->SetValidTime(Minutes(ctxOpt.GetValidTime()));
        context->SetContextPrefix(ctxOpt.GetContextPrefix());
        context->SetLastUpdateTime(Simulator::Now());

        AddContext(context);
    }

    for (const auto& pfxOpt : prefixList)
    {
        Ptr<SixLowPanNdPrefix> prefix = Create<SixLowPanNdPrefix>();
        prefix->SetPrefix(pfxOpt.GetPrefix());
        prefix->SetPrefixLength(pfxOpt.GetPrefixLength());
        prefix->SetPreferredLifeTime(Seconds(pfxOpt.GetPreferredTime()));
        prefix->SetValidLifeTime(Seconds(pfxOpt.GetValidTime()));

        AddPrefix(prefix);
    }
}

SixLowPanNdProtocol::SixLowPanRaEntry::~SixLowPanRaEntry()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::AddPrefix(Ptr<SixLowPanNdPrefix> prefix)
{
    NS_LOG_FUNCTION(this << prefix);

    for (const auto& pfx : m_prefixes)
    {
        if (pfx->GetPrefix() == prefix->GetPrefix())
        {
            NS_LOG_WARN("Ignoring an already-existing prefix: " << prefix->GetPrefix());
            return;
        }
    }

    m_prefixes.push_back(prefix);
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::RemovePrefix(Ptr<SixLowPanNdPrefix> prefix)
{
    NS_LOG_FUNCTION(this << prefix);

    for (auto it = m_prefixes.begin(); it != m_prefixes.end(); ++it)
    {
        if ((*it)->GetPrefix() == prefix->GetPrefix())
        {
            m_prefixes.erase(it);
            return;
        }
    }
}

std::list<Ptr<SixLowPanNdPrefix>>
SixLowPanNdProtocol::SixLowPanRaEntry::GetPrefixes() const
{
    NS_LOG_FUNCTION(this);
    return m_prefixes;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::AddContext(Ptr<SixLowPanNdContext> context)
{
    NS_LOG_FUNCTION(this << context);
    m_contexts.emplace(context->GetCid(), context);
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::RemoveContext(Ptr<SixLowPanNdContext> context)
{
    NS_LOG_FUNCTION(this);

    m_contexts.erase(context->GetCid());
}

std::map<uint8_t, Ptr<SixLowPanNdContext>>
SixLowPanNdProtocol::SixLowPanRaEntry::GetContexts() const
{
    NS_LOG_FUNCTION(this);
    return m_contexts;
}

Icmpv6RA
SixLowPanNdProtocol::SixLowPanRaEntry::BuildRouterAdvertisementHeader() const
{
    NS_LOG_FUNCTION(this);
    Icmpv6RA raHdr;
    // set RA header information
    raHdr.SetFlagM(IsManagedFlag());
    raHdr.SetFlagO(IsOtherConfigFlag());
    raHdr.SetFlagH(IsHomeAgentFlag());
    raHdr.SetCurHopLimit(GetCurHopLimit());
    raHdr.SetLifeTime(GetRouterLifeTime());
    raHdr.SetReachableTime(GetReachableTime());
    raHdr.SetRetransmissionTime(GetRetransTimer());

    return raHdr;
}

std::list<Icmpv6OptionPrefixInformation>
SixLowPanNdProtocol::SixLowPanRaEntry::BuildPrefixInformationOptions()
{
    NS_LOG_FUNCTION(this);
    std::list<Icmpv6OptionPrefixInformation> prefixHdrs;

    for (const auto& pfx : m_prefixes)
    {
        Icmpv6OptionPrefixInformation prefixHdr;
        prefixHdr.SetPrefixLength(pfx->GetPrefixLength());
        prefixHdr.SetFlags(0x40); // We set the Autonomous address configuration only.
        prefixHdr.SetValidTime(pfx->GetValidLifeTime().GetSeconds());
        prefixHdr.SetPreferredTime(pfx->GetPreferredLifeTime().GetSeconds());
        prefixHdr.SetPrefix(pfx->GetPrefix());
        prefixHdrs.push_back(prefixHdr);
    }

    return prefixHdrs;
}

bool
SixLowPanNdProtocol::SixLowPanRaEntry::IsManagedFlag() const
{
    NS_LOG_FUNCTION(this);
    return m_managedFlag;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetManagedFlag(bool managedFlag)
{
    NS_LOG_FUNCTION(this << managedFlag);
    m_managedFlag = managedFlag;
}

bool
SixLowPanNdProtocol::SixLowPanRaEntry::IsOtherConfigFlag() const
{
    NS_LOG_FUNCTION(this);
    return m_otherConfigFlag;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetOtherConfigFlag(bool otherConfigFlag)
{
    NS_LOG_FUNCTION(this << otherConfigFlag);
    m_otherConfigFlag = otherConfigFlag;
}

bool
SixLowPanNdProtocol::SixLowPanRaEntry::IsHomeAgentFlag() const
{
    NS_LOG_FUNCTION(this);
    return m_homeAgentFlag;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetHomeAgentFlag(bool homeAgentFlag)
{
    NS_LOG_FUNCTION(this << homeAgentFlag);
    m_homeAgentFlag = homeAgentFlag;
}

uint32_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetReachableTime() const
{
    NS_LOG_FUNCTION(this);
    return m_reachableTime;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetReachableTime(uint32_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_reachableTime = time;
}

uint32_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetRouterLifeTime() const
{
    NS_LOG_FUNCTION(this);
    return m_routerLifeTime;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetRouterLifeTime(uint32_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_routerLifeTime = time;
}

uint32_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetRetransTimer() const
{
    NS_LOG_FUNCTION(this);
    return m_retransTimer;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetRetransTimer(uint32_t timer)
{
    NS_LOG_FUNCTION(this << timer);
    m_retransTimer = timer;
}

uint8_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetCurHopLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_curHopLimit;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetCurHopLimit(uint8_t curHopLimit)
{
    NS_LOG_FUNCTION(this << curHopLimit);
    m_curHopLimit = curHopLimit;
}

uint32_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroVersion() const
{
    NS_LOG_FUNCTION(this);
    return m_abroVersion;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetAbroVersion(uint32_t version)
{
    NS_LOG_FUNCTION(this << version);
    m_abroVersion = version;
}

uint16_t
SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroValidLifeTime() const
{
    NS_LOG_FUNCTION(this);
    return m_abroValidLifeTime;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetAbroValidLifeTime(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_abroValidLifeTime = time;
}

Ipv6Address
SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroBorderRouterAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_abroBorderRouter;
}

void
SixLowPanNdProtocol::SixLowPanRaEntry::SetAbroBorderRouterAddress(Ipv6Address border)
{
    NS_LOG_FUNCTION(this << border);
    m_abroBorderRouter = border;
}

bool
SixLowPanNdProtocol::SixLowPanRaEntry::ParseAbro(
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter abro)
{
    Ipv6Address addr = abro.GetRouterAddress();
    if (addr == Ipv6Address::GetAny())
    {
        return false;
    }
    m_abroBorderRouter = addr;

    m_abroVersion = abro.GetVersion();
    m_abroValidLifeTime = abro.GetValidLifeTime();
    return true;
}

Icmpv6OptionSixLowPanAuthoritativeBorderRouter
SixLowPanNdProtocol::SixLowPanRaEntry::MakeAbro()
{
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter abro;

    abro.SetRouterAddress(m_abroBorderRouter);
    abro.SetValidLifeTime(m_abroValidLifeTime);
    abro.SetVersion(m_abroVersion);

    return abro;
}

Ptr<Packet>
SixLowPanNdProtocol::MakeNsEaroPacket(Ipv6Address src,
                                      Ipv6Address dst,
                                      Icmpv6NS& nsHdr,
                                      Icmpv6OptionLinkLayerAddress& slla,
                                      Icmpv6OptionLinkLayerAddress& tlla,
                                      Icmpv6OptionSixLowPanExtendedAddressRegistration& earo)
{
    Ptr<Packet> p = Create<Packet>();

    p->AddHeader(earo);
    p->AddHeader(tlla);
    p->AddHeader(slla);

    nsHdr.CalculatePseudoHeaderChecksum(src,
                                        dst,
                                        p->GetSize() + nsHdr.GetSerializedSize(),
                                        PROT_NUMBER);
    p->AddHeader(nsHdr);

    return p;
}

Ptr<Packet>
SixLowPanNdProtocol::MakeNaEaroPacket(Ipv6Address src,
                                      Ipv6Address dst,
                                      Icmpv6NA& naHdr,
                                      Icmpv6OptionSixLowPanExtendedAddressRegistration& earo)
{
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(earo);

    naHdr.CalculatePseudoHeaderChecksum(src,
                                        dst,
                                        p->GetSize() + naHdr.GetSerializedSize(),
                                        PROT_NUMBER);
    p->AddHeader(naHdr);

    return p;
}

Ptr<Packet>
SixLowPanNdProtocol::MakeRaPacket(Ipv6Address src,
                                  Ipv6Address dst,
                                  Icmpv6OptionLinkLayerAddress& slla,
                                  Icmpv6OptionSixLowPanCapabilityIndication& cio,
                                  Ptr<SixLowPanRaEntry> raEntry)
{
    NS_LOG_FUNCTION(src << dst << raEntry);
    Ptr<Packet> p = Create<Packet>();

    // Build RA Hdr
    Icmpv6RA ra = raEntry->BuildRouterAdvertisementHeader();

    // PIO
    for (const auto& pio : raEntry->BuildPrefixInformationOptions())
    {
        p->AddHeader(pio);
    }

    // ABRO
    p->AddHeader(raEntry->MakeAbro());

    // SLLAO
    p->AddHeader(slla);

    // 6CIO
    p->AddHeader(cio);

    // 6CO
    for (const auto& [cid, ctx] : raEntry->GetContexts())
    {
        Icmpv6OptionSixLowPanContext sixHdr;
        sixHdr.SetContextPrefix(ctx->GetContextPrefix());
        sixHdr.SetFlagC(ctx->IsFlagC());
        sixHdr.SetCid(ctx->GetCid());

        Time difference = Simulator::Now() - ctx->GetLastUpdateTime();
        double updatedValidTime =
            ctx->GetValidTime().GetMinutes() - std::floor(difference.GetMinutes());

        // we want to advertise only contexts with a remaining validity time greater than 1
        // minute.
        if (updatedValidTime > 1)
        {
            sixHdr.SetValidTime(updatedValidTime);
            p->AddHeader(sixHdr);
        }
    }

    // Compute checksum after everything is added
    ra.CalculatePseudoHeaderChecksum(src, dst, p->GetSize() + ra.GetSerializedSize(), PROT_NUMBER);
    p->AddHeader(ra);

    return p;
}

bool
SixLowPanNdProtocol::ParseAndValidateNsEaroPacket(
    Ptr<Packet> p,
    Icmpv6NS& nsHdr,
    Icmpv6OptionLinkLayerAddress& slla,
    Icmpv6OptionLinkLayerAddress& tlla,
    Icmpv6OptionSixLowPanExtendedAddressRegistration& earo,
    bool& hasEaro)
{
    NS_LOG_FUNCTION(p);
    p->RemoveHeader(nsHdr);
    bool hasSllao = false;
    bool hasTllao = false;
    hasEaro = false;
    bool next = true;

    while (next && p->GetSize() > 0)
    {
        uint8_t type;
        p->CopyData(&type, sizeof(type));

        switch (type)
        {
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE:
            if (!hasSllao)
            {
                p->RemoveHeader(slla);
                hasSllao = true;
            }
            break;
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_TARGET:
            if (!hasTllao)
            {
                p->RemoveHeader(tlla);
                hasTllao = true;
            }
            break;
        case Icmpv6Header::ICMPV6_OPT_EXTENDED_ADDRESS_REGISTRATION:
            if (!hasEaro)
            {
                p->RemoveHeader(earo);
                hasEaro = true;
            }
            break;
        default:
            // unknown option, quit
            next = false;
        }
        if (p->GetSize() == 0)
        {
            next = false;
        }
    }

    // If it contains EARO, then it must have SLLAO and TLLAO, and SLLAO address == TLLAO address
    if (hasEaro)
    {
        if (!(hasSllao && hasTllao)) // error
        {
            // Address registration proxy is not yet supported.
            NS_LOG_WARN(
                "NS(EARO) message MUST have both source and target link layer options. Ignoring.");
            return false;
        }
        if (slla.GetAddress() != tlla.GetAddress())
        {
            NS_LOG_LOGIC("Discarding NS(EARO) with different target and source addresses: TLLAO ("
                         << tlla.GetAddress() << "), SLLAO (" << slla.GetAddress() << ")");
            return false;
        }
    }

    return true; // Valid NS (May or may not contain EARO)
}

bool
SixLowPanNdProtocol::ParseAndValidateNaEaroPacket(
    Ptr<Packet> p,
    Icmpv6NA& naHdr,
    Icmpv6OptionLinkLayerAddress& tlla,
    Icmpv6OptionSixLowPanExtendedAddressRegistration& earo,
    bool& hasEaro)
{
    NS_LOG_FUNCTION(p);
    p->RemoveHeader(naHdr);
    hasEaro = false;
    bool next = true;

    // search all options following the NA header
    while (next && p->GetSize() > 0)
    {
        uint8_t type;
        p->CopyData(&type, sizeof(type));

        switch (type)
        {
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_TARGET: // NA + EARO + TLLAO
            p->RemoveHeader(tlla);
            break;
        case Icmpv6Header::ICMPV6_OPT_EXTENDED_ADDRESS_REGISTRATION: // NA + EARO
            p->RemoveHeader(earo);
            hasEaro = true;
            break;
        default:
            // unknown option, quit
            next = false;
        }
        if (p->GetSize() == 0)
        {
            next = false;
        }
    }

    return true;
}

bool
SixLowPanNdProtocol::ParseAndValidateRsPacket(Ptr<Packet> p,
                                              Icmpv6RS& rsHdr,
                                              Icmpv6OptionLinkLayerAddress& slla,
                                              Icmpv6OptionSixLowPanCapabilityIndication& cio)
{
    NS_LOG_FUNCTION(p);
    p->RemoveHeader(rsHdr);
    bool hasSlla = false;
    bool hasCio = false;
    bool next = true;

    while (next && p->GetSize() > 0)
    {
        uint8_t type;
        p->CopyData(&type, sizeof(type));

        switch (type)
        {
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE:
            p->RemoveHeader(slla);
            hasSlla = true;
            break;
        case Icmpv6Header::ICMPV6_OPT_CAPABILITY_INDICATION:
            p->RemoveHeader(cio);
            hasCio = true;
            break;
        default:
            // unknown option, quit
            next = false;
        }
        if (p->GetSize() == 0)
        {
            next = false;
        }
    }

    if (!hasSlla)
    {
        NS_LOG_LOGIC("RS message MUST have source link-layer option, discarding it.");
        return false;
    }

    if (!hasCio)
    {
        NS_LOG_LOGIC("RS message MUST have sixlowpan capability indication option, discarding it.");
        return false;
    }

    return true;
}

bool
SixLowPanNdProtocol::ParseAndValidateRaPacket(Ptr<Packet> p,
                                              Icmpv6RA& raHdr,
                                              std::list<Icmpv6OptionPrefixInformation>& pios,
                                              Icmpv6OptionSixLowPanAuthoritativeBorderRouter& abro,
                                              Icmpv6OptionLinkLayerAddress& slla,
                                              Icmpv6OptionSixLowPanCapabilityIndication& cio,
                                              std::list<Icmpv6OptionSixLowPanContext>& contexts)
{
    NS_LOG_FUNCTION(p);
    // Remove the RA header first
    p->RemoveHeader(raHdr);

    bool hasAbro = false;
    bool hasSlla = false;
    bool hasCio = false;

    bool next = true;
    while (next && p->GetSize() > 0)
    {
        uint8_t type = 0;
        p->CopyData(&type, sizeof(type));

        Icmpv6OptionPrefixInformation prefix;
        Icmpv6OptionSixLowPanContext context;

        switch (type)
        {
        case Icmpv6Header::ICMPV6_OPT_PREFIX:
            p->RemoveHeader(prefix);
            pios.push_back(prefix);
            break;
        case Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT:
            p->RemoveHeader(context);
            contexts.push_back(context);
            break;
        case Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER:
            p->RemoveHeader(abro);
            hasAbro = true;
            break;
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE:
            // generates an entry in NDISC table with m_router = true
            // Deferred to when we receive the address registration confirmation
            p->RemoveHeader(slla);
            hasSlla = true;
            break;
        case Icmpv6Header::ICMPV6_OPT_CAPABILITY_INDICATION:
            p->RemoveHeader(cio);
            hasCio = true;
            break;
        default:
            NS_LOG_WARN("Ignoring unknown option in RA (type=" << static_cast<uint32_t>(type)
                                                               << ")");
            next = false;
        }
        if (p->GetSize() == 0)
        {
            next = false;
        }
    }

    if (!hasAbro)
    {
        // RAs MUST contain one (and only one) ABRO
        NS_LOG_LOGIC("Ignoring RA: no ABRO");
        return false;
    }

    if (abro.GetRouterAddress() == Ipv6Address::GetAny())
    {
        NS_LOG_LOGIC("Ignoring RA: ABRO border router address is unspecified");
        return false;
    }

    if (!hasSlla)
    {
        // RAs must contain one (and only one) LLA
        NS_LOG_LOGIC("Ignoring RA: no SLLAO");
        return false;
    }

    if (!hasCio)
    {
        // RAs must contain one (and only one) 6CIO
        NS_LOG_LOGIC("Ignoring RA: no 6CIO");
        return false;
    }

    return true;
}
} /* namespace ns3 */
