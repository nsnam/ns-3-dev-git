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
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#include "dsss-phy.h"
#include "dsss-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-phy.h" //only used for static mode constructor
#include "wifi-utils.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DsssPhy");

/*******************************************************
 *       HR/DSSS PHY (IEEE 802.11-2016, clause 16)
 *******************************************************/

/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats DsssPhy::m_dsssPpduFormats {
  { WIFI_PREAMBLE_LONG,  { WIFI_PPDU_FIELD_PREAMBLE,      //PHY preamble
                           WIFI_PPDU_FIELD_NON_HT_HEADER, //PHY header
                           WIFI_PPDU_FIELD_DATA } },
  { WIFI_PREAMBLE_SHORT, { WIFI_PPDU_FIELD_PREAMBLE,      //Short PHY preamble
                           WIFI_PPDU_FIELD_NON_HT_HEADER, //Short PHY header
                           WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_ON* */

DsssPhy::DsssPhy ()
{
  NS_LOG_FUNCTION (this);
  for (const auto & rate : GetDsssRatesBpsList ())
    {
      WifiMode mode = GetDsssRate (rate);
      NS_LOG_LOGIC ("Add " << mode << " to list");
      m_modeList.emplace_back (mode);
    }
}

DsssPhy::~DsssPhy ()
{
  NS_LOG_FUNCTION (this);
}

WifiMode
DsssPhy::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_PREAMBLE: //consider header mode for preamble (useful for InterferenceHelper)
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetHeaderMode (txVector);
      default:
        return PhyEntity::GetSigMode (field, txVector);
    }
}

WifiMode
DsssPhy::GetHeaderMode (WifiTxVector txVector) const
{
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_LONG
      || txVector.GetMode () == GetDsssRate1Mbps ())
    {
      //Section 16.2.3 "PPDU field definitions" and Section 16.2.2.2 "Long PPDU format"; IEEE Std 802.11-2016
      return GetDsssRate1Mbps ();
    }
  else
    {
      //Section 16.2.2.3 "Short PPDU format"; IEEE Std 802.11-2016
      return GetDsssRate2Mbps ();
    }
}

const PhyEntity::PpduFormats &
DsssPhy::GetPpduFormats (void) const
{
  return m_dsssPpduFormats;
}

Time
DsssPhy::GetDuration (WifiPpduField field, WifiTxVector txVector) const
{
  if (field == WIFI_PPDU_FIELD_PREAMBLE)
    {
      return GetPreambleDuration (txVector); //SYNC + SFD or shortSYNC + shortSFD
    }
  else if (field == WIFI_PPDU_FIELD_NON_HT_HEADER)
    {
      return GetHeaderDuration (txVector); //PHY header or short PHY header
    }
  else
    {
      return PhyEntity::GetDuration (field, txVector);
    }
}

Time
DsssPhy::GetPreambleDuration (WifiTxVector txVector) const
{
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_SHORT
      && (txVector.GetMode ().GetDataRate (22) > 1000000))
    {
      //Section 16.2.2.3 "Short PPDU format" Figure 16-2 "Short PPDU format"; IEEE Std 802.11-2016
      return MicroSeconds (72);
    }
  else
    {
      //Section 16.2.2.2 "Long PPDU format" Figure 16-1 "Long PPDU format"; IEEE Std 802.11-2016
      return MicroSeconds (144);
    }
}

Time
DsssPhy::GetHeaderDuration (WifiTxVector txVector) const
{
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_SHORT
      && (txVector.GetMode ().GetDataRate (22) > 1000000))
    {
      //Section 16.2.2.3 "Short PPDU format" and Figure 16-2 "Short PPDU format"; IEEE Std 802.11-2016
      return MicroSeconds (24);
    }
  else
    {
      //Section 16.2.2.2 "Long PPDU format" and Figure 16-1 "Short PPDU format"; IEEE Std 802.11-2016
      return MicroSeconds (48);
    }
}

Time
DsssPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand /* band */, MpduType /* mpdutype */,
                             bool /* incFlag */, uint32_t & /* totalAmpduSize */, double & /* totalAmpduNumSymbols */,
                             uint16_t /* staId */) const
{
  return MicroSeconds (lrint (ceil ((size * 8.0) / (txVector.GetMode ().GetDataRate (22) / 1.0e6))));
}

Ptr<WifiPpdu>
DsssPhy::BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration)
{
  NS_LOG_FUNCTION (this << psdus << txVector << ppduDuration);
  return Create<DsssPpdu> (psdus.begin ()->second, txVector, ppduDuration,
                           ObtainNextUid (txVector));
}

PhyEntity::PhyFieldRxStatus
DsssPhy::DoEndReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  if (field == WIFI_PPDU_FIELD_NON_HT_HEADER)
    {
      return EndReceiveHeader (event); //PHY header or short PHY header
    }
  return PhyEntity::DoEndReceiveField (field, event);
}

PhyEntity::PhyFieldRxStatus
DsssPhy::EndReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  SnrPer snrPer = GetPhyHeaderSnrPer (WIFI_PPDU_FIELD_NON_HT_HEADER, event);
  NS_LOG_DEBUG ("Long/Short PHY header: SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per);
  PhyFieldRxStatus status (GetRandomValue () > snrPer.per);
  if (status.isSuccess)
    {
      NS_LOG_DEBUG ("Received long/short PHY header");
      if (!IsConfigSupported (event->GetPpdu ()))
        {
          status = PhyFieldRxStatus (false, UNSUPPORTED_SETTINGS, DROP);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Abort reception because long/short PHY header reception failed");
      status.reason = L_SIG_FAILURE;
      status.actionIfFailure = ABORT;
    }
  return status;
}

void
DsssPhy::InitializeModes (void)
{
  for (const auto & rate : GetDsssRatesBpsList ())
    {
      GetDsssRate (rate);
    }
}

WifiMode
DsssPhy::GetDsssRate (uint64_t rate)
{
  switch (rate)
    {
      case 1000000:
        return GetDsssRate1Mbps ();
      case 2000000:
        return GetDsssRate2Mbps ();
      case 5500000:
        return GetDsssRate5_5Mbps ();
      case 11000000:
        return GetDsssRate11Mbps ();
      default:
        NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for HR/DSSS");
        return WifiMode ();
    }
}

std::vector<uint64_t>
DsssPhy::GetDsssRatesBpsList (void)
{
  /* *NS_CHECK_STYLE_OFF* */
  return {
           1000000,  2000000,
           5500000, 11000000
  };
  /* *NS_CHECK_STYLE_ON* */
}

// Clause 15 rates (DSSS)

WifiMode
DsssPhy::GetDsssRate1Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate1Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     2);
  return mode;
}

WifiMode
DsssPhy::GetDsssRate2Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate2Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}


// Clause 16 rates (HR/DSSS)

WifiMode
DsssPhy::GetDsssRate5_5Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate5_5Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     16);
  return mode;
}

WifiMode
DsssPhy::GetDsssRate11Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate11Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     256);
  return mode;
}

} //namespace ns3

namespace {

/**
 * Constructor class for DSSS modes
 */
static class ConstructorDsss
{
public:
  ConstructorDsss ()
  {
    ns3::DsssPhy::InitializeModes ();
    ns3::Ptr<ns3::DsssPhy> phyEntity = ns3::Create<ns3::DsssPhy> ();
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_HR_DSSS, phyEntity);
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_DSSS, phyEntity); //use same entity when plain DSSS modes are used
  }
} g_constructor_dsss; ///< the constructor for DSSS modes

}
