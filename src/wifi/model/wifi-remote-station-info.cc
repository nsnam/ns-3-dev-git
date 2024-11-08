/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "wifi-remote-station-info.h"

#include "ns3/simulator.h"

namespace ns3
{

WifiRemoteStationInfo::WifiRemoteStationInfo()
    : m_memoryTime(Seconds(1)),
      m_lastUpdate(),
      m_failAvg(0.0)
{
}

WifiRemoteStationInfo::~WifiRemoteStationInfo()
{
}

double
WifiRemoteStationInfo::CalculateAveragingCoefficient()
{
    double retval = std::exp(((m_lastUpdate - Now()) / m_memoryTime).GetDouble());
    m_lastUpdate = Simulator::Now();
    return retval;
}

void
WifiRemoteStationInfo::NotifyTxSuccess(uint32_t retryCounter)
{
    double coefficient = CalculateAveragingCoefficient();
    m_failAvg = static_cast<double>(retryCounter) / (1 + retryCounter) * (1 - coefficient) +
                coefficient * m_failAvg;
}

void
WifiRemoteStationInfo::NotifyTxFailed()
{
    double coefficient = CalculateAveragingCoefficient();
    m_failAvg = (1 - coefficient) + coefficient * m_failAvg;
}

double
WifiRemoteStationInfo::GetFrameErrorRate() const
{
    return m_failAvg;
}

} // namespace ns3
