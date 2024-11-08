/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Federico Maguolo <maguolof@dei.unipd.it>
 */

#ifndef CARA_WIFI_MANAGER_H
#define CARA_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "ns3/wifi-remote-station-manager.h"

namespace ns3
{

/**
 * @brief implement the CARA rate control algorithm
 * @ingroup wifi
 *
 * Implement the CARA algorithm from:
 * J. Kim, S. Kim, S. Choi, and D. Qiao.
 * "CARA: Collision-Aware Rate Adaptation for IEEE 802.11 WLANs."
 *
 * Originally implemented by Federico Maguolo for a very early
 * prototype version of ns-3.
 *
 * This RAA does not support HT modes and will error
 * exit if the user tries to configure this RAA with a Wi-Fi MAC
 * that supports 802.11n or higher.
 */
class CaraWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    CaraWifiManager();
    ~CaraWifiManager() override;

  private:
    void DoInitialize() override;
    WifiRemoteStation* DoCreateStation() const override;
    void DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode) override;
    void DoReportRtsFailed(WifiRemoteStation* station) override;
    void DoReportDataFailed(WifiRemoteStation* station) override;
    void DoReportRtsOk(WifiRemoteStation* station,
                       double ctsSnr,
                       WifiMode ctsMode,
                       double rtsSnr) override;
    void DoReportDataOk(WifiRemoteStation* station,
                        double ackSnr,
                        WifiMode ackMode,
                        double dataSnr,
                        MHz_u dataChannelWidth,
                        uint8_t dataNss) override;
    void DoReportFinalRtsFailed(WifiRemoteStation* station) override;
    void DoReportFinalDataFailed(WifiRemoteStation* station) override;
    WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, MHz_u allowedWidth) override;
    WifiTxVector DoGetRtsTxVector(WifiRemoteStation* station) override;
    bool DoNeedRts(WifiRemoteStation* station, uint32_t size, bool normally) override;

    uint32_t m_timerTimeout;     ///< timer threshold
    uint32_t m_successThreshold; ///< success threshold
    uint32_t m_failureThreshold; ///< failure threshold
    uint32_t m_probeThreshold;   ///< probe threshold

    TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} // namespace ns3

#endif /* CARA_WIFI_MANAGER_H */
