/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "ofdm-downlink-frame-prefix.h"

#include "ns3/address-utils.h"

#include <cstdint>

namespace ns3
{

DlFramePrefixIe::DlFramePrefixIe()
    : m_rateId(0),
      m_diuc(0),
      m_preamblePresent(0),
      m_length(0),
      m_startTime(0)
{
}

DlFramePrefixIe::~DlFramePrefixIe()
{
}

void
DlFramePrefixIe::SetRateId(uint8_t rateId)
{
    m_rateId = rateId;
}

void
DlFramePrefixIe::SetDiuc(uint8_t diuc)
{
    m_diuc = diuc;
}

void
DlFramePrefixIe::SetPreamblePresent(uint8_t preamblePresent)
{
    m_preamblePresent = preamblePresent;
}

void
DlFramePrefixIe::SetLength(uint16_t length)
{
    m_length = length;
}

void
DlFramePrefixIe::SetStartTime(uint16_t startTime)
{
    m_startTime = startTime;
}

uint8_t
DlFramePrefixIe::GetRateId() const
{
    return m_rateId;
}

uint8_t
DlFramePrefixIe::GetDiuc() const
{
    return m_diuc;
}

uint8_t
DlFramePrefixIe::GetPreamblePresent() const
{
    return m_preamblePresent;
}

uint16_t
DlFramePrefixIe::GetLength() const
{
    return m_length;
}

uint16_t
DlFramePrefixIe::GetStartTime() const
{
    return m_startTime;
}

uint16_t
DlFramePrefixIe::GetSize() const
{
    return 1 + 1 + 1 + 2 + 2;
}

Buffer::Iterator
DlFramePrefixIe::Write(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_rateId);
    i.WriteU8(m_diuc);
    i.WriteU8(m_preamblePresent);
    i.WriteU16(m_length);
    i.WriteU16(m_startTime);
    return i;
}

Buffer::Iterator
DlFramePrefixIe::Read(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_rateId = i.ReadU8();
    m_diuc = i.ReadU8();
    m_preamblePresent = i.ReadU8();
    m_length = i.ReadU16();
    m_startTime = i.ReadU16();
    return i;
}

OfdmDownlinkFramePrefix::OfdmDownlinkFramePrefix()
    : m_baseStationId(Mac48Address("00:00:00:00:00:00")),
      m_frameNumber(0),
      m_configurationChangeCount(0),
      m_hcs(0)
{
}

OfdmDownlinkFramePrefix::~OfdmDownlinkFramePrefix()
{
}

/* static */
TypeId
OfdmDownlinkFramePrefix::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OfdmDownlinkFramePrefix").SetParent<Header>().SetGroupName("Wimax")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

void
OfdmDownlinkFramePrefix::SetBaseStationId(Mac48Address baseStationId)
{
    m_baseStationId = baseStationId;
}

void
OfdmDownlinkFramePrefix::SetFrameNumber(uint32_t frameNumber)
{
    m_frameNumber = frameNumber;
}

void
OfdmDownlinkFramePrefix::SetConfigurationChangeCount(uint8_t configurationChangeCount)
{
    m_configurationChangeCount = configurationChangeCount;
}

void
OfdmDownlinkFramePrefix::AddDlFramePrefixElement(DlFramePrefixIe dlFramePrefixElement)
{
    m_dlFramePrefixElements.push_back(dlFramePrefixElement);
}

void
OfdmDownlinkFramePrefix::SetHcs(uint8_t hcs)
{
    m_hcs = hcs;
}

Mac48Address
OfdmDownlinkFramePrefix::GetBaseStationId() const
{
    return m_baseStationId;
}

uint32_t
OfdmDownlinkFramePrefix::GetFrameNumber() const
{
    return m_frameNumber;
}

uint8_t
OfdmDownlinkFramePrefix::GetConfigurationChangeCount() const
{
    return m_configurationChangeCount;
}

std::vector<DlFramePrefixIe>
OfdmDownlinkFramePrefix::GetDlFramePrefixElements() const
{
    return m_dlFramePrefixElements;
}

uint8_t
OfdmDownlinkFramePrefix::GetHcs() const
{
    return m_hcs;
}

std::string
OfdmDownlinkFramePrefix::GetName() const
{
    return "OFDM Downlink Frame Prefix";
}

void
OfdmDownlinkFramePrefix::Print(std::ostream& os) const
{
    os << " base station id = " << m_baseStationId << ", frame number = " << m_frameNumber
       << ", configuration change count = " << (uint32_t)m_configurationChangeCount
       << ", number of dl frame prefix elements = " << m_dlFramePrefixElements.size()
       << ", hcs = " << (uint32_t)m_hcs;
}

uint32_t
OfdmDownlinkFramePrefix::GetSerializedSize() const
{
    int dlFramePrefixElementsSize = 0;

    for (const auto& dlFramePrefixElement : m_dlFramePrefixElements)
    {
        dlFramePrefixElementsSize += dlFramePrefixElement.GetSize();
    }

    return 6 + 4 + 1 + dlFramePrefixElementsSize + 1;
}

void
OfdmDownlinkFramePrefix::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    WriteTo(i, m_baseStationId);
    i.WriteU32(m_frameNumber);
    i.WriteU8(m_configurationChangeCount);

    for (const auto& dlFramePrefixElement : m_dlFramePrefixElements)
    {
        i = dlFramePrefixElement.Write(i);
    }

    i.WriteU8(m_hcs);
}

uint32_t
OfdmDownlinkFramePrefix::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    ReadFrom(i, m_baseStationId);
    m_frameNumber = i.ReadU32();
    m_configurationChangeCount = i.ReadU8();

    bool end = false;

    while (!end)
    {
        DlFramePrefixIe dlFramePrefixElement;
        i = dlFramePrefixElement.Read(i);

        AddDlFramePrefixElement(dlFramePrefixElement);

        if (dlFramePrefixElement.GetDiuc() == 14)
        {
            end = true;
        }
    }

    m_hcs = i.ReadU8();

    return GetSerializedSize();
}

} // namespace ns3
