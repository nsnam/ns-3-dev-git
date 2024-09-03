/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "capability-information.h"

namespace ns3
{

CapabilityInformation::CapabilityInformation()
    : m_capability(0)
{
}

void
CapabilityInformation::SetEss()
{
    Set(0);
    Clear(1);
}

void
CapabilityInformation::SetIbss()
{
    Clear(0);
    Set(1);
}

void
CapabilityInformation::SetShortPreamble(bool shortPreamble)
{
    if (shortPreamble)
    {
        Set(5);
    }
}

void
CapabilityInformation::SetShortSlotTime(bool shortSlotTime)
{
    if (shortSlotTime)
    {
        Set(10);
    }
}

void
CapabilityInformation::SetCfPollable()
{
    Set(2);
}

void
CapabilityInformation::SetCriticalUpdate(bool flag)
{
    if (flag)
    {
        Set(6);
    }
}

bool
CapabilityInformation::IsEss() const
{
    return Is(0);
}

bool
CapabilityInformation::IsIbss() const
{
    return Is(1);
}

bool
CapabilityInformation::IsShortPreamble() const
{
    return Is(5);
}

bool
CapabilityInformation::IsShortSlotTime() const
{
    return Is(10);
}

bool
CapabilityInformation::IsCfPollable() const
{
    return Is(2);
}

bool
CapabilityInformation::IsCriticalUpdate() const
{
    return Is(6);
}

void
CapabilityInformation::Set(uint8_t n)
{
    uint32_t mask = 1 << n;
    m_capability |= mask;
}

void
CapabilityInformation::Clear(uint8_t n)
{
    uint32_t mask = 1 << n;
    m_capability &= ~mask;
}

bool
CapabilityInformation::Is(uint8_t n) const
{
    uint32_t mask = 1 << n;
    return (m_capability & mask) == mask;
}

uint32_t
CapabilityInformation::GetSerializedSize() const
{
    return 2;
}

Buffer::Iterator
CapabilityInformation::Serialize(Buffer::Iterator start) const
{
    start.WriteHtolsbU16(m_capability);
    return start;
}

Buffer::Iterator
CapabilityInformation::Deserialize(Buffer::Iterator start)
{
    m_capability = start.ReadLsbtohU16();
    return start;
}

} // namespace ns3
