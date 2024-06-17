/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "ie-dot11s-metric-report.h"

#include "ns3/assert.h"

namespace ns3
{
namespace dot11s
{
IeLinkMetricReport::IeLinkMetricReport()
    : m_metric(0)
{
}

IeLinkMetricReport::IeLinkMetricReport(uint32_t metric)
{
    m_metric = metric;
}

WifiInformationElementId
IeLinkMetricReport::ElementId() const
{
    return IE_MESH_LINK_METRIC_REPORT;
}

uint16_t
IeLinkMetricReport::GetInformationFieldSize() const
{
    return sizeof(uint32_t);
}

uint32_t
IeLinkMetricReport::GetMetric() const
{
    return m_metric;
}

void
IeLinkMetricReport::SetMetric(uint32_t metric)
{
    m_metric = metric;
}

void
IeLinkMetricReport::SerializeInformationField(Buffer::Iterator i) const
{
    i.WriteHtolsbU32(m_metric);
}

uint16_t
IeLinkMetricReport::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_metric = i.ReadLsbtohU32();
    return i.GetDistanceFrom(start);
}

void
IeLinkMetricReport::Print(std::ostream& os) const
{
    os << "Metric=" << m_metric;
}

bool
operator==(const IeLinkMetricReport& a, const IeLinkMetricReport& b)
{
    return (a.m_metric == b.m_metric);
}

bool
operator<(const IeLinkMetricReport& a, const IeLinkMetricReport& b)
{
    return (a.m_metric < b.m_metric);
}

bool
operator>(const IeLinkMetricReport& a, const IeLinkMetricReport& b)
{
    return (a.m_metric > b.m_metric);
}

std::ostream&
operator<<(std::ostream& os, const IeLinkMetricReport& a)
{
    a.Print(os);
    return os;
}
} // namespace dot11s
} // namespace ns3
