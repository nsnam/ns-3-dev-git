/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "status-code.h"

namespace ns3
{

StatusCode::StatusCode()
{
}

void
StatusCode::SetSuccess()
{
    m_code = 0;
}

void
StatusCode::SetFailure()
{
    m_code = 1;
}

bool
StatusCode::IsSuccess() const
{
    return (m_code == 0);
}

uint32_t
StatusCode::GetSerializedSize() const
{
    return 2;
}

Buffer::Iterator
StatusCode::Serialize(Buffer::Iterator start) const
{
    start.WriteHtolsbU16(m_code);
    return start;
}

Buffer::Iterator
StatusCode::Deserialize(Buffer::Iterator start)
{
    m_code = start.ReadLsbtohU16();
    return start;
}

std::ostream&
operator<<(std::ostream& os, const StatusCode& code)
{
    if (code.IsSuccess())
    {
        os << "success";
    }
    else
    {
        os << "failure";
    }
    return os;
}

} // namespace ns3
