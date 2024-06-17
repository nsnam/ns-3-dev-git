/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#include "data-output-interface.h"

#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DataOutputInterface");

//--------------------------------------------------------------
//----------------------------------------------
DataOutputInterface::DataOutputInterface()
{
    NS_LOG_FUNCTION(this);
}

DataOutputInterface::~DataOutputInterface()
{
    NS_LOG_FUNCTION(this);
}

/* static */
TypeId
DataOutputInterface::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DataOutputInterface").SetParent<Object>().SetGroupName("Stats")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

void
DataOutputInterface::DoDispose()
{
    NS_LOG_FUNCTION(this);

    Object::DoDispose();
    // end DataOutputInterface::DoDispose
}

void
DataOutputInterface::SetFilePrefix(const std::string prefix)
{
    NS_LOG_FUNCTION(this << prefix);

    m_filePrefix = prefix;
}

std::string
DataOutputInterface::GetFilePrefix() const
{
    NS_LOG_FUNCTION(this);

    return m_filePrefix;
}
