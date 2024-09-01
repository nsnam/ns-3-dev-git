/*
 * Copyright (c) 2004,2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "constant-rate-wifi-manager.h"

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-utils.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ConstantRateWifiManager");

NS_OBJECT_ENSURE_REGISTERED(ConstantRateWifiManager);

TypeId
ConstantRateWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ConstantRateWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<ConstantRateWifiManager>()
            .AddAttribute("DataMode",
                          "The transmission mode to use for every data packet transmission",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&ConstantRateWifiManager::m_dataMode),
                          MakeWifiModeChecker())
            .AddAttribute("ControlMode",
                          "The transmission mode to use for every RTS packet transmission.",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&ConstantRateWifiManager::m_ctlMode),
                          MakeWifiModeChecker());
    return tid;
}

ConstantRateWifiManager::ConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);
}

ConstantRateWifiManager::~ConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);
}

WifiRemoteStation*
ConstantRateWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    auto station = new WifiRemoteStation();
    return station;
}

void
ConstantRateWifiManager::DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
}

void
ConstantRateWifiManager::DoReportRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
ConstantRateWifiManager::DoReportDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
ConstantRateWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                       double ctsSnr,
                                       WifiMode ctsMode,
                                       double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode << rtsSnr);
}

void
ConstantRateWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                        double ackSnr,
                                        WifiMode ackMode,
                                        double dataSnr,
                                        MHz_u dataChannelWidth,
                                        uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
}

void
ConstantRateWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
ConstantRateWifiManager::DoReportFinalDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

WifiTxVector
ConstantRateWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st << allowedWidth);
    uint8_t nss = std::min(GetMaxNumberOfTransmitStreams(), GetNumberOfSupportedStreams(st));
    if (m_dataMode.GetModulationClass() == WIFI_MOD_CLASS_HT)
    {
        nss = 1 + (m_dataMode.GetMcsValue() / 8);
    }
    return WifiTxVector(
        m_dataMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_dataMode.GetModulationClass(), GetShortPreambleEnabled()),
        GetGuardIntervalForMode(m_dataMode,
                                GetShortGuardIntervalSupported(st),
                                GetGuardInterval(st)),
        GetNumberOfAntennas(),
        nss,
        0,
        GetPhy()->GetTxBandwidth(m_dataMode, std::min(allowedWidth, GetChannelWidth(st))),
        GetAggregation(st));
}

WifiTxVector
ConstantRateWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    return WifiTxVector(
        m_ctlMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_ctlMode.GetModulationClass(), GetShortPreambleEnabled()),
        GetGuardIntervalForMode(m_ctlMode,
                                GetShortGuardIntervalSupported(st),
                                GetGuardInterval(st)),
        1,
        1,
        0,
        GetPhy()->GetTxBandwidth(m_ctlMode, GetChannelWidth(st)),
        GetAggregation(st));
}

} // namespace ns3
