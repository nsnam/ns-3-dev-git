/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "mac-low-transmission-parameters.h"

namespace ns3 {

MacLowTransmissionParameters::MacLowTransmissionParameters ()
  : m_nextSize (0),
    m_waitAck (ACK_NONE),
    m_sendBar (BLOCK_ACK_REQUEST_NONE),
    m_sendRts (false)
{
}

void
MacLowTransmissionParameters::EnableNextData (uint32_t size)
{
  m_nextSize = size;
}

void
MacLowTransmissionParameters::DisableNextData (void)
{
  m_nextSize = 0;
}

void
MacLowTransmissionParameters::EnableBlockAck (BlockAckType type)
{
  switch (type)
    {
    case BlockAckType::BASIC_BLOCK_ACK:
      m_waitAck = BLOCK_ACK_BASIC;
      break;
    case BlockAckType::COMPRESSED_BLOCK_ACK:
      m_waitAck = BLOCK_ACK_COMPRESSED;
      break;
    case BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK:
      m_waitAck = BLOCK_ACK_EXTENDED_COMPRESSED;
      break;
    case BlockAckType::MULTI_TID_BLOCK_ACK:
      m_waitAck = BLOCK_ACK_MULTI_TID;
      break;
    default:
      NS_FATAL_ERROR ("Unknown Block ack type");
      break;
    }

  // Reset m_sendBar
  m_sendBar = BLOCK_ACK_REQUEST_NONE;
}

void
MacLowTransmissionParameters::EnableBlockAckRequest (BlockAckType type)
{
  switch (type)
    {
    case BlockAckType::BASIC_BLOCK_ACK:
      m_sendBar = BLOCK_ACK_REQUEST_BASIC;
      break;
    case BlockAckType::COMPRESSED_BLOCK_ACK:
      m_sendBar = BLOCK_ACK_REQUEST_COMPRESSED;
      break;
    case BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK:
      m_sendBar = BLOCK_ACK_REQUEST_EXTENDED_COMPRESSED;
      break;
    case BlockAckType::MULTI_TID_BLOCK_ACK:
      m_sendBar = BLOCK_ACK_REQUEST_MULTI_TID;
      break;
    default:
      NS_FATAL_ERROR ("Unknown Block Ack Request type");
      break;
    }

  // Reset m_waitAck
  m_waitAck = ACK_NONE;
}

void
MacLowTransmissionParameters::EnableAck (void)
{
  m_waitAck = ACK_NORMAL;
  // Reset m_sendBar
  m_sendBar = BLOCK_ACK_REQUEST_NONE;
}

void
MacLowTransmissionParameters::DisableAck (void)
{
  m_waitAck = ACK_NONE;
}

void
MacLowTransmissionParameters::DisableBlockAckRequest (void)
{
  m_sendBar = BLOCK_ACK_REQUEST_NONE;
}

void
MacLowTransmissionParameters::EnableRts (void)
{
  m_sendRts = true;
}

void
MacLowTransmissionParameters::DisableRts (void)
{
  m_sendRts = false;
}

bool
MacLowTransmissionParameters::MustWaitNormalAck (void) const
{
  return (m_waitAck == ACK_NORMAL);
}

bool
MacLowTransmissionParameters::MustWaitBlockAck (void) const
{
  bool ret;
  switch (m_waitAck)
    {
    case BLOCK_ACK_BASIC:
    case BLOCK_ACK_COMPRESSED:
    case BLOCK_ACK_EXTENDED_COMPRESSED:
    case BLOCK_ACK_MULTI_TID:
      ret = true;
      break;
    default:
      ret = false;
      break;
    }
  return ret;
}

BlockAckType
MacLowTransmissionParameters::GetBlockAckType (void) const
{
  BlockAckType type;
  switch (m_waitAck)
    {
    case BLOCK_ACK_BASIC:
      type = BlockAckType::BASIC_BLOCK_ACK;
      break;
    case BLOCK_ACK_COMPRESSED:
      type = BlockAckType::COMPRESSED_BLOCK_ACK;
      break;
    case BLOCK_ACK_EXTENDED_COMPRESSED:
      type = BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK;
      break;
    case BLOCK_ACK_MULTI_TID:
      type = BlockAckType::MULTI_TID_BLOCK_ACK;
      break;
    default:
      NS_FATAL_ERROR ("Block ack is not used");
      break;
    }
  return type;
}

bool
MacLowTransmissionParameters::MustSendBlockAckRequest (void) const
{
  bool ret;
  switch (m_sendBar)
    {
    case BLOCK_ACK_REQUEST_BASIC:
    case BLOCK_ACK_REQUEST_COMPRESSED:
    case BLOCK_ACK_REQUEST_EXTENDED_COMPRESSED:
    case BLOCK_ACK_REQUEST_MULTI_TID:
      ret = true;
      break;
    default:
      ret = false;
      break;
    }
  return ret;
}

BlockAckType
MacLowTransmissionParameters::GetBlockAckRequestType (void) const
{
  BlockAckType type;
  switch (m_sendBar)
    {
    case BLOCK_ACK_REQUEST_BASIC:
      type = BlockAckType::BASIC_BLOCK_ACK;
      break;
    case BLOCK_ACK_REQUEST_COMPRESSED:
      type = BlockAckType::COMPRESSED_BLOCK_ACK;
      break;
    case BLOCK_ACK_REQUEST_EXTENDED_COMPRESSED:
      type = BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK;
      break;
    case BLOCK_ACK_REQUEST_MULTI_TID:
      type = BlockAckType::MULTI_TID_BLOCK_ACK;
      break;
    default:
      NS_FATAL_ERROR ("Block ack request must not be sent");
      break;
    }
  return type;
}

bool
MacLowTransmissionParameters::MustSendRts (void) const
{
  return m_sendRts;
}

bool
MacLowTransmissionParameters::HasNextPacket (void) const
{
  return (m_nextSize != 0);
}

uint32_t
MacLowTransmissionParameters::GetNextPacketSize (void) const
{
  NS_ASSERT (HasNextPacket ());
  return m_nextSize;
}

std::ostream &operator << (std::ostream &os, const MacLowTransmissionParameters &params)
{
  os << "["
     << "send rts=" << params.m_sendRts << ", "
     << "next size=" << params.m_nextSize << ", "
     << "ack=";
  switch (params.m_waitAck)
    {
    case MacLowTransmissionParameters::ACK_NONE:
      os << "none, ";
      break;
    case MacLowTransmissionParameters::ACK_NORMAL:
      os << "normal, ";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_BASIC:
      os << "basic-block-ack, ";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_COMPRESSED:
      os << "compressed-block-ack, ";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_EXTENDED_COMPRESSED:
      os << "extended-compressed-block-ack, ";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_MULTI_TID:
      os << "multi-tid-block-ack, ";
      break;
    }
  os << "bar=";
  switch (params.m_sendBar)
    {
    case MacLowTransmissionParameters::BLOCK_ACK_REQUEST_NONE:
      os << "none";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_REQUEST_BASIC:
      os << "basic";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_REQUEST_COMPRESSED:
      os << "compressed";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_REQUEST_EXTENDED_COMPRESSED:
      os << "extended-compressed";
      break;
    case MacLowTransmissionParameters::BLOCK_ACK_REQUEST_MULTI_TID:
      os << "multi-tid";
      break;
    }
  os << "]";
  return os;
}

} //namespace ns3
