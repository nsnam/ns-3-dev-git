/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/ipv4-header.h"
#include "ns3/drop-tail-queue.h"
#include "pfifo-fast-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfifoFastQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (PfifoFastQueueDisc);

TypeId PfifoFastQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfifoFastQueueDisc")
    .SetParent<QueueDisc> ()
    .AddConstructor<PfifoFastQueueDisc> ()
    .AddAttribute ("Mode",
                   "Whether to interpret the TOS byte as legacy TOS or DSCP",
                   EnumValue (QUEUE_MODE_DSCP),
                   MakeEnumAccessor (&PfifoFastQueueDisc::m_trafficClassMode),
                   MakeEnumChecker (QUEUE_MODE_TOS, "TOS semantics",
                                    QUEUE_MODE_DSCP, "DSCP semantics"))
    .AddAttribute ("Band0",
                   "A queue to use as the band 0.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&PfifoFastQueueDisc::m_band0),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("Band1",
                   "A queue to use as the band 1.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&PfifoFastQueueDisc::m_band1),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("Band2",
                   "A queue to use as the band 2.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&PfifoFastQueueDisc::m_band2),
                   MakePointerChecker<Queue> ())
  ;

  return tid;
}

PfifoFastQueueDisc::PfifoFastQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

PfifoFastQueueDisc::~PfifoFastQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
PfifoFastQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  uint32_t band = Classify (item);

  if (band == 0)
    {
      if (!m_band0->Enqueue (item))
        {
          NS_LOG_LOGIC ("Enqueue failed -- dropping pkt");
          Drop (item->GetPacket ());
          return false;
        }
      NS_LOG_LOGIC ("Number packets band 0: " << m_band0->GetNPackets ());
    }
  else if (band == 1)
    {
      if (!m_band1->Enqueue (item))
        {
          NS_LOG_LOGIC ("Enqueue failed -- dropping pkt");
          Drop (item->GetPacket ());
          return false;
        }
      NS_LOG_LOGIC ("Number packets band 1: " << m_band1->GetNPackets ());
    }
  else
    {
      if (!m_band2->Enqueue (item))
        {
          NS_LOG_LOGIC ("Enqueue failed -- dropping pkt");
          Drop (item->GetPacket ());
          return false;
        }
      NS_LOG_LOGIC ("Number packets band 2: " << m_band2->GetNPackets ());
    }
  return true;
}

Ptr<QueueDiscItem>
PfifoFastQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;
  if ((item = DynamicCast<QueueDiscItem> (m_band0->Dequeue ())) != 0)
    {
      NS_LOG_LOGIC ("Popped from band 0: " << item);
      NS_LOG_LOGIC ("Number packets band 0: " << m_band0->GetNPackets ());
    }
  else if ((item = DynamicCast<QueueDiscItem> (m_band1->Dequeue ())) != 0)
    {
      NS_LOG_LOGIC ("Popped from band 1: " << item);
      NS_LOG_LOGIC ("Number packets band 1: " << m_band1->GetNPackets ());
    }
  else if ((item = DynamicCast<QueueDiscItem> (m_band2->Dequeue ())) != 0)
    {
      NS_LOG_LOGIC ("Popped from band 2: " << item);
      NS_LOG_LOGIC ("Number packets band 2: " << m_band2->GetNPackets ());
    }
  else
    {
      NS_LOG_LOGIC ("Queue empty");
    }
  return item;
}

Ptr<const QueueDiscItem>
PfifoFastQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item;
  if (m_band0->GetNPackets ())
    {
      item = DynamicCast<const QueueDiscItem> (m_band0->Peek ());
      NS_LOG_LOGIC ("Peeked from band 0: " << item);
      NS_LOG_LOGIC ("Number packets band 0: " << m_band0->GetNPackets ());
    }
  else if (m_band1->GetNPackets ())
    {
      item = DynamicCast<const QueueDiscItem> (m_band1->Peek ());
      NS_LOG_LOGIC ("Peeked from band 1: " << item);
      NS_LOG_LOGIC ("Number packets band 1: " << m_band1->GetNPackets ());
    }
  else if (m_band2->GetNPackets ())
    {
      item = DynamicCast<const QueueDiscItem> (m_band2->Peek ());
      NS_LOG_LOGIC ("Peeked from band 2: " << item);
      NS_LOG_LOGIC ("Number packets band 2: " << m_band2->GetNPackets ());
    }

  if (item == 0)
    {
      NS_LOG_LOGIC ("Queue empty");
    }
  return item;
}

uint32_t
PfifoFastQueueDisc::GetNPackets (uint32_t band) const
{
  NS_LOG_FUNCTION (this << band);
  if (band == 0)
    {
      return m_band0->GetNPackets ();
    }
  else if (band == 1)
    {
      return m_band1->GetNPackets ();
    }
  else if (band == 2)
    {
      return m_band2->GetNPackets ();
    }
  else
    {
      NS_LOG_ERROR ("Invalid band " << band);
      return 0;
    }
}

uint32_t
PfifoFastQueueDisc::Classify (Ptr<const QueueDiscItem> item) const
{
  NS_LOG_FUNCTION (this << item);
  bool found = false;
  uint32_t band = 1;

  if (item->GetHeaderType () == QueueDiscItem::IPV4_HEADER)
    {
      const Ipv4Header & hdr4 = dynamic_cast<const Ipv4Header &> (item->GetHeader ());
      if (m_trafficClassMode == QUEUE_MODE_TOS)
        {
          band = TosToBand (hdr4.GetTos ());
          found = true;
          NS_LOG_DEBUG ("Found Ipv4 packet; TOS " << (uint16_t) hdr4.GetTos () << " band " << band);
        }
      else
        {
          band = DscpToBand (hdr4.GetDscp ());
          found = true;
          NS_LOG_DEBUG ("Found Ipv4 packet; DSCP " << Ipv4Header::DscpTypeToString (hdr4.GetDscp ()) << " band " << band);
        }
    }
  else
    {
      ///\todo classify IPv6 packets
      NS_LOG_DEBUG ("Only able to classify IPv4 packets");
    }
  if (!found)
    {
      NS_LOG_DEBUG ("Unable to classify; returning default band of " << band);
    }
  return band;
}

uint32_t
PfifoFastQueueDisc::TosToBand (uint8_t tos) const
{
  NS_LOG_FUNCTION (this << (uint16_t) tos);

  uint32_t band = 1;
  switch (tos) {
    case 0x10 :
    case 0x12 :
    case 0x14 :
    case 0x16 :
      band = 0;
      break;
    case 0x0 :
    case 0x4 :
    case 0x6 :
    case 0x18 :
    case 0x1a :
    case 0x1c :
    case 0x1e :
      band = 1;
      break;
    case 0x2 :
    case 0x8 :
    case 0xa :
    case 0xc :
    case 0xe :
      band = 2;
      break;
    default :
      NS_LOG_ERROR ("Invalid TOS " << (uint16_t) tos);
  }
  return band;
}

uint32_t
PfifoFastQueueDisc::DscpToBand (Ipv4Header::DscpType dscpType) const
{
  NS_LOG_FUNCTION (this << Ipv4Header::DscpTypeToString (dscpType));

  uint32_t band = 1;
  switch (dscpType) {
    case Ipv4Header::DSCP_EF :
    case Ipv4Header::DSCP_AF13 :
    case Ipv4Header::DSCP_AF23 :
    case Ipv4Header::DSCP_AF33 :
    case Ipv4Header::DSCP_AF43 :
    case Ipv4Header::DscpDefault :
    case Ipv4Header::DSCP_CS2 :
    case Ipv4Header::DSCP_CS3 :
      band = 1;
      break;
    case Ipv4Header::DSCP_AF11 :
    case Ipv4Header::DSCP_AF21 :
    case Ipv4Header::DSCP_AF31 :
    case Ipv4Header::DSCP_AF41 :
    case Ipv4Header::DSCP_CS1 :
      band = 2;
      break;
    case Ipv4Header::DSCP_AF12 :
    case Ipv4Header::DSCP_AF22 :
    case Ipv4Header::DSCP_AF32 :
    case Ipv4Header::DSCP_AF42 :
    case Ipv4Header::DSCP_CS4 :
    case Ipv4Header::DSCP_CS5 :
    case Ipv4Header::DSCP_CS6 :
    case Ipv4Header::DSCP_CS7 :
      band = 0;
      break;
    default :
      band = 1;
  }
  NS_LOG_DEBUG ("Band returned:  " << band);
  return band;
}

} // namespace ns3

