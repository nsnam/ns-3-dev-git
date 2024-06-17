/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "node-container.h"

#include "ns3/names.h"
#include "ns3/node-list.h"

namespace ns3
{

NodeContainer
NodeContainer::GetGlobal()
{
    NodeContainer c;
    for (auto i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        c.Add(*i);
    }
    return c;
}

NodeContainer::NodeContainer()
{
}

NodeContainer::NodeContainer(Ptr<Node> node)
{
    m_nodes.push_back(node);
}

NodeContainer::NodeContainer(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    m_nodes.push_back(node);
}

NodeContainer::NodeContainer(uint32_t n, uint32_t systemId /* = 0 */)
{
    m_nodes.reserve(n);
    Create(n, systemId);
}

NodeContainer::Iterator
NodeContainer::Begin() const
{
    return m_nodes.begin();
}

NodeContainer::Iterator
NodeContainer::End() const
{
    return m_nodes.end();
}

uint32_t
NodeContainer::GetN() const
{
    return m_nodes.size();
}

Ptr<Node>
NodeContainer::Get(uint32_t i) const
{
    return m_nodes[i];
}

void
NodeContainer::Create(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        m_nodes.push_back(CreateObject<Node>());
    }
}

void
NodeContainer::Create(uint32_t n, uint32_t systemId)
{
    for (uint32_t i = 0; i < n; i++)
    {
        m_nodes.push_back(CreateObject<Node>(systemId));
    }
}

void
NodeContainer::Add(const NodeContainer& nc)
{
    for (auto i = nc.Begin(); i != nc.End(); i++)
    {
        m_nodes.push_back(*i);
    }
}

void
NodeContainer::Add(Ptr<Node> node)
{
    m_nodes.push_back(node);
}

void
NodeContainer::Add(std::string nodeName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    m_nodes.push_back(node);
}

bool
NodeContainer::Contains(uint32_t id) const
{
    for (uint32_t i = 0; i < m_nodes.size(); i++)
    {
        if (m_nodes[i]->GetId() == id)
        {
            return true;
        }
    }
    return false;
}

} // namespace ns3
