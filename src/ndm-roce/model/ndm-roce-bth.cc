/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-roce-bth.h"

namespace ns3
{

TypeId
NdmRoceBth::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NdmRoceBth")
                            .SetParent<Header>()
                            .AddConstructor<NdmRoceBth>();
    return tid;
}

TypeId
NdmRoceBth::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
NdmRoceBth::GetSerializedSize() const
{
    return 16;
}

void
NdmRoceBth::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_opcode);
    start.WriteU8(m_flags);
    start.WriteHtonU32(m_psn);
    start.WriteHtonU32(m_destQp);
    start.WriteHtonU32(m_imm);
}

uint32_t
NdmRoceBth::Deserialize(Buffer::Iterator start)
{
    m_opcode = start.ReadU8();
    m_flags = start.ReadU8();
    m_psn = start.ReadNtohU32();
    m_destQp = start.ReadNtohU32();
    m_imm = start.ReadNtohU32();
    return 16;
}

void
NdmRoceBth::Print(std::ostream& os) const
{
    os << "op=" << static_cast<int>(m_opcode)
       << " flags=" << static_cast<int>(m_flags)
       << " psn=" << m_psn << " qp=" << m_destQp << " imm=" << m_imm;
}

} // namespace ns3
