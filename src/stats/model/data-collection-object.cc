/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Tiago G. Rodrigues (tgr002@bucknell.edu)
 */

#include "data-collection-object.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DataCollectionObject");

NS_OBJECT_ENSURE_REGISTERED(DataCollectionObject);

TypeId
DataCollectionObject::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DataCollectionObject")
                            .SetParent<Object>()
                            .SetGroupName("Stats")
                            .AddConstructor<DataCollectionObject>()
                            .AddAttribute("Name",
                                          "Object's name",
                                          StringValue("unnamed"),
                                          MakeStringAccessor(&DataCollectionObject::GetName,
                                                             &DataCollectionObject::SetName),
                                          MakeStringChecker())
                            .AddAttribute("Enabled",
                                          "Object's enabled status",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&DataCollectionObject::m_enabled),
                                          MakeBooleanChecker());
    return tid;
}

DataCollectionObject::DataCollectionObject()
{
}

DataCollectionObject::~DataCollectionObject()
{
    NS_LOG_FUNCTION(this);
}

bool
DataCollectionObject::IsEnabled() const
{
    return m_enabled;
}

std::string
DataCollectionObject::GetName() const
{
    return m_name;
}

void
DataCollectionObject::SetName(std::string name)
{
    NS_LOG_FUNCTION(this << name);
    for (size_t pos = name.find(' '); pos != std::string::npos; pos = name.find(" ", pos + 1, 1))
    {
        name[pos] = '_';
    }

    m_name = name;
}

void
DataCollectionObject::Enable()
{
    NS_LOG_FUNCTION(this);
    m_enabled = true;
}

void
DataCollectionObject::Disable()
{
    NS_LOG_FUNCTION(this);
    m_enabled = false;
}

} // namespace ns3
