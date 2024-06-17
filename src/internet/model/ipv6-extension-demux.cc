/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#include "ipv6-extension-demux.h"

#include "ipv6-extension.h"

#include "ns3/node.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"

#include <sstream>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Ipv6ExtensionDemux);

TypeId
Ipv6ExtensionDemux::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6ExtensionDemux")
            .SetParent<Object>()
            .SetGroupName("Internet")
            .AddAttribute("Extensions",
                          "The set of IPv6 extensions registered with this demux.",
                          ObjectVectorValue(),
                          MakeObjectVectorAccessor(&Ipv6ExtensionDemux::m_extensions),
                          MakeObjectVectorChecker<Ipv6Extension>());
    return tid;
}

Ipv6ExtensionDemux::Ipv6ExtensionDemux()
{
}

Ipv6ExtensionDemux::~Ipv6ExtensionDemux()
{
}

void
Ipv6ExtensionDemux::DoDispose()
{
    for (auto it = m_extensions.begin(); it != m_extensions.end(); it++)
    {
        (*it)->Dispose();
        *it = nullptr;
    }
    m_extensions.clear();
    m_node = nullptr;
    Object::DoDispose();
}

void
Ipv6ExtensionDemux::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void
Ipv6ExtensionDemux::Insert(Ptr<Ipv6Extension> extension)
{
    m_extensions.push_back(extension);
}

Ptr<Ipv6Extension>
Ipv6ExtensionDemux::GetExtension(uint8_t extensionNumber)
{
    for (auto i = m_extensions.begin(); i != m_extensions.end(); ++i)
    {
        if ((*i)->GetExtensionNumber() == extensionNumber)
        {
            return *i;
        }
    }
    return nullptr;
}

void
Ipv6ExtensionDemux::Remove(Ptr<Ipv6Extension> extension)
{
    m_extensions.remove(extension);
}

} /* namespace ns3 */
