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
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMac");

NS_OBJECT_ENSURE_REGISTERED (WifiMac);

TypeId
WifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMac")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
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
                     "A packet has been dropped in the MAC layer before being queued for transmission. "
                     "This trace source is fired, e.g., when an AP's MAC receives from the upper layer "
                     "a packet destined to a station that is not associated with the AP or a STA's MAC "
                     "receives a packet from the upper layer while it is not associated with any AP.",
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

