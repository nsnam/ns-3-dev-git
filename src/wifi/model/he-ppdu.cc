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
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 */

#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-utils.h"
#include "he-phy.h"
#include "he-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HePpdu");

HePpdu::HePpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector, Time ppduDuration,
                WifiPhyBand band, uint64_t uid)
  : OfdmPpdu (psdus.begin ()->second, txVector, band, uid, false) //don't instantiate LSigHeader of OfdmPpdu
{
  NS_LOG_FUNCTION (this << psdus << txVector << ppduDuration << band << uid);

  //overwrite with map (since only first element used by OfdmPpdu)
  m_psdus.begin ()->second = 0;
  m_psdus.clear ();
  m_psdus = psdus;
  if (IsMu ())
    {
      m_muUserInfos = txVector.GetHeMuUserInfoMap ();
    }

  SetPhyHeaders (txVector, ppduDuration);
}

HePpdu::HePpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration,
                WifiPhyBand band, uint64_t uid)
  : OfdmPpdu (psdu, txVector, band, uid, false) //don't instantiate LSigHeader of OfdmPpdu
{
  NS_LOG_FUNCTION (this << psdu << txVector << ppduDuration << band << uid);
  NS_ASSERT (!IsMu ());
  SetPhyHeaders (txVector, ppduDuration);
}

HePpdu::~HePpdu ()
{
}

void
HePpdu::SetPhyHeaders (WifiTxVector txVector, Time ppduDuration)
{
  NS_LOG_FUNCTION (this << txVector << ppduDuration);
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
  if (IsDlMu ())
    {
      m_heSig.SetMuFlag (true);
    }
  else if (!IsUlMu ())
    {
      m_heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
      m_heSig.SetNStreams (txVector.GetNss ());
    }
  m_heSig.SetBssColor (txVector.GetBssColor ());
  m_heSig.SetChannelWidth (m_channelWidth);
  m_heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2/*NLTF currently unused*/);
}

WifiTxVector
HePpdu::DoGetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  txVector.SetMode (HePhy::GetHeMcs (m_heSig.GetMcs ()));
  txVector.SetChannelWidth (m_heSig.GetChannelWidth ());
  txVector.SetNss (m_heSig.GetNStreams ());
  txVector.SetGuardInterval (m_heSig.GetGuardInterval ());
  txVector.SetBssColor (m_heSig.GetBssColor ());
  txVector.SetAggregation (m_psdus.size () > 1 || m_psdus.begin ()->second->IsAggregate ());
  for (auto const& muUserInfo : m_muUserInfos)
    {
      txVector.SetHeMuUserInfo (muUserInfo.first, muUserInfo.second);
    }
  return txVector;
}

Time
HePpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  Time tSymbol = NanoSeconds (12800 + txVector.GetGuardInterval ());
  Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
  uint8_t sigExtension = 0;
  if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
      sigExtension = 6;
    }
  uint8_t m = IsDlMu () ? 1 : 2;
  //Equation 27-11 of IEEE P802.11ax/D4.0
  Time calculatedDuration = MicroSeconds (((ceil (static_cast<double> (m_lSig.GetLength () + 3 + m) / 3)) * 4) + 20 + sigExtension);
  uint32_t nSymbols = floor (static_cast<double> ((calculatedDuration - preambleDuration).GetNanoSeconds () - (sigExtension * 1000)) / tSymbol.GetNanoSeconds ());
  ppduDuration = preambleDuration + (nSymbols * tSymbol) + MicroSeconds (sigExtension);
  return ppduDuration;
}

Ptr<WifiPpdu>
HePpdu::Copy (void) const
{
  return Create<HePpdu> (m_psdus, GetTxVector (), GetTxDuration (), m_band, m_uid);
}

WifiPpduType
HePpdu::GetType (void) const
{
  switch (m_preamble)
    {
      case WIFI_PREAMBLE_HE_MU:
        return WIFI_PPDU_TYPE_DL_MU;
      case WIFI_PREAMBLE_HE_TB:
        return WIFI_PPDU_TYPE_UL_MU;
      default:
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
HePpdu::IsMu (void) const
{
  return (IsDlMu () || IsUlMu ());
}

bool
HePpdu::IsDlMu (void) const
{
  return (m_preamble == WIFI_PREAMBLE_HE_MU);
}

bool
HePpdu::IsUlMu (void) const
{
  return (m_preamble == WIFI_PREAMBLE_HE_TB);
}

Ptr<const WifiPsdu>
HePpdu::GetPsdu (uint8_t bssColor, uint16_t staId /* = SU_STA_ID */) const
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
HePpdu::GetStaId (void) const
{
  NS_ASSERT (IsUlMu ());
  return m_psdus.begin ()->first;
}


std::string
HePpdu::PrintPayload (void) const
{
  std::ostringstream ss;
  if (IsMu ())
    {
      ss << m_psdus;
    }
  else
    {
      ss << "PSDU=" << m_psdus.at (SU_STA_ID);
    }
  return ss.str ();
}

} //namespace ns3
