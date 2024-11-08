/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef ARF_WIFI_MANAGER_H
#define ARF_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "ns3/wifi-remote-station-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 * @brief ARF Rate control algorithm
 *
 * This class implements the so-called ARF algorithm which was
 * initially described in <i>WaveLAN-II: A High-performance wireless
 * LAN for the unlicensed band</i>, by A. Kamerman and L. Monteban. in
 * Bell Lab Technical Journal, pages 118-133, Summer 1997.
 *
 * This implementation differs from the initial description in that it
 * uses a packet-based timer rather than a time-based timer as described
 * in XXX (I cannot find back the original paper which described how
 * the time-based timer could be easily replaced with a packet-based
 * timer.)
 *
 * This RAA does not support HT modes and will error
 * exit if the user tries to configure this RAA with a Wi-Fi MAC
 * that supports 802.11n or higher.
 */
class ArfWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ArfWifiManager();
    ~ArfWifiManager() override;

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

    uint32_t m_timerThreshold;   ///< timer threshold
    uint32_t m_successThreshold; ///< success threshold

    TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} // namespace ns3

#endif /* ARF_WIFI_MANAGER_H */
