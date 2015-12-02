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
#ifndef QUEUE_DISC_HELPER_H
#define QUEUE_DISC_HELPER_H

#include <string>

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/queue-disc-container.h"

namespace ns3 {

/**
 * \brief Build a set of QueueDisc objects
 *
 * This class can help to create QueueDisc objects and map them to
 * the corresponding devices. This map is stored at the Traffic Control
 * layer.
 */
class QueueDiscHelper
{
public:
  /**
   * Create a QueueDiscHelper to make life easier when creating QueueDisc
   * objects.
   */
  QueueDiscHelper ();
  virtual ~QueueDiscHelper () {}

  /**
   * Helper function used to set the type of queue disc to be created.
   *
   * \param type the type of queue
   */
  void SetQueueDiscType (std::string type);

  /**
   * Helper function used to set the queue disc attributes.
   *
   * \param name the name of the attribute to set on the queue disc
   * \param value the value of the attribute to set on the queue disc
   */
  void SetAttribute (std::string name, const AttributeValue &va1ue);

  /**
   * \param c set of devices
   * \returns a QueueDisc container with the queue discs installed on the devices
   *
   * This method creates a QueueDisc object of the type and with the
   * attributes configured by QueueDiscHelper::SetQueueDisc for
   * each device in the container. Then, stores the mapping between a
   * device and the associated queue disc into the traffic control layer
   * of the corresponding node.
   */
  QueueDiscContainer Install (NetDeviceContainer c);

  /**
   * \param d device
   * \returns a QueueDisc container with the queue disc installed on the device
   *
   * This method creates a QueueDisc object of the type and with the
   * attributes configured by QueueDiscHelper::SetQueueDisc for
   * the given device. Then, stores the mapping between the device
   * and the associated queue disc into the traffic control layer
   * of the corresponding node.
   */
  QueueDiscContainer Install (Ptr<NetDevice> d);

private:
  ObjectFactory m_queueDiscFactory;         //!< QueueDisc Factory
};

} // namespace ns3

#endif /* QUEUE_DISC_HELPER_H */
