/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
