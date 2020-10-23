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

#include "ofdm-phy.h"
#include "wifi-phy.h" //only used for static mode constructor
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OfdmPhy");

/*******************************************************
 *       OFDM PHY (IEEE 802.11-2016, clause 17)
 *******************************************************/

/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats OfdmPhy::m_ofdmPpduFormats {
  { WIFI_PREAMBLE_LONG, { WIFI_PPDU_FIELD_PREAMBLE,      //STF + LTF
                          WIFI_PPDU_FIELD_NON_HT_HEADER, //SIG
                          WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_ON* */

OfdmPhy::OfdmPhy (OfdmPhyVariant variant /* = OFDM_PHY_DEFAULT */, bool buildModeList /* = true */)
{
  NS_LOG_FUNCTION (this << variant << buildModeList);

  if (buildModeList)
    {
      auto bwRatesMap = GetOfdmRatesBpsList ();

      switch (variant)
        {
          case OFDM_PHY_DEFAULT:
            for (const auto & rate : bwRatesMap.at (20))
              {
                WifiMode mode = GetOfdmRate (rate, 20);
                NS_LOG_LOGIC ("Add " << mode << " to list");
                m_modeList.emplace_back (mode);
              }
            break;
          case OFDM_PHY_HOLLAND:
            NS_LOG_LOGIC ("Use Holland configuration");
            NS_LOG_LOGIC ("Add OfdmRate6Mbps to list");
            m_modeList.emplace_back (GetOfdmRate6Mbps ());
            NS_LOG_LOGIC ("Add OfdmRate12Mbps to list");
            m_modeList.emplace_back (GetOfdmRate12Mbps ());
            NS_LOG_LOGIC ("Add OfdmRate18Mbps to list");
            m_modeList.emplace_back (GetOfdmRate18Mbps ());
            NS_LOG_LOGIC ("Add OfdmRate36Mbps to list");
            m_modeList.emplace_back (GetOfdmRate36Mbps ());
            NS_LOG_LOGIC ("Add OfdmRate54Mbps to list");
            m_modeList.emplace_back (GetOfdmRate54Mbps ());
            break;
          case OFDM_PHY_10_MHZ:
            for (const auto & rate : bwRatesMap.at (10))
              {
                WifiMode mode = GetOfdmRate (rate, 10);
                NS_LOG_LOGIC ("Add " << mode << " to list");
                m_modeList.emplace_back (mode);
              }
            break;
          case OFDM_PHY_5_MHZ:
            for (const auto & rate : bwRatesMap.at (5))
              {
                WifiMode mode = GetOfdmRate (rate, 5);
                NS_LOG_LOGIC ("Add " << mode << " to list");
                m_modeList.emplace_back (mode);
              }
            break;
          default:
            NS_ABORT_MSG ("Unsupported 11a OFDM variant");
        }
    }
}

OfdmPhy::~OfdmPhy ()
{
  NS_LOG_FUNCTION (this);
}

WifiMode
OfdmPhy::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetHeaderMode (txVector);
      default:
        return PhyEntity::GetSigMode (field, txVector);
    }
}

WifiMode
OfdmPhy::GetHeaderMode (WifiTxVector txVector) const
{
  switch (txVector.GetChannelWidth ())
    {
      case 5:
        return GetOfdmRate1_5MbpsBW5MHz ();
      case 10:
        return GetOfdmRate3MbpsBW10MHz ();
      case 20:
      default:
        //Section 17.3.2 "PPDU frame format"; IEEE Std 802.11-2016.
        //Actually this is only the first part of the PhyHeader,
        //because the last 16 bits of the PhyHeader are using the
        //same mode of the payload
        return GetOfdmRate6Mbps ();
    }
}

const PhyEntity::PpduFormats &
OfdmPhy::GetPpduFormats (void) const
{
  return m_ofdmPpduFormats;
}

Time
OfdmPhy::GetDuration (WifiPpduField field, WifiTxVector txVector) const
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_PREAMBLE:
        return GetPreambleDuration (txVector); //L-STF + L-LTF
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetHeaderDuration (txVector); //L-SIG
      default:
        return PhyEntity::GetDuration (field, txVector);
    }
}

Time
OfdmPhy::GetPreambleDuration (WifiTxVector txVector) const
{
  switch (txVector.GetChannelWidth ())
    {
      case 20:
      default:
        //Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016
        return MicroSeconds (16);
      case 10:
        //Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016
        return MicroSeconds (32);
      case 5:
        //Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016
        return MicroSeconds (64);
    }
}

Time
OfdmPhy::GetHeaderDuration (WifiTxVector txVector) const
{
  switch (txVector.GetChannelWidth ())
    {
      case 20:
      default:
        //Section 17.3.3 "PHY preamble (SYNC)" and Figure 17-4 "OFDM training structure"; IEEE Std 802.11-2016
        //also Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016
        //We return the duration of the SIGNAL field only, since the
        //SERVICE field (which strictly speaking belongs to the PHY
        //header, see Section 17.3.2 and Figure 17-1) is sent using the
        //payload mode.
        return MicroSeconds (4);
      case 10:
        //Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016
        return MicroSeconds (8);
      case 5:
        //Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016
        return MicroSeconds (16);
    }
}

void
OfdmPhy::InitializeModes (void)
{
  for (const auto & ratesPerBw : GetOfdmRatesBpsList ())
    {
      for (const auto & rate : ratesPerBw.second)
        {
          GetOfdmRate (rate, ratesPerBw.first);
        }
    }
}

WifiMode
OfdmPhy::GetOfdmRate (uint64_t rate, uint16_t bw)
{
  switch (bw)
    {
      case 20:
        switch (rate)
          {
            case 6000000:
              return GetOfdmRate6Mbps ();
            case 9000000:
              return GetOfdmRate9Mbps ();
            case 12000000:
              return GetOfdmRate12Mbps ();
            case 18000000:
              return GetOfdmRate18Mbps ();
            case 24000000:
              return GetOfdmRate24Mbps ();
            case 36000000:
              return GetOfdmRate36Mbps ();
            case 48000000:
              return GetOfdmRate48Mbps ();
            case 54000000:
              return GetOfdmRate54Mbps ();
            default:
              NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for 11a OFDM (default)");
              return WifiMode ();
          }
        break;
      case 10:
        switch (rate)
          {
            case 3000000:
              return GetOfdmRate3MbpsBW10MHz ();
            case 4500000:
              return GetOfdmRate4_5MbpsBW10MHz ();
            case 6000000:
              return GetOfdmRate6MbpsBW10MHz ();
            case 9000000:
              return GetOfdmRate9MbpsBW10MHz ();
            case 12000000:
              return GetOfdmRate12MbpsBW10MHz ();
            case 18000000:
              return GetOfdmRate18MbpsBW10MHz ();
            case 24000000:
              return GetOfdmRate24MbpsBW10MHz ();
            case 27000000:
              return GetOfdmRate27MbpsBW10MHz ();
            default:
              NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for 11a OFDM (10 MHz)");
              return WifiMode ();
          }
        break;
      case 5:
        switch (rate)
          {
            case 1500000:
              return GetOfdmRate1_5MbpsBW5MHz ();
            case 2250000:
              return GetOfdmRate2_25MbpsBW5MHz ();
            case 3000000:
              return GetOfdmRate3MbpsBW5MHz ();
            case 4500000:
              return GetOfdmRate4_5MbpsBW5MHz ();
            case 6000000:
              return GetOfdmRate6MbpsBW5MHz ();
            case 9000000:
              return GetOfdmRate9MbpsBW5MHz ();
            case 12000000:
              return GetOfdmRate12MbpsBW5MHz ();
            case 13500000:
              return GetOfdmRate13_5MbpsBW5MHz ();
            default:
              NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for 11a OFDM (5 MHz)");
              return WifiMode ();
          }
        break;
      default:
        NS_ABORT_MSG ("Inexistent bandwidth (" << +bw << " MHz) requested for 11a OFDM");
        return WifiMode ();
    }
}

std::map<uint16_t, std::vector<uint64_t> >
OfdmPhy::GetOfdmRatesBpsList (void)
{
  /* *NS_CHECK_STYLE_OFF* */
  return {
           { 20, // MHz
             {  6000000,  9000000, 12000000, 18000000,
               24000000, 36000000, 48000000, 54000000 }},
           { 10, // MHz
             {  3000000,  4500000,  6000000,  9000000,
               12000000, 18000000, 24000000, 27000000 }},
           { 5, // MHz
             {  1500000,  2250000,  3000000,  4500000,
                6000000,  9000000, 12000000, 13500000 }}
  };
  /* *NS_CHECK_STYLE_ON* */
}


// 20 MHz channel rates (default)

WifiMode
OfdmPhy::GetOfdmRate6Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate9Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate12Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate18Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate24Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate36Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate36Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate48Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate48Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate54Mbps (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate54Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 10 MHz channel rates

WifiMode
OfdmPhy::GetOfdmRate3MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate4_5MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate6MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate9MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate12MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate18MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate24MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate27MbpsBW10MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate27MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 5 MHz channel rates

WifiMode
OfdmPhy::GetOfdmRate1_5MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate1_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate2_25MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate2_25MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate3MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate4_5MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate6MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate9MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate12MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
OfdmPhy::GetOfdmRate13_5MbpsBW5MHz (void)
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate13_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}

} //namespace ns3

namespace {

/**
 * Constructor class for OFDM modes
 */
static class ConstructorOfdm
{
public:
  ConstructorOfdm ()
  {
    ns3::OfdmPhy::InitializeModes ();
    ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_OFDM, ns3::Create<ns3::OfdmPhy> ()); //default variant will do
  }
} g_constructor_ofdm; ///< the constructor for OFDM modes

}
