//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#include "flow-monitor-helper.h"

#include "ns3/flow-monitor.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-flow-probe.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/ipv6-flow-probe.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/node-list.h"
#include "ns3/node.h"

namespace ns3
{

FlowMonitorHelper::FlowMonitorHelper()
{
    m_monitorFactory.SetTypeId("ns3::FlowMonitor");
}

FlowMonitorHelper::~FlowMonitorHelper()
{
    if (m_flowMonitor)
    {
        m_flowMonitor->Dispose();
        m_flowMonitor = nullptr;
        m_flowClassifier4 = nullptr;
        m_flowClassifier6 = nullptr;
    }
}

void
FlowMonitorHelper::SetMonitorAttribute(std::string n1, const AttributeValue& v1)
{
    m_monitorFactory.Set(n1, v1);
}

Ptr<FlowMonitor>
FlowMonitorHelper::GetMonitor()
{
    if (!m_flowMonitor)
    {
        m_flowMonitor = m_monitorFactory.Create<FlowMonitor>();
        m_flowClassifier4 = Create<Ipv4FlowClassifier>();
        m_flowMonitor->AddFlowClassifier(m_flowClassifier4);
        m_flowClassifier6 = Create<Ipv6FlowClassifier>();
        m_flowMonitor->AddFlowClassifier(m_flowClassifier6);
    }
    return m_flowMonitor;
}

Ptr<FlowClassifier>
FlowMonitorHelper::GetClassifier()
{
    if (!m_flowClassifier4)
    {
        m_flowClassifier4 = Create<Ipv4FlowClassifier>();
    }
    return m_flowClassifier4;
}

Ptr<FlowClassifier>
FlowMonitorHelper::GetClassifier6()
{
    if (!m_flowClassifier6)
    {
        m_flowClassifier6 = Create<Ipv6FlowClassifier>();
    }
    return m_flowClassifier6;
}

Ptr<FlowMonitor>
FlowMonitorHelper::Install(Ptr<Node> node)
{
    Ptr<FlowMonitor> monitor = GetMonitor();
    Ptr<FlowClassifier> classifier = GetClassifier();
    Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol>();
    if (ipv4)
    {
        auto probe =
            CreateObject<Ipv4FlowProbe>(monitor, DynamicCast<Ipv4FlowClassifier>(classifier), node);
    }
    Ptr<FlowClassifier> classifier6 = GetClassifier6();
    Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();
    if (ipv6)
    {
        auto probe6 = CreateObject<Ipv6FlowProbe>(monitor,
                                                  DynamicCast<Ipv6FlowClassifier>(classifier6),
                                                  node);
    }
    return m_flowMonitor;
}

Ptr<FlowMonitor>
FlowMonitorHelper::Install(NodeContainer nodes)
{
    for (auto i = nodes.Begin(); i != nodes.End(); ++i)
    {
        Ptr<Node> node = *i;
        if (node->GetObject<Ipv4L3Protocol>() || node->GetObject<Ipv6L3Protocol>())
        {
            Install(node);
        }
    }
    return m_flowMonitor;
}

Ptr<FlowMonitor>
FlowMonitorHelper::InstallAll()
{
    for (auto i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        if (node->GetObject<Ipv4L3Protocol>() || node->GetObject<Ipv6L3Protocol>())
        {
            Install(node);
        }
    }
    return m_flowMonitor;
}

void
FlowMonitorHelper::SerializeToXmlStream(std::ostream& os,
                                        uint16_t indent,
                                        bool enableHistograms,
                                        bool enableProbes)
{
    if (m_flowMonitor)
    {
        m_flowMonitor->SerializeToXmlStream(os, indent, enableHistograms, enableProbes);
    }
}

std::string
FlowMonitorHelper::SerializeToXmlString(uint16_t indent, bool enableHistograms, bool enableProbes)
{
    std::ostringstream os;
    if (m_flowMonitor)
    {
        m_flowMonitor->SerializeToXmlStream(os, indent, enableHistograms, enableProbes);
    }
    return os.str();
}

void
FlowMonitorHelper::SerializeToXmlFile(std::string fileName,
                                      bool enableHistograms,
                                      bool enableProbes)
{
    if (m_flowMonitor)
    {
        m_flowMonitor->SerializeToXmlFile(fileName, enableHistograms, enableProbes);
    }
}

} // namespace ns3
