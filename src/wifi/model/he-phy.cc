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

#include "he-phy.h"
#include "he-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-phy.h"
#include "he-configuration.h"
#include "wifi-net-device.h"
#include "sta-wifi-mac.h"
#include "ap-wifi-mac.h"
#include "wifi-utils.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HePhy");

/*******************************************************
 *       HE PHY (P802.11ax/D4.0, clause 27)
 *******************************************************/

/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats HePhy::m_hePpduFormats { //Ignoring PE (Packet Extension)
  { WIFI_PREAMBLE_HE_SU,    { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_SIG_A,         //HE-SIG-A
                              WIFI_PPDU_FIELD_TRAINING,      //HE-STF + HE-LTFs
                              WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_HE_MU,    { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_SIG_A,         //HE-SIG-A
                              WIFI_PPDU_FIELD_SIG_B,         //HE-SIG-B
                              WIFI_PPDU_FIELD_TRAINING,      //HE-STF + HE-LTFs
                              WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_HE_TB,    { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_SIG_A,         //HE-SIG-A
                              WIFI_PPDU_FIELD_TRAINING,      //HE-STF + HE-LTFs
                              WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_HE_ER_SU, { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_SIG_A,         //HE-SIG-A
                              WIFI_PPDU_FIELD_TRAINING,      //HE-STF + HE-LTFs
                              WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_ON* */

HePhy::HePhy (bool buildModeList /* = true */)
  : VhtPhy (false) //don't add VHT modes to list
{
  NS_LOG_FUNCTION (this << buildModeList);
  m_bssMembershipSelector = HE_PHY;
  m_maxMcsIndexPerSs = 11;
  m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
  m_currentHeTbPpduUid = UINT64_MAX;
  if (buildModeList)
    {
      BuildModeList ();
    }
}

HePhy::~HePhy ()
{
  NS_LOG_FUNCTION (this);
}

void
HePhy::BuildModeList (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_modeList.empty ());
  NS_ASSERT (m_bssMembershipSelector == HE_PHY);
  for (uint8_t index = 0; index <= m_maxSupportedMcsIndexPerSs; ++index)
    {
      NS_LOG_LOGIC ("Add HeMcs" << +index << " to list");
      m_modeList.emplace_back (GetHeMcs (index));
    }
}

WifiMode
HePhy::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_TRAINING: //consider SIG-A (SIG-B) mode for training for the time being for SU/ER-SU/TB (MU) (useful for InterferenceHelper)
        if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
          {
            //Training comes after SIG-B
            return GetSigBMode (txVector);
          }
        else
          {
            //Training comes after SIG-A
            return GetSigAMode ();
          }
      default:
        return VhtPhy::GetSigMode (field, txVector);
    }
}

WifiMode
HePhy::GetSigAMode (void) const
{
  return GetVhtMcs0 (); //same number of data tones as VHT for 20 MHz (i.e. 52)
}

WifiMode
HePhy::GetSigBMode (WifiTxVector txVector) const
{
  NS_ABORT_MSG_IF (txVector.GetPreambleType () != WIFI_PREAMBLE_HE_MU, "HE-SIG-B only available for HE MU");
  /**
   * Get smallest HE MCS index among station's allocations and use the
   * VHT version of the index. This enables to have 800 ns GI, 52 data
   * tones, and 312.5 kHz spacing while ensuring that MCS will be decoded
   * by all stations.
   */
  uint8_t smallestMcs = 5; //maximum MCS for HE-SIG-B
  for (auto & info : txVector.GetHeMuUserInfoMap ())
    {
      smallestMcs = std::min (smallestMcs, info.second.mcs.GetMcsValue ());
    }
  switch (smallestMcs) //GetVhtMcs (mcs) is not static
    {
      case 0:
        return GetVhtMcs0 ();
      case 1:
        return GetVhtMcs1 ();
      case 2:
        return GetVhtMcs2 ();
      case 3:
        return GetVhtMcs3 ();
      case 4:
        return GetVhtMcs4 ();
      case 5:
      default:
        return GetVhtMcs5 ();
    }
}

const PhyEntity::PpduFormats &
HePhy::GetPpduFormats (void) const
{
  return m_hePpduFormats;
}

Time
HePhy::GetLSigDuration (WifiPreamble /* preamble */) const
{
  return MicroSeconds (8); //L-SIG + RL-SIG
}

Time
HePhy::GetTrainingDuration (WifiTxVector txVector,
                            uint8_t nDataLtf, uint8_t nExtensionLtf /* = 0 */) const
{
  Time ltfDuration = MicroSeconds (8); //TODO extract from TxVector when available
  Time stfDuration = (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB) ? MicroSeconds (8) : MicroSeconds (4);
  NS_ABORT_MSG_IF (nDataLtf > 8, "Unsupported number of LTFs " << +nDataLtf << " for HE");
  NS_ABORT_MSG_IF (nExtensionLtf > 0, "No extension LTFs expected for HE");
  return stfDuration + ltfDuration * nDataLtf; //HE-STF + HE-LTFs
}

Time
HePhy::GetSigADuration (WifiPreamble preamble) const
{
  return (preamble == WIFI_PREAMBLE_HE_ER_SU) ? MicroSeconds (16) : MicroSeconds (8); //HE-SIG-A (first and second symbol)
}

Time
HePhy::GetSigBDuration (WifiTxVector txVector) const
{
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU) //See section 27.3.10.8 of IEEE 802.11ax draft 4.0.
    {
      /*
       * Compute the number of bits used by common field.
       * Assume that compression bit in HE-SIG-A is not set (i.e. not
       * full band MU-MIMO); the field is present.
       */
      uint16_t bw = txVector.GetChannelWidth ();
      std::size_t commonFieldSize = 4 /* CRC */ + 6 /* tail */;
      if (bw <= 40)
        {
          commonFieldSize += 8; //only one allocation subfield
        }
      else
        {
          commonFieldSize += 8 * (bw / 40) /* one allocation field per 40 MHz */ + 1 /* center RU */;
        }

      /*
       * Compute the number of bits used by user-specific field.
       * MU-MIMO is not supported; only one station per RU.
       * The user-specific field is composed of N user block fields
       * spread over each corresponding HE-SIG-B content channel.
       * Each user block field contains either two or one users' data
       * (the latter being for odd number of stations per content channel).
       * Padding will be handled further down in the code.
       */
      std::pair<std::size_t, std::size_t> numStaPerContentChannel = txVector.GetNumRusPerHeSigBContentChannel ();
      std::size_t maxNumStaPerContentChannel = std::max (numStaPerContentChannel.first, numStaPerContentChannel.second);
      std::size_t maxNumUserBlockFields = maxNumStaPerContentChannel / 2; //handle last user block with single user, if any, further down
      std::size_t userSpecificFieldSize = maxNumUserBlockFields * (2 * 21 /* user fields (2 users) */ + 4 /* tail */ + 6 /* CRC */);
      if (maxNumStaPerContentChannel % 2 != 0)
        {
          userSpecificFieldSize += 21 /* last user field */ + 4 /* CRC */ + 6 /* tail */;
        }

      /*
       * Compute duration of HE-SIG-B considering that padding
       * is added up to the next OFDM symbol.
       * Nss = 1 and GI = 800 ns for HE-SIG-B.
       */
      Time symbolDuration = MicroSeconds (4);
      double numDataBitsPerSymbol = GetSigBMode (txVector).GetDataRate (20, 800, 1) * symbolDuration.GetNanoSeconds () / 1e9;
      double numSymbols = ceil ((commonFieldSize + userSpecificFieldSize) / numDataBitsPerSymbol);

      return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
    }
  else
    {
      // no SIG-B
      return MicroSeconds (0);
    }
}

uint16_t
HePhy::ConvertHeTbPpduDurationToLSigLength (Time ppduDuration, WifiPhyBand band)
{
  uint8_t sigExtension = 0;
  if (band == WIFI_PHY_BAND_2_4GHZ)
    {
      sigExtension = 6;
    }
  uint8_t m = 2; //HE TB PPDU so m is set to 2
  uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3 - m);
  return length;
}

Time
HePhy::ConvertLSigLengthToHeTbPpduDuration (uint16_t length, WifiTxVector txVector, WifiPhyBand band)
{
  NS_ABORT_IF (txVector.GetPreambleType () != WIFI_PREAMBLE_HE_TB);
  Time tSymbol = NanoSeconds (12800 + txVector.GetGuardInterval ());
  Time preambleDuration = WifiPhy::GetStaticPhyEntity (WIFI_MOD_CLASS_HE)->CalculatePhyPreambleAndHeaderDuration (txVector); //this is quite convoluted but only way of keeping the method static
  uint8_t sigExtension = 0;
  if (band == WIFI_PHY_BAND_2_4GHZ)
    {
      sigExtension = 6;
    }
  uint8_t m = 2; //HE TB PPDU so m is set to 2
  //Equation 27-11 of IEEE P802.11ax/D4.0
  Time calculatedDuration = MicroSeconds (((ceil (static_cast<double> (length + 3 + m) / 3)) * 4) + 20 + sigExtension);
  uint32_t nSymbols = floor (static_cast<double> ((calculatedDuration - preambleDuration).GetNanoSeconds () - (sigExtension * 1000)) / tSymbol.GetNanoSeconds ());
  Time ppduDuration = preambleDuration + (nSymbols * tSymbol) + MicroSeconds (sigExtension);
  return ppduDuration;
}

Time
HePhy::CalculateNonOfdmaDurationForHeTb (WifiTxVector txVector) const
{
  NS_ABORT_IF (txVector.GetPreambleType () != WIFI_PREAMBLE_HE_TB);
  Time duration = GetDuration (WIFI_PPDU_FIELD_PREAMBLE, txVector)
    + GetDuration (WIFI_PPDU_FIELD_NON_HT_HEADER, txVector)
    + GetDuration (WIFI_PPDU_FIELD_SIG_A, txVector);
  return duration;
}

uint8_t
HePhy::GetNumberBccEncoders (WifiTxVector /* txVector */) const
{
  return 1; //only 1 BCC encoder for HE since higher rates are obtained using LDPC
}

Time
HePhy::GetSymbolDuration (WifiTxVector txVector) const
{
  uint16_t gi = txVector.GetGuardInterval ();
  NS_ASSERT (gi == 800 || gi == 1600 || gi == 3200);
  return NanoSeconds (12800 + gi);
}

Ptr<WifiPpdu>
HePhy::BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector,
                  Time ppduDuration, WifiPhyBand band, uint64_t uid) const
{
  return Create<HePpdu> (psdus, txVector, ppduDuration, band, uid);
}

void
HePhy::StartReceivePreamble (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW,
                             Time rxDuration, TxPsdFlag psdFlag)
{
  NS_LOG_FUNCTION (this << ppdu << rxDuration << psdFlag);
  WifiTxVector txVector = ppdu->GetTxVector ();
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB
      && psdFlag == PSD_HE_TB_OFDMA_PORTION)
    {
      if (m_currentHeTbPpduUid == ppdu->GetUid ()
          && GetCurrentEvent () != 0)
        {
          //AP or STA has already received non-OFDMA part, switch to OFDMA part, and schedule reception of payload (will be canceled for STAs by StartPayload)
          bool ofdmaStarted = !m_beginOfdmaPayloadRxEvents.empty ();
          NS_LOG_INFO ("Switch to OFDMA part (already started? " << (ofdmaStarted ? "Y" : "N") << ") " <<
                       "and schedule OFDMA payload reception in " << GetDuration (WIFI_PPDU_FIELD_TRAINING, txVector).As (Time::NS));
          Ptr<Event> event = CreateInterferenceEvent (ppdu, txVector, rxDuration, rxPowersW, !ofdmaStarted);
          uint16_t staId = GetStaId (ppdu);
          NS_ASSERT (m_beginOfdmaPayloadRxEvents.find (staId) == m_beginOfdmaPayloadRxEvents.end ());
          m_beginOfdmaPayloadRxEvents[staId] = Simulator::Schedule (GetDuration (WIFI_PPDU_FIELD_TRAINING, txVector),
                                                                    &HePhy::StartReceiveOfdmaPayload, this, event);
        }
      else
        {
          //PHY receives the OFDMA payload while having dropped the preamble
          NS_LOG_INFO ("Consider OFDMA part of the HE TB PPDU as interference since device dropped the preamble");
          CreateInterferenceEvent (ppdu, txVector, rxDuration, rxPowersW);
          //the OFDMA part of the HE TB PPDUs will be noise _after_ the completion of the current event
          ErasePreambleEvent (ppdu, rxDuration);
        }
    }
  else
    {
      PhyEntity::StartReceivePreamble (ppdu, rxPowersW, rxDuration, psdFlag);
    }
}

void
HePhy::CancelAllEvents (void)
{
  NS_LOG_FUNCTION (this);
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
  PhyEntity::CancelAllEvents ();
}

void
HePhy::DoAbortCurrentReception (WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << reason);
  if (reason != OBSS_PD_CCA_RESET)
    {
      for (auto & endMpduEvent : m_endOfMpduEvents)
        {
          endMpduEvent.Cancel ();
        }
      m_endOfMpduEvents.clear ();
    }
  else
    {
      PhyEntity::DoAbortCurrentReception (reason);
    }
}

void
HePhy::DoResetReceive (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  if (event->GetPpdu ()->GetType () != WIFI_PPDU_TYPE_UL_MU)
    {
      NS_ASSERT (event->GetEndTime () == Simulator::Now ());
    }
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
}

Ptr<Event>
HePhy::DoGetEvent (Ptr<const WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW)
{
  Ptr<Event> event;
  //We store all incoming preamble events, and a decision is made at the end of the preamble detection window.
  //If a preamble is received after the preamble detection window, it is stored anyway because this is needed for HE TB PPDUs in
  //order to properly update the received power in InterferenceHelper. The map is cleaned anyway at the end of the current reception.
  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      auto uidPreamblePair = std::make_pair (ppdu->GetUid (), ppdu->GetPreamble ());
      WifiTxVector txVector = ppdu->GetTxVector ();
      Time rxDuration = CalculateNonOfdmaDurationForHeTb (txVector); //the OFDMA part of the transmission will be added later on
      const auto & currentPreambleEvents = GetCurrentPreambleEvents ();
      auto it = currentPreambleEvents.find (uidPreamblePair);
      if (it != currentPreambleEvents.end ())
        {
          NS_LOG_DEBUG ("Received another HE TB PPDU for UID " << ppdu->GetUid () << " from STA-ID " << ppdu->GetStaId () << " and BSS color " << +txVector.GetBssColor ());
          event = it->second;
          if (Simulator::Now () - event->GetStartTime () > NanoSeconds (400))
            {
              //Section 27.3.14.3 from 802.11ax Draft 4.0: Pre-correction accuracy requirements.
              //A STA that transmits an HE TB PPDU, non-HT PPDU, or non-HT duplicate PPDU in response to a triggering PPDU
              //shall ensure that the transmission start time of the HE TB PPDU, non-HT PPDU, or non-HT duplicate PPDU is
              //within ±0.4 µs + 16 µs from the end, at the STA’s antenna connector, of the last OFDM symbol of the triggering
              //PPDU (if it contains no PE field) or of the PE field of the triggering PPDU (if the PE field is present).
              //As a result, if an HE TB PPDU arrives later than 0.4 µs, it is added as an interference but PPDU is dropped.
              event = CreateInterferenceEvent (ppdu, txVector, rxDuration, rxPowersW);
              NS_LOG_DEBUG ("Drop packet because not received within the 400ns window");
              m_wifiPhy->NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), HE_TB_PPDU_TOO_LATE);
            }
          else
            {
              //Update received power of the event associated to that UL MU transmission
              UpdateInterferenceEvent (event, rxPowersW);
            }
          if ((GetCurrentEvent () != 0) && (GetCurrentEvent ()->GetPpdu ()->GetUid () != ppdu->GetUid ()))
            {
              NS_LOG_DEBUG ("Drop packet because already receiving another HE TB PPDU");
              m_wifiPhy->NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), RXING);
            }
          return nullptr;
        }
      else
        {
          NS_LOG_DEBUG ("Received a new HE TB PPDU for UID " << ppdu->GetUid () << " from STA-ID " << ppdu->GetStaId () << " and BSS color " << +txVector.GetBssColor ());
          event = CreateInterferenceEvent (ppdu, txVector, rxDuration, rxPowersW);
          AddPreambleEvent (event);
        }
    }
  else
    {
      event = PhyEntity::DoGetEvent (ppdu, rxPowersW);
    }
  return event;
}

Ptr<const WifiPsdu>
HePhy::GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const
{
  if (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU || ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      auto hePpdu = DynamicCast<const HePpdu> (ppdu);
      NS_ASSERT (hePpdu);
      return hePpdu->GetPsdu (GetBssColor (), GetStaId (ppdu));
    }
  return PhyEntity::GetAddressedPsduInPpdu (ppdu);
}

uint8_t
HePhy::GetBssColor (void) const
{
  uint8_t bssColor = 0;
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
  if (device)
    {
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      if (heConfiguration)
        {
          UintegerValue bssColorAttribute;
          heConfiguration->GetAttribute ("BssColor", bssColorAttribute);
          bssColor = bssColorAttribute.Get ();
        }
    }
  return bssColor;
}

uint16_t
HePhy::GetStaId (const Ptr<const WifiPpdu> ppdu) const
{
  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      return ppdu->GetStaId ();
    }
  else if (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU)
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
      if (device)
        {
          Ptr<StaWifiMac> mac = DynamicCast<StaWifiMac> (device->GetMac ());
          if (mac && mac->IsAssociated ())
            {
              return mac->GetAssociationId ();
            }
        }
    }
  return PhyEntity::GetStaId (ppdu);
}

PhyEntity::PhyFieldRxStatus
HePhy::ProcessSigA (Ptr<Event> event, PhyFieldRxStatus status)
{
  NS_LOG_FUNCTION (this << *event << status);
  //Notify end of SIG-A (in all cases)
  HeSigAParameters params;
  params.rssiW = GetRxPowerWForPpdu (event);
  params.bssColor = event->GetTxVector ().GetBssColor ();
  Simulator::ScheduleNow (&WifiPhy::NotifyEndOfHeSigA, GetPointer (m_wifiPhy), params);

  if (status.isSuccess)
    {
      Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
      if (event->GetTxVector ().GetPreambleType () == WIFI_PREAMBLE_HE_TB)
        {
          m_currentHeTbPpduUid = ppdu->GetUid (); //to be able to correctly schedule start of OFDMA payload
        }

      //Check if PPDU is filtered only if the SIG-A content is supported
      if (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU) //Final decision on content of DL MU is reported to end of SIG-B (unless the PPDU is filtered)
        {
          uint8_t bssColor = GetBssColor ();
          if (bssColor != 0 && bssColor != event->GetTxVector ().GetBssColor ())
            {
              NS_LOG_DEBUG ("The BSS color of this DL MU PPDU does not match the device's. The PPDU is filtered.");
              return PhyFieldRxStatus (false, FILTERED, ABORT);
            }
        }
      else if (GetAddressedPsduInPpdu (ppdu))
        {
          //We are here because the SU or UL MU is addressed to the PPDU, so keep status to success
        }
      else
        {
          NS_ASSERT (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU);
          NS_LOG_DEBUG ("No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
          return PhyFieldRxStatus (false, FILTERED, ABORT);
        }
    }
  return status;
}

PhyEntity::PhyFieldRxStatus
HePhy::ProcessSigB (Ptr<Event> event, PhyFieldRxStatus status)
{
  NS_LOG_FUNCTION (this << *event << status);
  if (status.isSuccess)
    {
      //Check if PPDU is filtered only if the SIG-B content is supported (not explicitly stated but assumed based on behavior for SIG-A)
      if (!GetAddressedPsduInPpdu (event->GetPpdu ()))
        {
          NS_LOG_DEBUG ("No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
          return PhyFieldRxStatus (false, FILTERED, ABORT);
        }
    }
  return status;
}

bool
HePhy::IsConfigSupported (Ptr<const WifiPpdu> ppdu) const
{
  WifiTxVector txVector = ppdu->GetTxVector ();
  uint16_t staId = GetStaId (ppdu);
  WifiMode txMode = txVector.GetMode (staId);
  uint8_t nss = txVector.GetNssMax ();
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
    {
      for (auto info : txVector.GetHeMuUserInfoMap ())
        {
          if (info.first == staId)
            {
              nss = info.second.nss; //no need to look at other PSDUs
              break;
            }
        }
    }

  if (nss > m_wifiPhy->GetMaxSupportedRxSpatialStreams ())
    {
      NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
      return false;
    }
  if (!IsModeSupported (txMode))
    {
      NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txVector.GetMode () << ")");
      return false;
    }
  return true;
}

void
HePhy::DoStartReceivePayload (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  WifiTxVector txVector = event->GetTxVector ();
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_wifiPhy->GetDevice ());
      bool isAp = device != 0 && (DynamicCast<ApWifiMac> (device->GetMac ()) != 0);
      if (!isAp)
        {
          NS_LOG_DEBUG ("Ignore HE TB PPDU payload received by STA but keep state in Rx");
          m_endRxPayloadEvents.push_back (Simulator::Schedule (ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector),
                                                               &PhyEntity::ResetReceive, this, event));
          //Cancel all scheduled events for OFDMA payload reception
          NS_ASSERT (!m_beginOfdmaPayloadRxEvents.empty () && m_beginOfdmaPayloadRxEvents.begin ()->second.IsRunning ());
          for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
            {
              beginOfdmaPayloadRxEvent.second.Cancel ();
            }
          m_beginOfdmaPayloadRxEvents.clear ();
        }
      else
        {
          NS_LOG_DEBUG ("Receiving PSDU in HE TB PPDU");
          uint16_t staId = GetStaId (ppdu);
          m_signalNoiseMap.insert ({std::make_pair (ppdu->GetUid (), staId), SignalNoiseDbm ()});
          m_statusPerMpduMap.insert ({std::make_pair (ppdu->GetUid (), staId), std::vector<bool> ()});
          if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
            {
              //for HE TB PPDUs, ScheduleEndOfMpdus and EndReceive are scheduled by StartReceiveOfdmaPayload
              NS_ASSERT (isAp);
              NS_ASSERT (!m_beginOfdmaPayloadRxEvents.empty ());
              for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
                {
                  NS_ASSERT (beginOfdmaPayloadRxEvent.second.IsRunning ());
                }
            }
        }
    }
  else
    {
      PhyEntity::DoStartReceivePayload (event);
    }
}

void
HePhy::DoEndReceivePayload (Ptr<const WifiPpdu> ppdu)
{
  NS_LOG_FUNCTION (this << ppdu);
  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      for (auto it = m_endRxPayloadEvents.begin (); it != m_endRxPayloadEvents.end (); )
        {
          if (it->IsExpired ())
            {
              it = m_endRxPayloadEvents.erase (it);
            }
          else
            {
              it++;
            }
        }
      if (m_endRxPayloadEvents.empty ())
        {
          //We've got the last PPDU of the UL-OFDMA transmission
          NotifyInterferenceRxEndAndClear (true); //reset WifiPhy
        }
    }
  else
    {
      NS_ASSERT (m_wifiPhy->GetLastRxEndTime () == Simulator::Now ());
      PhyEntity::DoEndReceivePayload (ppdu);
    }
}

void
HePhy::StartReceiveOfdmaPayload (Ptr<Event> event)
{
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  RxPowerWattPerChannelBand rxPowersW = event->GetRxPowerWPerBand ();
  //The total RX power corresponds to the maximum over all the bands
  auto it = std::max_element (rxPowersW.begin (), rxPowersW.end (),
                              [] (const std::pair<WifiSpectrumBand, double> &p1, const std::pair<WifiSpectrumBand, double> &p2) {
                                return p1.second < p2.second;
                              });
  NS_LOG_FUNCTION (this << *event << it->second);
  NS_ASSERT (GetCurrentEvent () != 0);
  auto itEvent = m_beginOfdmaPayloadRxEvents.find (GetStaId (ppdu));
  /**
   * m_beginOfdmaPayloadRxEvents should still be running only for APs, since canceled in StartReceivePayload for STAs.
   * This is because SpectrumWifiPhy does not have access to the device type and thus blindly schedules things, letting
   * the parent WifiPhy class take into account device type.
   */
  NS_ASSERT (itEvent != m_beginOfdmaPayloadRxEvents.end () && itEvent->second.IsExpired ());
  m_beginOfdmaPayloadRxEvents.erase (itEvent);

  Time payloadDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (ppdu->GetTxVector ());
  Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
  ScheduleEndOfMpdus (event);
  m_endRxPayloadEvents.push_back (Simulator::Schedule (payloadDuration, &PhyEntity::EndReceivePayload, this, event));
  m_signalNoiseMap.insert ({std::make_pair (ppdu->GetUid (), ppdu->GetStaId ()), SignalNoiseDbm ()});
  m_statusPerMpduMap.insert ({std::make_pair (ppdu->GetUid (), ppdu->GetStaId ()), std::vector<bool> ()});
}

std::pair<uint16_t, WifiSpectrumBand>
HePhy::GetChannelWidthAndBand (WifiTxVector txVector, uint16_t staId) const
{
  if (txVector.IsMu ())
    {
      return std::make_pair (HeRu::GetBandwidth (txVector.GetRu (staId).ruType),
                             GetRuBand (txVector, staId));
    }
  else
    {
      return PhyEntity::GetChannelWidthAndBand (txVector, staId);
    }
}

WifiSpectrumBand
HePhy::GetRuBand (WifiTxVector txVector, uint16_t staId) const
{
  NS_ASSERT (txVector.IsMu ());
  WifiSpectrumBand band;
  HeRu::RuSpec ru = txVector.GetRu (staId);
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_ASSERT (channelWidth <= m_wifiPhy->GetChannelWidth ());
  HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup (channelWidth, ru.ruType, ru.index);
  HeRu::SubcarrierRange range = std::make_pair (group.front ().first, group.back ().second);
  band = m_wifiPhy->ConvertHeRuSubcarriers (channelWidth, range);
  return band;
}

WifiSpectrumBand
HePhy::GetNonOfdmaBand (WifiTxVector txVector, uint16_t staId) const
{
  NS_ASSERT (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB);
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_ASSERT (channelWidth <= m_wifiPhy->GetChannelWidth ());

  HeRu::RuSpec ru = txVector.GetRu (staId);
  uint16_t ruWidth = HeRu::GetBandwidth (ru.ruType);
  uint16_t nonOfdmaWidth = ruWidth < 20 ? 20 : ruWidth;

  // Find the RU that encompasses the non-OFDMA part of the HE TB PPDU for the STA-ID
  HeRu::RuSpec nonOfdmaRu = HeRu::FindOverlappingRu (channelWidth, ru, HeRu::GetRuType (nonOfdmaWidth));

  HeRu::SubcarrierGroup groupPreamble = HeRu::GetSubcarrierGroup (channelWidth, nonOfdmaRu.ruType, nonOfdmaRu.index);
  HeRu::SubcarrierRange range = std::make_pair (groupPreamble.front ().first, groupPreamble.back ().second);
  return m_wifiPhy->ConvertHeRuSubcarriers (channelWidth, range);
}

uint64_t
HePhy::GetCurrentHeTbPpduUid (void) const
{
  return m_currentHeTbPpduUid;
}

void
HePhy::InitializeModes (void)
{
  for (uint8_t i = 0; i < 12; ++i)
    {
      GetHeMcs (i);
    }
}

WifiMode
HePhy::GetHeMcs (uint8_t index)
{
  switch (index)
    {
      case 0:
        return GetHeMcs0 ();
      case 1:
        return GetHeMcs1 ();
      case 2:
        return GetHeMcs2 ();
      case 3:
        return GetHeMcs3 ();
      case 4:
        return GetHeMcs4 ();
      case 5:
        return GetHeMcs5 ();
      case 6:
        return GetHeMcs6 ();
      case 7:
        return GetHeMcs7 ();
      case 8:
        return GetHeMcs8 ();
      case 9:
        return GetHeMcs9 ();
      case 10:
        return GetHeMcs10 ();
      case 11:
        return GetHeMcs11 ();
      default:
        NS_ABORT_MSG ("Inexistent index (" << +index << ") requested for HE");
        return WifiMode ();
    }
}

WifiMode
HePhy::GetHeMcs0 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs0", 0, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs1 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs1", 1, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs2 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs2", 2, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs3 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs3", 3, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs4 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs4", 4, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs5 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs5", 5, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs6 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs6", 6, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs7 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs7", 7, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs8 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs8", 8, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs9 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs9", 9, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs10 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs10", 10, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
HePhy::GetHeMcs11 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs11", 11, WIFI_MOD_CLASS_HE);
  return mcs;
}

} //namespace ns3

namespace {

/**
 * Constructor class for HE modes
 */
static class ConstructorHe
{
public:
  ConstructorHe ()
  {
    ns3::HePhy::InitializeModes ();
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_HE, ns3::Create<ns3::HePhy> ());
  }
} g_constructor_he; ///< the constructor for HE modes

}
