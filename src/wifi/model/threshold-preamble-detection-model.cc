/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "threshold-preamble-detection-model.h"

#include "wifi-utils.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ThresholdPreambleDetectionModel");

NS_OBJECT_ENSURE_REGISTERED(ThresholdPreambleDetectionModel);

TypeId
ThresholdPreambleDetectionModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThresholdPreambleDetectionModel")
            .SetParent<PreambleDetectionModel>()
            .SetGroupName("Wifi")
            .AddConstructor<ThresholdPreambleDetectionModel>()
            .AddAttribute("Threshold",
                          "Preamble is successfully detected if the SNR is at or above this value "
                          "(expressed in dB).",
                          DoubleValue(4),
                          MakeDoubleAccessor(&ThresholdPreambleDetectionModel::m_threshold),
                          MakeDoubleChecker<dB_u>())
            .AddAttribute("MinimumRssi",
                          "Preamble is dropped if the RSSI is below this value (expressed in dBm).",
                          DoubleValue(-82),
                          MakeDoubleAccessor(&ThresholdPreambleDetectionModel::m_rssiMin),
                          MakeDoubleChecker<dBm_u>());
    return tid;
}

ThresholdPreambleDetectionModel::ThresholdPreambleDetectionModel()
{
    NS_LOG_FUNCTION(this);
}

ThresholdPreambleDetectionModel::~ThresholdPreambleDetectionModel()
{
    NS_LOG_FUNCTION(this);
}

bool
ThresholdPreambleDetectionModel::IsPreambleDetected(dBm_u rssi,
                                                    double snr,
                                                    MHz_u channelWidth) const
{
    NS_LOG_FUNCTION(this << rssi << RatioToDb(snr) << channelWidth);
    if (rssi >= m_rssiMin)
    {
        if (RatioToDb(snr) >= m_threshold)
        {
            return true;
        }
        else
        {
            NS_LOG_DEBUG("Received RSSI is above the target RSSI but SNR is too low");
            return false;
        }
    }
    else
    {
        NS_LOG_DEBUG("Received RSSI is below the target RSSI");
        return false;
    }
}

} // namespace ns3
