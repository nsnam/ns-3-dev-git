/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 *
 * Modified by: Mitch Watrous (watrous@u.washington.edu)
 */

#include "ipv4-packet-probe.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4PacketProbe");

NS_OBJECT_ENSURE_REGISTERED(Ipv4PacketProbe);

TypeId
Ipv4PacketProbe::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv4PacketProbe")
            .SetParent<Probe>()
            .SetGroupName("Internet")
            .AddConstructor<Ipv4PacketProbe>()
            .AddTraceSource("Output",
                            "The packet plus its IPv4 object and interface "
                            "that serve as the output for this probe",
                            MakeTraceSourceAccessor(&Ipv4PacketProbe::m_output),
                            "ns3::Ipv4L3Protocol::TxRxTracedCallback")
            .AddTraceSource("OutputBytes",
                            "The number of bytes in the packet",
                            MakeTraceSourceAccessor(&Ipv4PacketProbe::m_outputBytes),
                            "ns3::Packet::SizeTracedCallback");
    return tid;
}

Ipv4PacketProbe::Ipv4PacketProbe()
{
    NS_LOG_FUNCTION(this);
    m_packet = nullptr;
    m_packetSizeOld = 0;
    m_ipv4 = nullptr;
    m_interface = 0;
}

Ipv4PacketProbe::~Ipv4PacketProbe()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv4PacketProbe::SetValue(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    NS_LOG_FUNCTION(this << packet << ipv4 << interface);
    m_packet = packet;
    m_ipv4 = ipv4;
    m_interface = interface;
    m_output(packet, ipv4, interface);

    uint32_t packetSizeNew = packet->GetSize();
    m_outputBytes(m_packetSizeOld, packetSizeNew);
    m_packetSizeOld = packetSizeNew;
}

void
Ipv4PacketProbe::SetValueByPath(std::string path,
                                Ptr<const Packet> packet,
                                Ptr<Ipv4> ipv4,
                                uint32_t interface)
{
    NS_LOG_FUNCTION(path << packet << ipv4 << interface);
    Ptr<Ipv4PacketProbe> probe = Names::Find<Ipv4PacketProbe>(path);
    NS_ASSERT_MSG(probe, "Error:  Can't find probe for path " << path);
    probe->SetValue(packet, ipv4, interface);
}

bool
Ipv4PacketProbe::ConnectByObject(std::string traceSource, Ptr<Object> obj)
{
    NS_LOG_FUNCTION(this << traceSource << obj);
    NS_LOG_DEBUG("Name of probe (if any) in names database: " << Names::FindPath(obj));
    bool connected =
        obj->TraceConnectWithoutContext(traceSource,
                                        MakeCallback(&ns3::Ipv4PacketProbe::TraceSink, this));
    return connected;
}

void
Ipv4PacketProbe::ConnectByPath(std::string path)
{
    NS_LOG_FUNCTION(this << path);
    NS_LOG_DEBUG("Name of probe to search for in config database: " << path);
    Config::ConnectWithoutContext(path, MakeCallback(&ns3::Ipv4PacketProbe::TraceSink, this));
}

void
Ipv4PacketProbe::TraceSink(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    NS_LOG_FUNCTION(this << packet << ipv4 << interface);
    if (IsEnabled())
    {
        m_packet = packet;
        m_ipv4 = ipv4;
        m_interface = interface;
        m_output(packet, ipv4, interface);

        uint32_t packetSizeNew = packet->GetSize();
        m_outputBytes(m_packetSizeOld, packetSizeNew);
        m_packetSizeOld = packetSizeNew;
    }
}

} // namespace ns3
