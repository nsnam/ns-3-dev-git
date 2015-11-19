/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/uinteger.h"
#include "net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetDevice");

NS_OBJECT_ENSURE_REGISTERED (NetDeviceQueue);

TypeId NetDeviceQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetDeviceQueue")
    .SetParent<Object> ()
    .SetGroupName("Network")
  ;
  return tid;
}

NetDeviceQueue::~NetDeviceQueue ()
{
  NS_LOG_FUNCTION (this);
}

void
NetDeviceQueue::SetDevice (Ptr<NetDevice> device)
{
  NS_ABORT_MSG_UNLESS (m_device == 0, "Cannot change the device which a transmission queue belongs to.");
  m_device = device;
}


NS_OBJECT_ENSURE_REGISTERED (NetDevice);

TypeId NetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetDevice")
    .SetParent<Object> ()
    .SetGroupName("Network")
  ;
  return tid;
}

NetDevice::NetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<NetDeviceQueue> devQueue = Create<NetDeviceQueue> ();
  devQueue->SetDevice (this);
  m_txQueuesVector.push_back (devQueue);
}

NetDevice::~NetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
NetDevice::SetTxQueuesN (uint8_t numTxQueues)
{
  NS_ASSERT (numTxQueues > 0);

  /// \todo check whether a queue disc has been aggregated to the device

  uint8_t prevNumTxQueues = m_txQueuesVector.size ();
  m_txQueuesVector.resize (numTxQueues);

  // Allocate new NetDeviceQueues if the number of queues increased
  for (uint8_t i = prevNumTxQueues; i < numTxQueues; i++)
    {
      Ptr<NetDeviceQueue> devQueue = Create<NetDeviceQueue> ();
      devQueue->SetDevice (this);
      m_txQueuesVector[i] = devQueue;
    }
}

} // namespace ns3
