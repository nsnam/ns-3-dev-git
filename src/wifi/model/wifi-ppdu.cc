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
#include "dsss-phy.h"
#include "erp-ofdm-phy.h"
#include "he-phy.h" //includes OFDM, HT, and VHT

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPpdu");

WifiPpdu::WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, WifiPhyBand band, uint64_t uid)
  : m_preamble (txVector.GetPreambleType ()),
    m_modulation (txVector.IsValid () ? txVector.GetMode ().GetModulationClass () : WIFI_MOD_CLASS_UNKNOWN),
    m_truncatedTx (false),
    m_band (band),
    m_channelWidth (txVector.GetChannelWidth ()),
    m_txPowerLevel (txVector.GetTxPowerLevel ()),
    m_uid (uid)
{
  NS_LOG_FUNCTION (this << *psdu << txVector << ppduDuration << band << uid);
  m_psdus.insert (std::make_pair (SU_STA_ID, psdu));
  SetPhyHeaders (txVector, ppduDuration, band);
}

WifiPpdu::WifiPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration, WifiPhyBand band, uint64_t uid)
  : m_preamble (txVector.GetPreambleType ()),
    m_modulation (txVector.IsValid () ? txVector.GetMode (psdus.begin()->first).GetModulationClass () : WIFI_MOD_CLASS_UNKNOWN),
    m_psdus (psdus),
    m_truncatedTx (false),
    m_band (band),
    m_channelWidth (txVector.GetChannelWidth ()),
    m_txPowerLevel (txVector.GetTxPowerLevel ()),
    m_uid (uid)
{
  NS_LOG_FUNCTION (this << psdus << txVector << ppduDuration << band << uid);
  if (txVector.IsMu ())
    {
      m_muUserInfos = txVector.GetHeMuUserInfoMap ();
    }
  SetPhyHeaders (txVector, ppduDuration, band);
}

WifiPpdu::~WifiPpdu ()
{
}

void
WifiPpdu::SetPhyHeaders (WifiTxVector txVector, Time ppduDuration, WifiPhyBand band)
{
  if (!txVector.IsValid ())
    {
      return;
    }
  NS_LOG_FUNCTION (this << txVector << ppduDuration << band);
  switch (m_modulation)
    {
      case WIFI_MOD_CLASS_DSSS:
      case WIFI_MOD_CLASS_HR_DSSS:
        {
          NS_ASSERT (m_psdus.size () == 1);
          m_dsssSig.SetRate (txVector.GetMode ().GetDataRate (22));
          Time psduDuration = ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
          m_dsssSig.SetLength (psduDuration.GetMicroSeconds ());
          break;
        }
      case WIFI_MOD_CLASS_OFDM:
      case WIFI_MOD_CLASS_ERP_OFDM:
        {
          NS_ASSERT (m_psdus.size () == 1);
          m_lSig.SetRate (txVector.GetMode ().GetDataRate (txVector), m_channelWidth);
          m_lSig.SetLength (m_psdus.at (SU_STA_ID)->GetSize ());
          break;
        }
      case WIFI_MOD_CLASS_HT:
        {
          NS_ASSERT (m_psdus.size () == 1);
          uint8_t sigExtension = 0;
          if (m_band == WIFI_PHY_BAND_2_4GHZ)
            {
              sigExtension = 6;
            }
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3);
          m_lSig.SetLength (length);
          m_htSig.SetMcs (txVector.GetMode ().GetMcsValue ());
          m_htSig.SetChannelWidth (m_channelWidth);
          m_htSig.SetHtLength (m_psdus.at (SU_STA_ID)->GetSize ());
          m_htSig.SetAggregation (txVector.IsAggregation ());
          m_htSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
          m_htSig.SetFecCoding (txVector.IsLdpc ());
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
          m_vhtSig.SetCoding (txVector.IsLdpc ());
          break;
        }
      case WIFI_MOD_CLASS_HE:
        {
          uint8_t sigExtension = 0;
          if (m_band == WIFI_PHY_BAND_2_4GHZ)
            {
              sigExtension = 6;
            }
          uint8_t m = 0;
          if ((m_preamble == WIFI_PREAMBLE_HE_SU) || (m_preamble == WIFI_PREAMBLE_HE_TB))
            {
              m = 2;
            }
          else if (m_preamble == WIFI_PREAMBLE_HE_MU)
            {
              m = 1;
            }
          else
            {
              NS_ASSERT_MSG (false, "Unsupported preamble type");
            }
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3 - m);
          m_lSig.SetLength (length);
          if (m_preamble == WIFI_PREAMBLE_HE_MU)
            {
              m_heSig.SetMuFlag (true);
            }
          else if (m_preamble != WIFI_PREAMBLE_HE_TB)
            {
              m_heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
              m_heSig.SetNStreams (txVector.GetNss ());
            }
          m_heSig.SetBssColor (txVector.GetBssColor ());
          m_heSig.SetChannelWidth (m_channelWidth);
          m_heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2/*NLTF currently unused*/);
          m_heSig.SetCoding (txVector.IsLdpc ());
          break;
        }
        default:
          NS_FATAL_ERROR ("unsupported modulation class");
          break;
    }
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
          txVector.SetMode (DsssPhy::GetDsssRate (m_dsssSig.GetRate ()));
          txVector.SetChannelWidth (22);
          break;
        }
      case WIFI_MOD_CLASS_OFDM:
        {
          uint16_t channelWidth = m_channelWidth < 20 ? m_channelWidth : 20;
          txVector.SetMode (OfdmPhy::GetOfdmRate (m_lSig.GetRate (m_channelWidth), channelWidth));
          //OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
          txVector.SetChannelWidth (channelWidth);
          break;
        }
      case WIFI_MOD_CLASS_ERP_OFDM:
        {
          txVector.SetMode (ErpOfdmPhy::GetErpOfdmRate (m_lSig.GetRate ()));
          //ERP-OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
          txVector.SetChannelWidth (m_channelWidth < 20 ? m_channelWidth : 20);
          break;
        }
      case WIFI_MOD_CLASS_HT:
        {
          txVector.SetMode (HtPhy::GetHtMcs (m_htSig.GetMcs ()));
          txVector.SetChannelWidth (m_htSig.GetChannelWidth ());
          txVector.SetNss (1 + (m_htSig.GetMcs () / 8));
          txVector.SetGuardInterval(m_htSig.GetShortGuardInterval () ? 400 : 800);
          txVector.SetLdpc (m_htSig.IsLdpcFecCoding ());
          break;
        }
      case WIFI_MOD_CLASS_VHT:
        {
          txVector.SetMode (VhtPhy::GetVhtMcs (m_vhtSig.GetSuMcs ()));
          txVector.SetChannelWidth (m_vhtSig.GetChannelWidth ());
          txVector.SetNss (m_vhtSig.GetNStreams ());
          txVector.SetGuardInterval (m_vhtSig.GetShortGuardInterval () ? 400 : 800);
          txVector.SetLdpc (m_vhtSig.IsLdpcCoding ());
          break;
        }
      case WIFI_MOD_CLASS_HE:
        {
          txVector.SetMode (HePhy::GetHeMcs (m_heSig.GetMcs ()));
          txVector.SetChannelWidth (m_heSig.GetChannelWidth ());
          txVector.SetNss (m_heSig.GetNStreams ());
          txVector.SetGuardInterval (m_heSig.GetGuardInterval ());
          txVector.SetBssColor (m_heSig.GetBssColor ());
          txVector.SetLdpc (m_heSig.IsLdpcCoding ());
          break;
        }
      default:
        NS_FATAL_ERROR ("unsupported modulation class");
        break;
    }
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetAggregation (m_psdus.size () > 1 || m_psdus.begin ()->second->IsAggregate ());
  for (auto const& muUserInfo : m_muUserInfos)
    {
      txVector.SetHeMuUserInfo (muUserInfo.first, muUserInfo.second);
    }
  return txVector;
}

Ptr<const WifiPsdu>
WifiPpdu::GetPsdu (uint8_t bssColor, uint16_t staId) const
{
  if (!IsMu ())
    {
      NS_ASSERT (m_psdus.size () == 1);
      return m_psdus.at (SU_STA_ID);
    }
  else if (IsUlMu ())
    {
      NS_ASSERT (m_psdus.size () == 1);
      if (bssColor == 0 || (bssColor == m_heSig.GetBssColor ()))
        {
          return m_psdus.begin ()->second;
        }
    }
  else
    {
      if (bssColor == 0 || (bssColor == m_heSig.GetBssColor ()))
        {
          auto it = m_psdus.find (staId);
          if (it != m_psdus.end ())
            {
              return it->second;
            }
        }
    }
  return nullptr;
}

uint16_t
WifiPpdu::GetStaId (void) const
{
  NS_ASSERT (IsUlMu ());
  return m_psdus.begin ()->first;
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
          ppduDuration = WifiPhy::CalculateTxDuration (m_lSig.GetLength (), txVector, m_band);
          break;
      case WIFI_MOD_CLASS_HT:
          ppduDuration = WifiPhy::CalculateTxDuration (m_htSig.GetHtLength (), txVector, m_band);
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
          if (m_band == WIFI_PHY_BAND_2_4GHZ)
            {
              sigExtension = 6;
            }
          uint8_t m = 0;
          if ((m_preamble == WIFI_PREAMBLE_HE_SU) || (m_preamble == WIFI_PREAMBLE_HE_TB))
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

bool
WifiPpdu::IsMu (void) const
{
  return (IsDlMu () || IsUlMu ());
}

bool
WifiPpdu::IsDlMu (void) const
{
  return ((m_preamble == WIFI_PREAMBLE_VHT_MU) || (m_preamble == WIFI_PREAMBLE_HE_MU));
}

bool
WifiPpdu::IsUlMu (void) const
{
  return (m_preamble == WIFI_PREAMBLE_HE_TB);
}

WifiModulationClass
WifiPpdu::GetModulation (void) const
{
  return m_modulation;
}

uint64_t
WifiPpdu::GetUid (void) const
{
  return m_uid;
}

WifiPreamble
WifiPpdu::GetPreamble (void) const
{
  return m_preamble;
}

void
WifiPpdu::Print (std::ostream& os) const
{
  os << "preamble=" << m_preamble
     << ", modulation=" << m_modulation
     << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N")
     << ", uid=" << m_uid;
  IsMu () ? (os << ", " << m_psdus) : (os << ", PSDU=" << m_psdus.at (SU_STA_ID));
}

std::ostream & operator << (std::ostream &os, const WifiPpdu &ppdu)
{
  ppdu.Print (os);
  return os;
}

std::ostream & operator << (std::ostream &os, const WifiConstPsduMap &psdus)
{
  for (auto const& psdu : psdus)
    {
      os << "PSDU for STA_ID=" << psdu.first
         << " (" << *psdu.second << ") ";
    }
  return os;
}

} //namespace ns3
