/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "error-rate-model.h"
#include "dsss-error-rate-model.h"
#include "wifi-tx-vector.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ErrorRateModel);

TypeId ErrorRateModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ErrorRateModel")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

double
ErrorRateModel::CalculateSnr (WifiTxVector txVector, double ber) const
{
  //This is a very simple binary search.
  double low, high, precision;
  low = 1e-25;
  high = 1e25;
  precision = 2e-12;
  while (high - low > precision)
    {
      NS_ASSERT (high >= low);
      double middle = low + (high - low) / 2;
      if ((1 - GetChunkSuccessRate (txVector.GetMode (), txVector, middle, 1)) > ber)
        {
          low = middle;
        }
      else
        {
          high = middle;
        }
    }
  return low;
}

double
ErrorRateModel::GetChunkSuccessRate (WifiMode mode, WifiTxVector txVector, double snr, uint64_t nbits) const
{
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS || mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      switch (mode.GetDataRate (22, 0, 1))
        {
          case 1000000:
            return DsssErrorRateModel::GetDsssDbpskSuccessRate (snr, nbits);
          case 2000000:
            return DsssErrorRateModel::GetDsssDqpskSuccessRate (snr, nbits);
          case 5500000:
            return DsssErrorRateModel::GetDsssDqpskCck5_5SuccessRate (snr, nbits);
          case 11000000:
            return DsssErrorRateModel::GetDsssDqpskCck11SuccessRate (snr, nbits);
          default:
            NS_ASSERT ("undefined DSSS/HR-DSSS datarate");
        }
    }
  else
    {
      return DoGetChunkSuccessRate (mode, txVector, snr, nbits);
    }
  return 0;
}

} //namespace ns3
