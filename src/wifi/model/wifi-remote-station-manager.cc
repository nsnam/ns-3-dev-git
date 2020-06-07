/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/simulator.h"
#include "wifi-remote-station-manager.h"
#include "wifi-phy.h"
#include "wifi-mac.h"
#include "wifi-mac-header.h"
#include "wifi-mac-trailer.h"
#include "ht-configuration.h"
#include "vht-configuration.h"
#include "he-configuration.h"
#include "wifi-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiRemoteStationManager");

NS_OBJECT_ENSURE_REGISTERED (WifiRemoteStationManager);

TypeId
WifiRemoteStationManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiRemoteStationManager")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("MaxSsrc",
                   "The maximum number of retransmission attempts for any packet with size <= RtsCtsThreshold. "
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&WifiRemoteStationManager::SetMaxSsrc),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxSlrc",
                   "The maximum number of retransmission attempts for any packet with size > RtsCtsThreshold. "
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (4),
                   MakeUintegerAccessor (&WifiRemoteStationManager::SetMaxSlrc),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RtsCtsThreshold",
                   "If the size of the PSDU is bigger than this value, we use an RTS/CTS handshake before sending the data frame."
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&WifiRemoteStationManager::SetRtsCtsThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FragmentationThreshold",
                   "If the size of the PSDU is bigger than this value, we fragment it such that the size of the fragments are equal or smaller. "
                   "This value does not apply when it is carried in an A-MPDU. "
                   "This value will not have any effect on some rate control algorithms.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&WifiRemoteStationManager::DoSetFragmentationThreshold,
                                         &WifiRemoteStationManager::DoGetFragmentationThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NonUnicastMode",
                   "Wifi mode used for non-unicast transmissions.",
                   WifiModeValue (),
                   MakeWifiModeAccessor (&WifiRemoteStationManager::m_nonUnicastMode),
                   MakeWifiModeChecker ())
    .AddAttribute ("DefaultTxPowerLevel",
                   "Default power level to be used for transmissions. "
                   "This is the power level that is used by all those WifiManagers that do not implement TX power control.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiRemoteStationManager::m_defaultTxPowerLevel),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("ErpProtectionMode",
                   "Protection mode used when non-ERP STAs are connected to an ERP AP: Rts-Cts or Cts-To-Self",
                   EnumValue (WifiRemoteStationManager::CTS_TO_SELF),
                   MakeEnumAccessor (&WifiRemoteStationManager::m_erpProtectionMode),
                   MakeEnumChecker (WifiRemoteStationManager::RTS_CTS, "Rts-Cts",
                                    WifiRemoteStationManager::CTS_TO_SELF, "Cts-To-Self"))
    .AddAttribute ("HtProtectionMode",
                   "Protection mode used when non-HT STAs are connected to a HT AP: Rts-Cts or Cts-To-Self",
                   EnumValue (WifiRemoteStationManager::CTS_TO_SELF),
                   MakeEnumAccessor (&WifiRemoteStationManager::m_htProtectionMode),
                   MakeEnumChecker (WifiRemoteStationManager::RTS_CTS, "Rts-Cts",
                                    WifiRemoteStationManager::CTS_TO_SELF, "Cts-To-Self"))
    .AddTraceSource ("MacTxRtsFailed",
                     "The transmission of a RTS by the MAC layer has failed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxRtsFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxDataFailed",
                     "The transmission of a data packet by the MAC layer has failed",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxDataFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxFinalRtsFailed",
                     "The transmission of a RTS has exceeded the maximum number of attempts",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxFinalRtsFailed),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("MacTxFinalDataFailed",
                     "The transmission of a data packet has exceeded the maximum number of attempts",
                     MakeTraceSourceAccessor (&WifiRemoteStationManager::m_macTxFinalDataFailed),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

WifiRemoteStationManager::WifiRemoteStationManager ()
  : m_pcfSupported (false),
    m_useNonErpProtection (false),
    m_useNonHtProtection (false),
    m_useGreenfieldProtection (false),
    m_shortPreambleEnabled (false),
    m_shortSlotTimeEnabled (false)
{
  NS_LOG_FUNCTION (this);
}

WifiRemoteStationManager::~WifiRemoteStationManager ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiRemoteStationManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Reset ();
}

void
WifiRemoteStationManager::SetupPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  //We need to track our PHY because it is the object that knows the
  //full set of transmit rates that are supported. We need to know
  //this in order to find the relevant mandatory rates when choosing a
  //transmit rate for automatic control responses like
  //acknowledgments.
  m_wifiPhy = phy;
  m_defaultTxMode = phy->GetMode (0);
  NS_ASSERT (m_defaultTxMode.IsMandatory ());
  if (GetHtSupported ())
    {
      m_defaultTxMcs = phy->GetMcs (0);
    }
  Reset ();
}

void
WifiRemoteStationManager::SetupMac (const Ptr<WifiMac> mac)
{
  NS_LOG_FUNCTION (this << mac);
  //We need to track our MAC because it is the object that knows the
  //full set of interframe spaces.
  m_wifiMac = mac;
  Reset ();
}

void
WifiRemoteStationManager::SetMaxSsrc (uint32_t maxSsrc)
{
  NS_LOG_FUNCTION (this << maxSsrc);
  m_maxSsrc = maxSsrc;
}

void
WifiRemoteStationManager::SetMaxSlrc (uint32_t maxSlrc)
{
  NS_LOG_FUNCTION (this << maxSlrc);
  m_maxSlrc = maxSlrc;
}

void
WifiRemoteStationManager::SetRtsCtsThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_rtsCtsThreshold = threshold;
}

void
WifiRemoteStationManager::SetFragmentationThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  DoSetFragmentationThreshold (threshold);
}

void
WifiRemoteStationManager::SetShortPreambleEnabled (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortPreambleEnabled = enable;
}

void
WifiRemoteStationManager::SetShortSlotTimeEnabled (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeEnabled = enable;
}

bool
WifiRemoteStationManager::GetShortSlotTimeEnabled (void) const
{
  return m_shortSlotTimeEnabled;
}

bool
WifiRemoteStationManager::GetShortPreambleEnabled (void) const
{
  return m_shortPreambleEnabled;
}

bool
WifiRemoteStationManager::GetHtSupported (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
  Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
  if (htConfiguration)
    {
      return true;
    }
  return false;
}

bool
WifiRemoteStationManager::GetVhtSupported (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
  Ptr<VhtConfiguration> vhtConfiguration = device->GetVhtConfiguration ();
  if (vhtConfiguration)
    {
      return true;
    }
  return false;
}

bool
WifiRemoteStationManager::GetHeSupported (void) const
{
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
  Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
  if (heConfiguration)
    {
      return true;
    }
  return false;
}

void
WifiRemoteStationManager::SetPcfSupported (bool enable)
{
  m_pcfSupported = enable;
}

bool
WifiRemoteStationManager::GetPcfSupported (void) const
{
  return m_pcfSupported;
}

bool
WifiRemoteStationManager::GetGreenfieldSupported (void) const
{
  if (GetHtSupported ())
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      NS_ASSERT (htConfiguration); //If HT is supported, we should have a HT configuration attached
      if (htConfiguration->GetGreenfieldSupported ())
        {
          return true;
        }
    }
  return false;
}

bool
WifiRemoteStationManager::GetShortGuardIntervalSupported (void) const
{
  if (GetHtSupported ())
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
      Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration ();
      NS_ASSERT (htConfiguration); //If HT is supported, we should have a HT configuration attached
      if (htConfiguration->GetShortGuardIntervalSupported ())
        {
          return true;
        }
    }
  return false;
}

uint16_t
WifiRemoteStationManager::GetGuardInterval (void) const
{
  uint16_t gi = 0;
  if (GetHeSupported ())
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      NS_ASSERT (heConfiguration); //If HE is supported, we should have a HE configuration attached
      gi = static_cast<uint16_t>(heConfiguration->GetGuardInterval ().GetNanoSeconds ());
    }
  return gi;
}

uint32_t
WifiRemoteStationManager::GetFragmentationThreshold (void) const
{
  return DoGetFragmentationThreshold ();
}

void
WifiRemoteStationManager::AddSupportedPhyPreamble (Mac48Address address, bool isShortPreambleSupported)
{
  NS_LOG_FUNCTION (this << address << isShortPreambleSupported);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_shortPreamble = isShortPreambleSupported;
}

void
WifiRemoteStationManager::AddSupportedErpSlotTime (Mac48Address address, bool isShortSlotTimeSupported)
{
  NS_LOG_FUNCTION (this << address << isShortSlotTimeSupported);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_shortSlotTime = isShortSlotTimeSupported;
}

void
WifiRemoteStationManager::AddSupportedMode (Mac48Address address, WifiMode mode)
{
  NS_LOG_FUNCTION (this << address << mode);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  for (WifiModeListIterator i = state->m_operationalRateSet.begin (); i != state->m_operationalRateSet.end (); i++)
    {
      if ((*i) == mode)
        {
          //already in.
          return;
        }
    }
  state->m_operationalRateSet.push_back (mode);
}

void
WifiRemoteStationManager::AddAllSupportedModes (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalRateSet.clear ();
  for (uint8_t i = 0; i < m_wifiPhy->GetNModes (); i++)
    {
      state->m_operationalRateSet.push_back (m_wifiPhy->GetMode (i));
      if (m_wifiPhy->GetMode (i).IsMandatory ())
        {
          AddBasicMode (m_wifiPhy->GetMode (i));
        }
    }
}

void
WifiRemoteStationManager::AddAllSupportedMcs (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalMcsSet.clear ();
  for (uint8_t i = 0; i < m_wifiPhy->GetNMcs (); i++)
    {
      state->m_operationalMcsSet.push_back (m_wifiPhy->GetMcs (i));
    }
}

void
WifiRemoteStationManager::RemoveAllSupportedMcs (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  state->m_operationalMcsSet.clear ();
}

void
WifiRemoteStationManager::AddSupportedMcs (Mac48Address address, WifiMode mcs)
{
  NS_LOG_FUNCTION (this << address << mcs);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStationState *state = LookupState (address);
  for (WifiModeListIterator i = state->m_operationalMcsSet.begin (); i != state->m_operationalMcsSet.end (); i++)
    {
      if ((*i) == mcs)
        {
          //already in.
          return;
        }
    }
  state->m_operationalMcsSet.push_back (mcs);
}

bool
WifiRemoteStationManager::GetShortPreambleSupported (Mac48Address address) const
{
  return LookupState (address)->m_shortPreamble;
}

bool
WifiRemoteStationManager::GetShortSlotTimeSupported (Mac48Address address) const
{
  return LookupState (address)->m_shortSlotTime;
}

bool
WifiRemoteStationManager::GetQosSupported (Mac48Address address) const
{
  return LookupState (address)->m_qosSupported;
}

bool
WifiRemoteStationManager::IsBrandNew (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return false;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::BRAND_NEW;
}

bool
WifiRemoteStationManager::IsAssociated (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return true;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::GOT_ASSOC_TX_OK;
}

bool
WifiRemoteStationManager::IsWaitAssocTxOk (Mac48Address address) const
{
  if (address.IsGroup ())
    {
      return false;
    }
  return LookupState (address)->m_state == WifiRemoteStationState::WAIT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordWaitAssocTxOk (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::WAIT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordGotAssocTxOk (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::GOT_ASSOC_TX_OK;
}

void
WifiRemoteStationManager::RecordGotAssocTxFailed (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::DISASSOC;
}

void
WifiRemoteStationManager::RecordDisassociated (Mac48Address address)
{
  NS_ASSERT (!address.IsGroup ());
  LookupState (address)->m_state = WifiRemoteStationState::DISASSOC;
}

WifiTxVector
WifiRemoteStationManager::GetDataTxVector (const WifiMacHeader &header)
{
  NS_LOG_FUNCTION (this << header);
  Mac48Address address = header.GetAddr1 ();
  if (!header.IsMgt () && address.IsGroup ())
    {
      WifiMode mode = GetNonUnicastMode ();
      WifiTxVector v;
      v.SetMode (mode);
      v.SetPreambleType (GetPreambleForTransmission (mode.GetModulationClass (), GetShortPreambleEnabled (), UseGreenfieldForDestination (address)));
      v.SetTxPowerLevel (m_defaultTxPowerLevel);
      v.SetChannelWidth (GetChannelWidthForTransmission (mode, m_wifiPhy->GetChannelWidth ()));
      v.SetGuardInterval (ConvertGuardIntervalToNanoSeconds (mode, DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ())));
      v.SetNTx (1);
      v.SetNss (1);
      v.SetNess (0);
      v.SetStbc (0);
      return v;
    }
  WifiTxVector txVector;
  if (header.IsMgt ())
    {
      //Use the lowest basic rate for management frames
      WifiMode mgtMode;
      if (GetNBasicModes () > 0)
        {
          mgtMode = GetBasicMode (0);
        }
      else
        {
          mgtMode = GetDefaultMode ();
        }
      txVector.SetMode (mgtMode);
      txVector.SetPreambleType (GetPreambleForTransmission (mgtMode.GetModulationClass (), GetShortPreambleEnabled (), UseGreenfieldForDestination (address)));
      txVector.SetTxPowerLevel (m_defaultTxPowerLevel);
      txVector.SetChannelWidth (GetChannelWidthForTransmission (mgtMode, m_wifiPhy->GetChannelWidth ()));
      txVector.SetGuardInterval (ConvertGuardIntervalToNanoSeconds (mgtMode, DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ())));
    }
  else
    {
      txVector = DoGetDataTxVector (Lookup (address));
    }
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
  Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
  if (heConfiguration)
    {
      UintegerValue bssColor;
      heConfiguration->GetAttribute ("BssColor", bssColor);
      txVector.SetBssColor (bssColor.Get ());
    }
  return txVector;
}

WifiTxVector
WifiRemoteStationManager::GetCtsToSelfTxVector (void)
{
  WifiMode defaultMode = GetDefaultMode ();
  WifiPreamble defaultPreamble;
  if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      defaultPreamble = WIFI_PREAMBLE_HE_SU;
    }
  else if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      defaultPreamble = WIFI_PREAMBLE_VHT_SU;
    }
  else if (defaultMode.GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      defaultPreamble = WIFI_PREAMBLE_HT_MF;
    }
  else
    {
      defaultPreamble = WIFI_PREAMBLE_LONG;
    }

  return WifiTxVector (defaultMode,
                       GetDefaultTxPowerLevel (),
                       defaultPreamble,
                       ConvertGuardIntervalToNanoSeconds (defaultMode, DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ())),
                       GetNumberOfAntennas (),
                       GetMaxNumberOfTransmitStreams (),
                       0,
                       GetChannelWidthForTransmission (defaultMode, m_wifiPhy->GetChannelWidth ()),
                       false,
                       false);
}

WifiTxVector
WifiRemoteStationManager::GetRtsTxVector (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  if (address.IsGroup ())
    {
        WifiMode mode = GetNonUnicastMode ();
        WifiTxVector v;
        v.SetMode (mode);
        v.SetPreambleType (GetPreambleForTransmission (mode.GetModulationClass (), GetShortPreambleEnabled (), UseGreenfieldForDestination (address)));
        v.SetTxPowerLevel (m_defaultTxPowerLevel);
        v.SetChannelWidth (GetChannelWidthForTransmission (mode, m_wifiPhy->GetChannelWidth ()));
        v.SetGuardInterval (ConvertGuardIntervalToNanoSeconds (mode, DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ())));
        v.SetNTx (1);
        v.SetNss (1);
        v.SetNess (0);
        v.SetStbc (0);
        return v;
    }
  return DoGetRtsTxVector (Lookup (address));
}

void
WifiRemoteStationManager::ReportRtsFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  m_ssrc[ac]++;
  m_macTxRtsFailed (address);
  DoReportRtsFailed (Lookup (address));
}

void
WifiRemoteStationManager::ReportDataFailed (Mac48Address address, const WifiMacHeader *header,
                                            uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  bool longMpdu = (packetSize + header->GetSize () + WIFI_MAC_FCS_LENGTH) > m_rtsCtsThreshold;
  if (longMpdu)
    {
      m_slrc[ac]++;
    }
  else
    {
      m_ssrc[ac]++;
    }
  m_macTxDataFailed (address);
  DoReportDataFailed (Lookup (address));
}

void
WifiRemoteStationManager::ReportRtsOk (Mac48Address address, const WifiMacHeader *header,
                                       double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << address << *header << ctsSnr << ctsMode << rtsSnr);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address);
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  station->m_state->m_info.NotifyTxSuccess (m_ssrc[ac]);
  m_ssrc[ac] = 0;
  DoReportRtsOk (station, ctsSnr, ctsMode, rtsSnr);
}

void
WifiRemoteStationManager::ReportDataOk (Mac48Address address, const WifiMacHeader *header,
                                        double ackSnr, WifiMode ackMode, double dataSnr,
                                        WifiTxVector dataTxVector, uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << address << *header << ackSnr << ackMode << dataSnr << dataTxVector << packetSize);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address);
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  bool longMpdu = (packetSize + header->GetSize () + WIFI_MAC_FCS_LENGTH) > m_rtsCtsThreshold;
  if (longMpdu)
    {
      station->m_state->m_info.NotifyTxSuccess (m_slrc[ac]);
      m_slrc[ac] = 0;
    }
  else
    {
      station->m_state->m_info.NotifyTxSuccess (m_ssrc[ac]);
      m_ssrc[ac] = 0;
    }
  DoReportDataOk (station, ackSnr, ackMode, dataSnr, dataTxVector.GetChannelWidth (), dataTxVector.GetNss ());
}

void
WifiRemoteStationManager::ReportFinalRtsFailed (Mac48Address address, const WifiMacHeader *header)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address);
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  station->m_state->m_info.NotifyTxFailed ();
  m_ssrc[ac] = 0;
  m_macTxFinalRtsFailed (address);
  DoReportFinalRtsFailed (station);
}

void
WifiRemoteStationManager::ReportFinalDataFailed (Mac48Address address, const WifiMacHeader *header,
                                                 uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << address << *header);
  NS_ASSERT (!address.IsGroup ());
  WifiRemoteStation *station = Lookup (address);
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  station->m_state->m_info.NotifyTxFailed ();
  bool longMpdu = (packetSize + header->GetSize () + WIFI_MAC_FCS_LENGTH) > m_rtsCtsThreshold;
  if (longMpdu)
    {
      m_slrc[ac] = 0;
    }
  else
    {
      m_ssrc[ac] = 0;
    }
  m_macTxFinalDataFailed (address);
  DoReportFinalDataFailed (station);
}

void
WifiRemoteStationManager::ReportRxOk (Mac48Address address, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << address << rxSnr << txMode);
  if (address.IsGroup ())
    {
      return;
    }
  DoReportRxOk (Lookup (address), rxSnr, txMode);
}

void
WifiRemoteStationManager::ReportAmpduTxStatus (Mac48Address address,
                                               uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus,
                                               double rxSnr, double dataSnr, WifiTxVector dataTxVector)
{
  NS_LOG_FUNCTION (this << address << +nSuccessfulMpdus << +nFailedMpdus << rxSnr << dataSnr << dataTxVector);
  NS_ASSERT (!address.IsGroup ());
  for (uint8_t i = 0; i < nFailedMpdus; i++)
    {
      m_macTxDataFailed (address);
    }
  DoReportAmpduTxStatus (Lookup (address), nSuccessfulMpdus, nFailedMpdus, rxSnr, dataSnr, dataTxVector.GetChannelWidth (), dataTxVector.GetNss ());
}

bool
WifiRemoteStationManager::NeedRts (const WifiMacHeader &header, uint32_t size)
{
  NS_LOG_FUNCTION (this << header << size);
  Mac48Address address = header.GetAddr1 ();
  WifiTxVector txVector = GetDataTxVector (header);
  WifiMode mode = txVector.GetMode ();
  if (address.IsGroup ())
    {
      return false;
    }
  if (m_erpProtectionMode == RTS_CTS
      && ((mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HE))
      && m_useNonErpProtection)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedRTS returning true to protect non-ERP stations");
      return true;
    }
  else if (m_htProtectionMode == RTS_CTS
           && ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
               || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
           && m_useNonHtProtection
           && !(m_erpProtectionMode != RTS_CTS && m_useNonErpProtection))
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedRTS returning true to protect non-HT stations");
      return true;
    }
  bool normally = (size > m_rtsCtsThreshold);
  return DoNeedRts (Lookup (address), size, normally);
}

bool
WifiRemoteStationManager::NeedCtsToSelf (WifiTxVector txVector)
{
  WifiMode mode = txVector.GetMode ();
  NS_LOG_FUNCTION (this << mode);
  if (m_erpProtectionMode == CTS_TO_SELF
      && ((mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
          || (mode.GetModulationClass () == WIFI_MOD_CLASS_HE))
      && m_useNonErpProtection)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true to protect non-ERP stations");
      return true;
    }
  else if (m_htProtectionMode == CTS_TO_SELF
           && ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
               || (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
           && m_useNonHtProtection
           && !(m_erpProtectionMode != CTS_TO_SELF && m_useNonErpProtection))
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true to protect non-HT stations");
      return true;
    }
  else if (!m_useNonErpProtection)
    {
      //search for the BSS Basic Rate set, if the used mode is in the basic set then there is no need for CTS To Self
      for (WifiModeListIterator i = m_bssBasicRateSet.begin (); i != m_bssBasicRateSet.end (); i++)
        {
          if (mode == *i)
            {
              NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning false");
              return false;
            }
        }
      if (GetHtSupported ())
        {
          //search for the BSS Basic MCS set, if the used mode is in the basic set then there is no need for CTS To Self
          for (WifiModeListIterator i = m_bssBasicMcsSet.begin (); i != m_bssBasicMcsSet.end (); i++)
            {
              if (mode == *i)
                {
                  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning false");
                  return false;
                }
            }
        }
      NS_LOG_DEBUG ("WifiRemoteStationManager::NeedCtsToSelf returning true");
      return true;
    }
  return false;
}

void
WifiRemoteStationManager::SetUseNonErpProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useNonErpProtection = enable;
}

bool
WifiRemoteStationManager::GetUseNonErpProtection (void) const
{
  return m_useNonErpProtection;
}

void
WifiRemoteStationManager::SetUseNonHtProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useNonHtProtection = enable;
}

bool
WifiRemoteStationManager::GetUseNonHtProtection (void) const
{
  return m_useNonHtProtection;
}

void
WifiRemoteStationManager::SetUseGreenfieldProtection (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_useGreenfieldProtection = enable;
}

bool
WifiRemoteStationManager::GetUseGreenfieldProtection (void) const
{
  return m_useGreenfieldProtection;
}

bool
WifiRemoteStationManager::NeedRetransmission (Mac48Address address, const WifiMacHeader *header,
                                              Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << packet << *header);
  NS_ASSERT (!address.IsGroup ());
  AcIndex ac = QosUtilsMapTidToAc ((header->IsQosData ()) ? header->GetQosTid () : 0);
  bool longMpdu = (packet->GetSize () + header->GetSize () + WIFI_MAC_FCS_LENGTH) > m_rtsCtsThreshold;
  uint32_t retryCount, maxRetryCount;
  if (longMpdu)
    {
      retryCount = m_slrc[ac];
      maxRetryCount = m_maxSlrc;
    }
  else
    {
      retryCount = m_ssrc[ac];
      maxRetryCount = m_maxSsrc;
    }
  bool normally = retryCount < maxRetryCount;
  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedRetransmission count: " << retryCount << " result: " << std::boolalpha << normally);
  return DoNeedRetransmission (Lookup (address), packet, normally);
}

bool
WifiRemoteStationManager::NeedFragmentation (Mac48Address address, const WifiMacHeader *header,
                                             Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << address << packet << *header);
  if (address.IsGroup ())
    {
      return false;
    }
  bool normally = (packet->GetSize () + header->GetSize () + WIFI_MAC_FCS_LENGTH) > GetFragmentationThreshold ();
  NS_LOG_DEBUG ("WifiRemoteStationManager::NeedFragmentation result: " << std::boolalpha << normally);
  return DoNeedFragmentation (Lookup (address), packet, normally);
}

void
WifiRemoteStationManager::DoSetFragmentationThreshold (uint32_t threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  if (threshold < 256)
    {
      /*
       * ASN.1 encoding of the MAC and PHY MIB (256 ... 8000)
       */
      NS_LOG_WARN ("Fragmentation threshold should be larger than 256. Setting to 256.");
      m_nextFragmentationThreshold = 256;
    }
  else
    {
      /*
       * The length of each fragment shall be an even number of octets, except for the last fragment if an MSDU or
       * MMPDU, which may be either an even or an odd number of octets.
       */
      if (threshold % 2 != 0)
        {
          NS_LOG_WARN ("Fragmentation threshold should be an even number. Setting to " << threshold - 1);
          m_nextFragmentationThreshold = threshold - 1;
        }
      else
        {
          m_nextFragmentationThreshold = threshold;
        }
    }
}

void
WifiRemoteStationManager::UpdateFragmentationThreshold (void)
{
  m_fragmentationThreshold = m_nextFragmentationThreshold;
}

uint32_t
WifiRemoteStationManager::DoGetFragmentationThreshold (void) const
{
  return m_fragmentationThreshold;
}

uint32_t
WifiRemoteStationManager::GetNFragments (const WifiMacHeader *header, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << *header << packet);
  //The number of bytes a fragment can support is (Threshold - WIFI_HEADER_SIZE - WIFI_FCS).
  uint32_t nFragments = (packet->GetSize () / (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH));

  //If the size of the last fragment is not 0.
  if ((packet->GetSize () % (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH)) > 0)
    {
      nFragments++;
    }
  NS_LOG_DEBUG ("WifiRemoteStationManager::GetNFragments returning " << nFragments);
  return nFragments;
}

uint32_t
WifiRemoteStationManager::GetFragmentSize (Mac48Address address, const WifiMacHeader *header,
                                           Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  uint32_t nFragment = GetNFragments (header, packet);
  if (fragmentNumber >= nFragment)
    {
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning 0");
      return 0;
    }
  //Last fragment
  if (fragmentNumber == nFragment - 1)
    {
      uint32_t lastFragmentSize = packet->GetSize () - (fragmentNumber * (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH));
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning " << lastFragmentSize);
      return lastFragmentSize;
    }
  //All fragments but the last, the number of bytes is (Threshold - WIFI_HEADER_SIZE - WIFI_FCS).
  else
    {
      uint32_t fragmentSize = GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH;
      NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentSize returning " << fragmentSize);
      return fragmentSize;
    }
}

uint32_t
WifiRemoteStationManager::GetFragmentOffset (Mac48Address address, const WifiMacHeader *header,
                                             Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  NS_ASSERT (fragmentNumber < GetNFragments (header, packet));
  uint32_t fragmentOffset = fragmentNumber * (GetFragmentationThreshold () - header->GetSize () - WIFI_MAC_FCS_LENGTH);
  NS_LOG_DEBUG ("WifiRemoteStationManager::GetFragmentOffset returning " << fragmentOffset);
  return fragmentOffset;
}

bool
WifiRemoteStationManager::IsLastFragment (Mac48Address address, const WifiMacHeader *header,
                                          Ptr<const Packet> packet, uint32_t fragmentNumber)
{
  NS_LOG_FUNCTION (this << address << *header << packet << fragmentNumber);
  NS_ASSERT (!address.IsGroup ());
  bool isLast = fragmentNumber == (GetNFragments (header, packet) - 1);
  NS_LOG_DEBUG ("WifiRemoteStationManager::IsLastFragment returning " << std::boolalpha << isLast);
  return isLast;
}

uint8_t
WifiRemoteStationManager::GetDefaultTxPowerLevel (void) const
{
  return m_defaultTxPowerLevel;
}

WifiRemoteStationInfo
WifiRemoteStationManager::GetInfo (Mac48Address address)
{
  WifiRemoteStationState *state = LookupState (address);
  return state->m_info;
}

WifiRemoteStationState *
WifiRemoteStationManager::LookupState (Mac48Address address) const
{
  NS_LOG_FUNCTION (this << address);
  for (StationStates::const_iterator i = m_states.begin (); i != m_states.end (); i++)
    {
      if ((*i)->m_address == address)
        {
          NS_LOG_DEBUG ("WifiRemoteStationManager::LookupState returning existing state");
          return (*i);
        }
    }
  WifiRemoteStationState *state = new WifiRemoteStationState ();
  state->m_state = WifiRemoteStationState::BRAND_NEW;
  state->m_address = address;
  state->m_operationalRateSet.push_back (GetDefaultMode ());
  state->m_operationalMcsSet.push_back (GetDefaultMcs ());
  state->m_htCapabilities = 0;
  state->m_vhtCapabilities = 0;
  state->m_heCapabilities = 0;
  state->m_channelWidth = m_wifiPhy->GetChannelWidth ();
  state->m_guardInterval = GetGuardInterval ();
  state->m_ness = 0;
  state->m_aggregation = false;
  state->m_qosSupported = false;
  const_cast<WifiRemoteStationManager *> (this)->m_states.push_back (state);
  NS_LOG_DEBUG ("WifiRemoteStationManager::LookupState returning new state");
  return state;
}

WifiRemoteStation *
WifiRemoteStationManager::Lookup (Mac48Address address) const
{
  NS_LOG_FUNCTION (this << address);
  for (Stations::const_iterator i = m_stations.begin (); i != m_stations.end (); i++)
    {
      if ((*i)->m_state->m_address == address)
        {
          return (*i);
        }
    }
  WifiRemoteStationState *state = LookupState (address);

  WifiRemoteStation *station = DoCreateStation ();
  station->m_state = state;
  const_cast<WifiRemoteStationManager *> (this)->m_stations.push_back (station);
  return station;
}

void
WifiRemoteStationManager::SetQosSupport (Mac48Address from, bool qosSupported)
{
  NS_LOG_FUNCTION (this << from << qosSupported);
  WifiRemoteStationState *state;
  state = LookupState (from);
  state->m_qosSupported = qosSupported;
}

void
WifiRemoteStationManager::AddStationHtCapabilities (Mac48Address from, HtCapabilities htCapabilities)
{
  //Used by all stations to record HT capabilities of remote stations
  NS_LOG_FUNCTION (this << from << htCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  if (htCapabilities.GetSupportedChannelWidth () == 1)
    {
      state->m_channelWidth = 40;
    }
  else
    {
      state->m_channelWidth = 20;
    }
  SetQosSupport (from, true);
  for (uint8_t j = 0; j < m_wifiPhy->GetNMcs (); j++)
    {
      WifiMode mcs = m_wifiPhy->GetMcs (j);
      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HT && htCapabilities.IsSupportedMcs (mcs.GetMcsValue ()))
        {
          AddSupportedMcs (from, mcs);
        }
    }
  state->m_htCapabilities = Create<const HtCapabilities> (htCapabilities);
}

void
WifiRemoteStationManager::AddStationVhtCapabilities (Mac48Address from, VhtCapabilities vhtCapabilities)
{
  //Used by all stations to record VHT capabilities of remote stations
  NS_LOG_FUNCTION (this << from << vhtCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  if (vhtCapabilities.GetSupportedChannelWidthSet () == 1)
    {
      state->m_channelWidth = 160;
    }
  else
    {
      state->m_channelWidth = 80;
    }
  //This is a workaround to enable users to force a 20 or 40 MHz channel for a VHT-compliant device,
  //since IEEE 802.11ac standard says that 20, 40 and 80 MHz channels are mandatory.
  if (m_wifiPhy->GetChannelWidth () < state->m_channelWidth)
    {
      state->m_channelWidth = m_wifiPhy->GetChannelWidth ();
    }
  for (uint8_t i = 1; i <= m_wifiPhy->GetMaxSupportedTxSpatialStreams (); i++)
    {
      for (uint8_t j = 0; j < m_wifiPhy->GetNMcs (); j++)
        {
          WifiMode mcs = m_wifiPhy->GetMcs (j);
          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities.IsSupportedMcs (mcs.GetMcsValue (), i))
            {
              AddSupportedMcs (from, mcs);
            }
        }
    }
  state->m_vhtCapabilities = Create<const VhtCapabilities> (vhtCapabilities);
}

void
WifiRemoteStationManager::AddStationHeCapabilities (Mac48Address from, HeCapabilities heCapabilities)
{
  //Used by all stations to record HE capabilities of remote stations
  NS_LOG_FUNCTION (this << from << heCapabilities);
  WifiRemoteStationState *state;
  state = LookupState (from);
  if ((m_wifiPhy->GetPhyBand () == WIFI_PHY_BAND_5GHZ) || (m_wifiPhy->GetPhyBand () == WIFI_PHY_BAND_6GHZ))
    {
      if (heCapabilities.GetChannelWidthSet () & 0x04)
        {
          state->m_channelWidth = 160;
        }
      else if (heCapabilities.GetChannelWidthSet () & 0x02)
        {
          state->m_channelWidth = 80;
        }
      //For other cases at 5 GHz, the supported channel width is set by the VHT capabilities
    }
  else if (m_wifiPhy->GetPhyBand () == WIFI_PHY_BAND_2_4GHZ)
    {
      if (heCapabilities.GetChannelWidthSet () & 0x01)
        {
          state->m_channelWidth = 40;
        }
      else
        {
          state->m_channelWidth = 20;
        }
    }
  if (heCapabilities.GetHeLtfAndGiForHePpdus () >= 2)
    {
      state->m_guardInterval = 800;
    }
  else if (heCapabilities.GetHeLtfAndGiForHePpdus () == 1)
    {
      state->m_guardInterval = 1600;
    }
  else
    {
      state->m_guardInterval = 3200;
    }
  for (uint8_t i = 1; i <= m_wifiPhy->GetMaxSupportedTxSpatialStreams (); i++)
    {
      for (uint8_t j = 0; j < m_wifiPhy->GetNMcs (); j++)
        {
          WifiMode mcs = m_wifiPhy->GetMcs (j);
          if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HE
              && heCapabilities.GetHighestNssSupported () >= i
              && heCapabilities.GetHighestMcsSupported () >= j)
            {
              AddSupportedMcs (from, mcs);
            }
        }
    }
  state->m_heCapabilities = Create<const HeCapabilities> (heCapabilities);
  SetQosSupport (from, true);
}

Ptr<const HtCapabilities>
WifiRemoteStationManager::GetStationHtCapabilities (Mac48Address from)
{
  return LookupState (from)->m_htCapabilities;
}

Ptr<const VhtCapabilities>
WifiRemoteStationManager::GetStationVhtCapabilities (Mac48Address from)
{
  return LookupState (from)->m_vhtCapabilities;
}

Ptr<const HeCapabilities>
WifiRemoteStationManager::GetStationHeCapabilities (Mac48Address from)
{
  return LookupState (from)->m_heCapabilities;
}

bool
WifiRemoteStationManager::GetGreenfieldSupported (Mac48Address address) const
{
  Ptr<const HtCapabilities> htCapabilities = LookupState (address)->m_htCapabilities;

  if (!htCapabilities)
    {
      return false;
    }
  return htCapabilities->GetGreenfield ();
}

WifiMode
WifiRemoteStationManager::GetDefaultMode (void) const
{
  return m_defaultTxMode;
}

WifiMode
WifiRemoteStationManager::GetDefaultMcs (void) const
{
  return m_defaultTxMcs;
}

void
WifiRemoteStationManager::Reset (void)
{
  NS_LOG_FUNCTION (this);
  for (StationStates::const_iterator i = m_states.begin (); i != m_states.end (); i++)
    {
      delete (*i);
    }
  m_states.clear ();
  for (Stations::const_iterator i = m_stations.begin (); i != m_stations.end (); i++)
    {
      delete (*i);
    }
  m_stations.clear ();
  m_bssBasicRateSet.clear ();
  m_bssBasicMcsSet.clear ();
  m_ssrc.fill (0);
  m_slrc.fill (0);
}

void
WifiRemoteStationManager::AddBasicMode (WifiMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  if (mode.GetModulationClass () >= WIFI_MOD_CLASS_HT)
    {
      NS_FATAL_ERROR ("It is not allowed to add a HT rate in the BSSBasicRateSet!");
    }
  for (uint8_t i = 0; i < GetNBasicModes (); i++)
    {
      if (GetBasicMode (i) == mode)
        {
          return;
        }
    }
  m_bssBasicRateSet.push_back (mode);
}

uint8_t
WifiRemoteStationManager::GetNBasicModes (void) const
{
  return static_cast<uint8_t> (m_bssBasicRateSet.size ());
}

WifiMode
WifiRemoteStationManager::GetBasicMode (uint8_t i) const
{
  NS_ASSERT (i < GetNBasicModes ());
  return m_bssBasicRateSet[i];
}

uint32_t
WifiRemoteStationManager::GetNNonErpBasicModes (void) const
{
  uint32_t size = 0;
  for (WifiModeListIterator i = m_bssBasicRateSet.begin (); i != m_bssBasicRateSet.end (); i++)
    {
      if (i->GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
        {
          continue;
        }
      size++;
    }
  return size;
}

WifiMode
WifiRemoteStationManager::GetNonErpBasicMode (uint8_t i) const
{
  NS_ASSERT (i < GetNNonErpBasicModes ());
  uint32_t index = 0;
  bool found = false;
  for (WifiModeListIterator j = m_bssBasicRateSet.begin (); j != m_bssBasicRateSet.end (); )
    {
      if (i == index)
        {
          found = true;
        }
      if (j->GetModulationClass () != WIFI_MOD_CLASS_ERP_OFDM)
        {
          if (found)
            {
              break;
            }
        }
      index++;
      j++;
    }
  return m_bssBasicRateSet[index];
}

void
WifiRemoteStationManager::AddBasicMcs (WifiMode mcs)
{
  NS_LOG_FUNCTION (this << +mcs.GetMcsValue ());
  for (uint8_t i = 0; i < GetNBasicMcs (); i++)
    {
      if (GetBasicMcs (i) == mcs)
        {
          return;
        }
    }
  m_bssBasicMcsSet.push_back (mcs);
}

uint8_t
WifiRemoteStationManager::GetNBasicMcs (void) const
{
  return static_cast<uint8_t> (m_bssBasicMcsSet.size ());
}

WifiMode
WifiRemoteStationManager::GetBasicMcs (uint8_t i) const
{
  NS_ASSERT (i < GetNBasicMcs ());
  return m_bssBasicMcsSet[i];
}

WifiMode
WifiRemoteStationManager::GetNonUnicastMode (void) const
{
  if (m_nonUnicastMode == WifiMode ())
    {
      if (GetNBasicModes () > 0)
        {
          return GetBasicMode (0);
        }
      else
        {
          return GetDefaultMode ();
        }
    }
  else
    {
      return m_nonUnicastMode;
    }
}

bool
WifiRemoteStationManager::DoNeedRts (WifiRemoteStation *station,
                                     uint32_t size, bool normally)
{
  return normally;
}

bool
WifiRemoteStationManager::DoNeedRetransmission (WifiRemoteStation *station,
                                                Ptr<const Packet> packet, bool normally)
{
  return normally;
}

bool
WifiRemoteStationManager::DoNeedFragmentation (WifiRemoteStation *station,
                                               Ptr<const Packet> packet, bool normally)
{
  return normally;
}

void
WifiRemoteStationManager::DoReportAmpduTxStatus (WifiRemoteStation *station, uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus, double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss)
{
  NS_LOG_DEBUG ("DoReportAmpduTxStatus received but the manager does not handle A-MPDUs!");
}

WifiMode
WifiRemoteStationManager::GetSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNSupported (station));
  return station->m_state->m_operationalRateSet[i];
}

WifiMode
WifiRemoteStationManager::GetMcsSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNMcsSupported (station));
  return station->m_state->m_operationalMcsSet[i];
}

WifiMode
WifiRemoteStationManager::GetNonErpSupported (const WifiRemoteStation *station, uint8_t i) const
{
  NS_ASSERT (i < GetNNonErpSupported (station));
  //IEEE 802.11g standard defines that if the protection mechanism is enabled, RTS, CTS and CTS-To-Self
  //frames should select a rate in the BSSBasicRateSet that corresponds to an 802.11b basic rate.
  //This is a implemented here to avoid changes in every RAA, but should maybe be moved in case it breaks standard rules.
  uint32_t index = 0;
  bool found = false;
  for (WifiModeListIterator j = station->m_state->m_operationalRateSet.begin (); j != station->m_state->m_operationalRateSet.end (); )
    {
      if (i == index)
        {
          found = true;
        }
      if (j->GetModulationClass () != WIFI_MOD_CLASS_ERP_OFDM)
        {
          if (found)
            {
              break;
            }
        }
      index++;
      j++;
    }
  return station->m_state->m_operationalRateSet[index];
}

Mac48Address
WifiRemoteStationManager::GetAddress (const WifiRemoteStation *station) const
{
  return station->m_state->m_address;
}

uint16_t
WifiRemoteStationManager::GetChannelWidth (const WifiRemoteStation *station) const
{
  return station->m_state->m_channelWidth;
}

bool
WifiRemoteStationManager::GetShortGuardIntervalSupported (const WifiRemoteStation *station) const
{
  Ptr<const HtCapabilities> htCapabilities = station->m_state->m_htCapabilities;

  if (!htCapabilities)
    {
      return false;
    }
  return htCapabilities->GetShortGuardInterval20 ();
}

uint16_t
WifiRemoteStationManager::GetGuardInterval (const WifiRemoteStation *station) const
{
  return station->m_state->m_guardInterval;
}

bool
WifiRemoteStationManager::GetGreenfield (const WifiRemoteStation *station) const
{
  Ptr<const HtCapabilities> htCapabilities = station->m_state->m_htCapabilities;

  if (!htCapabilities)
    {
      return false;
    }
  return htCapabilities->GetGreenfield ();
}

bool
WifiRemoteStationManager::GetAggregation (const WifiRemoteStation *station) const
{
  return station->m_state->m_aggregation;
}

uint8_t
WifiRemoteStationManager::GetNumberOfSupportedStreams (const WifiRemoteStation *station) const
{
  Ptr<const HtCapabilities> htCapabilities = station->m_state->m_htCapabilities;

  if (!htCapabilities)
    {
      return 1;
    }
  return htCapabilities->GetRxHighestSupportedAntennas ();
}

uint8_t
WifiRemoteStationManager::GetNess (const WifiRemoteStation *station) const
{
  return station->m_state->m_ness;
}

Ptr<WifiPhy>
WifiRemoteStationManager::GetPhy (void) const
{
  return m_wifiPhy;
}

Ptr<WifiMac>
WifiRemoteStationManager::GetMac (void) const
{
  return m_wifiMac;
}

uint8_t
WifiRemoteStationManager::GetNSupported (const WifiRemoteStation *station) const
{
  return static_cast<uint8_t> (station->m_state->m_operationalRateSet.size ());
}

bool
WifiRemoteStationManager::GetQosSupported (const WifiRemoteStation *station) const
{
  return station->m_state->m_qosSupported;
}

bool
WifiRemoteStationManager::GetHtSupported (const WifiRemoteStation *station) const
{
  return (station->m_state->m_htCapabilities != 0);
}

bool
WifiRemoteStationManager::GetVhtSupported (const WifiRemoteStation *station) const
{
  return (station->m_state->m_vhtCapabilities != 0);
}

bool
WifiRemoteStationManager::GetHeSupported (const WifiRemoteStation *station) const
{
  return (station->m_state->m_heCapabilities != 0);
}

uint8_t
WifiRemoteStationManager::GetNMcsSupported (const WifiRemoteStation *station) const
{
  return static_cast<uint8_t> (station->m_state->m_operationalMcsSet.size ());
}

uint32_t
WifiRemoteStationManager::GetNNonErpSupported (const WifiRemoteStation *station) const
{
  uint32_t size = 0;
  for (WifiModeListIterator i = station->m_state->m_operationalRateSet.begin (); i != station->m_state->m_operationalRateSet.end (); i++)
    {
      if (i->GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
        {
          continue;
        }
      size++;
    }
  return size;
}

uint16_t
WifiRemoteStationManager::GetChannelWidthSupported (Mac48Address address) const
{
  return LookupState (address)->m_channelWidth;
}

bool
WifiRemoteStationManager::GetShortGuardIntervalSupported (Mac48Address address) const
{
  Ptr<const HtCapabilities> htCapabilities = LookupState (address)->m_htCapabilities;

  if (!htCapabilities)
    {
      return false;
    }
  return htCapabilities->GetShortGuardInterval20 ();
}

uint8_t
WifiRemoteStationManager::GetNumberOfSupportedStreams (Mac48Address address) const
{
  Ptr<const HtCapabilities> htCapabilities = LookupState (address)->m_htCapabilities;

  if (!htCapabilities)
    {
      return 1;
    }
  return htCapabilities->GetRxHighestSupportedAntennas ();
}

uint8_t
WifiRemoteStationManager::GetNMcsSupported (Mac48Address address) const
{
  return static_cast<uint8_t> (LookupState (address)->m_operationalMcsSet.size ());
}

bool
WifiRemoteStationManager::GetHtSupported (Mac48Address address) const
{
  return (LookupState (address)->m_htCapabilities != 0);
}

bool
WifiRemoteStationManager::GetVhtSupported (Mac48Address address) const
{
  return (LookupState (address)->m_vhtCapabilities != 0);
}

bool
WifiRemoteStationManager::GetHeSupported (Mac48Address address) const
{
  return (LookupState (address)->m_heCapabilities != 0);
}

void
WifiRemoteStationManager::SetDefaultTxPowerLevel (uint8_t txPower)
{
  m_defaultTxPowerLevel = txPower;
}

uint8_t
WifiRemoteStationManager::GetNumberOfAntennas (void) const
{
  return m_wifiPhy->GetNumberOfAntennas ();
}

uint8_t
WifiRemoteStationManager::GetMaxNumberOfTransmitStreams (void) const
{
  return m_wifiPhy->GetMaxSupportedTxSpatialStreams ();
}

bool
WifiRemoteStationManager::UseGreenfieldForDestination (Mac48Address dest) const
{
  return (GetGreenfieldSupported () && GetGreenfieldSupported (dest) && !GetUseGreenfieldProtection ());
}

} //namespace ns3
