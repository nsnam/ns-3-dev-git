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

StatusCode::StatusCode(StatusCodeValue code)
    : m_code(code)
{
}

void
StatusCode::SetSuccess()
{
    m_code = StatusCodeValue::SUCCESS;
}

void
StatusCode::SetFailure(StatusCodeValue code)
{
    m_code = code;
}

bool
StatusCode::IsSuccess() const
{
    return (m_code == StatusCodeValue::SUCCESS);
}

uint32_t
StatusCode::GetSerializedSize() const
{
    return WIFI_STATUS_CODE_SIZE_B;
}

Buffer::Iterator
StatusCode::Serialize(Buffer::Iterator start) const
{
    start.WriteU16(static_cast<uint16_t>(m_code));
    return start;
}

Buffer::Iterator
StatusCode::Deserialize(Buffer::Iterator start)
{
    m_code = static_cast<StatusCodeValue>(start.ReadU16());
    return start;
}

std::ostream&
operator<<(std::ostream& os, const StatusCode& code)
{
    return (code.IsSuccess()) ? os << "success"
                              : os << "failure: " << static_cast<uint16_t>(code.m_code);
}

} // namespace ns3
