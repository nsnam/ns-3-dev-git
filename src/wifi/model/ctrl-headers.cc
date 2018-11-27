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

namespace ns3 {


/***********************************
 *       Block ack request
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckRequestHeader);

CtrlBAckRequestHeader::CtrlBAckRequestHeader ()
  : m_barAckPolicy (false),
    m_baType (BASIC_BLOCK_ACK)
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        size += 2;
        break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        break;
      case COMPRESSED_BLOCK_ACK:
        res |= (0x02 << 1);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        res |= (0x01 << 1);
        break;
      case MULTI_TID_BLOCK_ACK:
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
      m_baType = MULTI_TID_BLOCK_ACK;
    }
  else if (((bar >> 1) & 0x0f) == 0x01)
    {
      m_baType = EXTENDED_COMPRESSED_BLOCK_ACK;
    }
  else if (((bar >> 1) & 0x0f) == 0x02)
    {
      m_baType = COMPRESSED_BLOCK_ACK;
    }
  else
    {
      m_baType = BASIC_BLOCK_ACK;
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
CtrlBAckRequestHeader::SetType (BlockAckType type)
{
  m_baType = type;
}

BlockAckType
CtrlBAckRequestHeader::GetType (void) const
{
  return m_baType;
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
  return (m_baType == BASIC_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsCompressed (void) const
{
  return (m_baType == COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsExtendedCompressed (void) const
{
  return (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckRequestHeader::IsMultiTid (void) const
{
  return (m_baType == MULTI_TID_BLOCK_ACK) ? true : false;
}


/***********************************
 *       Block ack response
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED (CtrlBAckResponseHeader);

CtrlBAckResponseHeader::CtrlBAckResponseHeader ()
  : m_baAckPolicy (false),
    m_baType (BASIC_BLOCK_ACK)
{
  memset (&bitmap, 0, sizeof (bitmap));
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        size += (2 + 128);
        break;
      case COMPRESSED_BLOCK_ACK:
        size += (2 + 8);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        size += (2 + 32);
        break;
      case MULTI_TID_BLOCK_ACK:
        size += (2 + 2 + 8) * (m_tidInfo + 1); //Multi-tid block ack
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        i.WriteHtolsbU16 (GetStartingSequenceControl ());
        i = SerializeBitmap (i);
        break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        SetStartingSequenceControl (i.ReadLsbtohU16 ());
        i = DeserializeBitmap (i);
        break;
      case MULTI_TID_BLOCK_ACK:
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
  return (m_baType == BASIC_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsCompressed (void) const
{
  return (m_baType == COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsExtendedCompressed (void) const
{
  return (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK) ? true : false;
}

bool
CtrlBAckResponseHeader::IsMultiTid (void) const
{
  return (m_baType == MULTI_TID_BLOCK_ACK) ? true : false;
}

uint16_t
CtrlBAckResponseHeader::GetBaControl (void) const
{
  uint16_t res = 0;
  if (m_baAckPolicy)
    {
      res |= 0x1;
    }
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        break;
      case COMPRESSED_BLOCK_ACK:
        res |= (0x02 << 1);
        break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        res |= (0x01 << 1);
        break;
      case MULTI_TID_BLOCK_ACK:
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
      m_baType = MULTI_TID_BLOCK_ACK;
    }
  else if (((ba >> 1) & 0x0f) == 0x01)
    {
      m_baType = EXTENDED_COMPRESSED_BLOCK_ACK;
    }
  else if (((ba >> 1) & 0x0f) == 0x02)
    {
      m_baType = COMPRESSED_BLOCK_ACK;
    }
  else
    {
      m_baType = BASIC_BLOCK_ACK;
    }
  m_tidInfo = (ba >> 12) & 0x0f;
}

uint16_t
CtrlBAckResponseHeader::GetStartingSequenceControl (void) const
{
  return (m_startingSeq << 4) & 0xfff0;
}

void
CtrlBAckResponseHeader::SetStartingSequenceControl (uint16_t seqControl)
{
  m_startingSeq = (seqControl >> 4) & 0x0fff;
}

Buffer::Iterator
CtrlBAckResponseHeader::SerializeBitmap (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
          for (uint8_t j = 0; j < 64; j++)
            {
              i.WriteHtolsbU16 (bitmap.m_bitmap[j]);
            }
          break;
      case COMPRESSED_BLOCK_ACK:
          i.WriteHtolsbU64 (bitmap.m_compressedBitmap);
          break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[0]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[1]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[2]);
          i.WriteHtolsbU64 (bitmap.m_extendedCompressedBitmap[3]);
          break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
          for (uint8_t j = 0; j < 64; j++)
            {
              bitmap.m_bitmap[j] = i.ReadLsbtohU16 ();
            }
          break;
      case COMPRESSED_BLOCK_ACK:
          bitmap.m_compressedBitmap = i.ReadLsbtohU64 ();
          break;
      case EXTENDED_COMPRESSED_BLOCK_ACK:
          bitmap.m_extendedCompressedBitmap[0] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[1] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[2] = i.ReadLsbtohU64 ();
          bitmap.m_extendedCompressedBitmap[3] = i.ReadLsbtohU64 ();
          break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          /* To set correctly basic block ack bitmap we need fragment number too.
             So if it's not specified, we consider packet not fragmented. */
          bitmap.m_bitmap[IndexInBitmap (seq)] |= 0x0001;
          break;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          bitmap.m_compressedBitmap |= (uint64_t (0x0000000000000001) << IndexInBitmap (seq));
          break;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint16_t index = IndexInBitmap (seq);
          bitmap.m_extendedCompressedBitmap[index/64] |= (uint64_t (0x0000000000000001) << index);
          break;
        }
      case MULTI_TID_BLOCK_ACK:
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
}

void
CtrlBAckResponseHeader::SetReceivedFragment (uint16_t seq, uint8_t frag)
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return;
    }
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        bitmap.m_bitmap[IndexInBitmap (seq)] |= (0x0001 << frag);
        break;
      case COMPRESSED_BLOCK_ACK:
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        /* We can ignore this...compressed block ack doesn't support
           acknowledgement of single fragments */
        break;
      case MULTI_TID_BLOCK_ACK:
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
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          /*It's impossible to say if an entire packet was correctly received. */
          return false;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          uint16_t index = IndexInBitmap (seq);
          return (((bitmap.m_extendedCompressedBitmap[index/64] >> index) & mask) == 1) ? true : false;
        }
      case MULTI_TID_BLOCK_ACK:
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

bool
CtrlBAckResponseHeader::IsFragmentReceived (uint16_t seq, uint8_t frag) const
{
  NS_ASSERT (frag < 16);
  if (!IsInBitmap (seq))
    {
      return false;
    }
  switch (m_baType)
    {
      case BASIC_BLOCK_ACK:
        {
          return ((bitmap.m_bitmap[IndexInBitmap (seq)] & (0x0001 << frag)) != 0x0000) ? true : false;
        }
      case COMPRESSED_BLOCK_ACK:
        {
          /* Although this could make no sense, if packet with sequence number
             equal to <i>seq</i> was correctly received, also all of its fragments
             were correctly received. */
          uint64_t mask = uint64_t (0x0000000000000001);
          return (((bitmap.m_compressedBitmap >> IndexInBitmap (seq)) & mask) == 1) ? true : false;
        }
      case EXTENDED_COMPRESSED_BLOCK_ACK:
        {
          uint64_t mask = uint64_t (0x0000000000000001);
          uint16_t index = IndexInBitmap (seq);
          return (((bitmap.m_extendedCompressedBitmap[index/64] >> index) & mask) == 1) ? true : false;
        }
      case MULTI_TID_BLOCK_ACK:
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
      index = 4096 - m_startingSeq + seq;
    }
  if (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK)
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
  if (m_baType == EXTENDED_COMPRESSED_BLOCK_ACK)
    {
      return (seq - m_startingSeq + 4096) % 4096 < 256;
    }
  else
    {
      return (seq - m_startingSeq + 4096) % 4096 < 64;
    }
}

const uint16_t*
CtrlBAckResponseHeader::GetBitmap (void) const
{
  return bitmap.m_bitmap;
}

uint64_t
CtrlBAckResponseHeader::GetCompressedBitmap (void) const
{
  return bitmap.m_compressedBitmap;
}

const uint64_t*
CtrlBAckResponseHeader::GetExtendedCompressedBitmap (void) const
{
  return bitmap.m_extendedCompressedBitmap;
}

void
CtrlBAckResponseHeader::ResetBitmap (void)
{
  memset (&bitmap, 0, sizeof (bitmap));
}

}  //namespace ns3
