/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#include "dl-mac-messages.h"

#include "ns3/address-utils.h"

#include <stdint.h>

namespace ns3
{

DcdChannelEncodings::DcdChannelEncodings()
    : m_bsEirp(0),
      m_eirXPIrMax(0),
      m_frequency(0)
{
}

DcdChannelEncodings::~DcdChannelEncodings()
{
}

void
DcdChannelEncodings::SetBsEirp(uint16_t bs_eirp)
{
    m_bsEirp = bs_eirp;
}

void
DcdChannelEncodings::SetEirxPIrMax(uint16_t eir_x_p_ir_max)
{
    m_eirXPIrMax = eir_x_p_ir_max;
}

void
DcdChannelEncodings::SetFrequency(uint32_t frequency)
{
    m_frequency = frequency;
}

uint16_t
DcdChannelEncodings::GetBsEirp() const
{
    return m_bsEirp;
}

uint16_t
DcdChannelEncodings::GetEirxPIrMax() const
{
    return m_eirXPIrMax;
}

uint32_t
DcdChannelEncodings::GetFrequency() const
{
    return m_frequency;
}

uint16_t
DcdChannelEncodings::GetSize() const
{
    return 2 + 2 + 4;
}

Buffer::Iterator
DcdChannelEncodings::Write(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU16(m_bsEirp);
    i.WriteU16(m_eirXPIrMax);
    i.WriteU32(m_frequency);
    return DoWrite(i);
}

Buffer::Iterator
DcdChannelEncodings::Read(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_bsEirp = i.ReadU16();
    m_eirXPIrMax = i.ReadU16();
    m_frequency = i.ReadU32();
    return DoRead(i);
}

// ----------------------------------------------------------------------------------------------------------

OfdmDcdChannelEncodings::OfdmDcdChannelEncodings()
    : m_channelNr(0),
      m_ttg(0),
      m_rtg(0),
      m_baseStationId(Mac48Address("00:00:00:00:00:00")),
      m_frameDurationCode(0),
      m_frameNumber(0)
{
}

OfdmDcdChannelEncodings::~OfdmDcdChannelEncodings()
{
}

void
OfdmDcdChannelEncodings::SetChannelNr(uint8_t channelNr)
{
    m_channelNr = channelNr;
}

void
OfdmDcdChannelEncodings::SetTtg(uint8_t ttg)
{
    m_ttg = ttg;
}

void
OfdmDcdChannelEncodings::SetRtg(uint8_t rtg)
{
    m_rtg = rtg;
}

void
OfdmDcdChannelEncodings::SetBaseStationId(Mac48Address baseStationId)
{
    m_baseStationId = baseStationId;
}

void
OfdmDcdChannelEncodings::SetFrameDurationCode(uint8_t frameDurationCode)
{
    m_frameDurationCode = frameDurationCode;
}

void
OfdmDcdChannelEncodings::SetFrameNumber(uint32_t frameNumber)
{
    m_frameNumber = frameNumber;
}

uint8_t
OfdmDcdChannelEncodings::GetChannelNr() const
{
    return m_channelNr;
}

uint8_t
OfdmDcdChannelEncodings::GetTtg() const
{
    return m_ttg;
}

uint8_t
OfdmDcdChannelEncodings::GetRtg() const
{
    return m_rtg;
}

Mac48Address
OfdmDcdChannelEncodings::GetBaseStationId() const
{
    return m_baseStationId;
}

uint8_t
OfdmDcdChannelEncodings::GetFrameDurationCode() const
{
    return m_frameDurationCode;
}

uint32_t
OfdmDcdChannelEncodings::GetFrameNumber() const
{
    return m_frameNumber;
}

uint16_t
OfdmDcdChannelEncodings::GetSize() const
{
    return DcdChannelEncodings::GetSize() + 1 + 1 + 1 + 6 + 1 + 4;
}

Buffer::Iterator
OfdmDcdChannelEncodings::DoWrite(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_channelNr);
    i.WriteU8(m_ttg);
    i.WriteU8(m_rtg);
    WriteTo(i, m_baseStationId);
    i.WriteU8(m_frameDurationCode);
    i.WriteU32(m_frameNumber);
    return i;
}

Buffer::Iterator
OfdmDcdChannelEncodings::DoRead(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_channelNr = i.ReadU8();
    m_ttg = i.ReadU8();
    m_rtg = i.ReadU8();
    ReadFrom(i, m_baseStationId); // length (6) shall also be written in packet instead of hard
                                  // coded, see ARP example
    m_frameDurationCode = i.ReadU8();
    m_frameNumber = i.ReadU32();
    return i;
}

// ----------------------------------------------------------------------------------------------------------

OfdmDlBurstProfile::OfdmDlBurstProfile()
    : m_type(0),
      m_length(0),
      m_diuc(0),
      m_fecCodeType(0)
{
}

OfdmDlBurstProfile::~OfdmDlBurstProfile()
{
}

void
OfdmDlBurstProfile::SetType(uint8_t type)
{
    m_type = type;
}

void
OfdmDlBurstProfile::SetLength(uint8_t length)
{
    m_length = length;
}

void
OfdmDlBurstProfile::SetDiuc(uint8_t diuc)
{
    m_diuc = diuc;
}

void
OfdmDlBurstProfile::SetFecCodeType(uint8_t fecCodeType)
{
    m_fecCodeType = fecCodeType;
}

uint8_t
OfdmDlBurstProfile::GetType() const
{
    return m_type;
}

uint8_t
OfdmDlBurstProfile::GetLength() const
{
    return m_length;
}

uint8_t
OfdmDlBurstProfile::GetDiuc() const
{
    return m_diuc;
}

uint8_t
OfdmDlBurstProfile::GetFecCodeType() const
{
    return m_fecCodeType;
}

uint16_t
OfdmDlBurstProfile::GetSize() const
{
    return 1 + 1 + 1 + 1;
}

Buffer::Iterator
OfdmDlBurstProfile::Write(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_type);
    i.WriteU8(m_length);
    i.WriteU8(m_diuc);
    i.WriteU8(m_fecCodeType);
    return i;
}

Buffer::Iterator
OfdmDlBurstProfile::Read(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_type = i.ReadU8();
    m_length = i.ReadU8();
    m_diuc = i.ReadU8();
    m_fecCodeType = i.ReadU8();
    return i;
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(Dcd);

Dcd::Dcd()
    : m_reserved(0),
      m_configurationChangeCount(0),
      m_nrDlBurstProfiles(0)
{
}

Dcd::~Dcd()
{
}

void
Dcd::SetConfigurationChangeCount(uint8_t configurationChangeCount)
{
    m_configurationChangeCount = configurationChangeCount;
}

void
Dcd::SetChannelEncodings(OfdmDcdChannelEncodings channelEncodings)
{
    m_channelEncodings = channelEncodings;
}

void
Dcd::SetNrDlBurstProfiles(uint8_t nrDlBurstProfiles)
{
    m_nrDlBurstProfiles = nrDlBurstProfiles;
}

void
Dcd::AddDlBurstProfile(OfdmDlBurstProfile dlBurstProfile)
{
    m_dlBurstProfiles.push_back(dlBurstProfile);
}

uint8_t
Dcd::GetConfigurationChangeCount() const
{
    return m_configurationChangeCount;
}

OfdmDcdChannelEncodings
Dcd::GetChannelEncodings() const
{
    return m_channelEncodings;
}

std::vector<OfdmDlBurstProfile>
Dcd::GetDlBurstProfiles() const
{
    return m_dlBurstProfiles;
}

uint8_t
Dcd::GetNrDlBurstProfiles() const
{
    return m_nrDlBurstProfiles;
}

std::string
Dcd::GetName() const
{
    return "DCD";
}

TypeId
Dcd::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Dcd").SetParent<Header>().SetGroupName("Wimax").AddConstructor<Dcd>();
    return tid;
}

TypeId
Dcd::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
Dcd::Print(std::ostream& os) const
{
    os << " configuration change count = " << (uint32_t)m_configurationChangeCount
       << ", number of dl burst profiles = " << m_dlBurstProfiles.size();
}

uint32_t
Dcd::GetSerializedSize() const
{
    uint32_t dlBurstProfilesSize = 0;

    for (const auto& burstProfile : m_dlBurstProfiles)
    {
        dlBurstProfilesSize += burstProfile.GetSize();
    }

    return 1 + 1 + m_channelEncodings.GetSize() + dlBurstProfilesSize;
}

void
Dcd::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_reserved);
    i.WriteU8(m_configurationChangeCount);
    i = m_channelEncodings.Write(i);

    for (const auto& burstProfile : m_dlBurstProfiles)
    {
        i = burstProfile.Write(i);
    }
}

uint32_t
Dcd::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    m_configurationChangeCount = i.ReadU8();
    i = m_channelEncodings.Read(i);

    for (uint8_t j = 0; j < m_nrDlBurstProfiles; j++)
    {
        OfdmDlBurstProfile burstProfile;
        i = burstProfile.Read(i);
        AddDlBurstProfile(burstProfile);
    }

    return i.GetDistanceFrom(start);
}

// ----------------------------------------------------------------------------------------------------------

OfdmDlMapIe::OfdmDlMapIe()
    : m_cid(),
      m_diuc(0),
      m_preamblePresent(0),
      m_startTime(0)
{
}

OfdmDlMapIe::~OfdmDlMapIe()
{
}

void
OfdmDlMapIe::SetCid(Cid cid)
{
    m_cid = cid;
}

void
OfdmDlMapIe::SetDiuc(uint8_t diuc)
{
    m_diuc = diuc;
}

void
OfdmDlMapIe::SetPreamblePresent(uint8_t preamblePresent)
{
    m_preamblePresent = preamblePresent;
}

void
OfdmDlMapIe::SetStartTime(uint16_t startTime)
{
    m_startTime = startTime;
}

Cid
OfdmDlMapIe::GetCid() const
{
    return m_cid;
}

uint8_t
OfdmDlMapIe::GetDiuc() const
{
    return m_diuc;
}

uint8_t
OfdmDlMapIe::GetPreamblePresent() const
{
    return m_preamblePresent;
}

uint16_t
OfdmDlMapIe::GetStartTime() const
{
    return m_startTime;
}

uint16_t
OfdmDlMapIe::GetSize() const
{
    return 2 + 1 + 1 + 2;
}

Buffer::Iterator
OfdmDlMapIe::Write(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU16(m_cid.GetIdentifier());
    i.WriteU8(m_diuc);
    i.WriteU8(m_preamblePresent);
    i.WriteU16(m_startTime);
    return i;
}

Buffer::Iterator
OfdmDlMapIe::Read(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_cid = i.ReadU16();
    m_diuc = i.ReadU8();
    m_preamblePresent = i.ReadU8();
    m_startTime = i.ReadU16();
    return i;
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(DlMap);

DlMap::DlMap()
    : m_dcdCount(0),
      m_baseStationId(Mac48Address("00:00:00:00:00:00"))
{
}

DlMap::~DlMap()
{
}

void
DlMap::SetDcdCount(uint8_t dcdCount)
{
    m_dcdCount = dcdCount;
}

void
DlMap::SetBaseStationId(Mac48Address baseStationId)
{
    m_baseStationId = baseStationId;
}

void
DlMap::AddDlMapElement(OfdmDlMapIe dlMapElement)
{
    m_dlMapElements.push_back(dlMapElement);
}

uint8_t
DlMap::GetDcdCount() const
{
    return m_dcdCount;
}

Mac48Address
DlMap::GetBaseStationId() const
{
    return m_baseStationId;
}

std::list<OfdmDlMapIe>
DlMap::GetDlMapElements() const
{
    return m_dlMapElements;
}

std::string
DlMap::GetName() const
{
    return "DL-MAP";
}

TypeId
DlMap::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DlMap").SetParent<Header>().SetGroupName("Wimax").AddConstructor<DlMap>();
    return tid;
}

TypeId
DlMap::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
DlMap::Print(std::ostream& os) const
{
    os << " dcd count = " << (uint32_t)m_dcdCount << ", base station id = " << m_baseStationId
       << ", number of dl-map elements = " << m_dlMapElements.size();
}

uint32_t
DlMap::GetSerializedSize() const
{
    uint32_t dlMapElementsSize = 0;

    for (const auto& dlMapIe : m_dlMapElements)
    {
        dlMapElementsSize += dlMapIe.GetSize();
    }

    return 1 + 6 + dlMapElementsSize;
}

void
DlMap::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dcdCount);
    WriteTo(i, m_baseStationId);

    for (const auto& dlMapIe : m_dlMapElements)
    {
        i = dlMapIe.Write(i);
    }
}

uint32_t
DlMap::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dcdCount = i.ReadU8();
    ReadFrom(i, m_baseStationId); // length (6) shall also be written in packet instead of hard
                                  // coded, see ARP example

    m_dlMapElements.clear(); // only for printing, otherwise it shows wrong number of elements

    while (true)
    {
        OfdmDlMapIe dlMapIe;
        i = dlMapIe.Read(i);

        AddDlMapElement(dlMapIe);

        if (dlMapIe.GetDiuc() == 14) // End of Map IE
        {
            break;
        }
    }
    return i.GetDistanceFrom(start);
}

} // namespace ns3
