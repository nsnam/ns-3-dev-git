/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-roce-rx-qpair.h"

#include "ns3/log.h"

#define NS_LOG_COMPONENT_NAME "NdmRoceRxQPair"

namespace ns3
{

NdmRoceRxQPair::NdmRoceRxQPair(uint32_t localQpn, uint32_t peerQpn)
    : m_localQpn(localQpn),
      m_peerQpn(peerQpn)
{
}

void
NdmRoceRxQPair::SendCtrl(uint8_t opcode, uint32_t psn, uint8_t flags)
{
    Ptr<Packet> pkt = Create<Packet>();
    NdmRoceBth bth(opcode, psn, m_peerQpn);
    bth.SetFlags(flags);
    pkt->AddHeader(bth);
    m_sendCtrl(pkt, m_peerQpn);
}

void
NdmRoceRxQPair::HandleData(const NdmRoceBth& bth, Ptr<Packet> payload, Time now)
{
    (void)now;

    // ECN-marked packet: congestion notification bookkeeping (DCQCN).
    if (bth.m_flags & NDM_ROCE_BTH_ECN_MARKED)
    {
        m_ecnSinceLastAck++;
        if (m_ecnSinceLastAck >= m_cnpThreshold && !m_cnpSinceLastAck)
        {
            SendCtrl(NDM_ROCE_CNP, bth.m_psn, 0);
            m_cnpsSent++;
            m_cnpSinceLastAck = true;
        }
    }

    if (bth.m_psn < m_nextPsn)
    {
        // Duplicate (retransmission): never re-deliver; re-ACK so the
        // sender's window advances (idempotent).
        m_duplicates++;
        if (bth.m_flags & NDM_ROCE_BTH_ACK_REQ)
        {
            const uint8_t flags =
                (m_ecnSinceLastAck > 0) ? NDM_ROCE_BTH_ECN_ECHO : 0;
            SendCtrl(NDM_ROCE_ACK, m_nextPsn - 1, flags);
            m_ecnSinceLastAck = 0;
            m_cnpSinceLastAck = false;
        }
        return;
    }

    if (bth.m_psn > m_nextPsn)
    {
        // Gap: tell the sender the next PSN we need (go-back-N trigger).
        NS_LOG_INFO("NdmRoceRxQPair " << m_localQpn << ": gap, got "
                                      << bth.m_psn << " want " << m_nextPsn);
        SendCtrl(NDM_ROCE_NACK, m_nextPsn, 0);
        m_nacksSent++;
        return;
    }

    // In order: deliver exactly once.
    const bool isLast = (bth.m_opcode == NDM_ROCE_WRITE_WITH_IMM);
    if (m_deliver.IsInitialized())
    {
        m_deliver(bth.m_psn, payload, bth.m_imm, isLast);
    }
    m_delivered++;
    m_bytesDelivered += payload->GetSize();
    m_nextPsn++;

    if (bth.m_flags & NDM_ROCE_BTH_ACK_REQ)
    {
        const uint8_t flags =
            (m_ecnSinceLastAck > 0) ? NDM_ROCE_BTH_ECN_ECHO : 0;
        SendCtrl(NDM_ROCE_ACK, bth.m_psn, flags);
        m_ecnSinceLastAck = 0;
        m_cnpSinceLastAck = false;
    }
}

} // namespace ns3
