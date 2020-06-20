/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#include "ns3/log.h"
#include "ns3/packet.h"
#include "wifi-mac.h"
#include "txop.h"
#include "ssid.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "vht-configuration.h"
#include "he-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMac");

NS_OBJECT_ENSURE_REGISTERED (WifiMac);

Time
WifiMac::GetDefaultSlot (void)
{
  //802.11-a specific
  return MicroSeconds (9);
}

Time
WifiMac::GetDefaultSifs (void)
{
  //802.11-a specific
  return MicroSeconds (16);
}

Time
WifiMac::GetDefaultRifs (void)
{
  //802.11n specific
  return MicroSeconds (2);
}

Time
WifiMac::GetDefaultEifsNoDifs (void)
{
  return GetDefaultSifs () + GetDefaultCtsAckDelay ();
}

Time
WifiMac::GetDefaultCtsAckDelay (void)
{
  //802.11-a specific: at 6 Mbit/s
  return MicroSeconds (44);
}

TypeId
WifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMac")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("EifsNoDifs", "The value of EIFS-DIFS.",
                   TimeValue (GetDefaultEifsNoDifs ()),
                   MakeTimeAccessor (&WifiMac::SetEifsNoDifs,
                                     &WifiMac::GetEifsNoDifs),
                   MakeTimeChecker ())
    .AddAttribute ("Pifs", "The value of the PIFS constant.",
                   TimeValue (GetDefaultSifs () + GetDefaultSlot ()),
                   MakeTimeAccessor (&WifiMac::SetPifs,
                                     &WifiMac::GetPifs),
                   MakeTimeChecker ())
    .AddAttribute ("Rifs", "The value of the RIFS constant.",
                   TimeValue (GetDefaultRifs ()),
                   MakeTimeAccessor (&WifiMac::SetRifs,
                                     &WifiMac::GetRifs),
                   MakeTimeChecker ())
    .AddAttribute ("Ssid", "The ssid we want to belong to.",
                   SsidValue (Ssid ("default")),
                   MakeSsidAccessor (&WifiMac::GetSsid,
                                     &WifiMac::SetSsid),
                   MakeSsidChecker ())
    .AddTraceSource ("MacTx",
                     "A packet has been received from higher layers and is being processed in preparation for "
                     "queueing for transmission.",
                     MakeTraceSourceAccessor (&WifiMac::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop",
                     "A packet has been dropped in the MAC layer before transmission.",
                     MakeTraceSourceAccessor (&WifiMac::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx",
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  This is a promiscuous trace.",
                     MakeTraceSourceAccessor (&WifiMac::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx",
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack. This is a non-promiscuous trace.",
                     MakeTraceSourceAccessor (&WifiMac::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRxDrop",
                     "A packet has been dropped in the MAC layer after it has been passed up from the physical layer.",
                     MakeTraceSourceAccessor (&WifiMac::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
WifiMac::DoDispose ()
{
  m_device = 0;
}

void
WifiMac::SetDevice (const Ptr<NetDevice> device)
{
  m_device = device;
}

Ptr<NetDevice>
WifiMac::GetDevice (void) const
{
  return m_device;
}

void
WifiMac::NotifyTx (Ptr<const Packet> packet)
{
  m_macTxTrace (packet);
}

void
WifiMac::NotifyTxDrop (Ptr<const Packet> packet)
{
  m_macTxDropTrace (packet);
}

void
WifiMac::NotifyRx (Ptr<const Packet> packet)
{
  m_macRxTrace (packet);
}

void
WifiMac::NotifyPromiscRx (Ptr<const Packet> packet)
{
  m_macPromiscRxTrace (packet);
}

void
WifiMac::NotifyRxDrop (Ptr<const Packet> packet)
{
  m_macRxDropTrace (packet);
}

void
WifiMac::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      Configure80211_10Mhz ();
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      Configure80211_5Mhz ();
      break;
    case WIFI_PHY_STANDARD_holland:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      Configure80211n_2_4Ghz ();
      break;
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      Configure80211n_5Ghz ();
      break;
    case WIFI_PHY_STANDARD_80211ac:
      Configure80211ac ();
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
      Configure80211ax_2_4Ghz ();
      break;
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      Configure80211ax_5Ghz ();
      break;
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      NS_FATAL_ERROR ("Wifi standard not found");
      break;
    }
  FinishConfigureStandard (standard);
}

void
WifiMac::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  SetEifsNoDifs (MicroSeconds (16 + 44));
  SetPifs (MicroSeconds (16 + 9));
}

void
WifiMac::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  SetEifsNoDifs (MicroSeconds (10 + 304));
  SetPifs (MicroSeconds (10 + 20));
}

void
WifiMac::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  SetEifsNoDifs (MicroSeconds (10 + 304));
  SetPifs (MicroSeconds (10 + 20));
}

void
WifiMac::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);
  SetEifsNoDifs (MicroSeconds (32 + 88));
  SetPifs (MicroSeconds (32 + 13));
}

void
WifiMac::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);
  SetEifsNoDifs (MicroSeconds (64 + 176));
  SetPifs (MicroSeconds (64 + 21));
}

void
WifiMac::Configure80211n_2_4Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211g ();
  SetRifs (MicroSeconds (2));
}
void
WifiMac::Configure80211n_5Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211a ();
  SetRifs (MicroSeconds (2));
}

void
WifiMac::Configure80211ac (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n_5Ghz ();
}

void
WifiMac::Configure80211ax_2_4Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n_2_4Ghz ();
}

void
WifiMac::Configure80211ax_5Ghz (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211ac ();
}

void
WifiMac::ConfigureDcf (Ptr<Txop> dcf, uint32_t cwmin, uint32_t cwmax, bool isDsss, AcIndex ac)
{
  NS_LOG_FUNCTION (this << dcf << cwmin << cwmax << isDsss << ac);
  /* see IEEE 802.11 section 7.3.2.29 */
  switch (ac)
    {
    case AC_VO:
      dcf->SetMinCw ((cwmin + 1) / 4 - 1);
      dcf->SetMaxCw ((cwmin + 1) / 2 - 1);
      dcf->SetAifsn (2);
      if (isDsss)
        {
          dcf->SetTxopLimit (MicroSeconds (3264));
        }
      else
        {
          dcf->SetTxopLimit (MicroSeconds (1504));
        }
      break;
    case AC_VI:
      dcf->SetMinCw ((cwmin + 1) / 2 - 1);
      dcf->SetMaxCw (cwmin);
      dcf->SetAifsn (2);
      if (isDsss)
        {
          dcf->SetTxopLimit (MicroSeconds (6016));
        }
      else
        {
          dcf->SetTxopLimit (MicroSeconds (3008));
        }
      break;
    case AC_BE:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (3);
      dcf->SetTxopLimit (MicroSeconds (0));
      break;
    case AC_BK:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (7);
      dcf->SetTxopLimit (MicroSeconds (0));
      break;
    case AC_BE_NQOS:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (2);
      dcf->SetTxopLimit (MicroSeconds (0));
      break;
    case AC_UNDEF:
      NS_FATAL_ERROR ("I don't know what to do with this");
      break;
    }
}

Ptr<HtConfiguration>
WifiMac::GetHtConfiguration (void) const
{
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
      return device->GetHtConfiguration ();
}

Ptr<VhtConfiguration>
WifiMac::GetVhtConfiguration (void) const
{
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
      return device->GetVhtConfiguration ();
}

Ptr<HeConfiguration>
WifiMac::GetHeConfiguration (void) const
{
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
      return device->GetHeConfiguration ();
}

} //namespace ns3

