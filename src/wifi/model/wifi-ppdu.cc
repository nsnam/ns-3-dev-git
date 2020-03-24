/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-phy.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPpdu");

WifiPpdu::WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, uint16_t frequency)
  : m_preamble (txVector.GetPreambleType ()),
    m_modulation (txVector.IsValid () ? txVector.GetMode ().GetModulationClass () : WIFI_MOD_CLASS_UNKNOWN),
    m_psdu (psdu),
    m_truncatedTx (false),
    m_frequency (frequency),
    m_channelWidth (txVector.GetChannelWidth ()),
    m_txPowerLevel (txVector.GetTxPowerLevel ())
{
  if (!txVector.IsValid ())
    {
      return;
    }
  NS_LOG_FUNCTION (this << psdu << txVector << ppduDuration << frequency);
  switch (m_modulation)
    {
      case WIFI_MOD_CLASS_DSSS:
      case WIFI_MOD_CLASS_HR_DSSS:
        {
          m_dsssSig.SetRate (txVector.GetMode ().GetDataRate (22));
          Time psduDuration = ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
          m_dsssSig.SetLength (psduDuration.GetMicroSeconds ());
          break;
        }
      case WIFI_MOD_CLASS_OFDM:
      case WIFI_MOD_CLASS_ERP_OFDM:
        {
          m_lSig.SetRate (txVector.GetMode ().GetDataRate (txVector), m_channelWidth);
          m_lSig.SetLength (psdu->GetSize ());
          break;
        }
      case WIFI_MOD_CLASS_HT:
        {
          uint8_t sigExtension = 0;
          if (Is2_4Ghz (frequency))
            {
              sigExtension = 6;
            }
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3);
          m_lSig.SetLength (length);
          m_htSig.SetMcs (txVector.GetMode ().GetMcsValue ());
          m_htSig.SetChannelWidth (m_channelWidth);
          m_htSig.SetHtLength (psdu->GetSize ());
          m_htSig.SetAggregation (txVector.IsAggregation ());
          m_htSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
          break;
        }
      case WIFI_MOD_CLASS_VHT:
        {
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000)) / 1000) / 4.0) * 3) - 3);
          m_lSig.SetLength (length);
          m_vhtSig.SetMuFlag (m_preamble == WIFI_PREAMBLE_VHT_MU);
          m_vhtSig.SetChannelWidth (m_channelWidth);
          m_vhtSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
          uint32_t nSymbols = (static_cast<double> ((ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector)).GetNanoSeconds ()) / (3200 + txVector.GetGuardInterval ()));
          if (txVector.GetGuardInterval () == 400)
            {
              m_vhtSig.SetShortGuardIntervalDisambiguation ((nSymbols % 10) == 9);
            }
          m_vhtSig.SetSuMcs (txVector.GetMode ().GetMcsValue ());
          m_vhtSig.SetNStreams (txVector.GetNss ());
          break;
        }
      case WIFI_MOD_CLASS_HE:
        {
          uint8_t sigExtension = 0;
          if (Is2_4Ghz (frequency))
            {
              sigExtension = 6;
            }
          uint8_t m = 0;
          if (m_preamble == WIFI_PREAMBLE_HE_SU)
            {
              m = 2;
            }
          else if (m_preamble == WIFI_PREAMBLE_HE_MU)
            {
              m = 1;
            }
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3 - m);
          m_lSig.SetLength (length);
          m_heSig.SetMuFlag (m_preamble == WIFI_PREAMBLE_HE_MU);
          m_heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
          m_heSig.SetBssColor (txVector.GetBssColor ());
          m_heSig.SetChannelWidth (m_channelWidth);
          m_heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2/*NLTF currently unused*/);
          m_heSig.SetNStreams (txVector.GetNss ());
          break;
        }
        default:
        NS_FATAL_ERROR ("unsupported modulation class");
        break;
    }
}

WifiPpdu::~WifiPpdu ()
{
}

WifiTxVector
WifiPpdu::GetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  switch (m_modulation)
    {
      case WIFI_MOD_CLASS_DSSS:
      case WIFI_MOD_CLASS_HR_DSSS:
        {
          WifiMode mode;
          switch (m_dsssSig.GetRate ())
            {
              case 1000000:
                mode = WifiPhy::GetDsssRate1Mbps ();
                break;
              case 2000000:
                mode = WifiPhy::GetDsssRate2Mbps ();
                break;
              case 5500000:
                mode = WifiPhy::GetDsssRate5_5Mbps ();
                break;
              case 11000000:
                mode = WifiPhy::GetDsssRate11Mbps ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid rate encoded in DSSS SIG header");
                break;
            }
          txVector.SetMode (mode);
          txVector.SetChannelWidth (22);
          break;
        }
      case WIFI_MOD_CLASS_OFDM:
        {
          WifiMode mode;
          switch (m_lSig.GetRate (m_channelWidth))
            {
              case 1500000:
                mode = WifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
                break;
              case 2250000:
                mode = WifiPhy::GetOfdmRate2_25MbpsBW5MHz ();
                break;
              case 3000000:
                mode = (m_channelWidth == 5) ? WifiPhy::GetOfdmRate3MbpsBW5MHz () : WifiPhy::GetOfdmRate3MbpsBW10MHz ();
                break;
              case 4500000:
                mode = (m_channelWidth == 5) ? WifiPhy::GetOfdmRate4_5MbpsBW5MHz () : WifiPhy::GetOfdmRate4_5MbpsBW10MHz ();
                break;
              case 6000000:
                if (m_channelWidth == 5)
                  {
                    mode = WifiPhy::GetOfdmRate6MbpsBW5MHz ();
                  }
                else if (m_channelWidth == 10)
                  {
                    mode = WifiPhy::GetOfdmRate6MbpsBW10MHz ();
                  }
                else
                  {
                    mode = WifiPhy::GetOfdmRate6Mbps ();
                  }
                break;
              case 9000000:
                if (m_channelWidth == 5)
                  {
                    mode = WifiPhy::GetOfdmRate9MbpsBW5MHz ();
                  }
                else if (m_channelWidth == 10)
                  {
                    mode = WifiPhy::GetOfdmRate9MbpsBW10MHz ();
                  }
                else
                  {
                    mode = WifiPhy::GetOfdmRate9Mbps ();
                  }
                break;
              case 12000000:
                if (m_channelWidth == 5)
                  {
                    mode = WifiPhy::GetOfdmRate12MbpsBW5MHz ();
                  }
                else if (m_channelWidth == 10)
                  {
                    mode = WifiPhy::GetOfdmRate12MbpsBW10MHz ();
                  }
                else
                  {
                    mode = WifiPhy::GetOfdmRate12Mbps ();
                  }
                break;
              case 13500000:
                mode = WifiPhy::GetOfdmRate13_5MbpsBW5MHz ();
                break;
              case 18000000:
                mode = (m_channelWidth == 10) ? WifiPhy::GetOfdmRate18MbpsBW10MHz () : WifiPhy::GetOfdmRate18Mbps ();
                break;
              case 24000000:
                mode = (m_channelWidth == 10) ? WifiPhy::GetOfdmRate24MbpsBW10MHz () : WifiPhy::GetOfdmRate24Mbps ();
                break;
              case 27000000:
                mode = WifiPhy::GetOfdmRate27MbpsBW10MHz ();
                break;
              case 36000000:
                mode = WifiPhy::GetOfdmRate36Mbps ();
                break;
              case 48000000:
                mode = WifiPhy::GetOfdmRate48Mbps ();
                break;
              case 54000000:
                mode = WifiPhy::GetOfdmRate54Mbps ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid rate encoded in L-SIG header");
                break;
            }
          txVector.SetMode (mode);
          //OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
          txVector.SetChannelWidth (m_channelWidth < 20 ? m_channelWidth : 20);
          break;
        }
      case WIFI_MOD_CLASS_ERP_OFDM:
        {
          WifiMode mode;
          switch (m_lSig.GetRate ())
            {
              case 6000000:
                mode = WifiPhy::GetErpOfdmRate6Mbps ();
                break;
              case 9000000:
                mode = WifiPhy::GetErpOfdmRate9Mbps ();
                break;
              case 12000000:
                mode = WifiPhy::GetErpOfdmRate12Mbps ();
                break;
              case 18000000:
                mode = WifiPhy::GetErpOfdmRate18Mbps ();
                break;
              case 24000000:
                mode = WifiPhy::GetErpOfdmRate24Mbps ();
                break;
              case 36000000:
                mode = WifiPhy::GetErpOfdmRate36Mbps ();
                break;
              case 48000000:
                mode = WifiPhy::GetErpOfdmRate48Mbps ();
                break;
              case 54000000:
                mode = WifiPhy::GetErpOfdmRate54Mbps ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid rate encoded in L-SIG header");
                break;
            }
          txVector.SetMode (mode);
          //ERP-OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
          txVector.SetChannelWidth (m_channelWidth < 20 ? m_channelWidth : 20);
          break;
        }
      case WIFI_MOD_CLASS_HT:
        {
          WifiMode mode;
          switch (m_htSig.GetMcs ())
            {
              case 0:
                mode = WifiPhy::GetHtMcs0 ();
                break;
              case 1:
                mode = WifiPhy::GetHtMcs1 ();
                break;
              case 2:
                mode = WifiPhy::GetHtMcs2 ();
                break;
              case 3:
                mode = WifiPhy::GetHtMcs3 ();
                break;
              case 4:
                mode = WifiPhy::GetHtMcs4 ();
                break;
              case 5:
                mode = WifiPhy::GetHtMcs5 ();
                break;
              case 6:
                mode = WifiPhy::GetHtMcs6 ();
                break;
              case 7:
                mode = WifiPhy::GetHtMcs7 ();
                break;
              case 8:
                mode = WifiPhy::GetHtMcs8 ();
                break;
              case 9:
                mode = WifiPhy::GetHtMcs9 ();
                break;
              case 10:
                mode = WifiPhy::GetHtMcs10 ();
                break;
              case 11:
                mode = WifiPhy::GetHtMcs11 ();
                break;
              case 12:
                mode = WifiPhy::GetHtMcs12 ();
                break;
              case 13:
                mode = WifiPhy::GetHtMcs13 ();
                break;
              case 14:
                mode = WifiPhy::GetHtMcs14 ();
                break;
              case 15:
                mode = WifiPhy::GetHtMcs15 ();
                break;
              case 16:
                mode = WifiPhy::GetHtMcs16 ();
                break;
              case 17:
                mode = WifiPhy::GetHtMcs17 ();
                break;
              case 18:
                mode = WifiPhy::GetHtMcs18 ();
                break;
              case 19:
                mode = WifiPhy::GetHtMcs19 ();
                break;
              case 20:
                mode = WifiPhy::GetHtMcs20 ();
                break;
              case 21:
                mode = WifiPhy::GetHtMcs21 ();
                break;
              case 22:
                mode = WifiPhy::GetHtMcs22 ();
                break;
              case 23:
                mode = WifiPhy::GetHtMcs23 ();
                break;
              case 24:
                mode = WifiPhy::GetHtMcs24 ();
                break;
              case 25:
                mode = WifiPhy::GetHtMcs25 ();
                break;
              case 26:
                mode = WifiPhy::GetHtMcs26 ();
                break;
              case 27:
                mode = WifiPhy::GetHtMcs27 ();
                break;
              case 28:
                mode = WifiPhy::GetHtMcs28 ();
                break;
              case 29:
                mode = WifiPhy::GetHtMcs29 ();
                break;
              case 30:
                mode = WifiPhy::GetHtMcs30 ();
                break;
              case 31:
                mode = WifiPhy::GetHtMcs31 ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid MCS encoded in HT-SIG header");
                break;
            }
          txVector.SetMode (mode);
          txVector.SetChannelWidth (m_htSig.GetChannelWidth ());
          txVector.SetNss (1 + (m_htSig.GetMcs () / 8));
          txVector.SetGuardInterval(m_htSig.GetShortGuardInterval () ? 400 : 800);
          txVector.SetAggregation (m_htSig.GetAggregation ());
          break;
        }
      case WIFI_MOD_CLASS_VHT:
        {
          WifiMode mode;
          switch (m_vhtSig.GetSuMcs ())
            {
              case 0:
                mode = WifiPhy::GetVhtMcs0 ();
                break;
              case 1:
                mode = WifiPhy::GetVhtMcs1 ();
                break;
              case 2:
                mode = WifiPhy::GetVhtMcs2 ();
                break;
              case 3:
                mode = WifiPhy::GetVhtMcs3 ();
                break;
              case 4:
                mode = WifiPhy::GetVhtMcs4 ();
                break;
              case 5:
                mode = WifiPhy::GetVhtMcs5 ();
                break;
              case 6:
                mode = WifiPhy::GetVhtMcs6 ();
                break;
              case 7:
                mode = WifiPhy::GetVhtMcs7 ();
                break;
              case 8:
                mode = WifiPhy::GetVhtMcs8 ();
                break;
              case 9:
                mode = WifiPhy::GetVhtMcs9 ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid MCS encoded in VHT-SIG header");
                break;
            }
          txVector.SetMode (mode);
          txVector.SetChannelWidth (m_vhtSig.GetChannelWidth ());
          txVector.SetNss (m_vhtSig.GetNStreams ());
          txVector.SetGuardInterval (m_vhtSig.GetShortGuardInterval () ? 400 : 800);
          txVector.SetAggregation (m_psdu->IsAggregate ());
          break;
        }
      case WIFI_MOD_CLASS_HE:
        {
          WifiMode mode;
          switch (m_heSig.GetMcs ())
            {
              case 0:
                mode = WifiPhy::GetHeMcs0 ();
                break;
              case 1:
                mode = WifiPhy::GetHeMcs1 ();
                break;
              case 2:
                mode = WifiPhy::GetHeMcs2 ();
                break;
              case 3:
                mode = WifiPhy::GetHeMcs3 ();
                break;
              case 4:
                mode = WifiPhy::GetHeMcs4 ();
                break;
              case 5:
                mode = WifiPhy::GetHeMcs5 ();
                break;
              case 6:
                mode = WifiPhy::GetHeMcs6 ();
                break;
              case 7:
                mode = WifiPhy::GetHeMcs7 ();
                break;
              case 8:
                mode = WifiPhy::GetHeMcs8 ();
                break;
              case 9:
                mode = WifiPhy::GetHeMcs9 ();
                break;
              case 10:
                mode = WifiPhy::GetHeMcs10 ();
                break;
              case 11:
                mode = WifiPhy::GetHeMcs11 ();
                break;
              default:
                NS_ASSERT_MSG (false, "Invalid MCS encoded in HE-SIG header");
                break;
            }
          txVector.SetMode (mode);
          txVector.SetChannelWidth (m_heSig.GetChannelWidth ());
          txVector.SetNss (m_heSig.GetNStreams ());
          txVector.SetGuardInterval (m_heSig.GetGuardInterval ());
          txVector.SetBssColor (m_heSig.GetBssColor ());
          txVector.SetAggregation (m_psdu->IsAggregate ());
          break;
        }
      default:
        NS_FATAL_ERROR ("unsupported modulation class");
        break;
    }
  txVector.SetTxPowerLevel (m_txPowerLevel);
  return txVector;
}

Ptr<const WifiPsdu>
WifiPpdu::GetPsdu (void) const
{
  return m_psdu;
}

bool
WifiPpdu::IsTruncatedTx (void) const
{
  return m_truncatedTx;
}

void
WifiPpdu::SetTruncatedTx (void)
{
  NS_LOG_FUNCTION (this);
  m_truncatedTx = true;
}

Time
WifiPpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  switch (m_modulation)
    {
      case WIFI_MOD_CLASS_DSSS:
      case WIFI_MOD_CLASS_HR_DSSS:
          ppduDuration = MicroSeconds (m_dsssSig.GetLength ()) + WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
          break;
      case WIFI_MOD_CLASS_OFDM:
      case WIFI_MOD_CLASS_ERP_OFDM:
          ppduDuration = WifiPhy::CalculateTxDuration (m_lSig.GetLength (), txVector, m_frequency);
          break;
      case WIFI_MOD_CLASS_HT:
          ppduDuration = WifiPhy::CalculateTxDuration (m_htSig.GetHtLength (), txVector, m_frequency);
          break;
      case WIFI_MOD_CLASS_VHT:
        {
          Time tSymbol = NanoSeconds (3200 + txVector.GetGuardInterval ());
          Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
          Time calculatedDuration = MicroSeconds (((ceil (static_cast<double> (m_lSig.GetLength () + 3) / 3)) * 4) + 20);
          uint32_t nSymbols = floor (static_cast<double> ((calculatedDuration - preambleDuration).GetNanoSeconds ()) / tSymbol.GetNanoSeconds ());
          if (m_vhtSig.GetShortGuardInterval () && m_vhtSig.GetShortGuardIntervalDisambiguation ())
            {
              nSymbols--;
            }
          ppduDuration = preambleDuration + (nSymbols * tSymbol);
          break;
        }
      case WIFI_MOD_CLASS_HE:
        {
          Time tSymbol = NanoSeconds (12800 + txVector.GetGuardInterval ());
          Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
          uint8_t sigExtension = 0;
          if (Is2_4Ghz (m_frequency))
            {
              sigExtension = 6;
            }
          uint8_t m = 0;
          if (m_preamble == WIFI_PREAMBLE_HE_SU)
            {
              m = 2;
            }
          else if (m_preamble == WIFI_PREAMBLE_HE_MU)
            {
              m = 1;
            }
          //Equation 27-11 of IEEE P802.11ax/D4.0
          Time calculatedDuration = MicroSeconds (((ceil (static_cast<double> (m_lSig.GetLength () + 3 + m) / 3)) * 4) + 20 + sigExtension);
          uint32_t nSymbols = floor (static_cast<double> ((calculatedDuration - preambleDuration).GetNanoSeconds () - (sigExtension * 1000)) / tSymbol.GetNanoSeconds ());
          ppduDuration = preambleDuration + (nSymbols * tSymbol) + MicroSeconds (sigExtension);
          break;
        }
      default:
        NS_FATAL_ERROR ("unsupported modulation class");
        break;
    }
  return ppduDuration;
}

void
WifiPpdu::Print (std::ostream& os) const
{
  os << "preamble=" << m_preamble
     << ", modulation=" << m_modulation
     << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N")
     << ", PSDU=" << m_psdu;
}

std::ostream & operator << (std::ostream &os, const WifiPpdu &ppdu)
{
  ppdu.Print (os);
  return os;
}

} //namespace ns3
