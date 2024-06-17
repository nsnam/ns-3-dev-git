//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Pedro Fortuna  <pedro.fortuna@inescporto.pt> <pedro.fortuna@gmail.com>
//

#include "histogram.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <cmath>

#define DEFAULT_BIN_WIDTH 1

// #define RESERVED_BINS_INC 10

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Histogram");

// uint32_t
// Histogram::GetSize () const
// {
//   return m_histogram.size ();
// }

uint32_t
Histogram::GetNBins() const
{
    return m_histogram.size();
}

double
Histogram::GetBinStart(uint32_t index) const
{
    return index * m_binWidth;
}

double
Histogram::GetBinEnd(uint32_t index) const
{
    return (index + 1) * m_binWidth;
}

double
Histogram::GetBinWidth(uint32_t index) const
{
    return m_binWidth;
}

void
Histogram::SetDefaultBinWidth(double binWidth)
{
    NS_ASSERT(m_histogram.empty()); // we can only change the bin width if no values were added
    m_binWidth = binWidth;
}

uint32_t
Histogram::GetBinCount(uint32_t index) const
{
    NS_ASSERT(index < m_histogram.size());
    return m_histogram[index];
}

void
Histogram::AddValue(double value)
{
    auto index = (uint32_t)std::floor(value / m_binWidth);

    // check if we need to resize the vector
    NS_LOG_DEBUG("AddValue: index=" << index << ", m_histogram.size()=" << m_histogram.size());

    if (index >= m_histogram.size())
    {
        m_histogram.resize(index + 1, 0);
    }
    m_histogram[index]++;
}

void
Histogram::Clear()
{
    m_histogram.clear();
}

Histogram::Histogram(double binWidth)
{
    m_binWidth = binWidth;
}

Histogram::Histogram()
{
    m_binWidth = DEFAULT_BIN_WIDTH;
}

void
Histogram::SerializeToXmlStream(std::ostream& os, uint16_t indent, std::string elementName) const
{
    os << std::string(indent, ' ') << "<" << elementName // << " binWidth=\"" << m_binWidth << "\""
       << " nBins=\"" << m_histogram.size() << "\""
       << " >\n";
    indent += 2;

#if 1 // two alternative forms of representing bin data, one more verbose than the other one
    for (uint32_t index = 0; index < m_histogram.size(); index++)
    {
        if (m_histogram[index])
        {
            os << std::string(indent, ' ');
            os << "<bin"
               << " index=\"" << (index) << "\""
               << " start=\"" << (index * m_binWidth) << "\""
               << " width=\"" << m_binWidth << "\""
               << " count=\"" << m_histogram[index] << "\""
               << " />\n";
        }
    }
#else
    os << std::string(indent + 2, ' ');
    for (uint32_t index = 0; index < m_histogram.size(); index++)
    {
        if (index > 0)
        {
            os << " ";
        }
        os << m_histogram[index];
    }
    os << "\n";
#endif
    indent -= 2;
    os << std::string(indent, ' ') << "</" << elementName << ">\n";
}

} // namespace ns3
