/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ctrl-headers.h"
#include "wifi-utils.h"

namespace ns3 {


/***********************************
 *       Block ack request
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckRequestHeader);

CtrlBAckRequestHeader::CtrlBAckRequestHeader ()
  : m_barAckPolicy (false),
    m_barType (BlockAckReqType::BASIC)
{
}

CtrlBAckRequestHeader::~CtrlBAckRequestHeader ()
{
}

TypeId
CtrlBAckRequestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CtrlBAckRequestHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<CtrlBAckRequestHeader> ()
  ;
  return tid;
}

TypeId
CtrlBAckRequestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CtrlBAckRequestHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq << std::dec;
}

uint32_t
CtrlBAckRequestHeader::GetSerializedSize () const
{
  uint32_t size = 0;
  size += 2; //Bar control
  switch (m_barType.m_variant)
    {
      case BlockAckReqType::BASIC:
      case BlockAckReqType::COMPRESSED:
      case BlockAckReqType::EXTENDED_COMPRESSED:
        size += 2;
        break;
      case BlockAckReqType::MULTI_TID:
        size += (2 + 2) * (m_tidInfo + 1);
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return size;
}

void
CtrlBAckRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBarControl ());
  switch (m_barType.m_variant)
    {
      case BlockAckReqType::BASIC:
      case BlockAckReqType::COMPRESSED:
      case BlockAckReqType::EXTENDED_COMPRESSED:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        break;
      case BlockAckReqType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

uint32_t
CtrlBAckRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBarControl (i.ReadLsbtohU16 ());
  switch (m_barType.m_variant)
    {
      case BlockAckReqType::BASIC:
      case BlockAckReqType::COMPRESSED:
      case BlockAckReqType::EXTENDED_COMPRESSED:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        break;
      case BlockAckReqType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i.GetDistanceFrom (start);
}

uint16_t
CtrlBAckRequestHeader::GetBarControl (void) const
{
  uint16_t res = 0;
  switch (m_barType.m_variant)
    {
      case BlockAckReqType::BASIC:
        break;
      case BlockAckReqType::COMPRESSED:
        res |= (0x02 << 1);
        break;
      case BlockAckReqType::EXTENDED_COMPRESSED:
        res |= (0x01 << 1);
        break;
      case BlockAckReqType::MULTI_TID:
        res |= (0x03 << 1);
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckRequestHeader::SetBarControl (uint16_t bar)
{
  m_barAckPolicy = ((bar & 0x01) == 1) ? true : false;
  if (((bar >> 1) & 0x0f) == 0x03)
    {
      m_barType.m_variant = BlockAckReqType::MULTI_TID;
    }
  else if (((bar >> 1) & 0x0f) == 0x01)
    {
      m_barType.m_variant = BlockAckReqType::EXTENDED_COMPRESSED;
    }
  else if (((bar >> 1) & 0x0f) == 0x02)
    {
      m_barType.m_variant = BlockAckReqType::COMPRESSED;
    }
  else
    {
      m_barType.m_variant = BlockAckReqType::BASIC;
    }
  m_tidInfo = (bar >> 12) & 0x0f;
}

uint16_t
CtrlBAckRequestHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
CtrlBAckRequestHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

void
CtrlBAckRequestHeader::SetHtImmediateAck (bool immediateAck)
{
  m_barAckPolicy = immediateAck;
}

void
CtrlBAckRequestHeader::SetType (BlockAckReqType type)
{
  m_barType = type;
}

BlockAckReqType
CtrlBAckRequestHeader::GetType (void) const
{
  return m_barType;
}

void
CtrlBAckRequestHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
CtrlBAckRequestHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
CtrlBAckRequestHeader::MustSendHtImmediateAck (void) const
{
  return m_barAckPolicy;
}

uint8_t
CtrlBAckRequestHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
CtrlBAckRequestHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
CtrlBAckRequestHeader::IsBasic (void) const
{
  return (m_barType.m_variant == BlockAckReqType::BASIC) ? true : false;
}

bool
CtrlBAckRequestHeader::IsCompressed (void) const
{
  return (m_barType.m_variant == BlockAckReqType::COMPRESSED) ? true : false;
}

bool
CtrlBAckRequestHeader::IsExtendedCompressed (void) const
{
  return (m_barType.m_variant == BlockAckReqType::EXTENDED_COMPRESSED) ? true : false;
}

bool
CtrlBAckRequestHeader::IsMultiTid (void) const
{
  return (m_barType.m_variant == BlockAckReqType::MULTI_TID) ? true : false;
}


/***********************************
 *       Block ack response
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckResponseHeader);

CtrlBAckResponseHeader::CtrlBAckResponseHeader ()
  : m_baAckPolicy (false),
    m_tidInfo (0),
    m_startingSeq (0)
{
  SetType (BlockAckType::BASIC);
}

CtrlBAckResponseHeader::~CtrlBAckResponseHeader ()
{
}

TypeId
CtrlBAckResponseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CtrlBAckResponseHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<CtrlBAckResponseHeader> ()
  ;
  return tid;
}

TypeId
CtrlBAckResponseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CtrlBAckResponseHeader::Print (std::ostream &os) const
{
  os << "TID_INFO=" << m_tidInfo << ", StartingSeq=" << std::hex << m_startingSeq << std::dec;
}

uint32_t
CtrlBAckResponseHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 2; //Bar control
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        size += (2 + m_baType.m_bitmapLen[0]);
        break;
      case BlockAckType::MULTI_TID:
        size += (2 + 2 + 8) * (m_tidInfo + 1); //Multi-TID block ack
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return size;
}

void
CtrlBAckResponseHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtolsbU16 (GetBaControl ());
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        i = SerializeBitmap (i);
        break;
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

uint32_t
CtrlBAckResponseHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  SetBaControl (i.ReadLsbtohU16 ());
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        i = DeserializeBitmap (i);
        break;
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i.GetDistanceFrom (start);
}

void
CtrlBAckResponseHeader::SetHtImmediateAck (bool immediateAck)
{
  m_baAckPolicy = immediateAck;
}

void
CtrlBAckResponseHeader::SetType (BlockAckType type)
{
  m_baType = type;
  m_bitmap.resize (m_baType.m_bitmapLen[0]);
  m_bitmap.assign (m_baType.m_bitmapLen[0], 0);
}

BlockAckType
CtrlBAckResponseHeader::GetType (void) const
{
  return m_baType;
}

void
CtrlBAckResponseHeader::SetTidInfo (uint8_t tid)
{
  m_tidInfo = static_cast<uint16_t> (tid);
}

void
CtrlBAckResponseHeader::SetStartingSequence (uint16_t seq)
{
  m_startingSeq = seq;
}

bool
CtrlBAckResponseHeader::MustSendHtImmediateAck (void) const
{
  return (m_baAckPolicy) ? true : false;
}

uint8_t
CtrlBAckResponseHeader::GetTidInfo (void) const
{
  uint8_t tid = static_cast<uint8_t> (m_tidInfo);
  return tid;
}

uint16_t
CtrlBAckResponseHeader::GetStartingSequence (void) const
{
  return m_startingSeq;
}

bool
CtrlBAckResponseHeader::IsBasic (void) const
{
  return (m_baType.m_variant == BlockAckType::BASIC) ? true : false;
}

bool
CtrlBAckResponseHeader::IsCompressed (void) const
{
  return (m_baType.m_variant == BlockAckType::COMPRESSED) ? true : false;
}

bool
CtrlBAckResponseHeader::IsExtendedCompressed (void) const
{
  return (m_baType.m_variant == BlockAckType::EXTENDED_COMPRESSED) ? true : false;
}

bool
CtrlBAckResponseHeader::IsMultiTid (void) const
{
  return (m_baType.m_variant == BlockAckType::MULTI_TID) ? true : false;
}

uint16_t
CtrlBAckResponseHeader::GetBaControl (void) const
{
  uint16_t res = 0;
  if (m_baAckPolicy)
    {
      res |= 0x1;
    }
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
        break;
      case BlockAckType::COMPRESSED:
        res |= (0x02 << 1);
        break;
      case BlockAckType::EXTENDED_COMPRESSED:
        res |= (0x01 << 1);
        break;
      case BlockAckType::MULTI_TID:
        res |= (0x03 << 1);
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  res |= (m_tidInfo << 12) & (0xf << 12);
  return res;
}

void
CtrlBAckResponseHeader::SetBaControl (uint16_t ba)
{
  m_baAckPolicy = ((ba & 0x01) == 1) ? true : false;
  if (((ba >> 1) & 0x0f) == 0x03)
    {
      SetType (BlockAckType::MULTI_TID);
    }
  else if (((ba >> 1) & 0x0f) == 0x01)
    {
      SetType (BlockAckType::EXTENDED_COMPRESSED);
    }
  else if (((ba >> 1) & 0x0f) == 0x02)
    {
      SetType (BlockAckType::COMPRESSED);
    }
  else if (((ba >> 1) & 0x0f) == 0)
    {
      SetType (BlockAckType::BASIC);
    }
  else
    {
      NS_FATAL_ERROR ("Invalid BA type");
    }
  m_tidInfo = (ba >> 12) & 0x0f;
}

uint16_t
CtrlBAckResponseHeader::GetStartingSequenceControl (void) const
{
  uint16_t ret = (m_startingSeq << 4) & 0xfff0;

  if (m_baType.m_variant == BlockAckType::COMPRESSED
      && m_baType.m_bitmapLen[0] == 32)
    {
      ret |= 0x0004;
    }
  return ret;
}

void
CtrlBAckResponseHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  if (m_baType.m_variant == BlockAckType::COMPRESSED)
    {
      if ((seqControl & 0x0001) == 1)
        {
          NS_FATAL_ERROR ("Fragmentation Level 3 unsupported");
        }
      if (((seqControl >> 3) & 0x0001) == 0 && ((seqControl >> 1) & 0x0003) == 0)
        {
          SetType ({BlockAckType::COMPRESSED, {8}});
        }
      else if (((seqControl >> 3) & 0x0001) == 0 && ((seqControl >> 1) & 0x0003) == 2)
        {
          SetType ({BlockAckType::COMPRESSED, {32}});
        }
      else
        {
          NS_FATAL_ERROR ("Reserved configurations");
        }
    }
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

Buffer::Iterator
CtrlBAckResponseHeader::SerializeBitmap (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
          for (auto& byte : m_bitmap)
            {
              i.WriteU8 (byte);
            }
          break;
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i;
}

Buffer::Iterator
CtrlBAckResponseHeader::DeserializeBitmap (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
          for (uint8_t j = 0; j < m_baType.m_bitmapLen[0]; j++)
            {
              m_bitmap[j] = i.ReadU8 ();
            }
          break;
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return i;
}

void
CtrlBAckResponseHeader::SetReceivedPacket (uint16_t seq)
{
  if (!IsInBitmap (seq))
    {
      return;
    }
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
        /* To set correctly basic block ack bitmap we need fragment number too.
            So if it's not specified, we consider packet not fragmented. */
        m_bitmap[IndexInBitmap (seq) * 2] |= 0x01;
        break;
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        {
          uint16_t index = IndexInBitmap (seq);
          m_bitmap[index / 8] |= (uint8_t (0x01) << (index % 8));
          break;
        }
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

void
CtrlBAckResponseHeader::SetReceivedFragment (uint16_t seq, uint8_t frag)
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return;
    }
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
        m_bitmap[IndexInBitmap (seq) * 2 + frag / 8] |= (0x01 << (frag % 8));
        break;
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        /* We can ignore this...compressed block ack doesn't support
           acknowledgment of single fragments */
        break;
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
}

bool
CtrlBAckResponseHeader::IsPacketReceived (uint16_t seq) const
{
  if (!IsInBitmap (seq))
    {
      return false;
    }
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
        /*It's impossible to say if an entire packet was correctly received. */
        return false;
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        {
          uint16_t index = IndexInBitmap (seq);
          uint8_t mask = uint8_t (0x01) << (index % 8) ;
          return ((m_bitmap[index / 8] & mask) != 0) ? true : false;
        }
      case BlockAckType::MULTI_TID:
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        break;
      default:
        NS_FATAL_ERROR ("Invalid BA type");
        break;
    }
  return false;
}

bool
CtrlBAckResponseHeader::IsFragmentReceived (uint16_t seq, uint8_t frag) const
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return false;
    }
  switch (m_baType.m_variant)
    {
      case BlockAckType::BASIC:
        return ((m_bitmap[IndexInBitmap (seq) * 2 + frag / 8] & (0x01 << (frag % 8))) != 0) ? true : false;
      case BlockAckType::COMPRESSED:
      case BlockAckType::EXTENDED_COMPRESSED:
        /* We can ignore this...compressed block ack doesn't support
           acknowledgement of single fragments */
        return false;
      case BlockAckType::MULTI_TID:
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
          break;
        }
      default:
        {
          NS_FATAL_ERROR ("Invalid BA type");
          break;
        }
    }
  return false;
}

uint16_t
CtrlBAckResponseHeader::IndexInBitmap (uint16_t seq) const
{
  uint16_t index;
  if (seq >= m_startingSeq)
    {
      index = seq - m_startingSeq;
    }
  else
    {
      index = SEQNO_SPACE_SIZE - m_startingSeq + seq;
    }
  if (m_baType.m_variant == BlockAckType::COMPRESSED && m_baType.m_bitmapLen[0] == 32)
    {
      NS_ASSERT (index <= 255);
    }
  else
    {
      NS_ASSERT (index <= 63);
    }
  return index;
}

bool
CtrlBAckResponseHeader::IsInBitmap (uint16_t seq) const
{
  if (m_baType.m_variant == BlockAckType::COMPRESSED && m_baType.m_bitmapLen[0] == 32)
    {
      return (seq - m_startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE < 256;
    }
  else
    {
      return (seq - m_startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE < 64;
    }
}

const std::vector<uint8_t>&
CtrlBAckResponseHeader::GetBitmap (void) const
{
  return m_bitmap;
}

void
CtrlBAckResponseHeader::ResetBitmap (void)
{
  m_bitmap.assign (m_baType.m_bitmapLen[0], 0);
}

}  //namespace ns3
