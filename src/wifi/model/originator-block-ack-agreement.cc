/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "originator-block-ack-agreement.h"

namespace ns3 {

OriginatorBlockAckAgreement::OriginatorBlockAckAgreement (Mac48Address recipient, uint8_t tid)
  : BlockAckAgreement (recipient, tid),
    m_state (PENDING)
{
}

OriginatorBlockAckAgreement::~OriginatorBlockAckAgreement ()
{
}

void
OriginatorBlockAckAgreement::SetState (State state)
{
  m_state = state;
}

bool
OriginatorBlockAckAgreement::IsPending (void) const
{
  return (m_state == PENDING) ? true : false;
}

bool
OriginatorBlockAckAgreement::IsEstablished (void) const
{
  return (m_state == ESTABLISHED) ? true : false;
}

bool
OriginatorBlockAckAgreement::IsRejected (void) const
{
  return (m_state == REJECTED) ? true : false;
}

bool
OriginatorBlockAckAgreement::IsNoReply (void) const
{
  return (m_state == NO_REPLY) ? true : false;
}

bool
OriginatorBlockAckAgreement::IsReset (void) const
{
  return (m_state == RESET) ? true : false;
}

} //namespace ns3
