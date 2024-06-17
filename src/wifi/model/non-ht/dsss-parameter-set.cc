/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "dsss-parameter-set.h"

namespace ns3
{

DsssParameterSet::DsssParameterSet()
    : m_currentChannel(0)
{
}

WifiInformationElementId
DsssParameterSet::ElementId() const
{
    return IE_DSSS_PARAMETER_SET;
}

void
DsssParameterSet::SetCurrentChannel(uint8_t currentChannel)
{
    m_currentChannel = currentChannel;
}

uint16_t
DsssParameterSet::GetInformationFieldSize() const
{
    return 1;
}

void
DsssParameterSet::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_currentChannel);
}

uint16_t
DsssParameterSet::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_currentChannel = i.ReadU8();
    return length;
}

} // namespace ns3
