/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-roce-endpoint.h"

#include "ns3/ipv6-header.h"
#include "ns3/log.h"
#include "ns3/udp-header.h"

#include <algorithm>
#include <cstdint>

NS_LOG_COMPONENT_DEFINE("NdmRoceEndpoint");

namespace ns3
{

NdmRoceEndpoint::NdmRoceEndpoint() = default;

NdmRoceEndpoint::~NdmRoceEndpoint() = default;

void
NdmRoceEndpoint::Attach(Ptr<Node> node, Ptr<NdmPathForwarder> fwd,
                        uint32_t pathId, uint32_t retPathId)
{
    NS_ASSERT_MSG(!m_fwd, "NdmRoceEndpoint: one endpoint per node");
    m_node = node;
    m_fwd = fwd;
    m_pathId = pathId;
    m_retPathId = retPathId;
    m_fwd->SetDeliveryPacketCallback(
        MakeCallback(&NdmRoceEndpoint::OnDelivered, this));
}

void
NdmRoceEndpoint::SetAddresses(Ipv6Address local, Ipv6Address peer)
{
    m_local = local;
    m_peer = peer;
}

Ptr<NdmRoceQPair>
NdmRoceEndpoint::CreateConnection(uint32_t localQpn, uint32_t peerQpn)
{
    NS_ASSERT_MSG(m_tx.find(localQpn) == m_tx.end(),
                  "NdmRoceEndpoint: QPN already in use");
    auto tx = Create<NdmRoceQPair>(localQpn, peerQpn);
    auto rx = Create<NdmRoceRxQPair>(localQpn, peerQpn);
    tx->m_sendTx = MakeCallback(&NdmRoceEndpoint::OnTxSend, this);
    rx->m_sendCtrl = MakeCallback(&NdmRoceEndpoint::OnCtrlSend, this);
    m_tx[localQpn] = tx;
    m_rx[localQpn] = rx;
    return tx;
}

Ptr<NdmRoceRxQPair>
NdmRoceEndpoint::GetRxQp(uint32_t qpn) const
{
    auto it = m_rx.find(qpn);
    NS_ASSERT_MSG(it != m_rx.end(), "NdmRoceEndpoint: unknown RX QP");
    return it->second;
}

void
NdmRoceEndpoint::OnTxSend(Ptr<Packet> pkt, uint32_t destQp)
{
    (void)destQp;
    Ptr<Packet> w = Create<Packet>(0);
    UdpHeader udp;
    udp.SetDestinationPort(kRoceUdpPort);
    udp.SetSourcePort(kRoceUdpPort);
    w->AddHeader(udp);
    Ipv6Header ip;
    ip.SetSource(m_local);
    ip.SetDestination(m_peer);
    ip.SetNextHeader(17); // UDP
    w->AddHeader(ip);
    w->AddAtEnd(pkt); // BTH + payload
    if (!m_fwd->SendPacket(m_pathId, w))
    {
        NS_LOG_INFO("NdmRoceEndpoint: drop (first link down), qp " << destQp);
    }
}

void
NdmRoceEndpoint::OnCtrlSend(Ptr<Packet> pkt, uint32_t destQp)
{
    (void)destQp;
    m_ctrlSent++;
    Ptr<Packet> w = Create<Packet>(0);
    UdpHeader udp;
    udp.SetDestinationPort(kRoceUdpPort);
    udp.SetSourcePort(kRoceUdpPort);
    w->AddHeader(udp);
    Ipv6Header ip;
    ip.SetSource(m_local);
    ip.SetDestination(m_peer);
    ip.SetNextHeader(17); // UDP
    w->AddHeader(ip);
    w->AddAtEnd(pkt); // BTH only
    if (!m_fwd->SendPacket(m_retPathId, w))
    {
        NS_LOG_INFO("NdmRoceEndpoint: ctrl drop (return link down)");
    }
}

void
NdmRoceEndpoint::OnDelivered(uint32_t pathId, Ptr<const Packet> p, Time t)
{
    (void)pathId;
    const uint32_t total = p->GetSize();
    const uint32_t outer = NdmPathForwarder::kPathHdrSize; // 8
    const uint32_t inner = kIpv6Size + kUdpSize + 16 /* NdmRoceBth */;
    NS_ASSERT_MSG(total >= outer + inner,
                  "NdmRoceEndpoint: short RoCE packet on the wire");

    // Decapsulate. NOTE: PeekHeader always reads from the start of the
    // packet, so a stacked header sequence must be peeled with
    // RemoveHeader on a mutable copy.
    Ptr<Packet> rest = p->Copy();
    rest->RemoveAtStart(outer);
    Ipv6Header ip;
    rest->RemoveHeader(ip);
    UdpHeader udp;
    rest->RemoveHeader(udp);
    NdmRoceBth bth;
    rest->RemoveHeader(bth);
    Ptr<Packet> payload = rest; // exactly the message fragment (or empty)

    switch (bth.m_opcode)
    {
        case NDM_ROCE_WRITE:
        case NDM_ROCE_WRITE_WITH_IMM:
        {
            auto it = m_rx.find(bth.m_destQp);
            NS_ASSERT_MSG(it != m_rx.end(),
                          "NdmRoceEndpoint: data for unknown RX QP");
            it->second->HandleData(bth, payload, t);
            break;
        }
        case NDM_ROCE_ACK:
        {
            auto it = m_tx.find(bth.m_destQp);
            NS_ASSERT_MSG(it != m_tx.end(),
                          "NdmRoceEndpoint: ACK for unknown TX QP");
            m_ctrlRx++;
            it->second->HandleAck(bth.m_psn,
                                  (bth.m_flags & NDM_ROCE_BTH_ECN_ECHO) != 0, t);
            break;
        }
        case NDM_ROCE_NACK:
        {
            auto it = m_tx.find(bth.m_destQp);
            NS_ASSERT_MSG(it != m_tx.end(),
                          "NdmRoceEndpoint: NACK for unknown TX QP");
            m_ctrlRx++;
            it->second->HandleNack(bth.m_psn, t);
            break;
        }
        case NDM_ROCE_CNP:
        {
            auto it = m_tx.find(bth.m_destQp);
            NS_ASSERT_MSG(it != m_tx.end(),
                          "NdmRoceEndpoint: CNP for unknown TX QP");
            m_ctrlRx++;
            it->second->HandleCnp(t);
            break;
        }
        default:
            NS_ABORT_MSG("NdmRoceEndpoint: unknown opcode "
                         << static_cast<int>(bth.m_opcode));
    }
}

} // namespace ns3
