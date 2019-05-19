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
#include "wifi-ppdu.h"
#include "wifi-psdu.h"
#include "wifi-phy.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPpdu");

WifiPpdu::WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, uint16_t frequency)
  : m_txVector (txVector),
    m_psdu (psdu),
    m_txDuration (ppduDuration),
    m_truncatedTx (false)
{
  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      m_htSig.SetMcs (txVector.GetMode ().GetMcsValue ());
      m_htSig.SetChannelWidth (txVector.GetChannelWidth ());
      m_htSig.SetHtLength (psdu->GetSize ());
      m_htSig.SetAggregation (txVector.IsAggregation ());
      m_htSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
    }
  else if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      m_vhtSig.SetMuFlag (txVector.GetPreambleType () == WIFI_PREAMBLE_VHT_MU);
      m_vhtSig.SetChannelWidth (txVector.GetChannelWidth ());
      m_vhtSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
      uint32_t nSymbols = (static_cast<double> ((ppduDuration - WifiPhy::CalculatePlcpPreambleAndHeaderDuration (txVector)).GetNanoSeconds ()) / (3200 + txVector.GetGuardInterval ()));
      m_vhtSig.SetShortGuardIntervalDisambiguation ((nSymbols % 10) == 9);
      m_vhtSig.SetSuMcs (txVector.GetMode ().GetMcsValue ());
      m_vhtSig.SetNStreams (txVector.GetNss ());
    }
  else if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      m_heSig.SetMuFlag (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU);
      m_heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
      m_heSig.SetBssColor (txVector.GetBssColor ());
      m_heSig.SetChannelWidth (txVector.GetChannelWidth ());
      m_heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2/*NLTF currently unused*/);
      m_heSig.SetNStreams (txVector.GetNss ());
    }
  if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_DSSS) || (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS))
    {
      m_dsssSig.SetRate (txVector.GetMode ().GetDataRate (22));
      Time psduDuration = ppduDuration - WifiPhy::CalculatePlcpPreambleAndHeaderDuration (txVector);
      m_dsssSig.SetLength (psduDuration.GetMicroSeconds ());
    }
  else if ((txVector.GetMode ().GetModulationClass () != WIFI_MOD_CLASS_HT) || (txVector.GetPreambleType () != WIFI_PREAMBLE_HT_GF))
    {
      if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_OFDM) || (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM))
        {
          m_lSig.SetRate (txVector.GetMode ().GetDataRate (txVector), txVector.GetChannelWidth ());
          m_lSig.SetLength (psdu->GetSize ());
        }
      else //HT, VHT or HE
        {
          uint8_t sigExtension = 0;
          if (Is2_4Ghz (frequency))
            {
              sigExtension = 6;
            }
          uint8_t m = 0;
          if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_SU)
            {
              m = 2;
            }
          else if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
            {
              m = 1;
            }
          //Equation 27-11 of IEEE P802.11/D4.0
          uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3 - m);
          m_lSig.SetLength (length);
        }
    }
}

WifiPpdu::~WifiPpdu ()
{
}

WifiTxVector
WifiPpdu::GetTxVector (void) const
{
  return m_txVector;
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
  return m_txDuration;
}

void
WifiPpdu::Print (std::ostream& os) const
{
  os << "txVector=" << m_txVector
     << ", duration=" << m_txDuration.GetMicroSeconds () << "us"
     << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N")
     << ", PSDU=" << m_psdu;
}

std::ostream & operator << (std::ostream &os, const WifiPpdu &ppdu)
{
  ppdu.Print (os);
  return os;
}

} //namespace ns3
