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
#include "wifi-phy.h"

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
      Simulator::ScheduleNow (&WifiDefaultAssocManager::EndScanning, this);
      return;
    }

  m_probeRequestEvent.Cancel ();
  m_waitBeaconEvent.Cancel ();

  if (GetScanParams ().type == WifiScanParams::ACTIVE)
    {
      Simulator::Schedule (GetScanParams ().probeDelay, &StaWifiMac::SendProbeRequest, m_mac);
      m_probeRequestEvent = Simulator::Schedule (GetScanParams ().probeDelay
                                                 + GetScanParams().maxChannelTime,
                                                 &WifiDefaultAssocManager::EndScanning,
                                                 this);
    }
  else
    {
      m_waitBeaconEvent = Simulator::Schedule (GetScanParams ().maxChannelTime,
                                               &WifiDefaultAssocManager::EndScanning,
                                               this);
    }
}

void
WifiDefaultAssocManager::EndScanning (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<MultiLinkElement> mle;
  Ptr<ReducedNeighborReport> rnr;
  std::list<WifiAssocManager::RnrLinkInfo> apList;

  // If multi-link setup is not possible, just call ScanningTimeout() and return
  if (!CanSetupMultiLink (mle, rnr) || (apList = GetAllAffiliatedAps (*rnr)).empty ())
    {
      ScanningTimeout ();
      return;
    }

  auto& bestAp = *GetSortedList ().begin ();

  // store AP MLD MAC address in the WifiRemoteStationManager associated with
  // the link on which the Beacon/Probe Response was received
  m_mac->GetWifiRemoteStationManager (bestAp.m_linkId)->SetMldAddress (bestAp.m_apAddr,
                                                                       mle->GetMldMacAddress ());
  auto& setupLinks = GetSetupLinks (bestAp);

  setupLinks.clear ();
  setupLinks.push_back ({bestAp.m_linkId, mle->GetLinkIdInfo ()});

  // sort local PHY objects so that radios with constrained PHY band comes first,
  // then radios with no constraint
  std::list<uint8_t> localLinkIds;

  for (uint8_t linkId = 0; linkId < m_mac->GetNLinks (); linkId++)
    {
      if (linkId == bestAp.m_linkId)
        {
          // this link has been already added (it is the link on which the Beacon/Probe
          // Response was received)
          continue;
        }

      if (m_mac->GetWifiPhy (linkId)->HasFixedPhyBand ())
        {
          localLinkIds.push_front (linkId);
        }
      else
        {
          localLinkIds.push_back (linkId);
        }
    }

  // iterate over all the local links and find if we can setup a link for each of them
  for (const auto& linkId : localLinkIds)
    {
      auto apIt = apList.begin ();

      while (apIt != apList.end ())
        {
          auto apChannel = rnr->GetOperatingChannel (apIt->m_nbrApInfoId);

          // we cannot setup a link with this affiliated AP if this PHY object is
          // constrained to operate in the current PHY band and this affiliated AP
          // is operating in a different PHY band than this PHY object
          if (m_mac->GetWifiPhy (linkId)->HasFixedPhyBand ()
              && m_mac->GetWifiPhy (linkId)->GetPhyBand () != apChannel.GetPhyBand ())
            {
              apIt++;
              continue;
            }

          // if we get here, it means we can setup a link with this affiliated AP
          // set the BSSID for this link
          Mac48Address bssid = rnr->GetBssid (apIt->m_nbrApInfoId, apIt->m_tbttInfoFieldId);
          NS_LOG_DEBUG ("Setting BSSID=" << bssid << " for link " << +linkId);
          m_mac->SetBssid (bssid, linkId);
          // store AP MLD MAC address in the WifiRemoteStationManager associated with
          // the link requested to setup
          m_mac->GetWifiRemoteStationManager (linkId)->SetMldAddress (bssid, mle->GetMldMacAddress ());
          setupLinks.push_back ({linkId, rnr->GetLinkId (apIt->m_nbrApInfoId, apIt->m_tbttInfoFieldId)});

          // switch this link to using the channel used by a reported AP
          // TODO check if the STA only supports a narrower channel width
          NS_LOG_DEBUG ("Switch link " << +linkId << " to using channel " << +apChannel.GetNumber ()
                        << " in band " << apChannel.GetPhyBand () << " frequency "
                        << apChannel.GetFrequency () << "MHz width " << apChannel.GetWidth () << "MHz");
          WifiPhy::ChannelTuple chTuple {apChannel.GetNumber (), apChannel.GetWidth (),
                                          apChannel.GetPhyBand (),
                                          apChannel.GetPrimaryChannelIndex (20)};
          m_mac->GetWifiPhy (linkId)->SetOperatingChannel (chTuple);

          // remove the affiliated AP with which we are going to setup a link and move
          // to the next local linkId
          apList.erase (apIt);
          break;
        }
    }

  // we are done
  ScanningTimeout ();
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
