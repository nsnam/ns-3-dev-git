/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-acknowledgment.h"
#include "wifi-utils.h"
#include "ns3/mac48-address.h"

namespace ns3 {

/*
 * WifiAcknowledgment
 */

WifiAcknowledgment::WifiAcknowledgment (Method m)
  : method (m),
    acknowledgmentTime (Time::Min ())   // uninitialized
{
}

WifiAcknowledgment::~WifiAcknowledgment ()
{
}

WifiMacHeader::QosAckPolicy
WifiAcknowledgment::GetQosAckPolicy (Mac48Address receiver, uint8_t tid) const
{
  auto it = m_ackPolicy.find ({receiver, tid});
  NS_ASSERT (it != m_ackPolicy.end ());
  return it->second;
}

void
WifiAcknowledgment::SetQosAckPolicy (Mac48Address receiver, uint8_t tid,
                                     WifiMacHeader::QosAckPolicy ackPolicy)
{
  NS_ABORT_MSG_IF (!CheckQosAckPolicy (receiver, tid, ackPolicy), "QoS Ack policy not admitted");
  m_ackPolicy[{receiver, tid}] = ackPolicy;
}

/*
 * WifiNoAck
 */

WifiNoAck::WifiNoAck ()
  : WifiAcknowledgment (NONE)
{
  acknowledgmentTime = Seconds (0);
}

bool
WifiNoAck::CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const
{
  if (ackPolicy == WifiMacHeader::NO_ACK || ackPolicy == WifiMacHeader::BLOCK_ACK)
    {
      return true;
    }
  return false;
}

void
WifiNoAck::Print (std::ostream &os) const
{
  os << "NONE";
}


/*
 * WifiNormalAck
 */

WifiNormalAck::WifiNormalAck ()
  : WifiAcknowledgment (NORMAL_ACK)
{
}

bool
WifiNormalAck::CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const
{
  if (ackPolicy == WifiMacHeader::NORMAL_ACK)
    {
      return true;
    }
  return false;
}

void
WifiNormalAck::Print (std::ostream &os) const
{
  os << "NORMAL_ACK";
}

/*
 * WifiBlockAck
 */

WifiBlockAck::WifiBlockAck ()
  : WifiAcknowledgment (BLOCK_ACK)
{
}

bool
WifiBlockAck::CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const
{
  if (ackPolicy == WifiMacHeader::NORMAL_ACK)
    {
      return true;
    }
  return false;
}

void
WifiBlockAck::Print (std::ostream &os) const
{
  os << "BLOCK_ACK";
}


/*
 * WifiBarBlockAck
 */

WifiBarBlockAck::WifiBarBlockAck ()
  : WifiAcknowledgment (BAR_BLOCK_ACK)
{
}

bool
WifiBarBlockAck::CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const
{
  if (ackPolicy == WifiMacHeader::BLOCK_ACK)
    {
      return true;
    }
  return false;
}

void
WifiBarBlockAck::Print (std::ostream &os) const
{
  os << "BAR_BLOCK_ACK";
}


std::ostream & operator << (std::ostream &os, const WifiAcknowledgment* acknowledgment)
{
  acknowledgment->Print (os);
  return os;
}

} //namespace ns3
