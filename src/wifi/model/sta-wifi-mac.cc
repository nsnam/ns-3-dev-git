/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHICONFLICT (content): Merge conflict in src/wifi/model/sta-wifi-mac.cc

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

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "qos-txop.h"
#include "sta-wifi-mac.h"
#include "wifi-phy.h"
#include "mgt-headers.h"
#include "snr-tag.h"
#include "wifi-assoc-manager.h"
#include "wifi-net-device.h"
#include "ns3/ht-configuration.h"
#include "ns3/he-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StaWifiMac");

NS_OBJECT_ENSURE_REGISTERED (StaWifiMac);

TypeId
StaWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StaWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<StaWifiMac> ()
    .AddAttribute ("ProbeRequestTimeout", "The duration to actively probe the channel.",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&StaWifiMac::m_probeRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("WaitBeaconTimeout", "The duration to dwell on a channel while passively scanning for beacon",
                   TimeValue (MilliSeconds (120)),
                   MakeTimeAccessor (&StaWifiMac::m_waitBeaconTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive association request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&StaWifiMac::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&StaWifiMac::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions. "
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&StaWifiMac::SetActiveProbing, &StaWifiMac::GetActiveProbing),
                   MakeBooleanChecker ())
    .AddAttribute ("ProbeDelay",
                   "Delay (in microseconds) to be used prior to transmitting a "
                   "Probe frame during active scanning.",
                   StringValue ("ns3::UniformRandomVariable[Min=50.0|Max=250.0]"),
                   MakePointerAccessor (&StaWifiMac::m_probeDelay),
                   MakePointerChecker<RandomVariableStream> ())
    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_assocLogger),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("DeAssoc", "Association with an access point lost.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_deAssocLogger),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("BeaconArrival",
                     "Time of beacons arrival from associated AP",
                     MakeTraceSourceAccessor (&StaWifiMac::m_beaconArrival),
                     "ns3::Time::TracedCallback")
  ;
  return tid;
}

StaWifiMac::StaWifiMac ()
  : m_state (UNASSOCIATED),
    m_aid (0),
    m_assocRequestEvent (),
    m_beaconWatchdogEnd (Seconds (0))
{
  NS_LOG_FUNCTION (this);

  //Let the lower layers know that we are acting as a non-AP STA in
  //an infrastructure BSS.
  SetTypeOfStation (STA);
}

void
StaWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  StartScanning ();
}

void
StaWifiMac::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_assocManager)
    {
      m_assocManager->Dispose ();
    }
  m_assocManager = nullptr;
  WifiMac::DoDispose ();
}

StaWifiMac::~StaWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

int64_t
StaWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_probeDelay->SetStream (stream);
  return 1;
}

void
StaWifiMac::SetAssocManager (Ptr<WifiAssocManager> assocManager)
{
  NS_LOG_FUNCTION (this << assocManager);
  m_assocManager = assocManager;
  m_assocManager->SetStaWifiMac (this);
}

uint16_t
StaWifiMac::GetAssociationId (void) const
{
  NS_ASSERT_MSG (IsAssociated (), "This station is not associated to any AP");
  return m_aid;
}

void
StaWifiMac::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_activeProbing = enable;
  if (m_state == SCANNING)
    {
      NS_LOG_DEBUG ("STA is still scanning, reset scanning process");
      StartScanning ();
    }
}

bool
StaWifiMac::GetActiveProbing (void) const
{
  return m_activeProbing;
}

void
StaWifiMac::SetWifiPhys (const std::vector<Ptr<WifiPhy>>& phys)
{
  NS_LOG_FUNCTION (this);
  WifiMac::SetWifiPhys (phys);
  for (auto& phy : phys)
    {
      phy->SetCapabilitiesChangedCallback (MakeCallback (&StaWifiMac::PhyCapabilitiesChanged, this));
    }
}

WifiScanParams::Channel
StaWifiMac::GetCurrentChannel (uint8_t linkId) const
{
  auto phy = GetWifiPhy (linkId);
  uint16_t width = phy->GetOperatingChannel ().IsOfdm () ? 20 : phy->GetChannelWidth ();
  uint8_t ch = phy->GetOperatingChannel ().GetPrimaryChannelNumber (width, phy->GetStandard ());
  return {ch, phy->GetPhyBand ()};
}

void
StaWifiMac::SendProbeRequest (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_PROBE_REQUEST);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (Mac48Address::GetBroadcast ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeRequestHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  if (GetHtSupported ())
    {
      probe.SetExtendedCapabilities (GetExtendedCapabilities ());
      probe.SetHtCapabilities (GetHtCapabilities ());
    }
  if (GetVhtSupported ())
    {
      probe.SetVhtCapabilities (GetVhtCapabilities ());
    }
  if (GetHeSupported ())
    {
      probe.SetHeCapabilities (GetHeCapabilities ());
    }
  if (GetEhtSupported ())
    {
      probe.SetEhtCapabilities (GetEhtCapabilities ());
    }
  packet->AddHeader (probe);

  if (!GetQosSupported ())
    {
      GetTxop ()->Queue (packet, hdr);
    }
  // "A QoS STA that transmits a Management frame determines access category used
  // for medium access in transmission of the Management frame as follows
  // (If dot11QMFActivated is false or not present)
  // — If the Management frame is individually addressed to a non-QoS STA, category
  //   AC_BE should be selected.
  // — If category AC_BE was not selected by the previous step, category AC_VO
  //   shall be selected." (Sec. 10.2.3.2 of 802.11-2020)
  else
    {
      GetVOQueue ()->Queue (packet, hdr);
    }
}

void
StaWifiMac::SendAssociationRequest (bool isReassoc)
{
  NS_LOG_FUNCTION (this << GetBssid (0) << isReassoc);  // TODO use appropriate linkId
  WifiMacHeader hdr;
  hdr.SetType (isReassoc ? WIFI_MAC_MGT_REASSOCIATION_REQUEST : WIFI_MAC_MGT_ASSOCIATION_REQUEST);
  hdr.SetAddr1 (GetBssid (0));  // TODO use appropriate linkId
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid (0));  // TODO use appropriate linkId
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  if (!isReassoc)
    {
      MgtAssocRequestHeader assoc;
      assoc.SetSsid (GetSsid ());
      assoc.SetSupportedRates (GetSupportedRates ());
      assoc.SetCapabilities (GetCapabilities ());
      assoc.SetListenInterval (0);
      if (GetHtSupported ())
        {
          assoc.SetExtendedCapabilities (GetExtendedCapabilities ());
          assoc.SetHtCapabilities (GetHtCapabilities ());
        }
      if (GetVhtSupported ())
        {
          assoc.SetVhtCapabilities (GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          assoc.SetHeCapabilities (GetHeCapabilities ());
        }
      if (GetEhtSupported ())
        {
          assoc.SetEhtCapabilities (GetEhtCapabilities ());
        }
      packet->AddHeader (assoc);
    }
  else
    {
      MgtReassocRequestHeader reassoc;
      reassoc.SetCurrentApAddress (GetBssid (0));  // TODO use appropriate linkId
      reassoc.SetSsid (GetSsid ());
      reassoc.SetSupportedRates (GetSupportedRates ());
      reassoc.SetCapabilities (GetCapabilities ());
      reassoc.SetListenInterval (0);
      if (GetHtSupported ())
        {
          reassoc.SetExtendedCapabilities (GetExtendedCapabilities ());
          reassoc.SetHtCapabilities (GetHtCapabilities ());
        }
      if (GetVhtSupported ())
        {
          reassoc.SetVhtCapabilities (GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          reassoc.SetHeCapabilities (GetHeCapabilities ());
        }
      if (GetEhtSupported ())
        {
          reassoc.SetEhtCapabilities (GetEhtCapabilities ());
        }
      packet->AddHeader (reassoc);
    }

  if (!GetQosSupported ())
    {
      GetTxop ()->Queue (packet, hdr);
    }
  // "A QoS STA that transmits a Management frame determines access category used
  // for medium access in transmission of the Management frame as follows
  // (If dot11QMFActivated is false or not present)
  // — If the Management frame is individually addressed to a non-QoS STA, category
  //   AC_BE should be selected.
  // — If category AC_BE was not selected by the previous step, category AC_VO
  //   shall be selected." (Sec. 10.2.3.2 of 802.11-2020)
  else if (!GetWifiRemoteStationManager ()->GetQosSupported (GetBssid (0)))  // TODO use appropriate linkId
    {
      GetBEQueue ()->Queue (packet, hdr);
    }
  else
    {
      GetVOQueue ()->Queue (packet, hdr);
    }

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                             &StaWifiMac::AssocRequestTimeout, this);
}

void
StaWifiMac::TryToEnsureAssociated (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case ASSOCIATED:
      return;
      break;
    case SCANNING:
      /* we have initiated active or passive scanning, continue to wait
         and gather beacons or probe responses until the scanning timeout
       */
      break;
    case UNASSOCIATED:
      /* we were associated but we missed a bunch of beacons
       * so we should assume we are not associated anymore.
       * We try to initiate a scan now.
       */
      m_linkDown ();
      StartScanning ();
      break;
    case WAIT_ASSOC_RESP:
      /* we have sent an association request so we do not need to
         re-send an association request right now. We just need to
         wait until either assoc-request-timeout or until
         we get an association response.
       */
      break;
    case REFUSED:
      /* we have sent an association request and received a negative
         association response. We wait until someone restarts an
         association with a given SSID.
       */
      break;
    }
}

void
StaWifiMac::StartScanning (void)
{
  NS_LOG_FUNCTION (this);
  SetState (SCANNING);
  NS_ASSERT (m_assocManager);

  WifiScanParams scanParams;
  scanParams.ssid = GetSsid ();
  for (uint8_t linkId = 0; linkId < GetNLinks (); linkId++)
    {
      WifiScanParams::ChannelList channel {(GetWifiPhy (linkId)->HasFixedPhyBand ())
                                           ? WifiScanParams::Channel {0, GetWifiPhy (linkId)->GetPhyBand ()}
                                           : WifiScanParams::Channel {0, WIFI_PHY_BAND_UNSPECIFIED}};

      scanParams.channelList.push_back (channel);
    }
  if (m_activeProbing)
    {
      scanParams.type = WifiScanParams::ACTIVE;
      scanParams.probeDelay = MicroSeconds (m_probeDelay->GetValue ());
      scanParams.minChannelTime = scanParams.maxChannelTime = m_probeRequestTimeout;
    }
  else
    {
      scanParams.type = WifiScanParams::PASSIVE;
      scanParams.maxChannelTime = m_waitBeaconTimeout;
    }

  m_assocManager->StartScanning (std::move (scanParams));
}

void
StaWifiMac::ScanningTimeout (const std::optional<ApInfo>& bestAp)
{
  NS_LOG_FUNCTION (this);

  if (!bestAp.has_value ())
    {
      NS_LOG_DEBUG ("Exhausted list of candidate AP; restart scanning");
      StartScanning ();
      return;
    }

  NS_LOG_DEBUG ("Attempting to associate with AP: " << *bestAp);
  UpdateApInfo (bestAp->m_frame, bestAp->m_apAddr, bestAp->m_bssid, bestAp->m_linkId);
  // lambda to get beacon interval from Beacon or Probe Response
  auto getBeaconInterval =
    [](auto&& frame)
    {
      using T = std::decay_t<decltype (frame)>;
      if constexpr (std::is_same_v<T, MgtBeaconHeader> || std::is_same_v<T, MgtProbeResponseHeader>)
        {
          return MicroSeconds (frame.GetBeaconIntervalUs ());
        }
      else
        {
          NS_ABORT_MSG ("Unexpected frame type");
          return Seconds (0);
        }
    };
  Time beaconInterval = std::visit (getBeaconInterval, bestAp->m_frame);
  Time delay = beaconInterval * m_maxMissedBeacons;
  RestartBeaconWatchdog (delay);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest (false);
}

void
StaWifiMac::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest (false);
}

void
StaWifiMac::MissedBeacons (void)
{
  NS_LOG_FUNCTION (this);
  if (m_beaconWatchdogEnd > Simulator::Now ())
    {
      if (m_beaconWatchdog.IsRunning ())
        {
          m_beaconWatchdog.Cancel ();
        }
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (),
                                              &StaWifiMac::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("beacon missed");
  // We need to switch to the UNASSOCIATED state. However, if we are receiving
  // a frame, wait until the RX is completed (otherwise, crashes may occur if
  // we are receiving a MU frame because its reception requires the STA-ID)
  Time delay = Seconds (0);
  if (GetWifiPhy ()->IsStateRx ())
    {
      delay = GetWifiPhy ()->GetDelayUntilIdle ();
    }
  Simulator::Schedule (delay, &StaWifiMac::Disassociated, this);
}

void
StaWifiMac::Disassociated (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Set state to UNASSOCIATED and start scanning");
  SetState (UNASSOCIATED);
  TryToEnsureAssociated ();
}

void
StaWifiMac::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay
      && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("really restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &StaWifiMac::MissedBeacons, this);
    }
}

bool
StaWifiMac::IsAssociated (void) const
{
  return m_state == ASSOCIATED;
}

bool
StaWifiMac::IsWaitAssocResp (void) const
{
  return m_state == WAIT_ASSOC_RESP;
}

bool
StaWifiMac::CanForwardPacketsTo (Mac48Address to) const
{
  return (IsAssociated ());
}

void
StaWifiMac::Enqueue (Ptr<Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (!CanForwardPacketsTo (to))
    {
      NotifyTxDrop (packet);
      TryToEnsureAssociated ();
      return;
    }
  WifiMacHeader hdr;

  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //For now, an AP that supports QoS does not support non-QoS
  //associations, and vice versa. In future the AP model should
  //support simultaneously associated QoS and non-QoS STAs, at which
  //point there will need to be per-association QoS state maintained
  //by the association state machine, and consulted here.
  if (GetQosSupported ())
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same TXOP is not
      //supported for now
      hdr.SetQosTxopLimit (0);

      //Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetType (WIFI_MAC_DATA);
    }
  if (GetQosSupported ())
    {
      hdr.SetNoOrder (); // explicitly set to 0 for the time being since HT control field is not yet implemented (set it to 1 when implemented)
    }

  hdr.SetAddr1 (GetBssid (0));  // TODO use appropriate linkId
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (to);
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();

  if (GetQosSupported ())
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      GetQosTxop (tid)->Queue (packet, hdr);
    }
  else
    {
      GetTxop ()->Queue (packet, hdr);
    }
}

void
StaWifiMac::Receive (Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << *mpdu << +linkId);
  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<const Packet> packet = mpdu->GetPacket ();
  NS_ASSERT (!hdr->IsCtl ());
  if (hdr->GetAddr3 () == GetAddress ())
    {
      NS_LOG_LOGIC ("packet sent by us.");
      return;
    }
  if (hdr->GetAddr1 () != GetAddress ()
           && !hdr->GetAddr1 ().IsGroup ())
    {
      NS_LOG_LOGIC ("packet is not for us");
      NotifyRxDrop (packet);
      return;
    }
  if (hdr->IsData ())
    {
      if (!IsAssociated ())
        {
          NS_LOG_LOGIC ("Received data frame while not associated: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (!(hdr->IsFromDs () && !hdr->IsToDs ()))
        {
          NS_LOG_LOGIC ("Received data frame not from the DS: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->GetAddr2 () != GetBssid (0))  // TODO use appropriate linkId
        {
          NS_LOG_LOGIC ("Received data frame not from the BSS we are associated with: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->IsQosData ())
        {
          if (hdr->IsQosAmsdu ())
            {
              NS_ASSERT (hdr->GetAddr3 () == GetBssid (0));  // TODO use appropriate linkId
              DeaggregateAmsduAndForward (mpdu);
              packet = 0;
            }
          else
            {
              ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
            }
        }
      else if (hdr->HasData ())
        {
          ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
        }
      return;
    }

  switch (hdr->GetType ())
    {
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
      //This is a frame aimed at an AP, so we can safely ignore it.
      NotifyRxDrop (packet);
      break;

    case WIFI_MAC_MGT_BEACON:
      ReceiveBeacon (mpdu, linkId);
      break;

    case WIFI_MAC_MGT_PROBE_RESPONSE:
      ReceiveProbeResp (mpdu, linkId);
      break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
      ReceiveAssocResp (mpdu, linkId);
      break;

    default:
      //Invoke the receive handler of our parent class to deal with any
      //other frames. Specifically, this will handle Block Ack-related
      //Management Action frames.
      WifiMac::Receive (mpdu, linkId);
    }
}

void
StaWifiMac::ReceiveBeacon (Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << *mpdu << +linkId);
  const WifiMacHeader& hdr = mpdu->GetHeader ();
  NS_ASSERT (hdr.IsBeacon ());

  NS_LOG_DEBUG ("Beacon received");
  MgtBeaconHeader beacon;
  mpdu->GetPacket ()->PeekHeader (beacon);
  const CapabilityInformation& capabilities = beacon.GetCapabilities ();
  NS_ASSERT (capabilities.IsEss ());
  bool goodBeacon;
  if (IsWaitAssocResp () || IsAssociated ())
    {
      // we have to process this Beacon only if sent by the AP we are associated
      // with or from which we are waiting an Association Response frame
      goodBeacon = (hdr.GetAddr3 () == GetBssid (linkId));
    }
  else
    {
      // we retain this Beacon as candidate AP if the supported rates fit the
      // configured BSS membership selector
      goodBeacon = CheckSupportedRates (beacon, linkId);
    }

  if (!goodBeacon)
    {
      NS_LOG_LOGIC ("Beacon is not for us");
      return;
    }
  if (m_state == ASSOCIATED)
    {
      m_beaconArrival (Simulator::Now ());
      Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxMissedBeacons);
      RestartBeaconWatchdog (delay);
      UpdateApInfo (beacon, hdr.GetAddr2 (), hdr.GetAddr3 (), linkId);
    }
  else
    {
      NS_LOG_DEBUG ("Beacon received from " << hdr.GetAddr2 ());
      SnrTag snrTag;
      bool found = mpdu->GetPacket ()->PeekPacketTag (snrTag);
      NS_ASSERT (found);
      m_assocManager->NotifyApInfo (ApInfo {.m_bssid = hdr.GetAddr3 (),
                                            .m_apAddr = hdr.GetAddr2 (),
                                            .m_snr = snrTag.Get (),
                                            .m_frame = std::move (beacon),
                                            .m_channel = {GetCurrentChannel (linkId)},
                                            .m_linkId = linkId});
    }
}

void
StaWifiMac::ReceiveProbeResp (Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << *mpdu << +linkId);
  const WifiMacHeader& hdr = mpdu->GetHeader ();
  NS_ASSERT (hdr.IsProbeResp ());

  NS_LOG_DEBUG ("Probe response received from " << hdr.GetAddr2 ());
  MgtProbeResponseHeader probeResp;
  mpdu->GetPacket ()->PeekHeader (probeResp);
  if (!CheckSupportedRates (probeResp, linkId))
    {
      return;
    }
  SnrTag snrTag;
  bool found = mpdu->GetPacket ()->PeekPacketTag (snrTag);
  NS_ASSERT (found);
  m_assocManager->NotifyApInfo (ApInfo {.m_bssid = hdr.GetAddr3 (),
                                        .m_apAddr = hdr.GetAddr2 (),
                                        .m_snr = snrTag.Get (),
                                        .m_frame = std::move (probeResp),
                                        .m_channel = {GetCurrentChannel (linkId)},
                                        .m_linkId = linkId});
}

void
StaWifiMac::ReceiveAssocResp (Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << *mpdu << +linkId);
  const WifiMacHeader& hdr = mpdu->GetHeader ();
  NS_ASSERT (hdr.IsAssocResp () || hdr.IsReassocResp ());

  if (m_state != WAIT_ASSOC_RESP)
    {
      return;
    }

  MgtAssocResponseHeader assocResp;
  mpdu->GetPacket ()->PeekHeader (assocResp);
  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  if (assocResp.GetStatusCode ().IsSuccess ())
    {
      SetState (ASSOCIATED);
      m_aid = assocResp.GetAssociationId ();
      if (hdr.IsReassocResp ())
        {
          NS_LOG_DEBUG ("reassociation done");
        }
      else
        {
          NS_LOG_DEBUG ("association completed");
        }
      UpdateApInfo (assocResp, hdr.GetAddr2 (), hdr.GetAddr3 (), linkId);
      if (!m_linkUp.IsNull ())
        {
          m_linkUp ();
        }
    }
  else
    {
      NS_LOG_DEBUG ("association refused");
      SetState (REFUSED);
      StartScanning ();
    }
}

bool
StaWifiMac::CheckSupportedRates (std::variant<MgtBeaconHeader, MgtProbeResponseHeader> frame,
                                 uint8_t linkId)
{
  NS_LOG_FUNCTION (this << +linkId);

  // lambda to invoke on the current frame variant
  auto check =
    [&](auto&& mgtFrame) -> bool
    {
      // check supported rates
      const SupportedRates& rates = mgtFrame.GetSupportedRates ();
      for (const auto & selector : GetWifiPhy (linkId)->GetBssMembershipSelectorList ())
        {
          if (!rates.IsBssMembershipSelectorRate (selector))
            {
              NS_LOG_DEBUG ("Supported rates do not fit with the BSS membership selector");
              return false;
            }
        }

      return true;
    };

  return std::visit (check, frame);
}

void
StaWifiMac::UpdateApInfo (const MgtFrameType& frame, const Mac48Address& apAddr,
                          const Mac48Address& bssid, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << frame.index () << apAddr << bssid << +linkId);

  SetBssid (bssid, linkId);

  // lambda processing Information Elements included in all frame types
  auto commonOps =
    [&](auto&& frame)
    {
      const CapabilityInformation& capabilities = frame.GetCapabilities ();
      const SupportedRates& rates = frame.GetSupportedRates ();
      for (const auto & mode : GetWifiPhy (linkId)->GetModeList ())
        {
          if (rates.IsSupportedRate (mode.GetDataRate (GetWifiPhy (linkId)->GetChannelWidth ())))
            {
              GetWifiRemoteStationManager (linkId)->AddSupportedMode (apAddr, mode);
              if (rates.IsBasicRate (mode.GetDataRate (GetWifiPhy (linkId)->GetChannelWidth ())))
                {
                  GetWifiRemoteStationManager (linkId)->AddBasicMode (mode);
                }
            }
        }

      bool isShortPreambleEnabled = capabilities.IsShortPreamble ();
      if (GetErpSupported (linkId))
        {
          const ErpInformation& erpInformation = frame.GetErpInformation ();
          isShortPreambleEnabled &= !erpInformation.GetBarkerPreambleMode ();
          if (erpInformation.GetUseProtection () != 0)
            {
              GetWifiRemoteStationManager (linkId)->SetUseNonErpProtection (true);
            }
          else
            {
              GetWifiRemoteStationManager (linkId)->SetUseNonErpProtection (false);
            }
          if (capabilities.IsShortSlotTime () == true)
            {
              //enable short slot time
              GetWifiPhy (linkId)->SetSlot (MicroSeconds (9));
            }
          else
            {
              //disable short slot time
              GetWifiPhy (linkId)->SetSlot (MicroSeconds (20));
            }
        }
      GetWifiRemoteStationManager (linkId)->SetShortPreambleEnabled (isShortPreambleEnabled);
      GetWifiRemoteStationManager (linkId)->SetShortSlotTimeEnabled (capabilities.IsShortSlotTime ());

      if (!GetQosSupported ()) return;
      /* QoS station */
      bool qosSupported = false;
      const EdcaParameterSet& edcaParameters = frame.GetEdcaParameterSet ();
      if (edcaParameters.IsQosSupported ())
        {
          qosSupported = true;
          //The value of the TXOP Limit field is specified as an unsigned integer, with the least significant octet transmitted first, in units of 32 μs.
          SetEdcaParameters (AC_BE, edcaParameters.GetBeCWmin (), edcaParameters.GetBeCWmax (), edcaParameters.GetBeAifsn (), 32 * MicroSeconds (edcaParameters.GetBeTxopLimit ()));
          SetEdcaParameters (AC_BK, edcaParameters.GetBkCWmin (), edcaParameters.GetBkCWmax (), edcaParameters.GetBkAifsn (), 32 * MicroSeconds (edcaParameters.GetBkTxopLimit ()));
          SetEdcaParameters (AC_VI, edcaParameters.GetViCWmin (), edcaParameters.GetViCWmax (), edcaParameters.GetViAifsn (), 32 * MicroSeconds (edcaParameters.GetViTxopLimit ()));
          SetEdcaParameters (AC_VO, edcaParameters.GetVoCWmin (), edcaParameters.GetVoCWmax (), edcaParameters.GetVoAifsn (), 32 * MicroSeconds (edcaParameters.GetVoTxopLimit ()));
        }
      GetWifiRemoteStationManager (linkId)->SetQosSupport (apAddr, qosSupported);

      if (!GetHtSupported ()) return;
      /* HT station */
      const HtCapabilities& htCapabilities = frame.GetHtCapabilities ();
      if (!htCapabilities.IsSupportedMcs (0))
        {
          GetWifiRemoteStationManager (linkId)->RemoveAllSupportedMcs (apAddr);
        }
      else
        {
          GetWifiRemoteStationManager (linkId)->AddStationHtCapabilities (apAddr, htCapabilities);
        }
      // TODO: process ExtendedCapabilities
      // ExtendedCapabilities extendedCapabilities = frame.GetExtendedCapabilities ();

      // we do not return if VHT is not supported because HE STAs operating in
      // the 2.4 GHz band do not support VHT
      if (GetVhtSupported ())
        {
          const VhtCapabilities& vhtCapabilities = frame.GetVhtCapabilities ();
          //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
          if (vhtCapabilities.GetRxHighestSupportedLgiDataRate () > 0)
            {
              GetWifiRemoteStationManager (linkId)->AddStationVhtCapabilities (apAddr, vhtCapabilities);
              VhtOperation vhtOperation = frame.GetVhtOperation ();
              for (const auto & mcs : GetWifiPhy (linkId)->GetMcsList (WIFI_MOD_CLASS_VHT))
                {
                  if (vhtCapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                    {
                      GetWifiRemoteStationManager (linkId)->AddSupportedMcs (apAddr, mcs);
                    }
                }
            }
        }

      if (!GetHeSupported ()) return;
      /* HE station */
      const HeCapabilities& heCapabilities = frame.GetHeCapabilities ();
      if (heCapabilities.GetSupportedMcsAndNss () != 0)
        {
          GetWifiRemoteStationManager (linkId)->AddStationHeCapabilities (apAddr, heCapabilities);
          HeOperation heOperation = frame.GetHeOperation ();
          for (const auto & mcs : GetWifiPhy (linkId)->GetMcsList (WIFI_MOD_CLASS_HE))
            {
              if (heCapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                {
                  GetWifiRemoteStationManager (linkId)->AddSupportedMcs (apAddr, mcs);
                }
            }
          GetHeConfiguration ()->SetAttribute ("BssColor", UintegerValue (heOperation.GetBssColor ()));
        }

      const MuEdcaParameterSet& muEdcaParameters = frame.GetMuEdcaParameterSet ();
      if (muEdcaParameters.IsPresent ())
        {
          SetMuEdcaParameters (AC_BE, muEdcaParameters.GetMuCwMin (AC_BE), muEdcaParameters.GetMuCwMax (AC_BE),
                              muEdcaParameters.GetMuAifsn (AC_BE), muEdcaParameters.GetMuEdcaTimer (AC_BE));
          SetMuEdcaParameters (AC_BK, muEdcaParameters.GetMuCwMin (AC_BK), muEdcaParameters.GetMuCwMax (AC_BK),
                              muEdcaParameters.GetMuAifsn (AC_BK), muEdcaParameters.GetMuEdcaTimer (AC_BK));
          SetMuEdcaParameters (AC_VI, muEdcaParameters.GetMuCwMin (AC_VI), muEdcaParameters.GetMuCwMax (AC_VI),
                              muEdcaParameters.GetMuAifsn (AC_VI), muEdcaParameters.GetMuEdcaTimer (AC_VI));
          SetMuEdcaParameters (AC_VO, muEdcaParameters.GetMuCwMin (AC_VO), muEdcaParameters.GetMuCwMax (AC_VO),
                              muEdcaParameters.GetMuAifsn (AC_VO), muEdcaParameters.GetMuEdcaTimer (AC_VO));
        }

      if (!GetEhtSupported ()) return;
      /* EHT station */
      const EhtCapabilities& ehtCapabilities = frame.GetEhtCapabilities ();
      //TODO: once we support non constant rate managers, we should add checks here whether EHT is supported by the peer
      GetWifiRemoteStationManager (linkId)->AddStationEhtCapabilities (apAddr, ehtCapabilities);
    };

  // process Information Elements included in the current frame variant
  std::visit (commonOps, frame);
}

SupportedRates
StaWifiMac::GetSupportedRates (void) const
{
  SupportedRates rates;
  for (const auto & mode : GetWifiPhy ()->GetModeList ())
    {
      uint64_t modeDataRate = mode.GetDataRate (GetWifiPhy ()->GetChannelWidth ());
      NS_LOG_DEBUG ("Adding supported rate of " << modeDataRate);
      rates.AddSupportedRate (modeDataRate);
    }
  if (GetHtSupported ())
    {
      for (const auto & selector : GetWifiPhy ()->GetBssMembershipSelectorList ())
        {
          rates.AddBssMembershipSelectorRate (selector);
        }
    }
  return rates;
}

CapabilityInformation
StaWifiMac::GetCapabilities (void) const
{
  CapabilityInformation capabilities;
  capabilities.SetShortPreamble (GetWifiPhy ()->GetShortPhyPreambleSupported () || GetErpSupported (SINGLE_LINK_OP_ID));
  capabilities.SetShortSlotTime (GetShortSlotTimeSupported () && GetErpSupported (SINGLE_LINK_OP_ID));
  return capabilities;
}

void
StaWifiMac::SetState (MacState value)
{
  if (value == ASSOCIATED
      && m_state != ASSOCIATED)
    {
      m_assocLogger (GetBssid (0));  // TODO use appropriate linkId
    }
  else if (value != ASSOCIATED
           && m_state == ASSOCIATED)
    {
      m_deAssocLogger (GetBssid (0));  // TODO use appropriate linkId
    }
  m_state = value;
}

void
StaWifiMac::SetEdcaParameters (AcIndex ac, uint32_t cwMin, uint32_t cwMax, uint8_t aifsn, Time txopLimit)
{
  Ptr<QosTxop> edca = GetQosTxop (ac);
  edca->SetMinCw (cwMin, SINGLE_LINK_OP_ID);
  edca->SetMaxCw (cwMax, SINGLE_LINK_OP_ID);
  edca->SetAifsn (aifsn, SINGLE_LINK_OP_ID);
  edca->SetTxopLimit (txopLimit, SINGLE_LINK_OP_ID);
}

void
StaWifiMac::SetMuEdcaParameters (AcIndex ac, uint16_t cwMin, uint16_t cwMax, uint8_t aifsn, Time muEdcaTimer)
{
  Ptr<QosTxop> edca = GetQosTxop (ac);
  edca->SetMuCwMin (cwMin, SINGLE_LINK_OP_ID);
  edca->SetMuCwMax (cwMax, SINGLE_LINK_OP_ID);
  edca->SetMuAifsn (aifsn, SINGLE_LINK_OP_ID);
  edca->SetMuEdcaTimer (muEdcaTimer, SINGLE_LINK_OP_ID);
}

void
StaWifiMac::PhyCapabilitiesChanged (void)
{
  NS_LOG_FUNCTION (this);
  if (IsAssociated ())
    {
      NS_LOG_DEBUG ("PHY capabilities changed: send reassociation request");
      SetState (WAIT_ASSOC_RESP);
      SendAssociationRequest (true);
    }
}

void
StaWifiMac::NotifyChannelSwitching (void)
{
  NS_LOG_FUNCTION (this);

  WifiMac::NotifyChannelSwitching ();

  if (IsInitialized ())
    {
      Disassociated ();
    }
}

std::ostream & operator << (std::ostream &os, const StaWifiMac::ApInfo& apInfo)
{
  os << "BSSID=" << apInfo.m_bssid
     << ", AP addr=" << apInfo.m_apAddr
     << ", SNR=" << apInfo.m_snr
     << ", Channel={" << apInfo.m_channel.number << "," << apInfo.m_channel.band
     << "}, Link ID=" << +apInfo.m_linkId
     << ", Frame=[";
  std::visit ([&os](auto&& frame){frame.Print (os);}, apInfo.m_frame);
  os << "]";
  return os;
}

} //namespace ns3
