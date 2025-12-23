/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "smd-bss-transition-parameters.h"

namespace
{
/// Size in bytes of the SMD BSS Transition Parameters ST Preparation Request ST Control field, IEEE
/// 802.11bn D2.0 Figure 9-aa47
constexpr uint16_t WIFI_SMD_ST_PREP_REQ_ST_CONTROL_SIZE_B = 1;
/// Size in bytes of the SMD BSS Transition Parameters ST Preparation Response ST Control field,
/// IEEE 802.11bn D2.0 Figure 9-aa51
constexpr uint16_t WIFI_SMD_ST_PREP_RESP_ST_CONTROL_SIZE_B = 1;
/// Size in bytes of the SMD BSS Transition Parameters ST Execution Request ST Control field, IEEE
/// 802.11bn D2.0 Figure 9-aa56
constexpr uint16_t WIFI_SMD_ST_EXEC_REQ_ST_CONTROL_SIZE_B = 1;
/// Size in bytes of the SMD BSS Transition Parameters ST Execution Response ST Control field, IEEE
/// 802.11bn D2.0 Figure 9-aa58
constexpr uint16_t WIFI_SMD_ST_EXEC_RESP_ST_CONTROL_SIZE_B = 1;
} // namespace

namespace ns3
{

SmdBssTransitionParameters::SmdBssTransitionParameters(
    const SmdBssTransitionParameters::StInfo& stInfo)
    : m_stInfo{stInfo}
{
}

SmdBssTransitionParameters::SmdBssTransitionParameters(
    SmdBssTransitionParameters::ContainingFrameType containingFrameType)
    : m_stInfo(std::in_place_type<std::monostate>) // initialize as UNSET
{
    SetContainingFrameType(containingFrameType);
}

void
SmdBssTransitionParameters::SetContainingFrameType(
    SmdBssTransitionParameters::ContainingFrameType type)
{
    NS_ABORT_MSG_IF(GetContainingFrameType() != UNSET, "Containing frame type already set");
    NS_ABORT_MSG_IF(type == UNSET, "Invalid containing frame type");

    switch (type)
    {
    case ST_PREPARATION_REQUEST:
        m_stInfo = StPrepReqStInfo{};
        break;
    case ST_PREPARATION_RESPONSE:
        m_stInfo = StPrepRespStInfo{};
        break;
    case ST_EXECUTION_REQUEST:
        m_stInfo = StExecReqStInfo{};
        break;
    case ST_EXECUTION_RESPONSE:
        m_stInfo = StExecRespStInfo{};
        break;
    default:
        NS_ABORT_MSG("Unsupported containing frame type: " + std::to_string(type));
    }
}

SmdBssTransitionParameters::ContainingFrameType
SmdBssTransitionParameters::GetContainingFrameType() const
{
    return static_cast<ContainingFrameType>(m_stInfo.index() - 1);
}

void
SmdBssTransitionParameters::Print(std::ostream& os) const
{
    os << "SMD BSS Transition Parameters=["
       << "Containing Frame Type=" << GetContainingFrameType() << ", ST Info=";
    std::visit(
        [&os](auto&& stInfo) {
            if constexpr (std::is_same_v<std::decay_t<decltype(stInfo)>, std::monostate>)
            {
                os << "unset";
            }
            else
            {
                os << "[";
                stInfo.Print(os);
                os << "]";
            }
        },
        m_stInfo);
    os << "]";
}

void
SmdBssTransitionParameters::ScsList::Serialize(Buffer::Iterator& start) const
{
    NS_ABORT_MSG_IF(m_scsIds.empty(), "Number Of SCS IDs value 0 is reserved");
    NS_ABORT_MSG_IF(m_scsIds.size() > 255, "Number Of SCS IDs exceeds maximum value");
    start.WriteU8(static_cast<uint8_t>(m_scsIds.size()));
    for (const auto scsId : m_scsIds)
    {
        start.WriteU8(scsId);
    }
}

uint16_t
SmdBssTransitionParameters::ScsList::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto numScsIds = i.ReadU8();
    NS_ABORT_MSG_IF(numScsIds == 0, "Number Of SCS IDs value 0 is reserved");
    m_scsIds.clear();
    m_scsIds.reserve(numScsIds);
    for (uint8_t n = 0; n < numScsIds; ++n)
    {
        m_scsIds.push_back(i.ReadU8());
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::ScsList::GetSerializedSize() const
{
    NS_ABORT_MSG_IF(m_scsIds.empty(), "Number Of SCS IDs value 0 is reserved");
    return 1 + static_cast<uint16_t>(m_scsIds.size());
}

void
SmdBssTransitionParameters::ScsList::Print(std::ostream& os) const
{
    os << "SCS IDs=[";
    for (std::size_t i = 0; i < m_scsIds.size(); ++i)
    {
        os << (i == 0 ? "" : ", ") << +m_scsIds[i];
    }
    os << "]";
}

void
SmdBssTransitionParameters::StPrepReqStInfo::StControl::Serialize(Buffer::Iterator& start) const
{
    const uint8_t val = m_reqDlSnNotTransferred | (m_reqUlSnNotTransferred << 1) |
                        ((m_scsListPresent ? 1 : 0) << 2);
    start.WriteU8(val);
}

uint16_t
SmdBssTransitionParameters::StPrepReqStInfo::StControl::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto val = i.ReadU8();
    m_reqDlSnNotTransferred = val & 0x01;
    m_reqUlSnNotTransferred = (val >> 1) & 0x01;
    m_scsListPresent = (val >> 2) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepReqStInfo::StControl::GetSerializedSize() const
{
    return WIFI_SMD_ST_PREP_REQ_ST_CONTROL_SIZE_B;
}

SmdBssTransitionParameters::StPrepReqStInfo::StPrepReqStInfo()
    : m_stControl(),
      m_listenInterval(0),
      m_scsList(m_stControl.m_scsListPresent)
{
}

SmdBssTransitionParameters::StPrepReqStInfo::StPrepReqStInfo(const StPrepReqStInfo& other)
    : m_stControl(other.m_stControl),
      m_listenInterval(other.m_listenInterval),
      m_scsList(m_stControl.m_scsListPresent)
{
    m_scsList = other.m_scsList.ToOptional();
}

SmdBssTransitionParameters::StPrepReqStInfo&
SmdBssTransitionParameters::StPrepReqStInfo::operator=(const StPrepReqStInfo& other)
{
    if (this != &other)
    {
        m_stControl = other.m_stControl;
        m_listenInterval = other.m_listenInterval;
        m_scsList = other.m_scsList.ToOptional();
    }
    return *this;
}

void
SmdBssTransitionParameters::StPrepReqStInfo::Serialize(Buffer::Iterator& start) const
{
    m_stControl.Serialize(start);
    start.WriteU16(m_listenInterval);
    if (m_scsList)
    {
        m_scsList->Serialize(start);
    }
}

uint16_t
SmdBssTransitionParameters::StPrepReqStInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    i.Next(m_stControl.Deserialize(i));
    m_listenInterval = i.ReadU16();
    if (m_stControl.m_scsListPresent)
    {
        m_scsList = ScsList{};
        i.Next(m_scsList->Deserialize(i));
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepReqStInfo::GetSerializedSize() const
{
    return m_stControl.GetSerializedSize() + sizeof(m_listenInterval) +
           (m_scsList ? m_scsList->GetSerializedSize() : 0);
}

void
SmdBssTransitionParameters::StPrepReqStInfo::Print(std::ostream& os) const
{
    os << "Request DL SN Not Transferred=" << +m_stControl.m_reqDlSnNotTransferred
       << ", Request UL SN Not Transferred=" << +m_stControl.m_reqUlSnNotTransferred
       << ", Listen Interval=" << m_listenInterval;
    if (m_scsList)
    {
        os << ", ";
        m_scsList->Print(os);
    }
}

void
SmdBssTransitionParameters::StPrepRespStInfo::StControl::Serialize(Buffer::Iterator& start) const
{
    const uint8_t val = (m_dlBaInfoPresent ? 0x01 : 0x00) | (m_ulBaInfoPresent ? 0x02 : 0x00) |
                        (m_scsListPresent ? 0x04 : 0x00);
    start.WriteU8(val);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::StControl::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto val = i.ReadU8();
    m_dlBaInfoPresent = val & 0x01;
    m_ulBaInfoPresent = (val >> 1) & 0x01;
    m_scsListPresent = (val >> 2) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::StControl::GetSerializedSize() const
{
    return WIFI_SMD_ST_PREP_RESP_ST_CONTROL_SIZE_B;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo::Serialize(
    Buffer::Iterator& start) const
{
    start = m_baParameterSet.Serialize(start);
    start = m_addBaExtParameterSet.Serialize(start);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    i = m_baParameterSet.Deserialize(i);
    i = m_addBaExtParameterSet.Deserialize(i);
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo::GetSerializedSize() const
{
    return m_baParameterSet.GetSerializedSize() + m_addBaExtParameterSet.GetSerializedSize();
}

void
SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo::Print(std::ostream& os) const
{
    os << m_baParameterSet << ", " << m_addBaExtParameterSet;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::Set(tid_t tid, const ExtBaParamsInfo& info)
{
    NS_ABORT_MSG_IF(tid >= 8, "TID must be in the range [0, 7]");
    m_extBaParamsInfos[tid] = info;
    m_tidBitmap |= (1 << tid);
}

std::optional<SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo>
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::Get(tid_t tid) const
{
    NS_ABORT_MSG_IF(tid >= 8, "TID must be in the range [0, 7]");
    const auto it = m_extBaParamsInfos.find(tid);
    if (it != m_extBaParamsInfos.cend())
    {
        NS_ABORT_MSG_IF(!(m_tidBitmap & (1 << tid)),
                        "TID bitmap does not indicate presence of TID " + std::to_string(tid));
        return it->second;
    }
    return std::nullopt;
}

uint8_t
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::GetTidBitmap() const
{
    return m_tidBitmap;
}

const std::map<tid_t, SmdBssTransitionParameters::StPrepRespStInfo::ExtBaParamsInfo>&
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::GetExtBaParamsInfos() const
{
    return m_extBaParamsInfos;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_tidBitmap);
    for (const auto& [tid, info] : m_extBaParamsInfos)
    {
        info.Serialize(start);
    }
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    m_tidBitmap = i.ReadU8();
    m_extBaParamsInfos.clear();
    for (tid_t tid = 0; tid < 8; ++tid)
    {
        if (m_tidBitmap & (1 << tid))
        {
            ExtBaParamsInfo info{};
            i.Next(info.Deserialize(i));
            m_extBaParamsInfos[tid] = info;
        }
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::GetSerializedSize() const
{
    uint16_t size = 1; // DL/UL TID Bitmap field
    for (const auto& [tid, info] : m_extBaParamsInfos)
    {
        size += info.GetSerializedSize();
    }
    return size;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::BaInfo::Print(std::ostream& os) const
{
    os << "TID Bitmap=0x" << std::hex << +m_tidBitmap << std::dec << ", Ext BA Params=[";
    bool first = true;
    for (const auto& [tid, info] : m_extBaParamsInfos)
    {
        os << (first ? "" : "; ") << "TID " << +tid << ": [";
        info.Print(os);
        os << "]";
        first = false;
    }
    os << "]";
}

SmdBssTransitionParameters::StPrepRespStInfo::StPrepRespStInfo()
    : m_stControl(),
      m_dlBaInfo(m_stControl.m_dlBaInfoPresent),
      m_ulBaInfo(m_stControl.m_ulBaInfoPresent),
      m_scsList(m_stControl.m_scsListPresent)
{
}

SmdBssTransitionParameters::StPrepRespStInfo::StPrepRespStInfo(const StPrepRespStInfo& other)
    : m_stControl(other.m_stControl),
      m_dlBaInfo(m_stControl.m_dlBaInfoPresent),
      m_ulBaInfo(m_stControl.m_ulBaInfoPresent),
      m_scsList(m_stControl.m_scsListPresent)
{
    m_dlBaInfo = other.m_dlBaInfo.ToOptional();
    m_ulBaInfo = other.m_ulBaInfo.ToOptional();
    m_scsList = other.m_scsList.ToOptional();
}

SmdBssTransitionParameters::StPrepRespStInfo&
SmdBssTransitionParameters::StPrepRespStInfo::operator=(const StPrepRespStInfo& other)
{
    if (this != &other)
    {
        m_stControl = other.m_stControl;
        m_dlBaInfo = other.m_dlBaInfo.ToOptional();
        m_ulBaInfo = other.m_ulBaInfo.ToOptional();
        m_scsList = other.m_scsList.ToOptional();
    }
    return *this;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::Serialize(Buffer::Iterator& start) const
{
    m_stControl.Serialize(start);
    if (m_dlBaInfo)
    {
        m_dlBaInfo->Serialize(start);
    }
    if (m_ulBaInfo)
    {
        m_ulBaInfo->Serialize(start);
    }
    if (m_scsList)
    {
        m_scsList->Serialize(start);
    }
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    i.Next(m_stControl.Deserialize(i));
    if (m_stControl.m_dlBaInfoPresent)
    {
        m_dlBaInfo = BaInfo{};
        i.Next(m_dlBaInfo->Deserialize(i));
    }
    if (m_stControl.m_ulBaInfoPresent)
    {
        m_ulBaInfo = BaInfo{};
        i.Next(m_ulBaInfo->Deserialize(i));
    }
    if (m_stControl.m_scsListPresent)
    {
        m_scsList = ScsList{};
        i.Next(m_scsList->Deserialize(i));
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StPrepRespStInfo::GetSerializedSize() const
{
    uint16_t size = m_stControl.GetSerializedSize();
    if (m_dlBaInfo)
    {
        size += m_dlBaInfo->GetSerializedSize();
    }
    if (m_ulBaInfo)
    {
        size += m_ulBaInfo->GetSerializedSize();
    }
    if (m_scsList)
    {
        size += m_scsList->GetSerializedSize();
    }
    return size;
}

void
SmdBssTransitionParameters::StPrepRespStInfo::Print(std::ostream& os) const
{
    bool first = true;
    if (m_dlBaInfo)
    {
        os << "DL BA Info=[";
        m_dlBaInfo->Print(os);
        os << "]";
        first = false;
    }
    if (m_ulBaInfo)
    {
        os << (first ? "" : ", ") << "UL BA Info=[";
        m_ulBaInfo->Print(os);
        os << "]";
        first = false;
    }
    if (m_scsList)
    {
        os << (first ? "" : ", ");
        m_scsList->Print(os);
    }
}

void
SmdBssTransitionParameters::StExecReqStInfo::StControl::Serialize(Buffer::Iterator& start) const
{
    const uint8_t val = (m_dlTidBmPresent ? 0x01 : 0x00) | (m_dlBaInfoPresent ? 0x02 : 0x00);
    start.WriteU8(val);
}

uint16_t
SmdBssTransitionParameters::StExecReqStInfo::StControl::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto val = i.ReadU8();
    m_dlTidBmPresent = val & 0x01;
    m_dlBaInfoPresent = (val >> 1) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StExecReqStInfo::StControl::GetSerializedSize() const
{
    return WIFI_SMD_ST_EXEC_REQ_ST_CONTROL_SIZE_B;
}

SmdBssTransitionParameters::StExecReqStInfo::StExecReqStInfo()
    : m_stControl(),
      m_dlTidBitmap(m_stControl.m_dlTidBmPresent),
      m_dlBaInfo(m_stControl.m_dlBaInfoPresent)
{
}

SmdBssTransitionParameters::StExecReqStInfo::StExecReqStInfo(const StExecReqStInfo& other)
    : m_stControl(other.m_stControl),
      m_dlTidBitmap(m_stControl.m_dlTidBmPresent),
      m_dlBaInfo(m_stControl.m_dlBaInfoPresent)
{
    m_dlTidBitmap = other.m_dlTidBitmap.ToOptional();
    m_dlBaInfo = other.m_dlBaInfo.ToOptional();
}

SmdBssTransitionParameters::StExecReqStInfo&
SmdBssTransitionParameters::StExecReqStInfo::operator=(const StExecReqStInfo& other)
{
    if (this != &other)
    {
        m_stControl = other.m_stControl;
        m_dlTidBitmap = other.m_dlTidBitmap.ToOptional();
        m_dlBaInfo = other.m_dlBaInfo.ToOptional();
    }
    return *this;
}

void
SmdBssTransitionParameters::StExecReqStInfo::Serialize(Buffer::Iterator& start) const
{
    m_stControl.Serialize(start);
    if (m_dlTidBitmap)
    {
        start.WriteU8(*m_dlTidBitmap);
    }
    if (m_dlBaInfo)
    {
        m_dlBaInfo->Serialize(start);
    }
}

uint16_t
SmdBssTransitionParameters::StExecReqStInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    i.Next(m_stControl.Deserialize(i));
    if (m_stControl.m_dlTidBmPresent)
    {
        m_dlTidBitmap = i.ReadU8();
    }
    if (m_stControl.m_dlBaInfoPresent)
    {
        m_dlBaInfo = StPrepRespStInfo::BaInfo{};
        i.Next(m_dlBaInfo->Deserialize(i));
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StExecReqStInfo::GetSerializedSize() const
{
    return m_stControl.GetSerializedSize() + (m_dlTidBitmap ? 1 : 0) +
           (m_dlBaInfo ? m_dlBaInfo->GetSerializedSize() : 0);
}

void
SmdBssTransitionParameters::StExecReqStInfo::Print(std::ostream& os) const
{
    bool first = true;
    if (m_dlTidBitmap)
    {
        os << "DL TID Bitmap=0x" << std::hex << +*m_dlTidBitmap << std::dec;
        first = false;
    }
    if (m_dlBaInfo)
    {
        os << (first ? "" : ", ") << "DL BA Info=[";
        m_dlBaInfo->Print(os);
        os << "]";
    }
}

SmdBssTransitionParameters::StExecRespStInfo::StExecRespStInfo()
    : m_stControl(),
      m_nomMaxDlDrainPeriodDur(m_stControl.m_nomMaxDlDrainPeriodDurPresent),
      m_latestUlSnList(m_stControl.m_latestUlSnListPresent)
{
}

SmdBssTransitionParameters::StExecRespStInfo::StExecRespStInfo(const StExecRespStInfo& other)
    : m_stControl(other.m_stControl),
      m_nomMaxDlDrainPeriodDur(m_stControl.m_nomMaxDlDrainPeriodDurPresent),
      m_latestUlSnList(m_stControl.m_latestUlSnListPresent)
{
    m_nomMaxDlDrainPeriodDur = other.m_nomMaxDlDrainPeriodDur.ToOptional();
    m_latestUlSnList = other.m_latestUlSnList.ToOptional();
}

SmdBssTransitionParameters::StExecRespStInfo&
SmdBssTransitionParameters::StExecRespStInfo::operator=(const StExecRespStInfo& other)
{
    if (this != &other)
    {
        m_stControl = other.m_stControl;
        m_nomMaxDlDrainPeriodDur = other.m_nomMaxDlDrainPeriodDur.ToOptional();
        m_latestUlSnList = other.m_latestUlSnList.ToOptional();
    }
    return *this;
}

void
SmdBssTransitionParameters::StExecRespStInfo::StControl::Serialize(Buffer::Iterator& start) const
{
    const uint8_t val =
        (m_nomMaxDlDrainPeriodDurPresent ? 0x01 : 0x00) | (m_latestUlSnListPresent ? 0x02 : 0x00);
    start.WriteU8(val);
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::StControl::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto val = i.ReadU8();
    m_nomMaxDlDrainPeriodDurPresent = val & 0x01;
    m_latestUlSnListPresent = (val >> 1) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::StControl::GetSerializedSize() const
{
    return WIFI_SMD_ST_EXEC_RESP_ST_CONTROL_SIZE_B;
}

void
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::Set(tid_t tid, uint16_t sn)
{
    NS_ABORT_MSG_IF(tid >= 8, "TID must be in the range 0-7");
    m_tidBitmap |= (1 << tid);
    m_latestUlSnPerTid[tid] = sn & 0x0FFF;
}

std::optional<uint16_t>
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::Get(tid_t tid) const
{
    NS_ABORT_MSG_IF(tid >= 8, "TID must be in the range 0-7");
    const auto it = m_latestUlSnPerTid.find(tid);
    if (it != m_latestUlSnPerTid.cend())
    {
        NS_ABORT_MSG_IF(!(m_tidBitmap & (1 << tid)),
                        "TID bitmap does not indicate presence of TID " + std::to_string(tid));
        return it->second;
    }
    return std::nullopt;
}

uint8_t
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::GetTidBitmap() const
{
    return m_tidBitmap;
}

const std::map<tid_t, uint16_t>&
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::GetLatestUlSnPerTid() const
{
    return m_latestUlSnPerTid;
}

void
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::Serialize(
    Buffer::Iterator& start) const
{
    start.WriteU8(m_tidBitmap);
    uint8_t currentOffset{0};
    uint16_t currentValue{0};
    for (const auto& [tid, sn] : m_latestUlSnPerTid)
    {
        currentValue |= (sn & 0x0FFF) << currentOffset;
        currentOffset += 12;
        while (currentOffset >= 8)
        {
            start.WriteU8(currentValue & 0xFF);
            currentValue >>= 8;
            currentOffset -= 8;
        }
    }
    if (currentOffset > 0)
    {
        start.WriteU8(currentValue & 0xFF);
    }
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    m_tidBitmap = i.ReadU8();
    m_latestUlSnPerTid.clear();
    uint8_t currentOffset{0};
    uint16_t currentValue{0};
    for (tid_t tid = 0; tid < 8; ++tid)
    {
        if (m_tidBitmap & (1 << tid))
        {
            while (currentOffset < 12)
            {
                currentValue |= static_cast<uint16_t>(i.ReadU8()) << currentOffset;
                currentOffset += 8;
            }
            m_latestUlSnPerTid[tid] = currentValue & 0x0FFF;
            currentValue >>= 12;
            currentOffset -= 12;
        }
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::GetSerializedSize() const
{
    return 1 + static_cast<uint16_t>(std::ceil((m_latestUlSnPerTid.size() * 12.0) / 8.0));
}

void
SmdBssTransitionParameters::StExecRespStInfo::LatestUlSnList::Print(std::ostream& os) const
{
    os << "TID Bitmap=0x" << std::hex << +m_tidBitmap << std::dec << ", Latest UL SN=[";
    bool first = true;
    for (const auto& [tid, sn] : m_latestUlSnPerTid)
    {
        os << (first ? "" : ", ") << "TID " << +tid << ": " << sn;
        first = false;
    }
    os << "]";
}

void
SmdBssTransitionParameters::StExecRespStInfo::Serialize(Buffer::Iterator& start) const
{
    m_stControl.Serialize(start);
    if (m_nomMaxDlDrainPeriodDur)
    {
        start.WriteU16(*m_nomMaxDlDrainPeriodDur);
    }
    if (m_latestUlSnList)
    {
        m_latestUlSnList->Serialize(start);
    }
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    i.Next(m_stControl.Deserialize(i));
    if (m_stControl.m_nomMaxDlDrainPeriodDurPresent)
    {
        m_nomMaxDlDrainPeriodDur = i.ReadU16();
    }
    if (m_stControl.m_latestUlSnListPresent)
    {
        m_latestUlSnList = LatestUlSnList{};
        i.Next(m_latestUlSnList->Deserialize(i));
    }
    return i.GetDistanceFrom(start);
}

uint16_t
SmdBssTransitionParameters::StExecRespStInfo::GetSerializedSize() const
{
    return m_stControl.GetSerializedSize() + (m_nomMaxDlDrainPeriodDur ? 2 : 0) +
           (m_latestUlSnList ? m_latestUlSnList->GetSerializedSize() : 0);
}

void
SmdBssTransitionParameters::StExecRespStInfo::Print(std::ostream& os) const
{
    bool first = true;
    if (m_nomMaxDlDrainPeriodDur)
    {
        os << "Nominal Maximum DL Draining Period Duration=" << *m_nomMaxDlDrainPeriodDur;
        first = false;
    }
    if (m_latestUlSnList)
    {
        os << (first ? "" : ", ") << "Latest UL SN List=[";
        m_latestUlSnList->Print(os);
        os << "]";
    }
}

WifiInformationElementId
SmdBssTransitionParameters::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
SmdBssTransitionParameters::ElementIdExt() const
{
    return IE_EXT_SMD_BSS_TRANSITION_PARAMETERS;
}

uint16_t
SmdBssTransitionParameters::GetInformationFieldSize() const
{
    auto getStInfoSize = [&](auto&& stInfo) -> uint16_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(stInfo)>, std::monostate>)
        {
            NS_ABORT_MSG("ST Info variant not set");
            return 0;
        }
        else
        {
            return stInfo.GetSerializedSize();
        }
    };
    return WIFI_IE_ELEMENT_ID_EXT_SIZE + std::visit(getStInfoSize, m_stInfo);
}

void
SmdBssTransitionParameters::SerializeInformationField(Buffer::Iterator start) const
{
    auto serializeStInfo = [&](auto&& stInfo) {
        if constexpr (std::is_same_v<std::decay_t<decltype(stInfo)>, std::monostate>)
        {
            NS_ABORT_MSG("ST Info variant not set");
        }
        else
        {
            stInfo.Serialize(start);
        }
    };
    std::visit(serializeStInfo, m_stInfo);
}

uint16_t
SmdBssTransitionParameters::DeserializeInformationField(Buffer::Iterator start,
                                                        uint16_t /* length */)
{
    auto deserializeStInfo = [&](auto&& stInfo) -> uint16_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(stInfo)>, std::monostate>)
        {
            NS_ABORT_MSG("ST Info variant not set");
            return 0;
        }
        else
        {
            return stInfo.Deserialize(start);
        }
    };
    return std::visit(deserializeStInfo, m_stInfo);
}

} // namespace ns3
