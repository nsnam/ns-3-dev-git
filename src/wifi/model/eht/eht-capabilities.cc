/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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

#include "eht-capabilities.h"

namespace ns3
{

EhtCapabilities::EhtCapabilities()
{
}

WifiInformationElementId
EhtCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
EhtCapabilities::ElementIdExt() const
{
    return IE_EXT_EHT_CAPABILITIES;
}

uint16_t
EhtCapabilities::GetInformationFieldSize() const
{
    return 1; // FIXME
}

void
EhtCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    // TODO
}

uint16_t
EhtCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    // TODO
    return length;
}

std::ostream&
operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities)
{
    // TODO
    return os;
}

} // namespace ns3
