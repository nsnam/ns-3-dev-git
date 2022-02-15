/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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

#ifndef FCFS_WIFI_QUEUE_SCHEDULER_H
#define FCFS_WIFI_QUEUE_SCHEDULER_H

#include "ns3/nstime.h"
#include "wifi-mac-queue-scheduler-impl.h"

namespace ns3 {

class WifiMacQueueItem;

/**
 * \ingroup wifi
 *
 * WifiMacQueueScheduler ...
 */
class FcfsWifiQueueScheduler : public WifiMacQueueSchedulerImpl<Time>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FcfsWifiQueueScheduler ();

  /// drop policy
  enum DropPolicy
  {
    DROP_NEWEST,
    DROP_OLDEST
  };

private:
  Ptr<WifiMacQueueItem> HasToDropBeforeEnqueuePriv (AcIndex ac, Ptr<WifiMacQueueItem> mpdu) override;
  void DoNotifyEnqueue (AcIndex ac, Ptr<WifiMacQueueItem> mpdu) override;
  void DoNotifyDequeue (AcIndex ac, const std::list<Ptr<WifiMacQueueItem>>& mpdus) override;
  void DoNotifyRemove (AcIndex ac, const std::list<Ptr<WifiMacQueueItem>>& mpdus) override;

  DropPolicy m_dropPolicy;                  //!< Drop behavior of queue
  NS_LOG_TEMPLATE_DECLARE;                  //!< redefinition of the log component
};

} //namespace ns3

#endif /* FCFS_WIFI_QUEUE_SCHEDULER_H */
