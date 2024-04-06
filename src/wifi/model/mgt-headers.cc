/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "mgt-headers.h"

#include "ns3/address-utils.h"
#include "ns3/simulator.h"

namespace ns3
{

/***********************************************************
 *          Probe Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeRequestHeader);

TypeId
MgtProbeRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeRequestHeader>();
    return tid;
}

TypeId
MgtProbeRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

/***********************************************************
 *          Probe Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeResponseHeader);

TypeId
MgtProbeResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeResponseHeader>();
    return tid;
}

TypeId
MgtProbeResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint64_t
MgtProbeResponseHeader::GetTimestamp() const
{
    return m_timestamp;
}

uint32_t
MgtProbeResponseHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 8 /* timestamp */ + 2 /* beacon interval */;
    size += m_capability.GetSerializedSize();
    size += WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::GetSerializedSizeImpl();
    return size;
}

void
MgtProbeResponseHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i.WriteHtolsbU64(Simulator::Now().GetMicroSeconds());
    i.WriteHtolsbU16(static_cast<uint16_t>(m_beaconInterval / 1024));
    i = m_capability.Serialize(i);
    WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::SerializeImpl(i);
}

uint32_t
MgtProbeResponseHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_timestamp = i.ReadLsbtohU64();
    m_beaconInterval = i.ReadLsbtohU16();
    m_beaconInterval *= 1024;
    i = m_capability.Deserialize(i);
    auto distance = i.GetDistanceFrom(start);
    distance += WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); ++id)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasProbeResponse())
            {
                auto& frameInPerStaProfile = perStaProfile.GetProbeResponse();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

/***********************************************************
 *          Beacons
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtBeaconHeader);

/* static */
TypeId
MgtBeaconHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtBeaconHeader")
                            .SetParent<MgtProbeResponseHeader>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtBeaconHeader>();
    return tid;
}

TypeId
MgtBeaconHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint64_t
MgtBeaconHeader::GetTimestamp() const
{
    return m_timestamp;
}

uint32_t
MgtBeaconHeader::GetSerializedSizeImpl() const
{
    uint32_t size = 8 /* timestamp */ + 2 /* beacon interval */;
    size += m_capability.GetSerializedSize();
    size += WifiMgtHeader<MgtBeaconHeader, BeaconElems>::GetSerializedSizeImpl();
    return size;
}

void
MgtBeaconHeader::SerializeImpl(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU64(Simulator::Now().GetMicroSeconds());
    i.WriteHtolsbU16(static_cast<uint16_t>(m_beaconInterval / 1024));
    i = m_capability.Serialize(i);
    WifiMgtHeader<MgtBeaconHeader, BeaconElems>::SerializeImpl(i);
}

uint32_t
MgtBeaconHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_timestamp = i.ReadLsbtohU64();
    m_beaconInterval = i.ReadLsbtohU16();
    m_beaconInterval *= 1024;
    i = m_capability.Deserialize(i);
    auto distance = i.GetDistanceFrom(start);
    return distance + WifiMgtHeader<MgtBeaconHeader, BeaconElems>::DeserializeImpl(i);
}

/***********************************************************
 *          Assoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocRequestHeader);

TypeId
MgtAssocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocRequestHeader>();
    return tid;
}

TypeId
MgtAssocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtAssocRequestHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2; // listen interval
    size += WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtAssocRequestHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtAssocRequestHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtAssocRequestHeader,
                                 AssocRequestElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtAssocRequestHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::SerializeImpl(i);
}

void
MgtAssocRequestHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                    const MgtAssocRequestHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    MgtHeaderInPerStaProfile<MgtAssocRequestHeader,
                             AssocRequestElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtAssocRequestHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasAssocRequest())
            {
                auto& frameInPerStaProfile =
                    std::get<std::reference_wrapper<MgtAssocRequestHeader>>(
                        perStaProfile.GetAssocRequest())
                        .get();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtAssocRequestHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                        uint16_t length,
                                                        const MgtAssocRequestHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = frame.m_listenInterval;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtAssocRequestHeader, AssocRequestElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

uint32_t
MgtProbeResponseHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtProbeResponseHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtProbeResponseHeader,
                                 ProbeResponseElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtProbeResponseHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                     const MgtProbeResponseHeader& frame) const
{
    auto i = start;
    i = m_capability.Serialize(i);
    MgtHeaderInPerStaProfile<MgtProbeResponseHeader,
                             ProbeResponseElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtProbeResponseHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                         uint16_t length,
                                                         const MgtProbeResponseHeader& frame)
{
    auto i = start;
    m_timestamp = frame.GetTimestamp();
    m_beaconInterval = frame.m_beaconInterval;
    i = m_capability.Deserialize(i);
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtProbeResponseHeader, ProbeResponseElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/***********************************************************
 *          Ressoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtReassocRequestHeader);

TypeId
MgtReassocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtReassocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtReassocRequestHeader>();
    return tid;
}

TypeId
MgtReassocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtReassocRequestHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2; // listen interval
    size += 6; // current AP address
    size += WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtReassocRequestHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtReassocRequestHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtReassocRequestHeader,
                                 AssocRequestElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtReassocRequestHeader::PrintImpl(std::ostream& os) const
{
    os << "current AP address=" << m_currentApAddr << ", ";
    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::PrintImpl(os);
}

void
MgtReassocRequestHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    WriteTo(i, m_currentApAddr);
    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::SerializeImpl(i);
}

void
MgtReassocRequestHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                      const MgtReassocRequestHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    MgtHeaderInPerStaProfile<MgtReassocRequestHeader,
                             AssocRequestElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtReassocRequestHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    ReadFrom(i, m_currentApAddr);
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasReassocRequest())
            {
                auto& frameInPerStaProfile =
                    std::get<std::reference_wrapper<MgtReassocRequestHeader>>(
                        perStaProfile.GetAssocRequest())
                        .get();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtReassocRequestHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                          uint16_t length,
                                                          const MgtReassocRequestHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = frame.m_listenInterval;
    m_currentApAddr = frame.m_currentApAddr;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtReassocRequestHeader, AssocRequestElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/***********************************************************
 *          Assoc/Reassoc Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocResponseHeader);

TypeId
MgtAssocResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocResponseHeader>();
    return tid;
}

TypeId
MgtAssocResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtAssocResponseHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += m_statusCode.GetSerializedSize();
    size += 2; // aid
    size += WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtAssocResponseHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtAssocResponseHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += m_statusCode.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtAssocResponseHeader,
                                 AssocResponseElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtAssocResponseHeader::PrintImpl(std::ostream& os) const
{
    os << "status code=" << m_statusCode << ", "
       << "aid=" << m_aid << ", ";
    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::PrintImpl(os);
}

void
MgtAssocResponseHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i = m_statusCode.Serialize(i);
    i.WriteHtolsbU16(m_aid);
    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::SerializeImpl(i);
}

void
MgtAssocResponseHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                     const MgtAssocResponseHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i = m_statusCode.Serialize(i);
    MgtHeaderInPerStaProfile<MgtAssocResponseHeader,
                             AssocResponseElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtAssocResponseHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    i = m_statusCode.Deserialize(i);
    m_aid = i.ReadLsbtohU16();
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasAssocResponse())
            {
                auto& frameInPerStaProfile = perStaProfile.GetAssocResponse();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtAssocResponseHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                         uint16_t length,
                                                         const MgtAssocResponseHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    i = m_statusCode.Deserialize(i);
    m_aid = frame.m_aid;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtAssocResponseHeader, AssocResponseElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/***********************************************************
 *          Disassociation
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtDisassociationHeader);

TypeId
MgtDisassociationHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtDisassociationHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtDisassociationHeader>();
    return tid;
}

TypeId
MgtDisassociationHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtDisassociationHeader::GetSerializedSize() const
{
    return 2; // reason code size
}

void
MgtDisassociationHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtolsbU16(m_reasonCode);
}

uint32_t
MgtDisassociationHeader::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    m_reasonCode = i.ReadLsbtohU16();
    return i.GetDistanceFrom(start);
}

void
MgtDisassociationHeader::Print(std::ostream& os) const
{
    os << "reason code=" << m_reasonCode;
}

} // namespace ns3
