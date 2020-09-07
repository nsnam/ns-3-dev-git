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
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "regular-wifi-mac.h"
#include "wifi-phy.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mac-low.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "mgt-headers.h"
#include "amsdu-subframe-header.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "vht-configuration.h"
#include "he-configuration.h"
#include <algorithm>
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RegularWifiMac");

NS_OBJECT_ENSURE_REGISTERED (RegularWifiMac);

RegularWifiMac::RegularWifiMac ()
  : m_qosSupported (0),
    m_erpSupported (0),
    m_dsssSupported (0)
{
  NS_LOG_FUNCTION (this);
  m_rxMiddle = Create<MacRxMiddle> ();
  m_rxMiddle->SetForwardCallback (MakeCallback (&RegularWifiMac::Receive, this));

  m_txMiddle = Create<MacTxMiddle> ();

  m_low = CreateObject<MacLow> ();
  m_low->SetRxCallback (MakeCallback (&MacRxMiddle::Receive, m_rxMiddle));
  m_low->SetMac (this);

  m_channelAccessManager = CreateObject<ChannelAccessManager> ();
  m_channelAccessManager->SetupLow (m_low);

  m_txop = CreateObject<Txop> ();
  m_txop->SetMacLow (m_low);
  m_txop->SetChannelAccessManager (m_channelAccessManager);
  m_txop->SetTxMiddle (m_txMiddle);
  m_txop->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  m_txop->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  m_txop->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));

  //Construct the EDCAFs. The ordering is important - highest
  //priority (Table 9-1 UP-to-AC mapping; IEEE 802.11-2012) must be created
  //first.
  SetupEdcaQueue (AC_VO);
  SetupEdcaQueue (AC_VI);
  SetupEdcaQueue (AC_BE);
  SetupEdcaQueue (AC_BK);
}

RegularWifiMac::~RegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
RegularWifiMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_txop->Initialize ();

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Initialize ();
    }
}

void
RegularWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rxMiddle = 0;
  m_txMiddle = 0;

  m_low->Dispose ();
  m_low = 0;

  m_phy = 0;
  m_stationManager = 0;

  m_txop->Dispose ();
  m_txop = 0;

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Dispose ();
      i->second = 0;
    }

  m_channelAccessManager->Dispose ();
  m_channelAccessManager = 0;
  
  WifiMac::DoDispose ();
}

void
RegularWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_stationManager = stationManager;
  m_low->SetWifiRemoteStationManager (stationManager);
  m_txop->SetWifiRemoteStationManager (stationManager);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetWifiRemoteStationManager (stationManager);
    }
}

Ptr<WifiRemoteStationManager>
RegularWifiMac::GetWifiRemoteStationManager () const
{
  return m_stationManager;
}

ExtendedCapabilities
RegularWifiMac::GetExtendedCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  ExtendedCapabilities capabilities;
  capabilities.SetHtSupported (GetHtSupported ());
  capabilities.SetVhtSupported (GetVhtSupported ());
  //TODO: to be completed
  return capabilities;
}

HtCapabilities
RegularWifiMac::GetHtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HtCapabilities capabilities;
  if (GetHtSupported ())
    {
      Ptr<HtConfiguration> htConfiguration = GetHtConfiguration ();
      bool greenfieldSupported = htConfiguration->GetGreenfieldSupported ();
      bool sgiSupported = htConfiguration->GetShortGuardIntervalSupported ();
      capabilities.SetHtSupported (1);
      capabilities.SetLdpc (0);
      capabilities.SetSupportedChannelWidth (m_phy->GetChannelWidth () >= 40);
      capabilities.SetShortGuardInterval20 (sgiSupported);
      capabilities.SetShortGuardInterval40 (m_phy->GetChannelWidth () >= 40 && sgiSupported);
      capabilities.SetGreenfield (greenfieldSupported);
      // Set Maximum A-MSDU Length subfield
      uint16_t maxAmsduSize = std::max ({m_voMaxAmsduSize, m_viMaxAmsduSize,
                                         m_beMaxAmsduSize, m_bkMaxAmsduSize});
      if (maxAmsduSize <= 3839)
        {
          capabilities.SetMaxAmsduLength (3839);
        }
      else
        {
          capabilities.SetMaxAmsduLength (7935);
        }
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in HT capabilities elements ranges from 2^13-1 to 2^16-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 8191u), 65535u));

      capabilities.SetLSigProtectionSupport (!greenfieldSupported);
      uint64_t maxSupportedRate = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_HT)
            {
              continue;
            }
          capabilities.SetRxMcsBitmask (mcs.GetMcsValue ());
          uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
          NS_ASSERT (nss > 0 && nss < 5);
          uint64_t dataRate = mcs.GetDataRate (m_phy->GetChannelWidth (), sgiSupported ? 400 : 800, nss);
          if (dataRate > maxSupportedRate)
            {
              maxSupportedRate = dataRate;
              NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
            }
        }
      capabilities.SetRxHighestSupportedDataRate (static_cast<uint16_t> (maxSupportedRate / 1e6)); //in Mbit/s
      capabilities.SetTxMcsSetDefined (m_phy->GetNMcs () > 0);
      capabilities.SetTxMaxNSpatialStreams (m_phy->GetMaxSupportedTxSpatialStreams ());
      //we do not support unequal modulations
      capabilities.SetTxRxMcsSetUnequal (0);
      capabilities.SetTxUnequalModulation (0);
    }
  return capabilities;
}

VhtCapabilities
RegularWifiMac::GetVhtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  VhtCapabilities capabilities;
  if (GetVhtSupported ())
    {
      Ptr<HtConfiguration> htConfiguration = GetHtConfiguration ();
      Ptr<VhtConfiguration> vhtConfiguration = GetVhtConfiguration ();
      bool sgiSupported = htConfiguration->GetShortGuardIntervalSupported ();
      capabilities.SetVhtSupported (1);
      if (m_phy->GetChannelWidth () == 160)
        {
          capabilities.SetSupportedChannelWidthSet (1);
        }
      else
        {
          capabilities.SetSupportedChannelWidthSet (0);
        }
      // Set Maximum MPDU Length subfield
      uint16_t maxAmsduSize = std::max ({m_voMaxAmsduSize, m_viMaxAmsduSize,
                                         m_beMaxAmsduSize, m_bkMaxAmsduSize});
      if (maxAmsduSize <= 3839)
        {
          capabilities.SetMaxMpduLength (3895);
        }
      else if (maxAmsduSize <= 7935)
        {
          capabilities.SetMaxMpduLength (7991);
        }
      else
        {
          capabilities.SetMaxMpduLength (11454);
        }
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in VHT capabilities elements ranges from 2^13-1 to 2^20-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 8191u), 1048575u));

      capabilities.SetRxLdpc (0);
      capabilities.SetShortGuardIntervalFor80Mhz ((m_phy->GetChannelWidth () == 80) && sgiSupported);
      capabilities.SetShortGuardIntervalFor160Mhz ((m_phy->GetChannelWidth () == 160) && sgiSupported);
      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      // Support same MaxMCS for each spatial stream
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedRxSpatialStreams (); nss++)
        {
          capabilities.SetRxMcsMap (maxMcs, nss);
        }
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedTxSpatialStreams (); nss++)
        {
          capabilities.SetTxMcsMap (maxMcs, nss);
        }
      uint64_t maxSupportedRateLGI = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_VHT || !mcs.IsAllowed (m_phy->GetChannelWidth (), 1))
            {
              continue;
            }
          if (mcs.GetDataRate (m_phy->GetChannelWidth ()) > maxSupportedRateLGI)
            {
              maxSupportedRateLGI = mcs.GetDataRate (m_phy->GetChannelWidth ());
              NS_LOG_DEBUG ("Updating maxSupportedRateLGI to " << maxSupportedRateLGI);
            }
        }
      capabilities.SetRxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      capabilities.SetTxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      //To be filled in once supported
      capabilities.SetRxStbc (0);
      capabilities.SetTxStbc (0);
    }
  return capabilities;
}

HeCapabilities
RegularWifiMac::GetHeCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HeCapabilities capabilities;
  if (GetHeSupported ())
    {
      Ptr<HeConfiguration> heConfiguration = GetHeConfiguration ();
      capabilities.SetHeSupported (1);
      uint8_t channelWidthSet = 0;
      if ((m_phy->GetChannelWidth () >= 40) && (m_phy->GetPhyBand () == WIFI_PHY_BAND_2_4GHZ))
        {
          channelWidthSet |= 0x01;
        }
      if ((m_phy->GetChannelWidth () >= 80) && ((m_phy->GetPhyBand () == WIFI_PHY_BAND_5GHZ) || (m_phy->GetPhyBand () == WIFI_PHY_BAND_6GHZ)))
        {
          channelWidthSet |= 0x02;
        }
      if ((m_phy->GetChannelWidth () >= 160) && ((m_phy->GetPhyBand () == WIFI_PHY_BAND_5GHZ) || (m_phy->GetPhyBand () == WIFI_PHY_BAND_6GHZ)))
        {
          channelWidthSet |= 0x04;
        }
      capabilities.SetChannelWidthSet (channelWidthSet);
      uint8_t gi = 0;
      if (heConfiguration->GetGuardInterval () <= NanoSeconds (1600))
        {
          //todo: We assume for now that if we support 800ns GI then 1600ns GI is supported as well
          gi |= 0x01;
        }
      if (heConfiguration->GetGuardInterval () == NanoSeconds (800))
        {
          gi |= 0x02;
        }
      capabilities.SetHeLtfAndGiForHePpdus (gi);
      uint32_t maxAmpduLength = std::max ({m_voMaxAmpduSize, m_viMaxAmpduSize,
                                           m_beMaxAmpduSize, m_bkMaxAmpduSize});
      // round to the next power of two minus one
      maxAmpduLength = (1ul << static_cast<uint32_t> (std::ceil (std::log2 (maxAmpduLength + 1)))) - 1;
      // The maximum A-MPDU length in HE capabilities elements ranges from 2^20-1 to 2^23-1
      capabilities.SetMaxAmpduLength (std::min (std::max (maxAmpduLength, 1048575u), 8388607u));

      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_HE)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      capabilities.SetHighestMcsSupported (maxMcs);
      capabilities.SetHighestNssSupported (m_phy->GetMaxSupportedTxSpatialStreams ());
    }
  return capabilities;
}

void
RegularWifiMac::SetVoBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVOQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetViBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBeBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBEQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBkBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBKQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetVoBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVOQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetViBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBeBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBEQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBkBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBKQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetupEdcaQueue (AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);

  //Our caller shouldn't be attempting to setup a queue that is
  //already configured.
  NS_ASSERT (m_edca.find (ac) == m_edca.end ());

  Ptr<QosTxop> edca = CreateObject<QosTxop> ();
  edca->SetMacLow (m_low);
  edca->SetChannelAccessManager (m_channelAccessManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  edca->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));
  edca->SetAccessCategory (ac);
  edca->CompleteConfig ();

  m_edca.insert (std::make_pair (ac, edca));
}

void
RegularWifiMac::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetTypeOfStation (type);
    }
}

Ptr<Txop>
RegularWifiMac::GetTxop () const
{
  return m_txop;
}

Ptr<QosTxop>
RegularWifiMac::GetVOQueue () const
{
  return m_edca.find (AC_VO)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetVIQueue () const
{
  return m_edca.find (AC_VI)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBEQueue () const
{
  return m_edca.find (AC_BE)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBKQueue () const
{
  return m_edca.find (AC_BK)->second;
}

void
RegularWifiMac::SetWifiPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
  m_channelAccessManager->SetupPhyListener (phy);
  m_low->SetPhy (phy);
}

Ptr<WifiPhy>
RegularWifiMac::GetWifiPhy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
RegularWifiMac::ResetWifiPhy (void)
{
  NS_LOG_FUNCTION (this);
  m_low->ResetPhy ();
  m_channelAccessManager->RemovePhyListener (m_phy);
  m_phy = 0;
}

void
RegularWifiMac::SetForwardUpCallback (ForwardUpCallback upCallback)
{
  NS_LOG_FUNCTION (this);
  m_forwardUp = upCallback;
}

void
RegularWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = linkUp;
}

void
RegularWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  m_linkDown = linkDown;
}

void
RegularWifiMac::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_qosSupported = enable;
}

bool
RegularWifiMac::GetQosSupported () const
{
  return m_qosSupported;
}

bool
RegularWifiMac::GetHtSupported () const
{
  if (GetHtConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetVhtSupported () const
{
  if (GetVhtConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetHeSupported () const
{
  if (GetHeConfiguration ())
    {
      return true;
    }
  return false;
}

bool
RegularWifiMac::GetErpSupported () const
{
  return m_erpSupported;
}

void
RegularWifiMac::SetErpSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  if (enable)
    {
      SetDsssSupported (true);
    }
  m_erpSupported = enable;
}

void
RegularWifiMac::SetDsssSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_dsssSupported = enable;
}

bool
RegularWifiMac::GetDsssSupported () const
{
  return m_dsssSupported;
}

void
RegularWifiMac::SetCtsToSelfSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_low->SetCtsToSelfSupported (enable);
}

void
RegularWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_low->SetAddress (address);
}

Mac48Address
RegularWifiMac::GetAddress (void) const
{
  return m_low->GetAddress ();
}

void
RegularWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_FUNCTION (this << ssid);
  m_ssid = ssid;
}

Ssid
RegularWifiMac::GetSsid (void) const
{
  return m_ssid;
}

void
RegularWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << bssid);
  m_low->SetBssid (bssid);
}

Mac48Address
RegularWifiMac::GetBssid (void) const
{
  return m_low->GetBssid ();
}

void
RegularWifiMac::SetPromisc (void)
{
  m_low->SetPromisc ();
}

void
RegularWifiMac::SetShortSlotTimeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeSupported = enable;
}

bool
RegularWifiMac::GetShortSlotTimeSupported (void) const
{
  return m_shortSlotTimeSupported;
}

void
RegularWifiMac::Enqueue (Ptr<Packet> packet,
                         Mac48Address to, Mac48Address from)
{
  //We expect RegularWifiMac subclasses which do support forwarding (e.g.,
  //AP) to override this method. Therefore, we throw a fatal error if
  //someone tries to invoke this method on a class which has not done
  //this.
  NS_FATAL_ERROR ("This MAC entity (" << this << ", " << GetAddress ()
                                      << ") does not support Enqueue() with from address");
}

bool
RegularWifiMac::SupportsSendFrom (void) const
{
  return false;
}

void
RegularWifiMac::ForwardUp (Ptr<const Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  m_forwardUp (packet, from, to);
}

void
RegularWifiMac::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<Packet> packet = mpdu->GetPacket ()->Copy ();
  Mac48Address to = hdr->GetAddr1 ();
  Mac48Address from = hdr->GetAddr2 ();

  //We don't know how to deal with any frame that is not addressed to
  //us (and odds are there is nothing sensible we could do anyway),
  //so we ignore such frames.
  //
  //The derived class may also do some such filtering, but it doesn't
  //hurt to have it here too as a backstop.
  if (to != GetAddress ())
    {
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      //There is currently only any reason for Management Action
      //frames to be flying about if we are a QoS STA.
      NS_ASSERT (m_qosSupported);

      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:

          switch (actionHdr.GetAction ().blockAck)
            {
            case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
              {
                MgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader (reqHdr);

                //We've received an ADDBA Request. Our policy here is
                //to automatically accept it, so we get the ADDBA
                //Response on it's way immediately.
                SendAddBaResponse (&reqHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
              {
                MgtAddBaResponseHeader respHdr;
                packet->RemoveHeader (respHdr);

                //We've received an ADDBA Response. We assume that it
                //indicates success after an ADDBA Request we have
                //sent (we could, in principle, check this, but it
                //seems a waste given the level of the current model)
                //and act by locally establishing the agreement on
                //the appropriate queue.
                AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
                m_edca[ac]->GotAddBaResponse (&respHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_DELBA:
              {
                MgtDelBaHeader delBaHdr;
                packet->RemoveHeader (delBaHdr);

                if (delBaHdr.IsByOriginator ())
                  {
                    //This DELBA frame was sent by the originator, so
                    //this means that an ingoing established
                    //agreement exists in MacLow and we need to
                    //destroy it.
                    m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                  }
                else
                  {
                    //We must have been the originator. We need to
                    //tell the correct queue that the agreement has
                    //been torn down
                    AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                    m_edca[ac]->GotDelBaFrame (&delBaHdr, from);
                  }
                //This frame is now completely dealt with, so we're done.
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
              return;
            }
        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
          return;
        }
    }
  NS_FATAL_ERROR ("Don't know how to handle frame (type=" << hdr->GetType ());
}

void
RegularWifiMac::DeaggregateAmsduAndForward (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  for (auto& msduPair : *PeekPointer (mpdu))
    {
      ForwardUp (msduPair.first, msduPair.second.GetSourceAddr (),
                 msduPair.second.GetDestinationAddr ());
    }
}

void
RegularWifiMac::SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                   Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  MgtAddBaResponseHeader respHdr;
  StatusCode code;
  code.SetSuccess ();
  respHdr.SetStatusCode (code);
  //Here a control about queues type?
  respHdr.SetAmsduSupport (reqHdr->IsAmsduSupported ());

  if (reqHdr->IsImmediateBlockAck ())
    {
      respHdr.SetImmediateBlockAck ();
    }
  else
    {
      respHdr.SetDelayedBlockAck ();
    }
  respHdr.SetTid (reqHdr->GetTid ());

  Ptr<HeConfiguration> heConfiguration = GetHeConfiguration ();
  if (heConfiguration && heConfiguration->GetMpduBufferSize () > 64)
    {
      respHdr.SetBufferSize (255);
    }
  else
    {
      respHdr.SetBufferSize (63);
    }
  respHdr.SetTimeout (reqHdr->GetTimeout ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (respHdr);
  packet->AddHeader (actionHdr);

  //We need to notify our MacLow object as it will have to buffer all
  //correctly received packets for this Block Ack session
  m_low->CreateBlockAckAgreement (&respHdr, originator,
                                  reqHdr->GetStartingSequence ());

  //It is unclear which queue this frame should go into. For now we
  //bung it into the queue corresponding to the TID for which we are
  //establishing an agreement, and push it to the head.
  m_edca[QosUtilsMapTidToAc (reqHdr->GetTid ())]->PushFront (packet, hdr);
}

TypeId
RegularWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RegularWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("QosSupported",
                   "This Boolean attribute is set to enable 802.11e/WMM-style QoS support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetQosSupported,
                                        &RegularWifiMac::GetQosSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("CtsToSelfSupported",
                   "Use CTS to Self when using a rate that is not in the basic rate set.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetCtsToSelfSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("VO_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VO access class "
                   "(capped to 7935 for HT PPDUs and 11398 for VHT/HE PPDUs). "
                   "Value 0 means A-MSDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_voMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11398))
    .AddAttribute ("VI_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VI access class "
                   "(capped to 7935 for HT PPDUs and 11398 for VHT/HE PPDUs). "
                   "Value 0 means A-MSDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_viMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11398))
    .AddAttribute ("BE_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BE access class "
                   "(capped to 7935 for HT PPDUs and 11398 for VHT/HE PPDUs). "
                   "Value 0 means A-MSDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_beMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11398))
    .AddAttribute ("BK_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BK access class "
                   "(capped to 7935 for HT PPDUs and 11398 for VHT/HE PPDUs). "
                   "Value 0 means A-MSDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_bkMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11398))
    .AddAttribute ("VO_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VO access class "
                   "(capped to 65535 for HT PPDUs, 1048575 for VHT PPDUs, and 8388607 for HE PPDUs). "
                   "Value 0 means A-MPDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_voMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("VI_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VI access class "
                   "(capped to 65535 for HT PPDUs, 1048575 for VHT PPDUs, and 8388607 for HE PPDUs). "
                   "Value 0 means A-MPDU aggregation is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&RegularWifiMac::m_viMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BE_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BE access class "
                   "(capped to 65535 for HT PPDUs, 1048575 for VHT PPDUs, and 8388607 for HE PPDUs). "
                   "Value 0 means A-MPDU aggregation is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&RegularWifiMac::m_beMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BK_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BK access class "
                   "(capped to 65535 for HT PPDUs, 1048575 for VHT PPDUs, and 8388607 for HE PPDUs). "
                   "Value 0 means A-MPDU aggregation is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::m_bkMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("VO_BlockAckThreshold",
                   "If number of packets in VO queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("VI_BlockAckThreshold",
                   "If number of packets in VI queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BE_BlockAckThreshold",
                   "If number of packets in BE queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BK_BlockAckThreshold",
                   "If number of packets in BK queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("VO_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 microseconds) allowed for block ack"
                   "inactivity for AC_VO. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("VI_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 microseconds) allowed for block ack"
                   "inactivity for AC_VI. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BE_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 microseconds) allowed for block ack"
                   "inactivity for AC_BE. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BK_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 microseconds) allowed for block ack"
                   "inactivity for AC_BK. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ShortSlotTimeSupported",
                   "Whether or not short slot time is supported (only used by ERP APs or STAs).",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RegularWifiMac::SetShortSlotTimeSupported,
                                        &RegularWifiMac::GetShortSlotTimeSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("Txop",
                   "The Txop object.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetTxop),
                   MakePointerChecker<Txop> ())
    .AddAttribute ("VO_Txop",
                   "Queue that manages packets belonging to AC_VO access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVOQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("VI_Txop",
                   "Queue that manages packets belonging to AC_VI access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVIQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("BE_Txop",
                   "Queue that manages packets belonging to AC_BE access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBEQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("BK_Txop",
                   "Queue that manages packets belonging to AC_BK access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBKQueue),
                   MakePointerChecker<QosTxop> ())
    .AddTraceSource ("TxOkHeader",
                     "The header of successfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txOkCallback),
                     "ns3::WifiMacHeader::TracedCallback")
    .AddTraceSource ("TxErrHeader",
                     "The header of unsuccessfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txErrCallback),
                     "ns3::WifiMacHeader::TracedCallback")
  ;
  return tid;
}

void
RegularWifiMac::ConfigureStandard (WifiStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  uint32_t cwmin = 0;
  uint32_t cwmax = 0;
  switch (standard)
    {
    case WIFI_STANDARD_80211n_5GHZ:
    case WIFI_STANDARD_80211ac:
    case WIFI_STANDARD_80211ax_5GHZ:
    case WIFI_STANDARD_80211ax_6GHZ:
      {
        EnableAggregation ();
        SetQosSupported (true);
        cwmin = 15;
        cwmax = 1023;
        break;
      }
    case WIFI_STANDARD_80211ax_2_4GHZ:
    case WIFI_STANDARD_80211n_2_4GHZ:
      {
        EnableAggregation ();
        SetQosSupported (true);
      }
    case WIFI_STANDARD_80211g:
      SetErpSupported (true);
    case WIFI_STANDARD_holland:
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211p:
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_STANDARD_80211b:
      SetDsssSupported (true);
      cwmin = 31;
      cwmax = 1023;
      break;
    default:
      NS_FATAL_ERROR ("Unsupported WifiPhyStandard in RegularWifiMac::FinishConfigureStandard ()");
    }

  ConfigureContentionWindow (cwmin, cwmax);
}

void
RegularWifiMac::ConfigureContentionWindow (uint32_t cwMin, uint32_t cwMax)
{
  bool isDsssOnly = m_dsssSupported && !m_erpSupported;
  //The special value of AC_BE_NQOS which exists in the Access
  //Category enumeration allows us to configure plain old DCF.
  ConfigureDcf (m_txop, cwMin, cwMax, isDsssOnly, AC_BE_NQOS);

  //Now we configure the EDCA functions
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      ConfigureDcf (i->second, cwMin, cwMax, isDsssOnly, i->first);
    }
}

void
RegularWifiMac::TxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txOkCallback (hdr);
}

void
RegularWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txErrCallback (hdr);
}

void
RegularWifiMac::EnableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  if (m_low->GetMsduAggregator () == 0)
    {
      Ptr<MsduAggregator> msduAggregator = CreateObject<MsduAggregator> ();
      msduAggregator->SetEdcaQueues (m_edca);
      m_low->SetMsduAggregator (msduAggregator);
    }
  if (m_low->GetMpduAggregator () == 0)
    {
      Ptr<MpduAggregator> mpduAggregator = CreateObject<MpduAggregator> ();
      mpduAggregator->SetEdcaQueues (m_edca);
      m_low->SetMpduAggregator (mpduAggregator);
    }
}

void
RegularWifiMac::DisableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  m_low->SetMsduAggregator (0);
  m_low->SetMpduAggregator (0);
}

} //namespace ns3
