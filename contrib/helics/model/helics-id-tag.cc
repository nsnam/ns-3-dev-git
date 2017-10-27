/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "helics-id-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsIdTag");

NS_OBJECT_ENSURE_REGISTERED (HelicsIdTag);

TypeId 
HelicsIdTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsIdTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<HelicsIdTag> ()
  ;
  return tid;
}
TypeId 
HelicsIdTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
HelicsIdTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}
void 
HelicsIdTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU32 (m_helicsId);
}
void 
HelicsIdTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_helicsId = buf.ReadU32 ();
}
void 
HelicsIdTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "HelicsId=" << m_helicsId;
}
HelicsIdTag::HelicsIdTag ()
  : Tag () 
{
  NS_LOG_FUNCTION (this);
}

HelicsIdTag::HelicsIdTag (uint32_t id)
  : Tag (),
    m_helicsId (id)
{
  NS_LOG_FUNCTION (this << id);
}

void
HelicsIdTag::SetHelicsId (uint32_t id)
{
  NS_LOG_FUNCTION (this << id);
  m_helicsId = id;
}
uint32_t
HelicsIdTag::GetHelicsId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_helicsId;
}

} // namespace ns3

