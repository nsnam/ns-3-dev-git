/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#include "ipv6-option-demux.h"

#include "ipv6-option.h"

#include "ns3/node.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"

#include <sstream>

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Ipv6OptionDemux);

TypeId
Ipv6OptionDemux::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv6OptionDemux")
                            .SetParent<Object>()
                            .SetGroupName("Internet")
                            .AddAttribute("Options",
                                          "The set of IPv6 options registered with this demux.",
                                          ObjectVectorValue(),
                                          MakeObjectVectorAccessor(&Ipv6OptionDemux::m_options),
                                          MakeObjectVectorChecker<Ipv6Option>());
    return tid;
}

Ipv6OptionDemux::Ipv6OptionDemux()
{
}

Ipv6OptionDemux::~Ipv6OptionDemux()
{
}

void
Ipv6OptionDemux::DoDispose()
{
    for (auto it = m_options.begin(); it != m_options.end(); it++)
    {
        (*it)->Dispose();
        *it = nullptr;
    }
    m_options.clear();
    m_node = nullptr;
    Object::DoDispose();
}

void
Ipv6OptionDemux::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void
Ipv6OptionDemux::Insert(Ptr<Ipv6Option> option)
{
    m_options.push_back(option);
}

Ptr<Ipv6Option>
Ipv6OptionDemux::GetOption(int optionNumber)
{
    for (auto i = m_options.begin(); i != m_options.end(); ++i)
    {
        if ((*i)->GetOptionNumber() == optionNumber)
        {
            return *i;
        }
    }
    return nullptr;
}

void
Ipv6OptionDemux::Remove(Ptr<Ipv6Option> option)
{
    m_options.remove(option);
}

} /* namespace ns3 */
