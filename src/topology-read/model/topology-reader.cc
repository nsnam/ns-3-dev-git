/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#include "topology-reader.h"

#include "ns3/log.h"

/**
 * @file
 * @ingroup topology
 * ns3::TopologyReader implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TopologyReader");

NS_OBJECT_ENSURE_REGISTERED(TopologyReader);

TypeId
TopologyReader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TopologyReader").SetParent<Object>().SetGroupName("TopologyReader");
    return tid;
}

TopologyReader::TopologyReader()
{
    NS_LOG_FUNCTION(this);
}

TopologyReader::~TopologyReader()
{
    NS_LOG_FUNCTION(this);
}

void
TopologyReader::SetFileName(const std::string& fileName)
{
    m_fileName = fileName;
}

std::string
TopologyReader::GetFileName() const
{
    return m_fileName;
}

/* Manipulating the address block */

TopologyReader::ConstLinksIterator
TopologyReader::LinksBegin() const
{
    return m_linksList.begin();
}

TopologyReader::ConstLinksIterator
TopologyReader::LinksEnd() const
{
    return m_linksList.end();
}

int
TopologyReader::LinksSize() const
{
    return m_linksList.size();
}

bool
TopologyReader::LinksEmpty() const
{
    return m_linksList.empty();
}

void
TopologyReader::AddLink(Link link)
{
    m_linksList.push_back(link);
}

TopologyReader::Link::Link(Ptr<Node> fromPtr,
                           const std::string& fromName,
                           Ptr<Node> toPtr,
                           const std::string& toName)
{
    m_fromPtr = fromPtr;
    m_fromName = fromName;
    m_toPtr = toPtr;
    m_toName = toName;
}

TopologyReader::Link::Link()
{
}

Ptr<Node>
TopologyReader::Link::GetFromNode() const
{
    return m_fromPtr;
}

std::string
TopologyReader::Link::GetFromNodeName() const
{
    return m_fromName;
}

Ptr<Node>
TopologyReader::Link::GetToNode() const
{
    return m_toPtr;
}

std::string
TopologyReader::Link::GetToNodeName() const
{
    return m_toName;
}

std::string
TopologyReader::Link::GetAttribute(const std::string& name) const
{
    NS_ASSERT_MSG(m_linkAttr.find(name) != m_linkAttr.end(),
                  "Requested topology link attribute not found");
    return m_linkAttr.find(name)->second;
}

bool
TopologyReader::Link::GetAttributeFailSafe(const std::string& name, std::string& value) const
{
    if (m_linkAttr.find(name) == m_linkAttr.end())
    {
        return false;
    }
    value = m_linkAttr.find(name)->second;
    return true;
}

void
TopologyReader::Link::SetAttribute(const std::string& name, const std::string& value)
{
    m_linkAttr[name] = value;
}

TopologyReader::Link::ConstAttributesIterator
TopologyReader::Link::AttributesBegin() const
{
    return m_linkAttr.begin();
}

TopologyReader::Link::ConstAttributesIterator
TopologyReader::Link::AttributesEnd() const
{
    return m_linkAttr.end();
}

} /* namespace ns3 */
