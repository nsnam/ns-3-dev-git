/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 */

#include "ht-phy.h"
#include "ht-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-phy.h"
#include "wifi-utils.h"
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HtPhy");

/*******************************************************
 *       HT PHY (IEEE 802.11-2016, clause 19)
 *******************************************************/

/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats HtPhy::m_htPpduFormats {
  { WIFI_PREAMBLE_HT_MF, { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                           WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG
                           WIFI_PPDU_FIELD_HT_SIG,        //HT-SIG
                           WIFI_PPDU_FIELD_TRAINING,      //HT-STF + HT-LTFs
                           WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_HT_GF, { WIFI_PPDU_FIELD_PREAMBLE, //HT-GF-STF + HT-LTF1
                           WIFI_PPDU_FIELD_HT_SIG,   //HT-SIG
                           WIFI_PPDU_FIELD_TRAINING, //Additional HT-LTFs
                           WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_ON* */

HtPhy::HtPhy (uint8_t maxNss /* = 1 */, bool buildModeList /* = true */)
  : OfdmPhy (OFDM_PHY_DEFAULT, false) //don't add OFDM modes to list
{
  NS_LOG_FUNCTION (this << +maxNss << buildModeList);
  m_maxSupportedNss = maxNss;
  m_bssMembershipSelector = HT_PHY;
  m_maxMcsIndexPerSs = 7;
  m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
  if (buildModeList)
    {
      NS_ABORT_MSG_IF (maxNss == 0 || maxNss > 4, "Unsupported max Nss " << +maxNss << " for HT PHY");
      BuildModeList ();
    }
}

HtPhy::~HtPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
HtPhy::BuildModeList (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_modeList.empty ());
  NS_ASSERT (m_bssMembershipSelector == HT_PHY);

  uint8_t index = 0;
  for (uint8_t nss = 1; nss <= m_maxSupportedNss; ++nss)
    {
      for (uint8_t i = 0; i <= m_maxSupportedMcsIndexPerSs; ++i)
        {
          NS_LOG_LOGIC ("Add HtMcs" << +index << " to list");
          m_modeList.emplace_back (GetHtMcs (index));
          ++index;
        }
      index = 8 * nss;
    }
}

WifiMode
HtPhy::GetMcs (uint8_t index) const
{
  for (const auto & mcs : m_modeList)
    {
      if (mcs.GetMcsValue () == index)
        {
          return mcs;
        }
    }

  // Should have returned if MCS found
  NS_ABORT_MSG ("Unsupported MCS index " << +index << " for this PHY entity");
  return WifiMode ();
}

bool
HtPhy::IsMcsSupported (uint8_t index) const
{
  for (const auto & mcs : m_modeList)
    {
      if (mcs.GetMcsValue () == index)
        {
          return true;
        }
    }
  return false;
}

bool
HtPhy::HandlesMcsModes (void) const
{
  return true;
}

const PhyEntity::PpduFormats &
HtPhy::GetPpduFormats (void) const
{
  return m_htPpduFormats;
}

WifiMode
HtPhy::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_PREAMBLE: //consider non-HT header mode for preamble (useful for InterferenceHelper)
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetLSigMode ();
      case WIFI_PPDU_FIELD_TRAINING: //consider HT-SIG mode for training (useful for InterferenceHelper)
      case WIFI_PPDU_FIELD_HT_SIG:
        return GetHtSigMode ();
      default:
        return PhyEntity::GetSigMode (field, txVector);
    }
}

WifiMode
HtPhy::GetLSigMode (void)
{
  return GetOfdmRate6Mbps ();
}

WifiMode
HtPhy::GetHtSigMode (void) const
{
  return GetLSigMode (); //same number of data tones as OFDM (i.e. 48)
}

uint8_t
HtPhy::GetBssMembershipSelector (void) const
{
  return m_bssMembershipSelector;
}

void
HtPhy::SetMaxSupportedMcsIndexPerSs (uint8_t maxIndex)
{
  NS_LOG_FUNCTION (this << +maxIndex);
  NS_ABORT_MSG_IF (maxIndex > m_maxMcsIndexPerSs, "Provided max MCS index " << +maxIndex << " per SS greater than max standard-defined value " << +m_maxMcsIndexPerSs);
  if (maxIndex != m_maxSupportedMcsIndexPerSs)
    {
      NS_LOG_LOGIC ("Rebuild mode list since max MCS index per spatial stream has changed");
      m_maxSupportedMcsIndexPerSs = maxIndex;
      m_modeList.clear ();
      BuildModeList ();
    }
}

uint8_t
HtPhy::GetMaxSupportedMcsIndexPerSs (void) const
{
  return m_maxSupportedMcsIndexPerSs;
}

void
HtPhy::SetMaxSupportedNss (uint8_t maxNss)
{
  NS_LOG_FUNCTION (this << +maxNss);
  NS_ASSERT (m_bssMembershipSelector == HT_PHY);
  NS_ABORT_MSG_IF (maxNss == 0 || maxNss > 4, "Unsupported max Nss " << +maxNss << " for HT PHY");
  if (maxNss != m_maxSupportedNss)
    {
      NS_LOG_LOGIC ("Rebuild mode list since max number of spatial streams has changed");
      m_maxSupportedNss = maxNss;
      m_modeList.clear ();
      BuildModeList ();
    }
}

Time
HtPhy::GetDuration (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_PREAMBLE:
        return MicroSeconds (16); //L-STF + L-LTF or HT-GF-STF + HT-LTF1
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetLSigDuration (txVector.GetPreambleType ());
      case WIFI_PPDU_FIELD_TRAINING:
        {
          //We suppose here that STBC = 0.
          //If STBC > 0, we need a different mapping between Nss and Nltf
          // (see IEEE 802.11-2016 , section 19.3.9.4.6 "HT-LTF definition").
          uint8_t nDataLtf = 8;
          uint8_t nss = txVector.GetNssMax (); //so as to cover also HE MU case (see section 27.3.10.10 of IEEE P802.11ax/D4.0)
          if (nss < 3)
            {
              nDataLtf = nss;
            }
          else if (nss < 5)
            {
              nDataLtf = 4;
            }
          else if (nss < 7)
            {
              nDataLtf = 6;
            }

          uint8_t nExtensionLtf = (txVector.GetNess () < 3) ? txVector.GetNess () : 4;

          return GetTrainingDuration (txVector, nDataLtf, nExtensionLtf);
        }
      case WIFI_PPDU_FIELD_HT_SIG:
        return GetHtSigDuration ();
      default:
        return PhyEntity::GetDuration (field, txVector);
    }
}

Time
HtPhy::GetLSigDuration (WifiPreamble preamble) const
{
  return (preamble == WIFI_PREAMBLE_HT_GF) ? MicroSeconds (0) : MicroSeconds (4); //no L-SIG for HT-GF
}

Time
HtPhy::GetTrainingDuration (WifiTxVector txVector,
                            uint8_t nDataLtf, uint8_t nExtensionLtf /* = 0 */) const
{
  NS_ABORT_MSG_IF (nDataLtf == 0 || nDataLtf > 4 || nExtensionLtf > 4 || (nDataLtf + nExtensionLtf) > 5,
                   "Unsupported combination of data (" << +nDataLtf << ")  and extension (" << +nExtensionLtf << ")  LTFs numbers for HT"); //see IEEE 802.11-2016, section 19.3.9.4.6 "HT-LTF definition"
  Time duration = MicroSeconds (4) * (nDataLtf + nExtensionLtf);
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HT_GF)
    {
      return MicroSeconds (4) * (nDataLtf - 1 + nExtensionLtf); //no HT-STF and first HT-LTF is already in preamble, see IEEE 802.11-2016, section 19.3.5.5 "HT-greenfield format LTF"
    }
  else //HT-MF
    {
      return MicroSeconds (4) * (1 /* HT-STF */ + nDataLtf + nExtensionLtf);
    }
}

Time
HtPhy::GetHtSigDuration (void) const
{
  return MicroSeconds (8); //HT-SIG
}

Time
HtPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                           bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                           uint16_t staId) const
{
  WifiMode payloadMode = txVector.GetMode (staId);
  uint8_t stbc = txVector.IsStbc () ? 2 : 1; //corresponding to m_STBC in Nsym computation (see IEEE 802.11-2016, equations (19-32) and (21-62))
  uint8_t nes = GetNumberBccEncoders (txVector);
  //TODO: Update station managers to consider GI capabilities
  Time symbolDuration = GetSymbolDuration (txVector);

  double numDataBitsPerSymbol = payloadMode.GetDataRate (txVector, staId) * symbolDuration.GetNanoSeconds () / 1e9;
  uint8_t service = GetNumberServiceBits ();

  double numSymbols = 0;
  switch (mpdutype)
    {
      case FIRST_MPDU_IN_AGGREGATE:
        {
          //First packet in an A-MPDU
          numSymbols = (stbc * (service + size * 8.0 + 6 * nes) / (stbc * numDataBitsPerSymbol));
          if (incFlag == 1)
            {
              totalAmpduSize += size;
              totalAmpduNumSymbols += numSymbols;
            }
          break;
        }
      case MIDDLE_MPDU_IN_AGGREGATE:
        {
          //consecutive packets in an A-MPDU
          numSymbols = (stbc * size * 8.0) / (stbc * numDataBitsPerSymbol);
          if (incFlag == 1)
            {
              totalAmpduSize += size;
              totalAmpduNumSymbols += numSymbols;
            }
          break;
        }
      case LAST_MPDU_IN_AGGREGATE:
        {
          //last packet in an A-MPDU
          uint32_t totalSize = totalAmpduSize + size;
          numSymbols = lrint (stbc * ceil ((service + totalSize * 8.0 + 6 * nes) / (stbc * numDataBitsPerSymbol)));
          NS_ASSERT (totalAmpduNumSymbols <= numSymbols);
          numSymbols -= totalAmpduNumSymbols;
          if (incFlag == 1)
            {
              totalAmpduSize = 0;
              totalAmpduNumSymbols = 0;
            }
          break;
        }
      case NORMAL_MPDU:
      case SINGLE_MPDU:
        {
          //Not an A-MPDU or single MPDU (i.e. the current payload contains both service and padding)
          //The number of OFDM symbols in the data field when BCC encoding
          //is used is given in equation 19-32 of the IEEE 802.11-2016 standard.
          numSymbols = lrint (stbc * ceil ((service + size * 8.0 + 6.0 * nes) / (stbc * numDataBitsPerSymbol)));
          break;
        }
      default:
        NS_FATAL_ERROR ("Unknown MPDU type");
    }

  Time payloadDuration = FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
  if (mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU || mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      payloadDuration += GetSignalExtension (band);
    }
  return payloadDuration;
}

uint8_t
HtPhy::GetNumberBccEncoders (WifiTxVector txVector) const
{
  /**
   * Add an encoder when crossing maxRatePerCoder frontier.
   *
   * The value of 320 Mbps and 350 Mbps for normal GI and short GI (resp.)
   * were obtained by observing the rates for which Nes was incremented in tables
   * 19-27 to 19-41 of IEEE 802.11-2016.
   */
  double maxRatePerCoder = (txVector.GetGuardInterval () == 800) ? 320e6 : 350e6;
  return ceil (txVector.GetMode ().GetDataRate (txVector) / maxRatePerCoder);
}

Time
HtPhy::GetSymbolDuration (WifiTxVector txVector) const
{
  uint16_t gi = txVector.GetGuardInterval ();
  NS_ASSERT (gi == 400 || gi == 800);
  return NanoSeconds (3200 + gi);
}

Ptr<WifiPpdu>
HtPhy::BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration)
{
  NS_LOG_FUNCTION (this << psdus << txVector << ppduDuration);
  return Create<HtPpdu> (psdus.begin ()->second, txVector, ppduDuration, m_wifiPhy->GetPhyBand (),
                         ObtainNextUid (txVector));
}

PhyEntity::PhyFieldRxStatus
HtPhy::DoEndReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  switch (field)
    {
      case WIFI_PPDU_FIELD_HT_SIG:
        return EndReceiveHtSig (event);
      case WIFI_PPDU_FIELD_TRAINING:
        return PhyFieldRxStatus (true); //always consider that training has been correctly received
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        NS_ASSERT (event->GetTxVector ().GetPreambleType () != WIFI_PREAMBLE_HT_GF);
      //no break so as to go to OfdmPhy for processing
      default:
        return OfdmPhy::DoEndReceiveField (field, event);
    }
}

PhyEntity::PhyFieldRxStatus
HtPhy::EndReceiveHtSig (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (IsHt (event->GetTxVector ().GetPreambleType ()));
  SnrPer snrPer = GetPhyHeaderSnrPer (WIFI_PPDU_FIELD_HT_SIG, event);
  NS_LOG_DEBUG ("HT-SIG: SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per);
  PhyFieldRxStatus status (GetRandomValue () > snrPer.per);
  if (status.isSuccess)
    {
      NS_LOG_DEBUG ("Received HT-SIG");
      if (!IsAllConfigSupported (WIFI_PPDU_FIELD_HT_SIG, event->GetPpdu ()))
        {
          status = PhyFieldRxStatus (false, UNSUPPORTED_SETTINGS, DROP);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because HT-SIG reception failed");
      status.reason = HT_SIG_FAILURE;
      status.actionIfFailure = DROP;
    }
  return status;
}

bool
HtPhy::IsAllConfigSupported (WifiPpduField field, Ptr<const WifiPpdu> ppdu) const
{
  if (field == WIFI_PPDU_FIELD_NON_HT_HEADER)
    {
      return true; //wait till reception of HT-SIG (or SIG-A) to make decision
    }
  return OfdmPhy::IsAllConfigSupported (field, ppdu);
}

bool
HtPhy::IsConfigSupported (Ptr<const WifiPpdu> ppdu) const
{
  WifiTxVector txVector = ppdu->GetTxVector ();
  if (txVector.GetNss () > m_wifiPhy->GetMaxSupportedRxSpatialStreams ())
    {
      NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
      return false;
    }
  if (!IsModeSupported (txVector.GetMode ()))
    {
      NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txVector.GetMode () << ")");
      return false;
    }
  return true;
}

Ptr<SpectrumValue>
HtPhy::GetTxPowerSpectralDensity (double txPowerW, Ptr<const WifiPpdu> ppdu) const
{
  WifiTxVector txVector = ppdu->GetTxVector ();
  uint16_t centerFrequency = GetCenterFrequencyForChannelWidth (txVector);
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_LOG_FUNCTION (this << centerFrequency << channelWidth << txPowerW);
  const auto & txMaskRejectionParams = GetTxMaskRejectionParams ();
  Ptr<SpectrumValue> v = WifiSpectrumValueHelper::CreateHtOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW, GetGuardBandwidth (channelWidth),
                                                                                      std::get<0> (txMaskRejectionParams), std::get<1> (txMaskRejectionParams), std::get<2> (txMaskRejectionParams));
  return v;
}

void
HtPhy::InitializeModes (void)
{
  for (uint8_t i = 0; i < 32; ++i)
    {
      GetHtMcs (i);
    }
}

WifiMode
HtPhy::GetHtMcs (uint8_t index)
{
  switch (index)
    {
      case 0:
        return GetHtMcs0 ();
      case 1:
        return GetHtMcs1 ();
      case 2:
        return GetHtMcs2 ();
      case 3:
        return GetHtMcs3 ();
      case 4:
        return GetHtMcs4 ();
      case 5:
        return GetHtMcs5 ();
      case 6:
        return GetHtMcs6 ();
      case 7:
        return GetHtMcs7 ();
      case 8:
        return GetHtMcs8 ();
      case 9:
        return GetHtMcs9 ();
      case 10:
        return GetHtMcs10 ();
      case 11:
        return GetHtMcs11 ();
      case 12:
        return GetHtMcs12 ();
      case 13:
        return GetHtMcs13 ();
      case 14:
        return GetHtMcs14 ();
      case 15:
        return GetHtMcs15 ();
      case 16:
        return GetHtMcs16 ();
      case 17:
        return GetHtMcs17 ();
      case 18:
        return GetHtMcs18 ();
      case 19:
        return GetHtMcs19 ();
      case 20:
        return GetHtMcs20 ();
      case 21:
        return GetHtMcs21 ();
      case 22:
        return GetHtMcs22 ();
      case 23:
        return GetHtMcs23 ();
      case 24:
        return GetHtMcs24 ();
      case 25:
        return GetHtMcs25 ();
      case 26:
        return GetHtMcs26 ();
      case 27:
        return GetHtMcs27 ();
      case 28:
        return GetHtMcs28 ();
      case 29:
        return GetHtMcs29 ();
      case 30:
        return GetHtMcs30 ();
      case 31:
        return GetHtMcs31 ();
      default:
        NS_ABORT_MSG ("Inexistent (or not supported) index (" << +index << ") requested for HT");
        return WifiMode ();
    }
}

WifiMode
HtPhy::GetHtMcs0 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs0", 0, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs1 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs1", 1, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs2 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs2", 2, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs3 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs3", 3, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs4 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs4", 4, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs5 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs5", 5, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs6 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs6", 6, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs7 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs7", 7, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs8 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs8", 8, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs9 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs9", 9, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs10 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs10", 10, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs11 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs11", 11, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs12 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs12", 12, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs13 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs13", 13, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs14 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs14", 14, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs15 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs15", 15, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs16 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs16", 16, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs17 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs17", 17, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs18 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs18", 18, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs19 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs19", 19, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs20 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs20", 20, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs21 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs21", 21, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs22 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs22", 22, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs23 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs23", 23, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs24 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs24", 24, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs25 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs25", 25, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs26 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs26", 26, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs27 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs27", 27, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs28 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs28", 28, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs29 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs29", 29, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs30 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs30", 30, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
HtPhy::GetHtMcs31 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs31", 31, WIFI_MOD_CLASS_HT);
  return mcs;
}

} //namespace ns3

namespace {

/**
 * Constructor class for HT modes
 */
static class ConstructorHt
{
public:
  ConstructorHt ()
  {
    ns3::HtPhy::InitializeModes ();
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_HT, ns3::Create<ns3::HtPhy> ()); //dummy Nss
  }
} g_constructor_ht; ///< the constructor for HT modes

}
