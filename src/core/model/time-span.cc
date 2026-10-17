/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "time-span.h"

#include "assert.h"

/**
 * @file
 * @ingroup time
 * Declaration of class ns3::TimeSpan and definition of the TimeSpanValue attribute value.
 */

namespace ns3
{

TimeSpan::TimeSpan(const std::string& s)
{
    std::istringstream iss{s};
    iss >> *this;
}

TimeSpan::TimeSpan(const Time& begin, const Time& end)
    : m_begin(begin),
      m_end(end)
{
    NS_ASSERT_MSG(m_begin <= m_end,
                  "Begin time (" << m_begin << ") must be less than or equal to end time (" << m_end
                                 << ")");
}

TimeSpan
TimeSpan::Create(const Time& begin, const Time& end)
{
    return TimeSpan(begin, end);
}

Time
TimeSpan::Begin() const
{
    return m_begin;
}

Time
TimeSpan::End() const
{
    return m_end;
}

Time
TimeSpan::Duration() const
{
    return m_end - m_begin;
}

std::ostream&
operator<<(std::ostream& os, const TimeSpan& timespan)
{
    os << "{" << timespan.Begin() << ", " << timespan.End() << "}";
    return os;
}

std::istream&
operator>>(std::istream& is, TimeSpan& timespan)
{
    char openingBrace;
    if (!(is >> openingBrace) || openingBrace != '{')
    {
        NS_ABORT_MSG("Can't parse TimeSpan: opening brace not found");
    }

    std::string beginStr;
    if (!std::getline(is, beginStr, ',') || is.eof())
    {
        NS_ABORT_MSG("Can't parse TimeSpan: ',' separator not found");
    }

    Time begin;
    std::istringstream beginStream{beginStr};
    beginStream >> begin;

    std::string endStr;
    if (!std::getline(is, endStr, '}') || is.eof())
    {
        NS_ABORT_MSG("Can't parse TimeSpan: closing brace not found");
    }

    Time end;
    std::istringstream endStream{endStr};
    endStream >> end;

    timespan = TimeSpan(begin, end);
    return is;
}

} // namespace ns3
