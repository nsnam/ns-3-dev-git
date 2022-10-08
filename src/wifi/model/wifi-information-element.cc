/*
 * Copyright (c) 2010 Dean Armstrong
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
 * Author: Dean Armstrong <deanarm@gmail.com>
 */

#include "wifi-information-element.h"

namespace ns3
{

WifiInformationElement::~WifiInformationElement()
{
}

void
WifiInformationElement::Print(std::ostream& os) const
{
}

uint16_t
WifiInformationElement::GetSerializedSize() const
{
    uint16_t size = GetInformationFieldSize();

    if (size <= 255) // size includes the Element ID Extension field
    {
        return (2 + size);
    }

    // the element needs to be fragmented (Sec. 10.28.11 of 802.11-2020)
    // Let M be the number of IEs of maximum size
    uint16_t m = size / 255;
    // N equals 1 if an IE not of maximum size is present at the end, 0 otherwise
    uint8_t remainder = size % 255;
    uint8_t n = (remainder > 0) ? 1 : 0;

    return m * (2 + 255) + n * (2 + remainder);
}

WifiInformationElementId
WifiInformationElement::ElementIdExt() const
{
    return 0;
}

Buffer::Iterator
WifiInformationElement::Serialize(Buffer::Iterator i) const
{
    auto size = GetInformationFieldSize();

    if (size > 255)
    {
        return SerializeFragments(i, size);
    }

    i.WriteU8(ElementId());
    i.WriteU8(size);
    if (ElementId() == IE_EXTENSION)
    {
        i.WriteU8(ElementIdExt());
        SerializeInformationField(i);
        i.Next(size - 1);
    }
    else
    {
        SerializeInformationField(i);
        i.Next(size);
    }
    return i;
}

Buffer::Iterator
WifiInformationElement::SerializeFragments(Buffer::Iterator i, uint16_t size) const
{
    NS_ASSERT(size > 255);
    // let the subclass serialize the IE in a temporary buffer
    Buffer buffer;
    buffer.AddAtStart(size);
    Buffer::Iterator source = buffer.Begin();
    SerializeInformationField(source);

    // Let M be the number of IEs of maximum size
    uint16_t m = size / 255;

    for (uint16_t index = 0; index < m; index++)
    {
        i.WriteU8((index == 0) ? ElementId() : IE_FRAGMENT);
        i.WriteU8(255);
        uint8_t length = 255;
        if (index == 0 && ElementId() == IE_EXTENSION)
        {
            i.WriteU8(ElementIdExt());
            length = 254;
        }
        for (uint8_t count = 0; count < length; count++)
        {
            i.WriteU8(source.ReadU8());
        }
    }

    // last fragment
    uint8_t remainder = size % 255;

    if (remainder > 0)
    {
        i.WriteU8(IE_FRAGMENT);
        i.WriteU8(remainder);
        for (uint8_t count = 0; count < remainder; count++)
        {
            i.WriteU8(source.ReadU8());
        }
    }

    return i;
}

Buffer::Iterator
WifiInformationElement::Deserialize(Buffer::Iterator i)
{
    Buffer::Iterator start = i;
    i = DeserializeIfPresent(i);
    // This IE was not optional, so confirm that we did actually
    // deserialise something.
    NS_ASSERT(i.GetDistanceFrom(start) != 0);
    return i;
}

Buffer::Iterator
WifiInformationElement::DeserializeIfPresent(Buffer::Iterator i)
{
    if (i.IsEnd())
    {
        return i;
    }
    Buffer::Iterator start = i;
    uint8_t elementId = i.ReadU8();

    // If the element here isn't the one we're after then we immediately
    // return the iterator we were passed indicating that we haven't
    // taken anything from the buffer.
    if (elementId != ElementId())
    {
        return start;
    }

    uint16_t length = i.ReadU8();
    if (ElementId() == IE_EXTENSION)
    {
        uint8_t elementIdExt = i.ReadU8();
        // If the element here isn't the one we're after then we immediately
        // return the iterator we were passed indicating that we haven't
        // taken anything from the buffer.
        if (elementIdExt != ElementIdExt())
        {
            return start;
        }
        length--;
    }

    return DoDeserialize(i, length);
}

Buffer::Iterator
WifiInformationElement::DoDeserialize(Buffer::Iterator i, uint16_t length)
{
    uint16_t limit = (ElementId() == IE_EXTENSION) ? 254 : 255;

    auto tmp = i;
    tmp.Next(length); // tmp points to past the last byte of the IE/first fragment

    if (length < limit || tmp.IsEnd() || (tmp.PeekU8() != IE_FRAGMENT))
    {
        // no fragments
        DeserializeInformationField(i, length);
        return tmp;
    }

    NS_ASSERT(length == limit);

    // the IE is fragmented, create a new buffer for the subclass to deserialize from.
    // Such a destination buffer will not contain the Element ID and Length fields
    Buffer buffer;             // destination buffer
    buffer.AddAtStart(length); // size of the first fragment
    Buffer::Iterator bufferIt = buffer.Begin();

    uint16_t count = length;
    length = 0; // reset length

    // Loop invariant:
    // - i points to the first byte of the fragment to copy (current fragment)
    // - bufferIt points to the first location of the destination buffer to write
    // - there is room in the destination buffer to write the current fragment
    // - count is the size in bytes of the current fragment
    // - length is the number of bytes written into the destination buffer
    while (true)
    {
        for (uint16_t index = 0; index < count; index++)
        {
            bufferIt.WriteU8(i.ReadU8());
        }
        length += count;

        if (i.IsEnd() || (i.PeekU8() != IE_FRAGMENT))
        {
            break;
        }
        i.Next(1);          // skip the Element ID byte
        count = i.ReadU8(); // length of the next fragment

        buffer.AddAtEnd(count);
        bufferIt = buffer.Begin();
        bufferIt.Next(length);
    }

    DeserializeInformationField(buffer.Begin(), length);
    return i;
}

bool
WifiInformationElement::operator==(const WifiInformationElement& a) const
{
    if (ElementId() != a.ElementId())
    {
        return false;
    }

    if (GetInformationFieldSize() != a.GetInformationFieldSize())
    {
        return false;
    }

    if (ElementIdExt() != a.ElementIdExt())
    {
        return false;
    }

    uint32_t ieSize = GetInformationFieldSize();

    Buffer myIe;
    Buffer hisIe;
    myIe.AddAtEnd(ieSize);
    hisIe.AddAtEnd(ieSize);

    SerializeInformationField(myIe.Begin());
    a.SerializeInformationField(hisIe.Begin());

    return (memcmp(myIe.PeekData(), hisIe.PeekData(), ieSize) == 0);
}

} // namespace ns3
