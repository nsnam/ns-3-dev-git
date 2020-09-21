/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 University of Washington
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
 * Authors: Rohan Patidar <rpatidar@uw.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include <cmath>
#include <algorithm>
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "table-based-error-rate-model.h"
#include "wifi-utils.h"
#include "wifi-tx-vector.h"
#include "dsss-error-rate-model.h"
#include "yans-error-rate-model.h"

static const double SNR_PRECISION = 2;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TableBasedErrorRateModel);

NS_LOG_COMPONENT_DEFINE ("TableBasedErrorRateModel");

TypeId
TableBasedErrorRateModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TableBasedErrorRateModel")
    .SetParent<ErrorRateModel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<TableBasedErrorRateModel> ()
    .AddAttribute ("FallbackErrorRateModel",
                   "Ptr to the fallback error rate model to be used when no matching value is found in a table",
                   PointerValue (CreateObject<YansErrorRateModel> ()),
                   MakePointerAccessor (&TableBasedErrorRateModel::m_fallbackErrorModel),
                   MakePointerChecker <ErrorRateModel> ())
    .AddAttribute ("SizeThreshold",
                   "Threshold in bytes over which the table for large size frames is used",
                   UintegerValue (400),
                   MakeUintegerAccessor (&TableBasedErrorRateModel::m_threshold),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

TableBasedErrorRateModel::TableBasedErrorRateModel ()
{
  NS_LOG_FUNCTION (this);
}

TableBasedErrorRateModel::~TableBasedErrorRateModel ()
{
  NS_LOG_FUNCTION (this);
  m_fallbackErrorModel = 0;
}


double
TableBasedErrorRateModel::RoundSnr (double snr, double precision) const
{
  NS_LOG_FUNCTION (this << snr);
  double multiplier = std::round (std::pow (10.0, precision));
  return std::floor (snr * multiplier + 0.5) / multiplier;
}

uint8_t
TableBasedErrorRateModel::GetMcsForMode (WifiMode mode)
{
  uint8_t mcs = 0xff; // Initialize to invalid mcs
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM || mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
    {
      if (mode.GetConstellationSize () == 2)
        {
          if (mode.GetCodeRate () == WIFI_CODE_RATE_1_2)
            {
              mcs = 0;
            }
          if (mode.GetCodeRate () == WIFI_CODE_RATE_3_4)
            {
              mcs = 1;
            }
        }
      else if (mode.GetConstellationSize () == 4)
        {
          if (mode.GetCodeRate () == WIFI_CODE_RATE_1_2)
            {
              mcs = 2;
            }
          else if (mode.GetCodeRate () == WIFI_CODE_RATE_3_4)
            {
              mcs = 3;
            }
        }
      else if (mode.GetConstellationSize () == 16)
        {
          if (mode.GetCodeRate () == WIFI_CODE_RATE_1_2)
            {
              mcs = 4;
            }
          else if (mode.GetCodeRate () == WIFI_CODE_RATE_3_4)
            {
              mcs = 5;
            }
        }
      else if (mode.GetConstellationSize () == 64)
        {
          if (mode.GetCodeRate () == WIFI_CODE_RATE_2_3)
            {
              mcs = 6;
            }
          else if (mode.GetCodeRate () == WIFI_CODE_RATE_3_4)
            {
              mcs = 7;
            }
        }
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT || mode.GetModulationClass () == WIFI_MOD_CLASS_VHT || mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      mcs = mode.GetMcsValue ();
    }
  NS_ABORT_MSG_IF (mcs == 0xff, "Error, MCS value for mode not found");
  return mcs;
}

double
TableBasedErrorRateModel::DoGetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const
{
  NS_LOG_FUNCTION (this << mode << txVector << snr << nbits);
  uint64_t size = std::max<uint64_t> (1, (nbits / 8));
  double roundedSnr = RoundSnr (RatioToDb (snr), SNR_PRECISION);
  uint8_t mcs = GetMcsForMode (mode);
  bool ldpc = txVector.IsLdpc ();
  NS_LOG_FUNCTION (this << +mcs << roundedSnr << size << ldpc);

  // HT: for mcs greater than 7, use 0 - 7 curves for data rate
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
    {
      mcs = mcs % 8;
    }

  if (mcs > (ldpc ? ERROR_TABLE_LDPC_MAX_NUM_MCS : ERROR_TABLE_BCC_MAX_NUM_MCS))
    {
      NS_LOG_WARN ("Table missing for MCS: " << +mcs << " in TableBasedErrorRateModel: use fallback error rate model");
      return m_fallbackErrorModel->GetChunkSuccessRate (mode, txVector, snr, nbits);
    }

  auto errorTable = (ldpc ? AwgnErrorTableLdpc1458 : (size < m_threshold ? AwgnErrorTableBcc32 : AwgnErrorTableBcc1458));
  auto itVector = errorTable[mcs];
  auto itTable = std::find_if (itVector.begin(), itVector.end(),
      [&roundedSnr](const std::pair<double, double>& element) {
          return element.first == roundedSnr;
      });
  double minSnr = itVector.begin ()->first;
  double maxSnr = (--itVector.end ())->first;
  double per;
  if (itTable == itVector.end ())
    {
      if (roundedSnr < minSnr)
        {
          per = 1.0;
        }
      else if (roundedSnr > maxSnr)
        {
          per = 0.0;
        }
      else
        {
          double a = 0.0, b = 0.0, previousSnr = 0.0, nextSnr = 0.0;
          for (auto i = itVector.begin (); i != itVector.end (); ++i)
            {
              if (i->first < roundedSnr)
                {
                  previousSnr = i->first;
                  a = i->second;
                }
              else
                {
                  nextSnr = i->first;
                  b = i->second;
                  break;
                }
            }
          per = a + (roundedSnr - previousSnr) * (b - a) / (nextSnr - previousSnr);
        }
    }
  else
    {
      per = itTable->second;
    }

  uint16_t tableSize = (ldpc ? ERROR_TABLE_LDPC_FRAME_SIZE : (size < m_threshold ? ERROR_TABLE_BCC_SMALL_FRAME_SIZE : ERROR_TABLE_BCC_LARGE_FRAME_SIZE));
  if (size != tableSize)
    {
      // From IEEE document 11-14/0803r1 (Packet Length for Box 0 Calibration)
      per = (1.0 - std::pow ((1 - per), (static_cast<double> (size) / tableSize)));
    }

  return 1.0 - per;
}

} //namespace ns3
