/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "pcap-file-wrapper.h"

#include "ns3/boolean.h"
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PcapFileWrapper");

NS_OBJECT_ENSURE_REGISTERED(PcapFileWrapper);

TypeId
PcapFileWrapper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PcapFileWrapper")
            .SetParent<Object>()
            .SetGroupName("Network")
            .AddConstructor<PcapFileWrapper>()
            .AddAttribute("CaptureSize",
                          "Maximum length of captured packets (cf. pcap snaplen)",
                          UintegerValue(PcapFile::SNAPLEN_DEFAULT),
                          MakeUintegerAccessor(&PcapFileWrapper::m_snapLen),
                          MakeUintegerChecker<uint32_t>(0, PcapFile::SNAPLEN_DEFAULT))
            .AddAttribute("NanosecMode",
                          "Whether packet timestamps in the PCAP file are nanoseconds or "
                          "microseconds(default).",
                          BooleanValue(false),
                          MakeBooleanAccessor(&PcapFileWrapper::m_nanosecMode),
                          MakeBooleanChecker());
    return tid;
}

PcapFileWrapper::PcapFileWrapper()
{
    NS_LOG_FUNCTION(this);
}

PcapFileWrapper::~PcapFileWrapper()
{
    NS_LOG_FUNCTION(this);
    Close();
}

bool
PcapFileWrapper::Fail() const
{
    NS_LOG_FUNCTION(this);
    return m_file.Fail();
}

bool
PcapFileWrapper::Eof() const
{
    NS_LOG_FUNCTION(this);
    return m_file.Eof();
}

void
PcapFileWrapper::Clear()
{
    NS_LOG_FUNCTION(this);
    m_file.Clear();
}

void
PcapFileWrapper::Close()
{
    NS_LOG_FUNCTION(this);
    m_file.Close();
}

void
PcapFileWrapper::Open(const std::string& filename, std::ios::openmode mode)
{
    NS_LOG_FUNCTION(this << filename << mode);
    m_file.Open(filename, mode);
}

void
PcapFileWrapper::Init(uint32_t dataLinkType, uint32_t snapLen, int32_t tzCorrection)
{
    //
    // If the user doesn't provide a snaplen, the default value will come in.  If
    // this happens, we use the "CaptureSize" Attribute.  If the user does provide
    // a snaplen, we use the one provided.
    //
    NS_LOG_FUNCTION(this << dataLinkType << snapLen << tzCorrection);
    if (snapLen != std::numeric_limits<uint32_t>::max())
    {
        m_file.Init(dataLinkType, snapLen, tzCorrection, false, m_nanosecMode);
    }
    else
    {
        m_file.Init(dataLinkType, m_snapLen, tzCorrection, false, m_nanosecMode);
    }
}

void
PcapFileWrapper::Write(Time t, Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this << t << p);
    if (m_file.IsNanoSecMode())
    {
        uint64_t current = t.GetNanoSeconds();
        uint64_t s = current / 1000000000;
        uint64_t ns = current % 1000000000;
        m_file.Write(s, ns, p);
    }
    else
    {
        uint64_t current = t.GetMicroSeconds();
        uint64_t s = current / 1000000;
        uint64_t us = current % 1000000;
        m_file.Write(s, us, p);
    }
}

void
PcapFileWrapper::Write(Time t, const Header& header, Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this << t << &header << p);
    if (m_file.IsNanoSecMode())
    {
        uint64_t current = t.GetNanoSeconds();
        uint64_t s = current / 1000000000;
        uint64_t ns = current % 1000000000;
        m_file.Write(s, ns, header, p);
    }
    else
    {
        uint64_t current = t.GetMicroSeconds();
        uint64_t s = current / 1000000;
        uint64_t us = current % 1000000;
        m_file.Write(s, us, header, p);
    }
}

void
PcapFileWrapper::Write(Time t, const uint8_t* buffer, uint32_t length)
{
    NS_LOG_FUNCTION(this << t << &buffer << length);
    if (m_file.IsNanoSecMode())
    {
        uint64_t current = t.GetNanoSeconds();
        uint64_t s = current / 1000000000;
        uint64_t ns = current % 1000000000;
        m_file.Write(s, ns, buffer, length);
    }
    else
    {
        uint64_t current = t.GetMicroSeconds();
        uint64_t s = current / 1000000;
        uint64_t us = current % 1000000;
        m_file.Write(s, us, buffer, length);
    }
}

Ptr<Packet>
PcapFileWrapper::Read(Time& t)
{
    uint32_t tsSec;
    uint32_t tsUsec;
    uint32_t inclLen;
    uint32_t origLen;
    uint32_t readLen;

    uint8_t datbuf[65536];

    m_file.Read(datbuf, 65536, tsSec, tsUsec, inclLen, origLen, readLen);

    if (m_file.Fail())
    {
        return nullptr;
    }

    if (m_file.IsNanoSecMode())
    {
        t = NanoSeconds(tsSec * 1000000000ULL + tsUsec);
    }
    else
    {
        t = MicroSeconds(tsSec * 1000000ULL + tsUsec);
    }

    return Create<Packet>(datbuf, origLen);
}

uint32_t
PcapFileWrapper::GetMagic()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetMagic();
}

uint16_t
PcapFileWrapper::GetVersionMajor()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetVersionMajor();
}

uint16_t
PcapFileWrapper::GetVersionMinor()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetVersionMinor();
}

int32_t
PcapFileWrapper::GetTimeZoneOffset()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetTimeZoneOffset();
}

uint32_t
PcapFileWrapper::GetSigFigs()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetSigFigs();
}

uint32_t
PcapFileWrapper::GetSnapLen()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetSnapLen();
}

uint32_t
PcapFileWrapper::GetDataLinkType()
{
    NS_LOG_FUNCTION(this);
    return m_file.GetDataLinkType();
}

} // namespace ns3
