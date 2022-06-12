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

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "sta-wifi-mac.h"
#include "wifi-default-assoc-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiDefaultAssocManager");

NS_OBJECT_ENSURE_REGISTERED (WifiDefaultAssocManager);

TypeId
WifiDefaultAssocManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiDefaultAssocManager")
    .SetParent<WifiAssocManager> ()
    .AddConstructor<WifiDefaultAssocManager> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

WifiDefaultAssocManager::WifiDefaultAssocManager ()
{
  NS_LOG_FUNCTION (this);
}

WifiDefaultAssocManager::~WifiDefaultAssocManager ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiDefaultAssocManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_probeRequestEvent.Cancel ();
  m_waitBeaconEvent.Cancel ();
  WifiAssocManager::DoDispose();
}

bool
WifiDefaultAssocManager::Compare (const StaWifiMac::ApInfo& lhs,
                                  const StaWifiMac::ApInfo& rhs) const
{
  return lhs.m_snr > rhs.m_snr;
}

void
WifiDefaultAssocManager::DoStartScanning (void)
{
  NS_LOG_FUNCTION (this);

  // if there are entries in the sorted list of AP information, reuse them and
  // do not perform scanning
  if (!GetSortedList ().empty ())
    {
      Simulator::ScheduleNow (&WifiDefaultAssocManager::ScanningTimeout, this);
      return;
    }

  m_probeRequestEvent.Cancel ();
  m_waitBeaconEvent.Cancel ();

  if (GetScanParams ().type == WifiScanParams::ACTIVE)
    {
      Simulator::Schedule (GetScanParams ().probeDelay, &StaWifiMac::SendProbeRequest, m_mac);
      m_probeRequestEvent = Simulator::Schedule (GetScanParams ().probeDelay
                                                 + GetScanParams().maxChannelTime,
                                                 &WifiDefaultAssocManager::ScanningTimeout,
                                                 this);
    }
  else
    {
      m_waitBeaconEvent = Simulator::Schedule (GetScanParams ().maxChannelTime,
                                               &WifiDefaultAssocManager::ScanningTimeout,
                                               this);
    }
}

bool
WifiDefaultAssocManager::CanBeInserted (const StaWifiMac::ApInfo& apInfo) const
{
  return (m_waitBeaconEvent.IsRunning () || m_probeRequestEvent.IsRunning ());
}

bool
WifiDefaultAssocManager::CanBeReturned (const StaWifiMac::ApInfo& apInfo) const
{
  return true;
}

} //namespace ns3
