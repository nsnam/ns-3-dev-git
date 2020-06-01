/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/nstime.h"
#include "wifi-utils.h"
#include "ctrl-headers.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "he-configuration.h"
#include "wifi-mode.h"
#include "ampdu-subframe-header.h"

namespace ns3 {

double
DbToRatio (double dB)
{
  return std::pow (10.0, 0.1 * dB);
}

double
DbmToW (double dBm)
{
  return std::pow (10.0, 0.1 * (dBm - 30.0));
}

double
WToDbm (double w)
{
  return 10.0 * std::log10 (w) + 30.0;
}

double
RatioToDb (double ratio)
{
  return 10.0 * std::log10 (ratio);
}

uint16_t
ConvertGuardIntervalToNanoSeconds (WifiMode mode, const Ptr<WifiNetDevice> device)
{
  uint16_t gi = 800;
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      NS_ASSERT (heConfiguration); //If HE modulation is used, we should have a HE configuration attached
      gi = static_cast<uint16_t> (heConfiguration->GetGuardInterval ().GetNanoSeconds ());
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT || mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      NS_ASSERT (htConfiguration); //If HT/VHT modulation is used, we should have a HT configuration attached
      gi = htConfiguration->GetShortGuardIntervalSupported () ? 400 : 800;
    }
  return gi;
}

uint16_t
ConvertGuardIntervalToNanoSeconds (WifiMode mode, bool htShortGuardInterval, Time heGuardInterval)
{
  uint16_t gi;
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      gi = static_cast<uint16_t> (heGuardInterval.GetNanoSeconds ());
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT || mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      gi = htShortGuardInterval ? 400 : 800;
    }
  else
    {
      gi = 800;
    }
  return gi;
}

uint16_t
GetChannelWidthForTransmission (WifiMode mode, uint16_t maxSupportedChannelWidth)
{
  WifiModulationClass modulationClass = mode.GetModulationClass ();
  if (maxSupportedChannelWidth > 20
      && (modulationClass == WifiModulationClass::WIFI_MOD_CLASS_OFDM // all non-HT OFDM control and management frames
          || modulationClass == WifiModulationClass::WIFI_MOD_CLASS_ERP_OFDM)) // special case of beacons at 2.4 GHz
    {
      return 20;
    }
  //at 2.4 GHz basic rate can be non-ERP DSSS
  if (modulationClass == WifiModulationClass::WIFI_MOD_CLASS_DSSS
      || modulationClass == WifiModulationClass::WIFI_MOD_CLASS_HR_DSSS)
    {
      return 22;
    }
  return maxSupportedChannelWidth;
}

WifiPreamble
GetPreambleForTransmission (WifiModulationClass modulation, bool useShortPreamble, bool useGreenfield)
{
  if (modulation == WIFI_MOD_CLASS_HE)
    {
      return WIFI_PREAMBLE_HE_SU;
    }
  else if (modulation == WIFI_MOD_CLASS_VHT)
    {
      return WIFI_PREAMBLE_VHT_SU;
    }
  else if (modulation == WIFI_MOD_CLASS_HT && useGreenfield)
    {
      //If protection for Greenfield is used we go for HT_MF preamble which is the default protection for GF format defined in the standard.
      return WIFI_PREAMBLE_HT_GF;
    }
  else if (modulation == WIFI_MOD_CLASS_HT)
    {
      return WIFI_PREAMBLE_HT_MF;
    }
  else if (useShortPreamble)
    {
      return WIFI_PREAMBLE_SHORT;
    }
  else
    {
      return WIFI_PREAMBLE_LONG;
    }
}

bool
IsAllowedControlAnswerModulationClass (WifiModulationClass modClassReq, WifiModulationClass modClassAnswer)
{
  switch (modClassReq)
    {
    case WIFI_MOD_CLASS_DSSS:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS);
    case WIFI_MOD_CLASS_HR_DSSS:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS);
    case WIFI_MOD_CLASS_ERP_OFDM:
      return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS || modClassAnswer == WIFI_MOD_CLASS_ERP_OFDM);
    case WIFI_MOD_CLASS_OFDM:
      return (modClassAnswer == WIFI_MOD_CLASS_OFDM);
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
      return true;
    default:
      NS_FATAL_ERROR ("Modulation class not defined");
      return false;
    }
}

uint32_t
GetAckSize (void)
{
  WifiMacHeader ack;
  ack.SetType (WIFI_MAC_CTL_ACK);
  return ack.GetSize () + 4;
}

uint32_t
GetBlockAckSize (BlockAckType type)
{
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKRESP);
  CtrlBAckResponseHeader blockAck;
  blockAck.SetType (type);
  return hdr.GetSize () + blockAck.GetSerializedSize () + 4;
}

uint32_t
GetBlockAckRequestSize (BlockAckType type)
{
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKREQ);
  CtrlBAckRequestHeader bar;
  bar.SetType (type);
  return hdr.GetSize () + bar.GetSerializedSize () + 4;
}

uint32_t
GetRtsSize (void)
{
  WifiMacHeader rts;
  rts.SetType (WIFI_MAC_CTL_RTS);
  return rts.GetSize () + 4;
}

uint32_t
GetCtsSize (void)
{
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_CTS);
  return cts.GetSize () + 4;
}

bool
IsInWindow (uint16_t seq, uint16_t winstart, uint16_t winsize)
{
  return ((seq - winstart + 4096) % 4096) < winsize;
}

void
AddWifiMacTrailer (Ptr<Packet> packet)
{
  WifiMacTrailer fcs;
  packet->AddTrailer (fcs);
}

uint32_t
GetSize (Ptr<const Packet> packet, const WifiMacHeader *hdr, bool isAmpdu)
{
  uint32_t size;
  WifiMacTrailer fcs;
  if (isAmpdu)
    {
      size = packet->GetSize ();
    }
  else
    {
      size = packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
    }
  return size;
}

Time
GetPpduMaxTime (WifiPreamble preamble)
{
  Time duration;

  switch (preamble)
    {
    case WIFI_PREAMBLE_HT_GF:
      duration = MicroSeconds (10000);
      break;
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
      duration = MicroSeconds (5484);
      break;
    default:
      duration = MicroSeconds (0);
      break;
    }
  return duration;
}

} //namespace ns3
