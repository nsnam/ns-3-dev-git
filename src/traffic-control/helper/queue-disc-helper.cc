/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
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
#include "ns3/traffic-control-layer.h"
#include "queue-disc-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QueueDiscHelper");

QueueDiscHelper::QueueDiscHelper ()
{
  m_queueDiscFactory.SetTypeId ("ns3::PfifoFastQueueDisc");
}

void
QueueDiscHelper::SetQueueDiscType (std::string type)
{
  m_queueDiscFactory.SetTypeId (type);
}

void
QueueDiscHelper::SetAttribute (std::string name, const AttributeValue& value)
{
  m_queueDiscFactory.Set (name, value);
}

QueueDiscContainer
QueueDiscHelper::Install (Ptr<NetDevice> d)
{
  QueueDiscContainer container;

  // A TrafficControlLayer object is aggregated by the InternetStackHelper, but check
  // anyway because a queue disc has no effect without a TrafficControlLayer object
  NS_ASSERT (d->GetNode ()->GetObject<TrafficControlLayer> ());

  // Check if a queue disc is already installed on this netdevice
  Ptr<QueueDisc> q = d->GetObject<QueueDisc> ();

  if (q != 0)
    {
      NS_FATAL_ERROR ("QueueDiscHelper::Install (): Installing a queue disc "
                      "on a device already having an associated queue disc");
      return container;
    }

  q = m_queueDiscFactory.Create<QueueDisc> ();
  // This is a root queue disc, so it points to the first transmission queue of the
  // netdevice and is aggregated to the netdevice object
  q->SetNetDeviceQueue (d->GetTxQueue (0));
  q->SetWakeCbOnAllTxQueues ();
  d->AggregateObject (q);
  container.Add (q);

  return container;
}

QueueDiscContainer
QueueDiscHelper::Install (NetDeviceContainer c)
{
  QueueDiscContainer container;

  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      container.Add (Install (*i));
    }

  return container;
}

} // namespace ns3
