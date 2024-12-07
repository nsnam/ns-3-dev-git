/*
 * Copyright (c) 2003,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "onoe-wifi-manager.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-tx-vector.h"

#define Min(a, b) ((a < b) ? a : b)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OnoeWifiManager");

/**
 * @brief hold per-remote-station state for ONOE Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the ONOE Wifi manager
 */
struct OnoeWifiRemoteStation : public WifiRemoteStation
{
    Time m_nextModeUpdate; ///< next mode update
    bool m_rateBlocked;    ///< whether the rate cannot be changed
    uint32_t m_shortRetry; ///< short retry
    uint32_t m_longRetry;  ///< long retry
    uint32_t m_tx_ok;      ///< transmit OK
    uint32_t m_tx_err;     ///< transmit error
    uint32_t m_tx_retr;    ///< transmit retry
    uint32_t m_tx_upper;   ///< transmit upper
    uint8_t m_txrate;      ///< transmit rate
};

NS_OBJECT_ENSURE_REGISTERED(OnoeWifiManager);

TypeId
OnoeWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OnoeWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<OnoeWifiManager>()
            .AddAttribute("UpdatePeriod",
                          "The interval between decisions about rate control changes",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&OnoeWifiManager::m_updatePeriod),
                          MakeTimeChecker())
            .AddAttribute("RaiseThreshold",
                          "Attempt to raise the rate if we hit that threshold",
                          UintegerValue(10),
                          MakeUintegerAccessor(&OnoeWifiManager::m_raiseThreshold),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("AddCreditThreshold",
                          "Add credit threshold",
                          UintegerValue(10),
                          MakeUintegerAccessor(&OnoeWifiManager::m_addCreditThreshold),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("Rate",
                            "Traced value for rate changes (b/s)",
                            MakeTraceSourceAccessor(&OnoeWifiManager::m_currentRate),
                            "ns3::TracedValueCallback::Uint64");
    return tid;
}

OnoeWifiManager::OnoeWifiManager()
    : WifiRemoteStationManager(),
      m_currentRate(0)
{
    NS_LOG_FUNCTION(this);
}

OnoeWifiManager::~OnoeWifiManager()
{
    NS_LOG_FUNCTION(this);
}

void
OnoeWifiManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    if (GetHtSupported())
    {
        NS_FATAL_ERROR("WifiRemoteStationManager selected does not support HT rates");
    }
    if (GetVhtSupported())
    {
        NS_FATAL_ERROR("WifiRemoteStationManager selected does not support VHT rates");
    }
    if (GetHeSupported())
    {
        NS_FATAL_ERROR("WifiRemoteStationManager selected does not support HE rates");
    }
}

WifiRemoteStation*
OnoeWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    auto station = new OnoeWifiRemoteStation();
    station->m_nextModeUpdate = Simulator::Now() + m_updatePeriod;
    station->m_rateBlocked = false;
    station->m_shortRetry = 0;
    station->m_longRetry = 0;
    station->m_tx_ok = 0;
    station->m_tx_err = 0;
    station->m_tx_retr = 0;
    station->m_tx_upper = 0;
    station->m_txrate = 0;
    return station;
}

void
OnoeWifiManager::DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
}

void
OnoeWifiManager::DoReportRtsFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    station->m_shortRetry++;
    station->m_rateBlocked = true; // do not change rate for retransmission
}

void
OnoeWifiManager::DoReportDataFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    station->m_longRetry++;
    station->m_rateBlocked = true; // do not change rate for retransmission
}

void
OnoeWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                               double ctsSnr,
                               WifiMode ctsMode,
                               double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode << rtsSnr);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    station->m_rateBlocked = true; // do not change rate
}

void
OnoeWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                double ackSnr,
                                WifiMode ackMode,
                                double dataSnr,
                                MHz_u dataChannelWidth,
                                uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    UpdateRetry(station);
    station->m_tx_ok++;
    station->m_rateBlocked = false; // we can change the rate for next packet
}

void
OnoeWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    UpdateRetry(station);
    station->m_tx_err++;
    station->m_rateBlocked = false; // we can change the rate for next packet
}

void
OnoeWifiManager::DoReportFinalDataFailed(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    UpdateRetry(station);
    station->m_tx_err++;
    station->m_rateBlocked = false; // we can change the rate for next packet
}

void
OnoeWifiManager::UpdateRetry(OnoeWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    station->m_tx_retr += station->m_shortRetry + station->m_longRetry;
    station->m_shortRetry = 0;
    station->m_longRetry = 0;
}

void
OnoeWifiManager::UpdateMode(OnoeWifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
    if (Simulator::Now() < station->m_nextModeUpdate || station->m_rateBlocked)
    {
        return;
    }
    station->m_nextModeUpdate = Simulator::Now() + m_updatePeriod;
    /**
     * The following 20 lines of code were copied from the Onoe
     * rate control kernel module used in the madwifi driver.
     */

    int dir = 0;
    uint8_t nrate;
    bool enough = (station->m_tx_ok + station->m_tx_err >= 10);

    /* no packet reached -> down */
    if (station->m_tx_err > 0 && station->m_tx_ok == 0)
    {
        dir = -1;
    }

    /* all packets needs retry in average -> down */
    if (enough && station->m_tx_ok < station->m_tx_retr)
    {
        dir = -1;
    }

    /* no error and less than rate_raise% of packets need retry -> up */
    if (enough && station->m_tx_err == 0 &&
        station->m_tx_retr < (station->m_tx_ok * m_addCreditThreshold) / 100)
    {
        dir = 1;
    }

    NS_LOG_DEBUG(this << " ok " << station->m_tx_ok << " err " << station->m_tx_err << " retr "
                      << station->m_tx_retr << " upper " << station->m_tx_upper << " dir " << dir);

    nrate = station->m_txrate;
    switch (dir)
    {
    case 0:
        if (enough && station->m_tx_upper > 0)
        {
            station->m_tx_upper--;
        }
        break;
    case -1:
        if (nrate > 0)
        {
            nrate--;
        }
        station->m_tx_upper = 0;
        break;
    case 1:
        /* raise rate if we hit rate_raise_threshold */
        if (++station->m_tx_upper < m_raiseThreshold)
        {
            break;
        }
        station->m_tx_upper = 0;
        if (nrate + 1 < GetNSupported(station))
        {
            nrate++;
        }
        break;
    }

    if (nrate != station->m_txrate)
    {
        NS_ASSERT(nrate < GetNSupported(station));
        station->m_txrate = nrate;
        station->m_tx_ok = station->m_tx_err = station->m_tx_retr = station->m_tx_upper = 0;
    }
    else if (enough)
    {
        station->m_tx_ok = station->m_tx_err = station->m_tx_retr = 0;
    }
}

WifiTxVector
OnoeWifiManager::DoGetDataTxVector(WifiRemoteStation* st, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << st << allowedWidth);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    UpdateMode(station);
    NS_ASSERT(station->m_txrate < GetNSupported(station));
    uint8_t rateIndex;
    if (station->m_longRetry < 4)
    {
        rateIndex = station->m_txrate;
    }
    else if (station->m_longRetry < 6)
    {
        if (station->m_txrate > 0)
        {
            rateIndex = station->m_txrate - 1;
        }
        else
        {
            rateIndex = station->m_txrate;
        }
    }
    else if (station->m_longRetry < 8)
    {
        if (station->m_txrate > 1)
        {
            rateIndex = station->m_txrate - 2;
        }
        else
        {
            rateIndex = station->m_txrate;
        }
    }
    else
    {
        if (station->m_txrate > 2)
        {
            rateIndex = station->m_txrate - 3;
        }
        else
        {
            rateIndex = station->m_txrate;
        }
    }
    auto channelWidth = GetChannelWidth(station);
    if (channelWidth > MHz_u{20} && channelWidth != MHz_u{22})
    {
        channelWidth = MHz_u{20};
    }
    WifiMode mode = GetSupported(station, rateIndex);
    uint64_t rate = mode.GetDataRate(channelWidth);
    if (m_currentRate != rate)
    {
        NS_LOG_DEBUG("New datarate: " << rate);
        m_currentRate = rate;
    }
    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        NanoSeconds(800),
        1,
        1,
        0,
        channelWidth,
        GetAggregation(station));
}

WifiTxVector
OnoeWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    auto station = static_cast<OnoeWifiRemoteStation*>(st);
    auto channelWidth = GetChannelWidth(station);
    if (channelWidth > MHz_u{20} && channelWidth != MHz_u{22})
    {
        channelWidth = MHz_u{20};
    }
    UpdateMode(station);
    WifiMode mode;
    if (!GetUseNonErpProtection())
    {
        mode = GetSupported(station, 0);
    }
    else
    {
        mode = GetNonErpSupported(station, 0);
    }
    return WifiTxVector(
        mode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(mode.GetModulationClass(), GetShortPreambleEnabled()),
        NanoSeconds(800),
        1,
        1,
        0,
        channelWidth,
        GetAggregation(station));
}

} // namespace ns3
