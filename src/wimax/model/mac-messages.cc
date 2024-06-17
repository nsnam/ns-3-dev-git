/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#include "mac-messages.h"

#include "wimax-tlv.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MACMESSAGES");

NS_OBJECT_ENSURE_REGISTERED(ManagementMessageType);

ManagementMessageType::ManagementMessageType()
    : m_type(~0)
{
}

ManagementMessageType::ManagementMessageType(uint8_t type)
    : m_type(type)
{
}

ManagementMessageType::~ManagementMessageType()
{
}

void
ManagementMessageType::SetType(uint8_t type)
{
    m_type = type;
}

uint8_t
ManagementMessageType::GetType() const
{
    return m_type;
}

std::string
ManagementMessageType::GetName() const
{
    return "Management Message Type";
}

TypeId
ManagementMessageType::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ManagementMessageType")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<ManagementMessageType>();
    return tid;
}

TypeId
ManagementMessageType::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
ManagementMessageType::Print(std::ostream& os) const
{
    os << " management message type = " << (uint32_t)m_type;
}

uint32_t
ManagementMessageType::GetSerializedSize() const
{
    return 1;
}

void
ManagementMessageType::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_type);
}

uint32_t
ManagementMessageType::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_type = i.ReadU8();
    return i.GetDistanceFrom(start);
}

// ---------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(RngReq);

RngReq::RngReq()
    : m_reserved(0),
      m_reqDlBurstProfile(0),
      m_macAddress(Mac48Address("00:00:00:00:00:00")),
      m_rangingAnomalies(0)
{
}

RngReq::~RngReq()
{
}

void
RngReq::SetReqDlBurstProfile(uint8_t reqDlBurstProfile)
{
    m_reqDlBurstProfile = reqDlBurstProfile;
}

void
RngReq::SetMacAddress(Mac48Address macAddress)
{
    m_macAddress = macAddress;
}

void
RngReq::SetRangingAnomalies(uint8_t rangingAnomalies)
{
    m_rangingAnomalies = rangingAnomalies;
}

uint8_t
RngReq::GetReqDlBurstProfile() const
{
    return m_reqDlBurstProfile;
}

Mac48Address
RngReq::GetMacAddress() const
{
    return m_macAddress;
}

uint8_t
RngReq::GetRangingAnomalies() const
{
    return m_rangingAnomalies;
}

std::string
RngReq::GetName() const
{
    return "RNG-REQ";
}

TypeId
RngReq::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RngReq").SetParent<Header>().SetGroupName("Wimax").AddConstructor<RngReq>();
    return tid;
}

TypeId
RngReq::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RngReq::Print(std::ostream& os) const
{
    os << " requested dl burst profile = " << (uint32_t)m_reqDlBurstProfile
       << ", mac address = " << m_macAddress
       << ", ranging anomalies = " << (uint32_t)m_rangingAnomalies;
}

void
RngReq::PrintDebug() const
{
    NS_LOG_DEBUG(" requested dl burst profile = "
                 << (uint32_t)m_reqDlBurstProfile << ", mac address = " << m_macAddress
                 << ", ranging anomalies = " << (uint32_t)m_rangingAnomalies);
}

uint32_t
RngReq::GetSerializedSize() const
{
    return 1 + 1 + 6 + 1;
}

void
RngReq::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_reserved);
    i.WriteU8(m_reqDlBurstProfile);
    WriteTo(i, m_macAddress);
    i.WriteU8(m_rangingAnomalies);
}

uint32_t
RngReq::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    m_reqDlBurstProfile = i.ReadU8();
    ReadFrom(i, m_macAddress);
    m_rangingAnomalies = i.ReadU8();

    return i.GetDistanceFrom(start);
}

// ---------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(RngRsp);

RngRsp::RngRsp()
    : m_reserved(0),
      m_timingAdjust(0),
      m_powerLevelAdjust(0),
      m_offsetFreqAdjust(0),
      m_rangStatus(0),
      m_dlFreqOverride(0),
      m_ulChnlIdOverride(0),
      m_dlOperBurstProfile(0),
      m_macAddress(Mac48Address("00:00:00:00:00:00")),
      m_basicCid(),
      m_primaryCid(),
      m_aasBdcastPermission(0),
      m_frameNumber(0),
      m_initRangOppNumber(0),
      m_rangSubchnl(0)
{
}

RngRsp::~RngRsp()
{
}

void
RngRsp::SetTimingAdjust(uint32_t timingAdjust)
{
    m_timingAdjust = timingAdjust;
}

void
RngRsp::SetPowerLevelAdjust(uint8_t powerLevelAdjust)
{
    m_powerLevelAdjust = powerLevelAdjust;
}

void
RngRsp::SetOffsetFreqAdjust(uint32_t offsetFreqAdjust)
{
    m_offsetFreqAdjust = offsetFreqAdjust;
}

void
RngRsp::SetRangStatus(uint8_t rangStatus)
{
    m_rangStatus = rangStatus;
}

void
RngRsp::SetDlFreqOverride(uint32_t dlFreqOverride)
{
    m_dlFreqOverride = dlFreqOverride;
}

void
RngRsp::SetUlChnlIdOverride(uint8_t ulChnlIdOverride)
{
    m_ulChnlIdOverride = ulChnlIdOverride;
}

void
RngRsp::SetDlOperBurstProfile(uint16_t dlOperBurstProfile)
{
    m_dlOperBurstProfile = dlOperBurstProfile;
}

void
RngRsp::SetMacAddress(Mac48Address macAddress)
{
    m_macAddress = macAddress;
}

void
RngRsp::SetBasicCid(Cid basicCid)
{
    m_basicCid = basicCid;
}

void
RngRsp::SetPrimaryCid(Cid primaryCid)
{
    m_primaryCid = primaryCid;
}

void
RngRsp::SetAasBdcastPermission(uint8_t aasBdcastPermission)
{
    m_aasBdcastPermission = aasBdcastPermission;
}

void
RngRsp::SetFrameNumber(uint32_t frameNumber)
{
    m_frameNumber = frameNumber;
}

void
RngRsp::SetInitRangOppNumber(uint8_t initRangOppNumber)
{
    m_initRangOppNumber = initRangOppNumber;
}

void
RngRsp::SetRangSubchnl(uint8_t rangSubchnl)
{
    m_rangSubchnl = rangSubchnl;
}

uint32_t
RngRsp::GetTimingAdjust() const
{
    return m_timingAdjust;
}

uint8_t
RngRsp::GetPowerLevelAdjust() const
{
    return m_powerLevelAdjust;
}

uint32_t
RngRsp::GetOffsetFreqAdjust() const
{
    return m_offsetFreqAdjust;
}

uint8_t
RngRsp::GetRangStatus() const
{
    return m_rangStatus;
}

uint32_t
RngRsp::GetDlFreqOverride() const
{
    return m_dlFreqOverride;
}

uint8_t
RngRsp::GetUlChnlIdOverride() const
{
    return m_ulChnlIdOverride;
}

uint16_t
RngRsp::GetDlOperBurstProfile() const
{
    return m_dlOperBurstProfile;
}

Mac48Address
RngRsp::GetMacAddress() const
{
    return m_macAddress;
}

Cid
RngRsp::GetBasicCid() const
{
    return m_basicCid;
}

Cid
RngRsp::GetPrimaryCid() const
{
    return m_primaryCid;
}

uint8_t
RngRsp::GetAasBdcastPermission() const
{
    return m_aasBdcastPermission;
}

uint32_t
RngRsp::GetFrameNumber() const
{
    return m_frameNumber;
}

uint8_t
RngRsp::GetInitRangOppNumber() const
{
    return m_initRangOppNumber;
}

uint8_t
RngRsp::GetRangSubchnl() const
{
    return m_rangSubchnl;
}

std::string
RngRsp::GetName() const
{
    return "RNG-RSP";
}

TypeId
RngRsp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RngRsp").SetParent<Header>().SetGroupName("Wimax").AddConstructor<RngRsp>();
    return tid;
}

TypeId
RngRsp::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
RngRsp::Print(std::ostream& os) const
{
    os << " timing adjust = " << m_timingAdjust
       << ", power level adjust = " << (uint32_t)m_powerLevelAdjust
       << ", offset freq adjust = " << m_offsetFreqAdjust
       << ", ranging status = " << (uint32_t)m_rangStatus
       << ", dl freq override = " << m_dlFreqOverride
       << ", ul channel id override = " << (uint32_t)m_ulChnlIdOverride
       << ", dl operational burst profile = " << (uint32_t)m_dlOperBurstProfile
       << ", mac address = " << m_macAddress << ", basic cid = " << m_basicCid
       << ", primary management cid = " << m_primaryCid
       << ", aas broadcast permission = " << (uint32_t)m_aasBdcastPermission
       << ", frame number = " << m_frameNumber
       << ", initial ranging opportunity number = " << (uint32_t)m_initRangOppNumber
       << ", ranging subchannel = " << (uint32_t)m_rangSubchnl;
}

uint32_t
RngRsp::GetSerializedSize() const
{
    return 1 + 4 + 1 + 4 + 1 + 4 + 1 + 2 + 6 + 2 + 2 + 1 + 4 + 1 + 1;
}

void
RngRsp::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_reserved);
    i.WriteU32(m_timingAdjust);
    i.WriteU8(m_powerLevelAdjust);
    i.WriteU32(m_offsetFreqAdjust);
    i.WriteU8(m_rangStatus);
    i.WriteU32(m_dlFreqOverride);
    i.WriteU8(m_ulChnlIdOverride);
    i.WriteU16(m_dlOperBurstProfile);
    WriteTo(i, m_macAddress);
    i.WriteU16(m_basicCid.GetIdentifier());
    i.WriteU16(m_primaryCid.GetIdentifier());
    i.WriteU8(m_aasBdcastPermission);
    i.WriteU32(m_frameNumber);
    i.WriteU8(m_initRangOppNumber);
    i.WriteU8(m_rangSubchnl);
}

uint32_t
RngRsp::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    m_timingAdjust = i.ReadU32();
    m_powerLevelAdjust = i.ReadU8();
    m_offsetFreqAdjust = i.ReadU32();
    m_rangStatus = i.ReadU8();
    m_dlFreqOverride = i.ReadU32();
    m_ulChnlIdOverride = i.ReadU8();
    m_dlOperBurstProfile = i.ReadU16();
    ReadFrom(i, m_macAddress); // length (6) shall also be written in packet instead of hard coded,
                               // see ARP example
    m_basicCid = i.ReadU16();
    m_primaryCid = i.ReadU16();
    m_aasBdcastPermission = i.ReadU8();
    m_frameNumber = i.ReadU32();
    m_initRangOppNumber = i.ReadU8();
    m_rangSubchnl = i.ReadU8();

    return i.GetDistanceFrom(start);
}

// ---------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(DsaReq);

DsaReq::DsaReq()
    : m_transactionId(0),
      m_sfid(0),
      m_cid(),
      m_serviceFlow(ServiceFlow::SF_DIRECTION_DOWN)
{
}

DsaReq::DsaReq(ServiceFlow sf)
{
    m_transactionId = 0;
    m_serviceFlow = sf;
}

DsaReq::~DsaReq()
{
}

void
DsaReq::SetTransactionId(uint16_t transactionId)
{
    m_transactionId = transactionId;
}

uint16_t
DsaReq::GetTransactionId() const
{
    return m_transactionId;
}

void
DsaReq::SetSfid(uint32_t sfid)
{
    m_sfid = sfid;
}

uint32_t
DsaReq::GetSfid() const
{
    return m_sfid;
}

void
DsaReq::SetCid(Cid cid)
{
    m_cid = cid;
}

Cid
DsaReq::GetCid() const
{
    return m_cid;
}

std::string
DsaReq::GetName() const
{
    return "DSA-REQ";
}

TypeId
DsaReq::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DsaReq").SetParent<Header>().SetGroupName("Wimax").AddConstructor<DsaReq>();
    return tid;
}

TypeId
DsaReq::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
DsaReq::Print(std::ostream& os) const
{
    os << " transaction id = " << (uint32_t)m_transactionId << ", m_sfid = " << m_sfid
       << ", cid = " << m_cid;
}

uint32_t
DsaReq::GetSerializedSize() const
{
    Tlv t = m_serviceFlow.ToTlv();
    uint32_t size = 2 + t.GetSerializedSize();
    return size;
}

void
DsaReq::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU16(m_transactionId);
    Tlv t = m_serviceFlow.ToTlv();
    t.Serialize(i);
}

uint32_t
DsaReq::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_transactionId = i.ReadU16();
    Tlv tlv;
    uint32_t size = tlv.Deserialize(i);
    m_serviceFlow = ServiceFlow(tlv);
    return size + 2;
}

ServiceFlow
DsaReq::GetServiceFlow() const
{
    return m_serviceFlow;
}

void
DsaReq::SetServiceFlow(ServiceFlow sf)
{
    m_serviceFlow = sf;
}

// ---------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(DsaRsp);

DsaRsp::DsaRsp()
    : m_transactionId(0),
      m_confirmationCode(0),
      m_sfid(0),
      m_cid()
{
}

DsaRsp::~DsaRsp()
{
}

void
DsaRsp::SetTransactionId(uint16_t transactionId)
{
    m_transactionId = transactionId;
}

uint16_t
DsaRsp::GetTransactionId() const
{
    return m_transactionId;
}

ServiceFlow
DsaRsp::GetServiceFlow() const
{
    return m_serviceFlow;
}

void
DsaRsp::SetServiceFlow(ServiceFlow sf)
{
    m_serviceFlow = sf;
}

void
DsaRsp::SetConfirmationCode(uint16_t confirmationCode)
{
    m_confirmationCode = confirmationCode;
}

uint16_t
DsaRsp::GetConfirmationCode() const
{
    return m_confirmationCode;
}

void
DsaRsp::SetSfid(uint32_t sfid)
{
    m_sfid = sfid;
}

uint32_t
DsaRsp::GetSfid() const
{
    return m_sfid;
}

void
DsaRsp::SetCid(Cid cid)
{
    m_cid = cid;
}

Cid
DsaRsp::GetCid() const
{
    return m_cid;
}

std::string
DsaRsp::GetName() const
{
    return "DSA-RSP";
}

TypeId
DsaRsp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DsaRsp").SetParent<Header>().SetGroupName("Wimax").AddConstructor<DsaRsp>();
    return tid;
}

TypeId
DsaRsp::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
DsaRsp::Print(std::ostream& os) const
{
    os << " transaction id = " << (uint32_t)m_transactionId
       << ", confirmation code = " << (uint32_t)m_confirmationCode << ", m_sfid = " << m_sfid
       << ", cid = " << m_cid;
}

uint32_t
DsaRsp::GetSerializedSize() const
{
    return 2 + 1 + m_serviceFlow.ToTlv().GetSerializedSize();
}

void
DsaRsp::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU16(m_transactionId);
    i.WriteU8(m_confirmationCode);
    m_serviceFlow.ToTlv().Serialize(i);
}

uint32_t
DsaRsp::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_transactionId = i.ReadU16();
    m_confirmationCode = i.ReadU8();
    Tlv tlv;
    uint32_t size = tlv.Deserialize(i);
    m_serviceFlow = ServiceFlow(tlv);
    return size + 3;
}

// ---------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(DsaAck);

DsaAck::DsaAck()
    : m_transactionId(0),
      m_confirmationCode(0)
{
}

DsaAck::~DsaAck()
{
}

void
DsaAck::SetTransactionId(uint16_t transactionId)
{
    m_transactionId = transactionId;
}

uint16_t
DsaAck::GetTransactionId() const
{
    return m_transactionId;
}

void
DsaAck::SetConfirmationCode(uint16_t confirmationCode)
{
    m_confirmationCode = confirmationCode;
}

uint16_t
DsaAck::GetConfirmationCode() const
{
    return m_confirmationCode;
}

std::string
DsaAck::GetName() const
{
    return "DSA-ACK";
}

TypeId
DsaAck::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DsaAck").SetParent<Header>().SetGroupName("Wimax").AddConstructor<DsaAck>();
    return tid;
}

TypeId
DsaAck::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
DsaAck::Print(std::ostream& os) const
{
    os << " transaction id = " << (uint32_t)m_transactionId
       << ", confirmation code = " << (uint32_t)m_confirmationCode;
}

uint32_t
DsaAck::GetSerializedSize() const
{
    return 2 + 1;
}

void
DsaAck::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU16(m_transactionId);
    i.WriteU8(m_confirmationCode);
}

uint32_t
DsaAck::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_transactionId = i.ReadU16();
    m_confirmationCode = i.ReadU8();

    return i.GetDistanceFrom(start);
}

} // namespace ns3
