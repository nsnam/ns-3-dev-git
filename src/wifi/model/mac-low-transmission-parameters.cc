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
    m_waitAck ({WaitAckType::NONE}),
    m_sendBar ({SendBarType::NONE}),
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
  m_waitAck = {WaitAckType::BLOCK_ACK, type};

  // Reset m_sendBar
  m_sendBar = {SendBarType::NONE};
}

void
MacLowTransmissionParameters::EnableBlockAckRequest (BlockAckType type)
{
  m_sendBar = {SendBarType::BLOCK_ACK_REQ, type};

  // Reset m_waitAck
  m_waitAck = {WaitAckType::NONE};
}

void
MacLowTransmissionParameters::EnableAck (void)
{
  m_waitAck = {WaitAckType::NORMAL};

  // Reset m_sendBar
  m_sendBar = {SendBarType::NONE};
}

void
MacLowTransmissionParameters::DisableAck (void)
{
  m_waitAck = {WaitAckType::NONE};
}

void
MacLowTransmissionParameters::DisableBlockAckRequest (void)
{
  m_sendBar = {SendBarType::NONE};
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
  return (m_waitAck.m_type == WaitAckType::NORMAL);
}

bool
MacLowTransmissionParameters::MustWaitBlockAck (void) const
{
  return (m_waitAck.m_type == WaitAckType::BLOCK_ACK);
}

BlockAckType
MacLowTransmissionParameters::GetBlockAckType (void) const
{
  NS_ABORT_MSG_IF (m_waitAck.m_type != WaitAckType::BLOCK_ACK, "Block ack is not used");
  return m_waitAck.m_baType;
}

bool
MacLowTransmissionParameters::MustSendBlockAckRequest (void) const
{
  return (m_sendBar.m_type == SendBarType::BLOCK_ACK_REQ);
}

BlockAckType
MacLowTransmissionParameters::GetBlockAckRequestType (void) const
{
  NS_ABORT_MSG_IF (m_sendBar.m_type != SendBarType::BLOCK_ACK_REQ, "Block ack request must not be sent");
  return m_sendBar.m_barType;
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
     << "next size=" << params.m_nextSize << ", ";

  if (params.m_waitAck.m_type == MacLowTransmissionParameters::WaitAckType::NORMAL)
    {
      os << "ack=normal";
    }
  else if (params.m_waitAck.m_type == MacLowTransmissionParameters::WaitAckType::BLOCK_ACK)
    {
      os << "ack=" << params.m_waitAck.m_baType;
    }
  else if (params.m_sendBar.m_type == MacLowTransmissionParameters::SendBarType::BLOCK_ACK_REQ)
    {
      os << "bar=" << params.m_sendBar.m_barType;
    }
  else
    {
      os << "ack=none";
    }

  os << "]";
  return os;
}

} //namespace ns3
