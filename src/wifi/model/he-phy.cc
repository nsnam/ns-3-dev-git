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
#include "ns3/log.h"
#include "ns3/assert.h"

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
  }
} g_constructor_he; ///< the constructor for HE modes

}
