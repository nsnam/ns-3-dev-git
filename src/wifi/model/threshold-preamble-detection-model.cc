/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Washington
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/double.h"
#include "threshold-preamble-detection-model.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ThresholdPreambleDetectionModel");

NS_OBJECT_ENSURE_REGISTERED (ThresholdPreambleDetectionModel);

TypeId
ThresholdPreambleDetectionModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThresholdPreambleDetectionModel")
    .SetParent<PreambleDetectionModel> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ThresholdPreambleDetectionModel> ()
    .AddAttribute ("Threshold",
                   "Preamble is successfully detection if the SNR is at or above this value (expressed in dB).",
                   DoubleValue (2),
                   MakeDoubleAccessor (&ThresholdPreambleDetectionModel::m_threshold),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

ThresholdPreambleDetectionModel::ThresholdPreambleDetectionModel ()
{
  NS_LOG_FUNCTION (this);
}

ThresholdPreambleDetectionModel::~ThresholdPreambleDetectionModel ()
{
  NS_LOG_FUNCTION (this);
}

bool
ThresholdPreambleDetectionModel::IsPreambleDetected (double snr, double channelWidth) const
{
  NS_LOG_FUNCTION (this);
  return (RatioToDb (snr) >= m_threshold);
}

} //namespace ns3
