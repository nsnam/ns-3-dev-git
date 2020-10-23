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
#include "wifi-phy.h" //only used for static mode constructor
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
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetLSigMode ();
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
      return MicroSeconds (4) * (nDataLtf /* - 1 */ + nExtensionLtf); //FIXME: no HT-STF and first HT-LTF is already in preamble, see IEEE 802.11-2016, section 19.3.5.5 "HT-greenfield format LTF"
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
