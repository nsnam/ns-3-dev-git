/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-stack-container.h"

#include "ns3/names.h"

namespace ns3
{
namespace zigbee
{

ZigbeeStackContainer::ZigbeeStackContainer()
{
}

ZigbeeStackContainer::ZigbeeStackContainer(Ptr<ZigbeeStack> stack)
{
    m_stacks.emplace_back(stack);
}

ZigbeeStackContainer::ZigbeeStackContainer(std::string stackName)
{
    Ptr<ZigbeeStack> stack = Names::Find<ZigbeeStack>(stackName);
    m_stacks.emplace_back(stack);
}

ZigbeeStackContainer::Iterator
ZigbeeStackContainer::Begin() const
{
    return m_stacks.begin();
}

ZigbeeStackContainer::Iterator
ZigbeeStackContainer::End() const
{
    return m_stacks.end();
}

uint32_t
ZigbeeStackContainer::GetN() const
{
    return m_stacks.size();
}

Ptr<ZigbeeStack>
ZigbeeStackContainer::Get(uint32_t i) const
{
    return m_stacks[i];
}

void
ZigbeeStackContainer::Add(ZigbeeStackContainer other)
{
    for (auto i = other.Begin(); i != other.End(); i++)
    {
        m_stacks.emplace_back(*i);
    }
}

void
ZigbeeStackContainer::Add(Ptr<ZigbeeStack> stack)
{
    m_stacks.emplace_back(stack);
}

void
ZigbeeStackContainer::Add(std::string stackName)
{
    Ptr<ZigbeeStack> stack = Names::Find<ZigbeeStack>(stackName);
    m_stacks.emplace_back(stack);
}

} // namespace zigbee
} // namespace ns3
