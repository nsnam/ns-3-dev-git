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

#include "vht-phy.h"
#include "wifi-phy.h" //only used for static mode constructor
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VhtPhy");

/*******************************************************
 *       VHT PHY (IEEE 802.11-2016, clause 21)
 *******************************************************/

/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats VhtPhy::m_vhtPpduFormats {
  { WIFI_PREAMBLE_VHT_SU, { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                            WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG
                            WIFI_PPDU_FIELD_SIG_A,         //VHT-SIG-A
                            WIFI_PPDU_FIELD_TRAINING,      //VHT-STF + VHT-LTFs
                            WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_VHT_MU, { WIFI_PPDU_FIELD_PREAMBLE,      //L-STF + L-LTF
                            WIFI_PPDU_FIELD_NON_HT_HEADER, //L-SIG
                            WIFI_PPDU_FIELD_SIG_A,         //VHT-SIG-A
                            WIFI_PPDU_FIELD_TRAINING,      //VHT-STF + VHT-LTFs
                            WIFI_PPDU_FIELD_SIG_B,         //VHT-SIG-B
                            WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_ON* */

VhtPhy::VhtPhy (bool buildModeList /* = true */)
  : HtPhy (1, false) //don't add HT modes to list
{
  NS_LOG_FUNCTION (this << buildModeList);
  m_bssMembershipSelector = VHT_PHY;
  m_maxMcsIndexPerSs = 9;
  m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
  if (buildModeList)
    {
      BuildModeList ();
    }
}

VhtPhy::~VhtPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
VhtPhy::BuildModeList (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_modeList.empty ());
  NS_ASSERT (m_bssMembershipSelector == VHT_PHY);
  for (uint8_t index = 0; index <= m_maxSupportedMcsIndexPerSs; ++index)
    {
      NS_LOG_LOGIC ("Add VhtMcs" << +index << " to list");
      m_modeList.emplace_back (GetVhtMcs (index));
    }
}

const PhyEntity::PpduFormats &
VhtPhy::GetPpduFormats (void) const
{
  return m_vhtPpduFormats;
}

WifiMode
VhtPhy::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_SIG_A:
        return GetSigAMode ();
      case WIFI_PPDU_FIELD_SIG_B:
        return GetSigBMode (txVector);
      default:
        return HtPhy::GetSigMode (field, txVector);
    }
}

WifiMode
VhtPhy::GetHtSigMode (void) const
{
  NS_ASSERT (m_bssMembershipSelector != HT_PHY);
  NS_FATAL_ERROR ("No HT-SIG");
  return WifiMode ();
}

WifiMode
VhtPhy::GetSigAMode (void) const
{
  return GetLSigMode (); //same number of data tones as OFDM (i.e. 48)
}

WifiMode
VhtPhy::GetSigBMode (WifiTxVector txVector) const
{
  NS_ABORT_MSG_IF (txVector.GetPreambleType () != WIFI_PREAMBLE_VHT_MU, "VHT-SIG-B only available for VHT MU");
  return GetVhtMcs0 ();
}

void
VhtPhy::InitializeModes (void)
{
  for (uint8_t i = 0; i < 10; ++i)
    {
      GetVhtMcs (i);
    }
}

WifiMode
VhtPhy::GetVhtMcs (uint8_t index)
{
  switch (index)
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
        return GetVhtMcs5 ();
      case 6:
        return GetVhtMcs6 ();
      case 7:
        return GetVhtMcs7 ();
      case 8:
        return GetVhtMcs8 ();
      case 9:
        return GetVhtMcs9 ();
      default:
        NS_ABORT_MSG ("Inexistent index (" << +index << ") requested for VHT");
        return WifiMode ();
    }
}

WifiMode
VhtPhy::GetVhtMcs0 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs0", 0, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs1 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs1", 1, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs2 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs2", 2, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs3 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs3", 3, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs4 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs4", 4, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs5 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs5", 5, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs6 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs6", 6, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs7 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs7", 7, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs8 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs8", 8, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
VhtPhy::GetVhtMcs9 (void)
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs9", 9, WIFI_MOD_CLASS_VHT);
  return mcs;
}

} //namespace ns3

namespace {

/**
 * Constructor class for VHT modes
 */
static class ConstructorVht
{
public:
  ConstructorVht ()
  {
    ns3::VhtPhy::InitializeModes ();
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_VHT, ns3::Create<ns3::VhtPhy> ());
  }
} g_constructor_vht; ///< the constructor for VHT modes

}
