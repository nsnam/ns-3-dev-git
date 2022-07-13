/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
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

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ap-wifi-mac.h"
#include "channel-access-manager.h"
#include "frame-exchange-manager.h"
#include "mac-tx-middle.h"
#include "mac-rx-middle.h"
#include "mgt-headers.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "wifi-phy.h"
#include "wifi-net-device.h"
#include "wifi-mac-queue.h"
#include "ns3/ht-configuration.h"
#include "ns3/he-configuration.h"
#include "qos-txop.h"
#include "reduced-neighbor-report.h"
#include "ns3/multi-link-element.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (ApWifiMac);

TypeId
ApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ApWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ApWifiMac> ()
    .AddAttribute ("BeaconInterval",
                   "Delay between two beacons",
                   TimeValue (MicroSeconds (102400)),
                   MakeTimeAccessor (&ApWifiMac::GetBeaconInterval,
                                     &ApWifiMac::SetBeaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("BeaconJitter",
                   "A uniform random variable to cause the initial beacon starting time (after simulation time 0) "
                   "to be distributed between 0 and the BeaconInterval.",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&ApWifiMac::m_beaconJitter),
                   MakePointerChecker<UniformRandomVariable> ())
    .AddAttribute ("EnableBeaconJitter",
                   "If beacons are enabled, whether to jitter the initial send event.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_enableBeaconJitter),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconGeneration",
                   "Whether or not beacons are generated.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::SetBeaconGeneration),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonErpProtection", "Whether or not protection mechanism should be used when non-ERP STAs are present within the BSS."
                   "This parameter is only used when ERP is supported by the AP.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::m_enableNonErpProtection),
                   MakeBooleanChecker ())
    .AddAttribute ("BsrLifetime",
                   "Lifetime of Buffer Status Reports received from stations.",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&ApWifiMac::m_bsrLifetime),
                   MakeTimeChecker ())
    .AddTraceSource ("AssociatedSta",
                     "A station associated with this access point.",
                     MakeTraceSourceAccessor (&ApWifiMac::m_assocLogger),
                     "ns3::ApWifiMac::AssociationCallback")
    .AddTraceSource ("DeAssociatedSta",
                     "A station lost association with this access point.",
                     MakeTraceSourceAccessor (&ApWifiMac::m_deAssocLogger),
                     "ns3::ApWifiMac::AssociationCallback")
  ;
  return tid;
}

ApWifiMac::ApWifiMac ()
  : m_enableBeaconGeneration (false),
    m_numNonErpStations (0),
    m_numNonHtStations (0),
    m_shortSlotTimeEnabled (false),
    m_shortPreambleEnabled (false)
{
  NS_LOG_FUNCTION (this);
  m_beaconTxop = CreateObject<Txop> (CreateObject<WifiMacQueue> (AC_BEACON));
  m_beaconTxop->SetTxMiddle (m_txMiddle);

  //Let the lower layers know that we are acting as an AP.
  SetTypeOfStation (AP);
}

ApWifiMac::~ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
ApWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_beaconTxop->Dispose ();
  m_beaconTxop = 0;
  m_enableBeaconGeneration = false;
  WifiMac::DoDispose ();
}

ApWifiMac::ApLinkEntity::~ApLinkEntity ()
{
  NS_LOG_FUNCTION_NOARGS ();
  beaconEvent.Cancel ();
}

std::unique_ptr<WifiMac::LinkEntity>
ApWifiMac::CreateLinkEntity (void) const
{
  return std::make_unique<ApLinkEntity> ();
}

ApWifiMac::ApLinkEntity&
ApWifiMac::GetLink (uint8_t linkId) const
{
  return static_cast<ApLinkEntity&> (WifiMac::GetLink (linkId));
}

void
ApWifiMac::ConfigureStandard (WifiStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  WifiMac::ConfigureStandard (standard);
  m_beaconTxop->SetWifiMac (this);
  m_beaconTxop->SetAifsns (std::vector<uint8_t> (GetNLinks (), 1));
  m_beaconTxop->SetMinCws (std::vector<uint32_t> (GetNLinks (), 0));
  m_beaconTxop->SetMaxCws (std::vector<uint32_t> (GetNLinks (), 0));
  for (uint8_t linkId = 0; linkId < GetNLinks (); linkId++)
    {
      GetLink (linkId).channelAccessManager->Add (m_beaconTxop);
    }
}

Ptr<WifiMacQueue>
ApWifiMac::GetTxopQueue (AcIndex ac) const
{
  if (ac == AC_BEACON)
    {
      return m_beaconTxop->GetWifiMacQueue ();
    }
  return WifiMac::GetTxopQueue (ac);
}

void
ApWifiMac::SetBeaconGeneration (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  for (uint8_t linkId = 0; linkId < GetNLinks (); ++linkId)
    {
      if (!enable)
        {
          GetLink (linkId).beaconEvent.Cancel ();
        }
      else if (!m_enableBeaconGeneration)
        {
          GetLink (linkId).beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon,
                                                                 this, linkId);
        }
    }
  m_enableBeaconGeneration = enable;
}

Time
ApWifiMac::GetBeaconInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}

void
ApWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  WifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of an AP,
  //the link is always up, so we immediately invoke the callback if
  //one is set
  linkUp ();
}

void
ApWifiMac::SetBeaconInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  if ((interval.GetMicroSeconds () % 1024) != 0)
    {
      NS_FATAL_ERROR ("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
    }
  if (interval.GetMicroSeconds () > (1024 * 65535))
    {
      NS_FATAL_ERROR ("beacon interval should be smaller then or equal to 65535 * 1024us (802.11 time unit)");
    }
  m_beaconInterval = interval;
}

int64_t
ApWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconJitter->SetStream (stream);
  return 1;
}

void
ApWifiMac::UpdateShortSlotTimeEnabled (void)
{
  NS_LOG_FUNCTION (this);
  if (GetErpSupported (SINGLE_LINK_OP_ID) && GetShortSlotTimeSupported () && (m_numNonErpStations == 0))
    {
      for (const auto& sta : GetLink (SINGLE_LINK_OP_ID).staList)
        {
          if (!GetWifiRemoteStationManager ()->GetShortSlotTimeSupported (sta.second))
            {
              m_shortSlotTimeEnabled = false;
              return;
            }
        }
      m_shortSlotTimeEnabled = true;
    }
  else
    {
      m_shortSlotTimeEnabled = false;
    }
}

void
ApWifiMac::UpdateShortPreambleEnabled (void)
{
  NS_LOG_FUNCTION (this);
  if (GetErpSupported (SINGLE_LINK_OP_ID) && GetWifiPhy ()->GetShortPhyPreambleSupported ())
    {
      for (const auto& sta : GetLink (SINGLE_LINK_OP_ID).staList)
        {
          if (!GetWifiRemoteStationManager ()->GetErpOfdmSupported (sta.second) ||
              !GetWifiRemoteStationManager ()->GetShortPreambleSupported (sta.second))
            {
              m_shortPreambleEnabled = false;
              return;
            }
        }
      m_shortPreambleEnabled = true;
    }
  else
    {
      m_shortPreambleEnabled = false;
    }
}

void
ApWifiMac::ForwardDown (Ptr<Packet> packet, Mac48Address from,
                        Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //If we are a QoS AP then we attempt to get a TID for this packet
  if (GetQosSupported ())
    {
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
    }

  ForwardDown (packet, from, to, tid);
}

void
ApWifiMac::ForwardDown (Ptr<Packet> packet, Mac48Address from,
                        Mac48Address to, uint8_t tid)
{
  NS_LOG_FUNCTION (this << packet << from << to << +tid);
  WifiMacHeader hdr;

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
      //Transmission of multiple frames in the same Polled TXOP is not supported for now
      hdr.SetQosTxopLimit (0);
      //Fill in the QoS control field in the MAC header
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
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();

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

bool
ApWifiMac::CanForwardPacketsTo (Mac48Address to) const
{
  return (to.IsGroup () || GetWifiRemoteStationManager ()->IsAssociated (to));
}

void
ApWifiMac::Enqueue (Ptr<Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  if (CanForwardPacketsTo (to))
    {
      ForwardDown (packet, from, to);
    }
  else
    {
      NotifyTxDrop (packet);
    }
}

void
ApWifiMac::Enqueue (Ptr<Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //We're sending this packet with a from address that is our own. We
  //get that address from the lower MAC and make use of the
  //from-spoofing Enqueue() method to avoid duplicated code.
  Enqueue (packet, to, GetAddress ());
}

bool
ApWifiMac::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

SupportedRates
ApWifiMac::GetSupportedRates (void) const
{
  NS_LOG_FUNCTION (this);
  SupportedRates rates;
  //Send the set of supported rates and make sure that we indicate
  //the Basic Rate set in this set of supported rates.
  for (const auto & mode : GetWifiPhy ()->GetModeList ())
    {
      uint64_t modeDataRate = mode.GetDataRate (GetWifiPhy ()->GetChannelWidth ());
      NS_LOG_DEBUG ("Adding supported rate of " << modeDataRate);
      rates.AddSupportedRate (modeDataRate);
      //Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
      //here we choose to add the mandatory rates to the BSSBasicRateSet,
      //except for 802.11b where we assume that only the non HR-DSSS rates are part of the BSSBasicRateSet
      if (mode.IsMandatory () && (mode.GetModulationClass () != WIFI_MOD_CLASS_HR_DSSS))
        {
          NS_LOG_DEBUG ("Adding basic mode " << mode.GetUniqueName ());
          GetWifiRemoteStationManager ()->AddBasicMode (mode);
        }
    }
  //set the basic rates
  for (uint8_t j = 0; j < GetWifiRemoteStationManager ()->GetNBasicModes (); j++)
    {
      WifiMode mode = GetWifiRemoteStationManager ()->GetBasicMode (j);
      uint64_t modeDataRate = mode.GetDataRate (GetWifiPhy ()->GetChannelWidth ());
      NS_LOG_DEBUG ("Setting basic rate " << mode.GetUniqueName ());
      rates.SetBasicRate (modeDataRate);
    }
  //If it is a HT AP, then add the BSSMembershipSelectorSet
  //The standard says that the BSSMembershipSelectorSet
  //must have its MSB set to 1 (must be treated as a Basic Rate)
  //Also the standard mentioned that at least 1 element should be included in the SupportedRates the rest can be in the ExtendedSupportedRates
  if (GetHtSupported ())
    {
      for (const auto & selector : GetWifiPhy ()->GetBssMembershipSelectorList ())
        {
          rates.AddBssMembershipSelectorRate (selector);
        }
    }
  return rates;
}

DsssParameterSet
ApWifiMac::GetDsssParameterSet (uint8_t linkId) const
{
  NS_LOG_FUNCTION (this << +linkId);
  NS_ASSERT (GetDsssSupported (linkId));
  DsssParameterSet dsssParameters;
  dsssParameters.SetCurrentChannel (GetWifiPhy (linkId)->GetChannelNumber ());
  return dsssParameters;
}

CapabilityInformation
ApWifiMac::GetCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  CapabilityInformation capabilities;
  capabilities.SetShortPreamble (m_shortPreambleEnabled);
  capabilities.SetShortSlotTime (m_shortSlotTimeEnabled);
  capabilities.SetEss ();
  return capabilities;
}

ErpInformation
ApWifiMac::GetErpInformation (uint8_t linkId) const
{
  NS_LOG_FUNCTION (this << +linkId);
  NS_ASSERT (GetErpSupported (linkId));
  ErpInformation information;

  information.SetNonErpPresent (m_numNonErpStations > 0);
  information.SetUseProtection (GetUseNonErpProtection ());
  if (m_shortPreambleEnabled)
    {
      information.SetBarkerPreambleMode (0);
    }
  else
    {
      information.SetBarkerPreambleMode (1);
    }

  return information;
}

EdcaParameterSet
ApWifiMac::GetEdcaParameterSet (uint8_t linkId) const
{
  NS_LOG_FUNCTION (this << +linkId);
  NS_ASSERT (GetQosSupported ());
  EdcaParameterSet edcaParameters;

  Ptr<QosTxop> edca;
  Time txopLimit;

  edca = GetQosTxop (AC_BE);
  txopLimit = edca->GetTxopLimit (linkId);
  edcaParameters.SetBeAci (0);
  edcaParameters.SetBeCWmin (edca->GetMinCw (linkId));
  edcaParameters.SetBeCWmax (edca->GetMaxCw (linkId));
  edcaParameters.SetBeAifsn (edca->GetAifsn (linkId));
  edcaParameters.SetBeTxopLimit (static_cast<uint16_t> (txopLimit.GetMicroSeconds () / 32));

  edca = GetQosTxop (AC_BK);
  txopLimit = edca->GetTxopLimit (linkId);
  edcaParameters.SetBkAci (1);
  edcaParameters.SetBkCWmin (edca->GetMinCw (linkId));
  edcaParameters.SetBkCWmax (edca->GetMaxCw (linkId));
  edcaParameters.SetBkAifsn (edca->GetAifsn (linkId));
  edcaParameters.SetBkTxopLimit (static_cast<uint16_t> (txopLimit.GetMicroSeconds () / 32));

  edca = GetQosTxop (AC_VI);
  txopLimit = edca->GetTxopLimit (linkId);
  edcaParameters.SetViAci (2);
  edcaParameters.SetViCWmin (edca->GetMinCw (linkId));
  edcaParameters.SetViCWmax (edca->GetMaxCw (linkId));
  edcaParameters.SetViAifsn (edca->GetAifsn (linkId));
  edcaParameters.SetViTxopLimit (static_cast<uint16_t> (txopLimit.GetMicroSeconds () / 32));

  edca = GetQosTxop (AC_VO);
  txopLimit = edca->GetTxopLimit (linkId);
  edcaParameters.SetVoAci (3);
  edcaParameters.SetVoCWmin (edca->GetMinCw (linkId));
  edcaParameters.SetVoCWmax (edca->GetMaxCw (linkId));
  edcaParameters.SetVoAifsn (edca->GetAifsn (linkId));
  edcaParameters.SetVoTxopLimit (static_cast<uint16_t> (txopLimit.GetMicroSeconds () / 32));

  edcaParameters.SetQosInfo (0);

  return edcaParameters;
}

std::optional<MuEdcaParameterSet>
ApWifiMac::GetMuEdcaParameterSet (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetHeSupported ());

  Ptr<HeConfiguration> heConfiguration = GetHeConfiguration ();
  NS_ASSERT (heConfiguration);

  MuEdcaParameterSet muEdcaParameters;
  muEdcaParameters.SetQosInfo (0);

  UintegerValue uintegerValue;
  TimeValue timeValue;

  heConfiguration->GetAttribute ("MuBeAifsn", uintegerValue);
  muEdcaParameters.SetMuAifsn (AC_BE, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuBeCwMin", uintegerValue);
  muEdcaParameters.SetMuCwMin (AC_BE, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuBeCwMax", uintegerValue);
  muEdcaParameters.SetMuCwMax (AC_BE, uintegerValue.Get ());
  heConfiguration->GetAttribute ("BeMuEdcaTimer", timeValue);
  muEdcaParameters.SetMuEdcaTimer (AC_BE, timeValue.Get ());

  heConfiguration->GetAttribute ("MuBkAifsn", uintegerValue);
  muEdcaParameters.SetMuAifsn (AC_BK, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuBkCwMin", uintegerValue);
  muEdcaParameters.SetMuCwMin (AC_BK, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuBkCwMax", uintegerValue);
  muEdcaParameters.SetMuCwMax (AC_BK, uintegerValue.Get ());
  heConfiguration->GetAttribute ("BkMuEdcaTimer", timeValue);
  muEdcaParameters.SetMuEdcaTimer (AC_BK, timeValue.Get ());

  heConfiguration->GetAttribute ("MuViAifsn", uintegerValue);
  muEdcaParameters.SetMuAifsn (AC_VI, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuViCwMin", uintegerValue);
  muEdcaParameters.SetMuCwMin (AC_VI, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuViCwMax", uintegerValue);
  muEdcaParameters.SetMuCwMax (AC_VI, uintegerValue.Get ());
  heConfiguration->GetAttribute ("ViMuEdcaTimer", timeValue);
  muEdcaParameters.SetMuEdcaTimer (AC_VI, timeValue.Get ());

  heConfiguration->GetAttribute ("MuVoAifsn", uintegerValue);
  muEdcaParameters.SetMuAifsn (AC_VO, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuVoCwMin", uintegerValue);
  muEdcaParameters.SetMuCwMin (AC_VO, uintegerValue.Get ());
  heConfiguration->GetAttribute ("MuVoCwMax", uintegerValue);
  muEdcaParameters.SetMuCwMax (AC_VO, uintegerValue.Get ());
  heConfiguration->GetAttribute ("VoMuEdcaTimer", timeValue);
  muEdcaParameters.SetMuEdcaTimer (AC_VO, timeValue.Get ());

  // The timers of the MU EDCA Parameter Set must be either all zero or all
  // non-zero. The information element is advertised if all timers are non-zero
  auto timerNotNull = [&muEdcaParameters](uint8_t aci)
                      { return !muEdcaParameters.GetMuEdcaTimer (aci).IsZero (); };
  auto aci = {0, 1, 2, 3};
  if (std::all_of (aci.begin (), aci.end (), timerNotNull))
    {
      return muEdcaParameters;
    }

  NS_ABORT_MSG_UNLESS (std::none_of (aci.begin (), aci.end (), timerNotNull),
                      "MU EDCA Timers must be all zero if the IE is not advertised.");

  return std::nullopt;
}

std::optional<ReducedNeighborReport>
ApWifiMac::GetReducedNeighborReport (uint8_t linkId) const
{
  NS_LOG_FUNCTION (this << +linkId);

  if (GetNLinks () <= 1)
    {
      return std::nullopt;
    }

  NS_ABORT_IF (!GetEhtSupported ());
  ReducedNeighborReport rnr;

  for (uint8_t index = 0; index < GetNLinks (); ++index)
    {
      if (index != linkId)  // all links but the one used to send this Beacon frame
        {
          rnr.AddNbrApInfoField ();
          std::size_t nbrId = rnr.GetNNbrApInfoFields () - 1;
          rnr.SetOperatingChannel (nbrId, GetLink (index).phy->GetOperatingChannel ());
          rnr.AddTbttInformationField (nbrId);
          rnr.SetBssid (nbrId, 0, GetLink (index).feManager->GetAddress ());
          rnr.SetShortSsid (nbrId, 0, 0);
          rnr.SetBssParameters (nbrId, 0, 0);
          rnr.SetPsd20MHz (nbrId, 0, 0);
          rnr.SetMldParameters (nbrId, 0, 0, index, 0);
        }
    }
  return rnr;
}

MultiLinkElement
ApWifiMac::GetMultiLinkElement (uint8_t linkId, WifiMacType frameType, const Mac48Address& to)
{
  NS_LOG_FUNCTION (this << +linkId << frameType << to);
  NS_ABORT_IF (GetNLinks () == 1);

  MultiLinkElement mle (MultiLinkElement::BASIC_VARIANT, frameType);
  mle.SetMldMacAddress (GetAddress ());
  mle.SetLinkIdInfo (linkId);
  mle.SetBssParamsChangeCount (0);

  // if the Multi-Link Element is being inserted in a (Re)Association Response frame
  // and the remote station is affiliated with an MLD, try multi-link setup
  if (auto staMldAddress = GetWifiRemoteStationManager (linkId)->GetMldAddress (to);
      (frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE
       || frameType == WIFI_MAC_MGT_REASSOCIATION_RESPONSE)
      && staMldAddress.has_value ())
    {
      for (uint8_t i = 0; i < GetNLinks (); i++)
        {
          auto remoteStationManager = GetWifiRemoteStationManager (i);
          if (auto staAddress = remoteStationManager->GetAffiliatedStaAddress (*staMldAddress);
              i != linkId
              && staAddress.has_value ()
              && (remoteStationManager->IsWaitAssocTxOk (*staAddress)
                  || remoteStationManager->IsAssocRefused (*staAddress)))
            {
              // For each requested link in addition to the link on which the
              // (Re)Association Response frame is transmitted, the Link Info field
              // of the Basic Multi-Link element carried in the (Re)Association
              // Response frame shall contain the corresponding Per-STA Profile
              // subelement(s) (Sec. 35.3.5.4 of 802.11be D2.0)
              mle.AddPerStaProfileSubelement ();
              auto& perStaProfile = mle.GetPerStaProfile (mle.GetNPerStaProfileSubelements () - 1);
              // The Link ID subfield of the STA Control field of the Per-STA Profile
              // subelement for the AP corresponding to a link is set to the link ID
              // of the AP affiliated with the AP MLD that is operating on that link.
              perStaProfile.SetLinkId (i);
              perStaProfile.SetCompleteProfile ();
              // For each Per-STA Profile subelement included in the Link Info field,
              // the Complete Profile subfield of the STA Control field shall be set to 1
              perStaProfile.SetStaMacAddress (GetFrameExchangeManager (i)->GetAddress ());
              perStaProfile.SetAssocResponse (GetAssocResp (*staAddress,
                                                            frameType == WIFI_MAC_MGT_REASSOCIATION_RESPONSE, i));
            }
        }
    }

  return mle;
}

HtOperation
ApWifiMac::GetHtOperation (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetHtSupported ());
  HtOperation operation;

  operation.SetPrimaryChannel (GetWifiPhy ()->GetPrimaryChannelNumber (20));
  operation.SetRifsMode (false);
  operation.SetNonGfHtStasPresent (true);
  if (GetWifiPhy ()->GetChannelWidth () > 20)
    {
      operation.SetSecondaryChannelOffset (1);
      operation.SetStaChannelWidth (1);
    }
  if (m_numNonHtStations == 0)
    {
      operation.SetHtProtection (NO_PROTECTION);
    }
  else
    {
      operation.SetHtProtection (MIXED_MODE_PROTECTION);
    }
  uint64_t maxSupportedRate = 0; //in bit/s
  for (const auto & mcs : GetWifiPhy ()->GetMcsList (WIFI_MOD_CLASS_HT))
    {
      uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
      NS_ASSERT (nss > 0 && nss < 5);
      uint64_t dataRate = mcs.GetDataRate (GetWifiPhy ()->GetChannelWidth (), GetHtConfiguration ()->GetShortGuardIntervalSupported () ? 400 : 800, nss);
      if (dataRate > maxSupportedRate)
        {
          maxSupportedRate = dataRate;
          NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
        }
    }
  uint8_t maxSpatialStream = GetWifiPhy ()->GetMaxSupportedTxSpatialStreams ();
  auto mcsList = GetWifiPhy ()->GetMcsList (WIFI_MOD_CLASS_HT);
  uint8_t nMcs = mcsList.size ();
  for (const auto& sta : GetLink (SINGLE_LINK_OP_ID).staList)
    {
      if (GetWifiRemoteStationManager ()->GetHtSupported (sta.second))
        {
          uint64_t maxSupportedRateByHtSta = 0; //in bit/s
          auto itMcs = mcsList.begin ();
          for (uint8_t j = 0; j < (std::min (nMcs, GetWifiRemoteStationManager ()->GetNMcsSupported (sta.second))); j++)
            {
              WifiMode mcs = *itMcs++;
              uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
              NS_ASSERT (nss > 0 && nss < 5);
              uint64_t dataRate = mcs.GetDataRate (GetWifiRemoteStationManager ()->GetChannelWidthSupported (sta.second),
                                                    GetWifiRemoteStationManager ()->GetShortGuardIntervalSupported (sta.second) ? 400 : 800, nss);
              if (dataRate > maxSupportedRateByHtSta)
                {
                  maxSupportedRateByHtSta = dataRate;
                }
            }
          if (maxSupportedRateByHtSta < maxSupportedRate)
            {
              maxSupportedRate = maxSupportedRateByHtSta;
            }
          if (GetWifiRemoteStationManager ()->GetNMcsSupported (sta.second) < nMcs)
            {
              nMcs = GetWifiRemoteStationManager ()->GetNMcsSupported (sta.second);
            }
          if (GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second) < maxSpatialStream)
            {
              maxSpatialStream = GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second);
            }
        }
    }
  operation.SetRxHighestSupportedDataRate (static_cast<uint16_t> (maxSupportedRate / 1e6)); //in Mbit/s
  operation.SetTxMcsSetDefined (nMcs > 0);
  operation.SetTxMaxNSpatialStreams (maxSpatialStream);
  //To be filled in once supported
  operation.SetObssNonHtStasPresent (0);
  operation.SetDualBeacon (0);
  operation.SetDualCtsProtection (0);
  operation.SetStbcBeacon (0);
  operation.SetLSigTxopProtectionFullSupport (0);
  operation.SetPcoActive (0);
  operation.SetPhase (0);
  operation.SetRxMcsBitmask (0);
  operation.SetTxRxMcsSetUnequal (0);
  operation.SetTxUnequalModulation (0);

  return operation;
}

VhtOperation
ApWifiMac::GetVhtOperation (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetVhtSupported ());
  VhtOperation operation;

  const uint16_t bssBandwidth = GetWifiPhy ()->GetChannelWidth ();
  // Set to 0 for 20 MHz or 40 MHz BSS bandwidth.
  // Set to 1 for 80 MHz, 160 MHz or 80+80 MHz BSS bandwidth.
  operation.SetChannelWidth ((bssBandwidth > 40) ? 1 : 0);
  // For 20, 40, or 80 MHz BSS bandwidth, indicates the channel center frequency
  // index for the 20, 40, or 80 MHz channel on which the VHT BSS operates.
  // For 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
  // indicates the channel center frequency index of the 80 MHz channel
  // segment that contains the primary channel.
  operation.SetChannelCenterFrequencySegment0 ((bssBandwidth == 160) ?
                                                GetWifiPhy ()->GetOperatingChannel ().GetPrimaryChannelNumber (80, WIFI_STANDARD_80211ac) :
                                                GetWifiPhy ()->GetChannelNumber ());
  // For a 20, 40, or 80 MHz BSS bandwidth, this subfield is set to 0.
  // For a 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
  // indicates the channel center frequency index of the 160 MHz channel on
  // which the VHT BSS operates.
  operation.SetChannelCenterFrequencySegment1 ((bssBandwidth == 160) ? GetWifiPhy ()->GetChannelNumber () : 0);
  uint8_t maxSpatialStream = GetWifiPhy ()->GetMaxSupportedRxSpatialStreams ();
  for (const auto& sta : GetLink (SINGLE_LINK_OP_ID).staList)
    {
      if (GetWifiRemoteStationManager ()->GetVhtSupported (sta.second))
        {
          if (GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second) < maxSpatialStream)
            {
              maxSpatialStream = GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second);
            }
        }
    }
  for (uint8_t nss = 1; nss <= maxSpatialStream; nss++)
    {
      uint8_t maxMcs = 9; //TBD: hardcode to 9 for now since we assume all MCS values are supported
      operation.SetMaxVhtMcsPerNss (nss, maxMcs);
    }

  return operation;
}

HeOperation
ApWifiMac::GetHeOperation (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetHeSupported ());
  HeOperation operation;

  uint8_t maxSpatialStream = GetWifiPhy ()->GetMaxSupportedRxSpatialStreams ();
  for (const auto& sta : GetLink (SINGLE_LINK_OP_ID).staList)
    {
      if (GetWifiRemoteStationManager ()->GetHeSupported (sta.second))
        {
          if (GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second) < maxSpatialStream)
            {
              maxSpatialStream = GetWifiRemoteStationManager ()->GetNumberOfSupportedStreams (sta.second);
            }
        }
    }
  for (uint8_t nss = 1; nss <= maxSpatialStream; nss++)
    {
      operation.SetMaxHeMcsPerNss (nss, 11); //TBD: hardcode to 11 for now since we assume all MCS values are supported
    }
  operation.SetBssColor (GetHeConfiguration ()->GetBssColor ());

  return operation;
}

void
ApWifiMac::SendProbeResp (Mac48Address to, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << to << +linkId);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_PROBE_RESPONSE);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetLink (linkId).feManager->GetAddress ());
  hdr.SetAddr3 (GetLink (linkId).feManager->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeResponseHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  probe.SetBeaconIntervalUs (GetBeaconInterval ().GetMicroSeconds ());
  probe.SetCapabilities (GetCapabilities ());
  GetWifiRemoteStationManager (linkId)->SetShortPreambleEnabled (m_shortPreambleEnabled);
  GetWifiRemoteStationManager (linkId)->SetShortSlotTimeEnabled (m_shortSlotTimeEnabled);
  if (GetDsssSupported (linkId))
    {
      probe.SetDsssParameterSet (GetDsssParameterSet (linkId));
    }
  if (GetErpSupported (linkId))
    {
      probe.SetErpInformation (GetErpInformation (linkId));
    }
  if (GetQosSupported ())
    {
      probe.SetEdcaParameterSet (GetEdcaParameterSet (linkId));
    }
  if (GetHtSupported ())
    {
      probe.SetExtendedCapabilities (GetExtendedCapabilities ());
      probe.SetHtCapabilities (GetHtCapabilities ());
      probe.SetHtOperation (GetHtOperation ());
    }
  if (GetVhtSupported ())
    {
      probe.SetVhtCapabilities (GetVhtCapabilities ());
      probe.SetVhtOperation (GetVhtOperation ());
    }
  if (GetHeSupported ())
    {
      probe.SetHeCapabilities (GetHeCapabilities ());
      probe.SetHeOperation (GetHeOperation ());
      if (auto muEdcaParameterSet = GetMuEdcaParameterSet (); muEdcaParameterSet.has_value ())
        {
          probe.SetMuEdcaParameterSet (std::move (*muEdcaParameterSet));
        }
    }
  if (GetEhtSupported ())
    {
      probe.SetEhtCapabilities (GetEhtCapabilities ());

      if (GetNLinks () > 1)
        {
          /*
           * If an AP is affiliated with an AP MLD and does not correspond to a nontransmitted
           * BSSID, then the Beacon and Probe Response frames transmitted by the AP shall
           * include a TBTT Information field in a Reduced Neighbor Report element with the
           * TBTT Information Length field set to 16 or higher, for each of the other APs
           * (if any) affiliated with the same AP MLD. (Sec. 35.3.4.1 of 802.11be D2.1.1)
           */
          if (auto rnr = GetReducedNeighborReport (linkId); rnr.has_value ())
            {
              probe.SetReducedNeighborReport (std::move (*rnr));
            }
          /*
           * If an AP affiliated with an AP MLD is not in a multiple BSSID set [..], the AP
           * shall include, in a Beacon frame or a Probe Response frame, which is not a
           * Multi-Link probe response, only the Common Info field of the Basic Multi-Link
           * element for the AP MLD unless conditions in 35.3.11 (Multi-link procedures for
           * channel switching, extended channel switching, and channel quieting) are
           * satisfied. (Sec. 35.3.4.4 of 802.11be D2.1.1)
           */
          probe.SetMultiLinkElement (GetMultiLinkElement (linkId, WIFI_MAC_MGT_PROBE_RESPONSE));
        }
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
  else if (!GetWifiRemoteStationManager (linkId)->GetQosSupported (to))
    {
      GetBEQueue ()->Queue (packet, hdr);
    }
  else
    {
      GetVOQueue ()->Queue (packet, hdr);
    }
}

MgtAssocResponseHeader
ApWifiMac::GetAssocResp (Mac48Address to, bool isReassoc, uint8_t linkId)
{
  MgtAssocResponseHeader assoc;
  StatusCode code;
  auto remoteStationManager = GetWifiRemoteStationManager (linkId);
  if (remoteStationManager->IsWaitAssocTxOk (to))
    {
      code.SetSuccess ();
      uint16_t aid = 0;
      bool found = false;
      if (isReassoc)
        {
          for (const auto& sta : GetLink (linkId).staList)
            {
              if (sta.second == to)
                {
                  aid = sta.first;
                  found = true;
                  break;
                }
            }
        }
      if (!found)
        {
          aid = GetNextAssociationId ({linkId});
          GetLink (linkId).staList.insert (std::make_pair (aid, to));
          m_assocLogger (aid, to);
          remoteStationManager->SetAssociationId (to, aid);
          if (remoteStationManager->GetDsssSupported (to) && !remoteStationManager->GetErpOfdmSupported (to))
            {
              m_numNonErpStations++;
            }
          if (!remoteStationManager->GetHtSupported (to))
            {
              m_numNonHtStations++;
            }
          UpdateShortSlotTimeEnabled ();
          UpdateShortPreambleEnabled ();
        }
      assoc.SetAssociationId (aid);
    }
  else
    {
      NS_ABORT_IF (!remoteStationManager->IsAssocRefused (to));
      // reset state
      remoteStationManager->RecordDisassociated (to);
      code.SetFailure ();
    }
  assoc.SetSupportedRates (GetSupportedRates ());
  assoc.SetStatusCode (code);
  assoc.SetCapabilities (GetCapabilities ());
  if (GetErpSupported (linkId))
    {
      assoc.SetErpInformation (GetErpInformation (linkId));
    }
  if (GetQosSupported ())
    {
      assoc.SetEdcaParameterSet (GetEdcaParameterSet (linkId));
    }
  if (GetHtSupported ())
    {
      assoc.SetExtendedCapabilities (GetExtendedCapabilities ());
      assoc.SetHtCapabilities (GetHtCapabilities ());
      assoc.SetHtOperation (GetHtOperation ());
    }
  if (GetVhtSupported ())
    {
      assoc.SetVhtCapabilities (GetVhtCapabilities ());
      assoc.SetVhtOperation (GetVhtOperation ());
    }
  if (GetHeSupported ())
    {
      assoc.SetHeCapabilities (GetHeCapabilities ());
      assoc.SetHeOperation (GetHeOperation ());
      if (auto muEdcaParameterSet = GetMuEdcaParameterSet (); muEdcaParameterSet.has_value ())
        {
          assoc.SetMuEdcaParameterSet (std::move (*muEdcaParameterSet));
        }
    }
  if (GetEhtSupported ())
    {
      assoc.SetEhtCapabilities (GetEhtCapabilities ());
    }
  return assoc;
}

void
ApWifiMac::SendAssocResp (Mac48Address to, bool isReassoc, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << to << isReassoc << +linkId);
  WifiMacHeader hdr;
  hdr.SetType (isReassoc ? WIFI_MAC_MGT_REASSOCIATION_RESPONSE : WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetFrameExchangeManager (linkId)->GetAddress ());
  hdr.SetAddr3 (GetFrameExchangeManager (linkId)->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  MgtAssocResponseHeader assoc = GetAssocResp (to, isReassoc, linkId);

  // The AP that is affiliated with the AP MLD and that responds to an (Re)Association
  // Request frame that carries a Basic Multi-Link element shall include a Basic
  // Multi-Link element in the (Re)Association Response frame that it transmits
  // (Sec. 35.3.5.4 of 802.11be D2.0)
  // If the STA included a Multi-Link Element in the (Re)Association Request, we
  // stored its MLD address in the remote station manager
  if (GetNLinks () > 1
      && GetWifiRemoteStationManager (linkId)->GetMldAddress (to).has_value ())
    {
      assoc.SetMultiLinkElement (GetMultiLinkElement (linkId, hdr.GetType (), to));
    }

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (assoc);

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
  else if (!GetWifiRemoteStationManager (linkId)->GetQosSupported (to))
    {
      GetBEQueue ()->Queue (packet, hdr);
    }
  else
    {
      GetVOQueue ()->Queue (packet, hdr);
    }
}

void
ApWifiMac::SendOneBeacon (uint8_t linkId)
{
  NS_LOG_FUNCTION (this << +linkId);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_BEACON);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetLink (linkId).feManager->GetAddress ());
  hdr.SetAddr3 (GetLink (linkId).feManager->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtBeaconHeader beacon;
  beacon.SetSsid (GetSsid ());
  beacon.SetSupportedRates (GetSupportedRates ());
  beacon.SetBeaconIntervalUs (GetBeaconInterval ().GetMicroSeconds ());
  beacon.SetCapabilities (GetCapabilities ());
  GetWifiRemoteStationManager (linkId)->SetShortPreambleEnabled (m_shortPreambleEnabled);
  GetWifiRemoteStationManager (linkId)->SetShortSlotTimeEnabled (m_shortSlotTimeEnabled);
  if (GetDsssSupported (linkId))
    {
      beacon.SetDsssParameterSet (GetDsssParameterSet (linkId));
    }
  if (GetErpSupported (linkId))
    {
      beacon.SetErpInformation (GetErpInformation (linkId));
    }
  if (GetQosSupported ())
    {
      beacon.SetEdcaParameterSet (GetEdcaParameterSet (linkId));
    }
  if (GetHtSupported ())
    {
      beacon.SetExtendedCapabilities (GetExtendedCapabilities ());
      beacon.SetHtCapabilities (GetHtCapabilities ());
      beacon.SetHtOperation (GetHtOperation ());
    }
  if (GetVhtSupported ())
    {
      beacon.SetVhtCapabilities (GetVhtCapabilities ());
      beacon.SetVhtOperation (GetVhtOperation ());
    }
  if (GetHeSupported ())
    {
      beacon.SetHeCapabilities (GetHeCapabilities ());
      beacon.SetHeOperation (GetHeOperation ());
      if (auto muEdcaParameterSet = GetMuEdcaParameterSet (); muEdcaParameterSet.has_value ())
        {
          beacon.SetMuEdcaParameterSet (std::move (*muEdcaParameterSet));
        }
    }
  if (GetEhtSupported ())
    {
      beacon.SetEhtCapabilities (GetEhtCapabilities ());

      if (GetNLinks () > 1)
        {
          /*
           * If an AP is affiliated with an AP MLD and does not correspond to a nontransmitted
           * BSSID, then the Beacon and Probe Response frames transmitted by the AP shall
           * include a TBTT Information field in a Reduced Neighbor Report element with the
           * TBTT Information Length field set to 16 or higher, for each of the other APs
           * (if any) affiliated with the same AP MLD. (Sec. 35.3.4.1 of 802.11be D2.1.1)
           */
          if (auto rnr = GetReducedNeighborReport (linkId); rnr.has_value ())
            {
              beacon.SetReducedNeighborReport (std::move (*rnr));
            }
          /*
           * If an AP affiliated with an AP MLD is not in a multiple BSSID set [..], the AP
           * shall include, in a Beacon frame or a Probe Response frame, which is not a
           * Multi-Link probe response, only the Common Info field of the Basic Multi-Link
           * element for the AP MLD unless conditions in 35.3.11 (Multi-link procedures for
           * channel switching, extended channel switching, and channel quieting) are
           * satisfied. (Sec. 35.3.4.4 of 802.11be D2.1.1)
           */
          beacon.SetMultiLinkElement (GetMultiLinkElement (linkId, WIFI_MAC_MGT_BEACON));
        }
    }
  packet->AddHeader (beacon);

  //The beacon has it's own special queue, so we load it in there
  m_beaconTxop->Queue (packet, hdr);
  GetLink(linkId).beaconEvent = Simulator::Schedule (GetBeaconInterval (), &ApWifiMac::SendOneBeacon, this, linkId);

  //If a STA that does not support Short Slot Time associates,
  //the AP shall use long slot time beginning at the first Beacon
  //subsequent to the association of the long slot time STA.
  if (GetErpSupported (linkId))
    {
      if (m_shortSlotTimeEnabled)
        {
          //Enable short slot time
          GetWifiPhy (linkId)->SetSlot (MicroSeconds (9));
        }
      else
        {
          //Disable short slot time
          GetWifiPhy (linkId)->SetSlot (MicroSeconds (20));
        }
    }
}

void
ApWifiMac::TxOk (Ptr<const WifiMpdu> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  const WifiMacHeader& hdr = mpdu->GetHeader ();

  if (hdr.IsAssocResp () || hdr.IsReassocResp ())
    {
      auto linkId = GetLinkIdByAddress (hdr.GetAddr2 ());
      NS_ABORT_MSG_IF (!linkId.has_value (), "No link ID matching the TA");

      if (GetWifiRemoteStationManager (*linkId)->IsWaitAssocTxOk (hdr.GetAddr1 ()))
        {
          NS_LOG_DEBUG ("AP=" << hdr.GetAddr2 () << " associated with STA=" << hdr.GetAddr1 ());
          GetWifiRemoteStationManager (*linkId)->RecordGotAssocTxOk (hdr.GetAddr1 ());
        }

      if (auto staMldAddress = GetWifiRemoteStationManager (*linkId)->GetMldAddress (hdr.GetAddr1 ());
          staMldAddress.has_value ())
        {
          // the STA is affiliated with an MLD
          for (uint8_t i = 0; i < GetNLinks (); i++)
            {
              auto stationManager = GetWifiRemoteStationManager (i);
              if (auto staAddress = stationManager->GetAffiliatedStaAddress (*staMldAddress);
                  staAddress.has_value ()
                  && i != *linkId
                  && stationManager->IsWaitAssocTxOk (*staAddress))
                {
                  NS_LOG_DEBUG ("AP=" << GetFrameExchangeManager (i)->GetAddress ()
                                << " associated with STA=" << *staAddress);
                  stationManager->RecordGotAssocTxOk (*staAddress);
                }
            }
        }
    }
}

void
ApWifiMac::TxFailed (WifiMacDropReason timeoutReason, Ptr<const WifiMpdu> mpdu)
{
  NS_LOG_FUNCTION (this << +timeoutReason << *mpdu);
  const WifiMacHeader& hdr = mpdu->GetHeader ();

  if (hdr.IsAssocResp () || hdr.IsReassocResp ())
    {
      auto linkId = GetLinkIdByAddress (hdr.GetAddr2 ());
      NS_ABORT_MSG_IF (!linkId.has_value (), "No link ID matching the TA");

      if (GetWifiRemoteStationManager (*linkId)->IsWaitAssocTxOk (hdr.GetAddr1 ()))
        {
          NS_LOG_DEBUG ("AP=" << hdr.GetAddr2 () << " association failed with STA=" << hdr.GetAddr1 ());
          GetWifiRemoteStationManager (*linkId)->RecordGotAssocTxFailed (hdr.GetAddr1 ());
        }

      if (auto staMldAddress = GetWifiRemoteStationManager (*linkId)->GetMldAddress (hdr.GetAddr1 ());
          staMldAddress.has_value ())
        {
          // the STA is affiliated with an MLD
          for (uint8_t i = 0; i < GetNLinks (); i++)
            {
              auto stationManager = GetWifiRemoteStationManager (i);
              if (auto staAddress = stationManager->GetAffiliatedStaAddress (*staMldAddress);
                  staAddress.has_value ()
                  && i != *linkId
                  && stationManager->IsWaitAssocTxOk (*staAddress))
                {
                  NS_LOG_DEBUG ("AP=" << GetFrameExchangeManager (i)->GetAddress ()
                                << " association failed with STA=" << *staAddress);
                  stationManager->RecordGotAssocTxFailed (*staAddress);
                }
            }
        }
    }
}

void
ApWifiMac::Receive (Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << *mpdu << +linkId);
  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<const Packet> packet = mpdu->GetPacket ();
  Mac48Address from = hdr->GetAddr2 ();
  if (hdr->IsData ())
    {
      Mac48Address bssid = hdr->GetAddr1 ();
      if (!hdr->IsFromDs ()
          && hdr->IsToDs ()
          && bssid == GetAddress ()
          && GetWifiRemoteStationManager ()->IsAssociated (from))
        {
          Mac48Address to = hdr->GetAddr3 ();
          if (to == GetAddress ())
            {
              NS_LOG_DEBUG ("frame for me from=" << from);
              if (hdr->IsQosData ())
                {
                  if (hdr->IsQosAmsdu ())
                    {
                      NS_LOG_DEBUG ("Received A-MSDU from=" << from << ", size=" << packet->GetSize ());
                      DeaggregateAmsduAndForward (mpdu);
                      packet = 0;
                    }
                  else
                    {
                      ForwardUp (packet, from, bssid);
                    }
                }
              else if (hdr->HasData ())
                {
                  ForwardUp (packet, from, bssid);
                }
            }
          else if (to.IsGroup ()
                   || GetWifiRemoteStationManager ()->IsAssociated (to))
            {
              NS_LOG_DEBUG ("forwarding frame from=" << from << ", to=" << to);
              Ptr<Packet> copy = packet->Copy ();

              //If the frame we are forwarding is of type QoS Data,
              //then we need to preserve the UP in the QoS control
              //header...
              if (hdr->IsQosData ())
                {
                  ForwardDown (copy, from, to, hdr->GetQosTid ());
                }
              else
                {
                  ForwardDown (copy, from, to);
                }
              ForwardUp (packet, from, to);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
        }
      else if (hdr->IsFromDs ()
               && hdr->IsToDs ())
        {
          //this is an AP-to-AP frame
          //we ignore for now.
          NotifyRxDrop (packet);
        }
      else
        {
          //we can ignore these frames since
          //they are not targeted at the AP
          NotifyRxDrop (packet);
        }
      return;
    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ()
          && (hdr->GetAddr1 ().IsGroup ()
              || hdr->GetAddr1 () == GetFrameExchangeManager (linkId)->GetAddress ()))
        {
          // In the case where the Address 1 field contains a group address, the
          // Address 3 field also is validated to verify that the group addressed
          // frame originated from a STA in the BSS of which the receiving STA is
          // a member (Section 9.3.3.1 of 802.11-2020)
          if (hdr->GetAddr1 ().IsGroup ()
              && !hdr->GetAddr3 ().IsBroadcast ()
              && hdr->GetAddr3 () != GetFrameExchangeManager (linkId)->GetAddress ())
            {
              // not addressed to us
              return;
            }
          MgtProbeRequestHeader probeRequestHeader;
          packet->PeekHeader (probeRequestHeader);
          const Ssid& ssid = probeRequestHeader.GetSsid ();
          if (ssid == GetSsid () || ssid.IsBroadcast ())
            {
              NS_LOG_DEBUG ("Probe request received from " << from << ": send probe response");
              SendProbeResp (from, linkId);
            }
          return;
        }
      else if (hdr->GetAddr1 () == GetFrameExchangeManager (linkId)->GetAddress ())
        {
          if (hdr->IsAssocReq () || hdr->IsReassocReq ())
            {
              NS_LOG_DEBUG (((hdr->IsAssocReq ()) ? "Association" : "Reassociation")
                            << " request received from " << from
                            << ((GetNLinks () > 1) ? " on link ID " + std::to_string (linkId) : ""));

              MgtAssocRequestHeader assocReq;
              MgtReassocRequestHeader reassocReq;
              AssocReqRefVariant frame = assocReq;
              if (hdr->IsAssocReq ())
                {
                  packet->PeekHeader (assocReq);
                }
              else
                {
                  packet->PeekHeader (reassocReq);
                  frame = reassocReq;
                }
              ReceiveAssocRequest (frame, from, linkId);
              if (GetNLinks () > 1)
                {
                  ParseReportedStaInfo (frame, from, linkId);
                }
              SendAssocResp (hdr->GetAddr2 (), hdr->IsReassocReq (), linkId);
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              NS_LOG_DEBUG ("Disassociation received from " << from);
              GetWifiRemoteStationManager ()->RecordDisassociated (from);
              auto& staList = GetLink (SINGLE_LINK_OP_ID).staList;
              for (auto it = staList.begin (); it != staList.end (); ++it)
                {
                  if (it->second == from)
                    {
                      staList.erase (it);
                      m_deAssocLogger (it->first, it->second);
                      if (GetWifiRemoteStationManager ()->GetDsssSupported (from) && !GetWifiRemoteStationManager ()->GetErpOfdmSupported (from))
                        {
                          m_numNonErpStations--;
                        }
                      if (!GetWifiRemoteStationManager ()->GetHtSupported (from))
                        {
                          m_numNonHtStations--;
                        }
                      UpdateShortSlotTimeEnabled ();
                      UpdateShortPreambleEnabled ();
                      break;
                    }
                }
              return;
            }
        }
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  WifiMac::Receive (Create<WifiMpdu> (packet, *hdr), linkId);
}

bool
ApWifiMac::ReceiveAssocRequest (const AssocReqRefVariant& assoc, const Mac48Address& from, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << from << +linkId);

  auto remoteStationManager = GetWifiRemoteStationManager (linkId);

  auto failure = [&](const std::string& msg) -> bool
                    {
                      NS_LOG_DEBUG ("Association Request from " << from << " refused: " << msg);
                      remoteStationManager->RecordAssocRefused (from);
                      return false;
                    };

  // lambda to process received (Re)Association Request
  auto recvAssocRequest =
    [&](auto&& frameRefWrapper) -> bool
    {
      const auto& frame = frameRefWrapper.get ();

      //first, verify that the the station's supported
      //rate set is compatible with our Basic Rate set
      const CapabilityInformation& capabilities = frame.GetCapabilities ();
      remoteStationManager->AddSupportedPhyPreamble (from, capabilities.IsShortPreamble ());
      const SupportedRates& rates = frame.GetSupportedRates ();

      if (rates.GetNRates () == 0)
        {
          return failure ("STA's supported rate set not compatible with our Basic Rate set");
        }

      if (GetHtSupported ())
        {
          //check whether the HT STA supports all MCSs in Basic MCS Set
          const auto& htCapabilities = frame.GetHtCapabilities ();
          if (htCapabilities.has_value () && htCapabilities->IsSupportedMcs (0))
            {
              for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs (); i++)
                {
                  WifiMode mcs = remoteStationManager->GetBasicMcs (i);
                  if (!htCapabilities->IsSupportedMcs (mcs.GetMcsValue ()))
                    {
                      return failure ("HT STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
        }
      if (GetVhtSupported ())
        {
          //check whether the VHT STA supports all MCSs in Basic MCS Set
          const auto& vhtCapabilities = frame.GetVhtCapabilities ();
          if (vhtCapabilities.has_value () && vhtCapabilities->GetVhtCapabilitiesInfo () != 0)
            {
              for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs (); i++)
                {
                  WifiMode mcs = remoteStationManager->GetBasicMcs (i);
                  if (!vhtCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                    {
                      return failure ("VHT STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
        }
      if (GetHeSupported ())
        {
          //check whether the HE STA supports all MCSs in Basic MCS Set
          const auto& heCapabilities = frame.GetHeCapabilities ();
          if (heCapabilities.has_value () && heCapabilities->GetSupportedMcsAndNss () != 0)
            {
              for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs (); i++)
                {
                  WifiMode mcs = remoteStationManager->GetBasicMcs (i);
                  if (!heCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                    {
                      return failure ("HE STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
        }
      if (GetEhtSupported ())
        {
          //check whether the EHT STA supports all MCSs in Basic MCS Set
          // const auto& ehtCapabilities = frame.GetEhtCapabilities ();
          //TODO: to be completed
        }

      // The association request from the station can be accepted.
      // Record all its supported modes in its associated WifiRemoteStation
      auto phy = GetWifiPhy (linkId);

      for (const auto & mode : phy->GetModeList ())
        {
          if (rates.IsSupportedRate (mode.GetDataRate (phy->GetChannelWidth ())))
            {
              remoteStationManager->AddSupportedMode (from, mode);
            }
        }
      if (GetErpSupported (linkId) && remoteStationManager->GetErpOfdmSupported (from) && capabilities.IsShortSlotTime ())
        {
          remoteStationManager->AddSupportedErpSlotTime (from, true);
        }
      if (GetHtSupported ())
        {
          const auto& htCapabilities = frame.GetHtCapabilities ();
          if (htCapabilities.has_value () && htCapabilities->IsSupportedMcs (0))
            {
              remoteStationManager->AddStationHtCapabilities (from, *htCapabilities);
            }
          // const ExtendedCapabilities& extendedCapabilities = frame.GetExtendedCapabilities ();
          //TODO: to be completed
        }
      if (GetVhtSupported ())
        {
          const auto& vhtCapabilities = frame.GetVhtCapabilities ();
          //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
          if (vhtCapabilities.has_value () && vhtCapabilities->GetRxHighestSupportedLgiDataRate () > 0)
            {
              remoteStationManager->AddStationVhtCapabilities (from, *vhtCapabilities);
              for (const auto & mcs : phy->GetMcsList (WIFI_MOD_CLASS_VHT))
                {
                  if (vhtCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                    {
                      remoteStationManager->AddSupportedMcs (from, mcs);
                      //here should add a control to add basic MCS when it is implemented
                    }
                }
            }
        }
      if (GetHeSupported ())
        {
          const auto& heCapabilities = frame.GetHeCapabilities ();
          if (heCapabilities.has_value () && heCapabilities->GetSupportedMcsAndNss () != 0)
            {
              remoteStationManager->AddStationHeCapabilities (from, *heCapabilities);
              for (const auto & mcs : phy->GetMcsList (WIFI_MOD_CLASS_HE))
                {
                  if (heCapabilities->IsSupportedTxMcs (mcs.GetMcsValue ()))
                    {
                      remoteStationManager->AddSupportedMcs (from, mcs);
                      //here should add a control to add basic MCS when it is implemented
                    }
                }
            }
        }
      if (GetEhtSupported ())
        {
          const auto& ehtCapabilities = frame.GetEhtCapabilities ();
          //TODO: once we support non constant rate managers, we should add checks here whether EHT is supported by the peer
          remoteStationManager->AddStationEhtCapabilities (from, *ehtCapabilities);
          for (const auto & mcs : phy->GetMcsList (WIFI_MOD_CLASS_EHT))
            {
              //TODO: Add check whether MCS is supported from the capabilities
              remoteStationManager->AddSupportedMcs (from, mcs);
              //here should add a control to add basic MCS when it is implemented
            }
        }

      NS_LOG_DEBUG ("Association Request from " << from << " accepted");
      remoteStationManager->RecordWaitAssocTxOk (from);
      return true;;
    };

  return std::visit (recvAssocRequest, assoc);
}

void
ApWifiMac::ParseReportedStaInfo (const AssocReqRefVariant& assoc,
                                 Mac48Address from, uint8_t linkId)
{
  NS_LOG_FUNCTION (this << from << +linkId);

  // lambda to process received Multi-Link Element
  auto recvMle =
    [&](auto&& frame)
    {
      const auto& mle = frame.get ().GetMultiLinkElement ();

      if (!mle.has_value ())
        {
          return;
        }

      GetWifiRemoteStationManager (linkId)->SetMldAddress (from, mle->GetMldMacAddress ());

      for (std::size_t i = 0; i < mle->GetNPerStaProfileSubelements (); i++)
        {
          auto& perStaProfile = mle->GetPerStaProfile (i);
          if (!perStaProfile.HasStaMacAddress ())
            {
              NS_LOG_DEBUG ("[i=" << i << "] Cannot setup a link if the STA MAC address is missing");
              continue;
            }
          uint8_t newLinkId = perStaProfile.GetLinkId ();
          if (newLinkId == linkId || newLinkId >= GetNLinks ())
            {
              NS_LOG_DEBUG ("[i=" << i << "] Link ID " << newLinkId << " not valid");
              continue;
            }
          if (!perStaProfile.HasAssocRequest () && !perStaProfile.HasReassocRequest ())
            {
              NS_LOG_DEBUG ("[i=" << i << "] No (Re)Association Request frame body present");
              continue;
            }

          ReceiveAssocRequest (perStaProfile.GetAssocRequest(), perStaProfile.GetStaMacAddress (),
                               newLinkId);
          GetWifiRemoteStationManager (newLinkId)->SetMldAddress (perStaProfile.GetStaMacAddress (),
                                                                  mle->GetMldMacAddress ());
        }
    };

  std::visit (recvMle, assoc);
}

void
ApWifiMac::DeaggregateAmsduAndForward (Ptr<WifiMpdu> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  for (auto& i : *PeekPointer (mpdu))
    {
      if (i.second.GetDestinationAddr () == GetAddress ())
        {
          ForwardUp (i.first, i.second.GetSourceAddr (),
                     i.second.GetDestinationAddr ());
        }
      else
        {
          Mac48Address from = i.second.GetSourceAddr ();
          Mac48Address to = i.second.GetDestinationAddr ();
          NS_LOG_DEBUG ("forwarding QoS frame from=" << from << ", to=" << to);
          ForwardDown (i.first->Copy (), from, to, mpdu->GetHeader ().GetQosTid ());
        }
    }
}

void
ApWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_beaconTxop->Initialize ();

  for (uint8_t linkId = 0; linkId < GetNLinks (); ++linkId)
    {
      GetLink (linkId).beaconEvent.Cancel ();
      if (m_enableBeaconGeneration)
        {
          uint64_t jitterUs = (m_enableBeaconJitter
                              ? static_cast <uint64_t> (m_beaconJitter->GetValue (0, 1) * (GetBeaconInterval ().GetMicroSeconds ()))
                              : 0);
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress ()
                        << " at time " << jitterUs << "us");
          GetLink (linkId).beaconEvent = Simulator::Schedule (MicroSeconds (jitterUs),
                                                              &ApWifiMac::SendOneBeacon,
                                                              this, linkId);
        }
    }

  NS_ABORT_IF (!TraceConnectWithoutContext ("AckedMpdu", MakeCallback (&ApWifiMac::TxOk, this)));
  NS_ABORT_IF (!TraceConnectWithoutContext ("DroppedMpdu", MakeCallback (&ApWifiMac::TxFailed, this)));
  WifiMac::DoInitialize ();
  UpdateShortSlotTimeEnabled ();
  UpdateShortPreambleEnabled ();
}

bool
ApWifiMac::GetUseNonErpProtection (void) const
{
  bool useProtection = (m_numNonErpStations > 0) && m_enableNonErpProtection;
  GetWifiRemoteStationManager ()->SetUseNonErpProtection (useProtection);
  return useProtection;
}

uint16_t
ApWifiMac::GetNextAssociationId (std::list<uint8_t> linkIds)
{
  // Return the first AID value between 1 and 2007 that is free for all the given links
  for (uint16_t nextAid = 1; nextAid <= 2007; nextAid++)
    {
      if (std::all_of (linkIds.begin (), linkIds.end (),
                       [&](auto&& linkId)
                       {
                         auto& staList = GetLink (linkId).staList;
                         return staList.find (nextAid) == staList.end ();
                       }))
        {
          return nextAid;
        }
    }
  NS_FATAL_ERROR ("No free association ID available!");
  return 0;
}

const std::map<uint16_t, Mac48Address>&
ApWifiMac::GetStaList (uint8_t linkId) const
{
  return GetLink (linkId).staList;
}

uint16_t
ApWifiMac::GetAssociationId (Mac48Address addr, uint8_t linkId) const
{
  return GetWifiRemoteStationManager (linkId)->GetAssociationId (addr);
}

uint8_t
ApWifiMac::GetBufferStatus (uint8_t tid, Mac48Address address) const
{
  auto it = m_bufferStatus.find (WifiAddressTidPair (address, tid));
  if (it == m_bufferStatus.end ()
      || it->second.timestamp + m_bsrLifetime < Simulator::Now ())
    {
      return 255;
    }
  return it->second.value;
}

void
ApWifiMac::SetBufferStatus (uint8_t tid, Mac48Address address, uint8_t size)
{
  if (size == 255)
    {
      // no point in storing an unspecified size
      m_bufferStatus.erase (WifiAddressTidPair (address, tid));
    }
  else
    {
      m_bufferStatus[WifiAddressTidPair (address, tid)] = {size, Simulator::Now ()};
    }
}

uint8_t
ApWifiMac::GetMaxBufferStatus (Mac48Address address) const
{
  uint8_t maxSize = 0;
  bool found = false;

  for (uint8_t tid = 0; tid < 8; tid++)
    {
      uint8_t size = GetBufferStatus (tid, address);
      if (size != 255)
        {
          maxSize = std::max (maxSize, size);
          found = true;
        }
    }

  if (found)
    {
      return maxSize;
    }
  return 255;
}

} //namespace ns3
