/*
 * Copyright (c) 2014 Universidad de la República - Uruguay
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Matias Richart <mrichart@fing.edu.uy>
 */

#ifndef PARF_WIFI_MANAGER_H
#define PARF_WIFI_MANAGER_H

#include "ns3/wifi-remote-station-manager.h"

namespace ns3
{

struct ParfWifiRemoteStation;

/**
 * @ingroup wifi
 * PARF Rate control algorithm
 *
 * This class implements the PARF algorithm as described in
 * <i>Self-management in chaotic wireless deployments</i>, by
 * Akella, A.; Judd, G.; Seshan, S. and Steenkiste, P. in
 * Wireless Networks, Kluwer Academic Publishers, 2007, 13, 737-755
 * https://web.archive.org/web/20210413094117/https://www.cs.odu.edu/~nadeem/classes/cs795-WNS-S13/papers/enter-006.pdf
 *
 * This RAA does not support HT modes and will error
 * exit if the user tries to configure this RAA with a Wi-Fi MAC
 * that supports 802.11n or higher.
 */
class ParfWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    ParfWifiManager();
    ~ParfWifiManager() override;

    void SetupPhy(const Ptr<WifiPhy> phy) override;

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

    /** Check for initializations.
     *
     * @param station The remote station.
     */
    void CheckInit(ParfWifiRemoteStation* station);

    uint32_t m_attemptThreshold; //!< The minimum number of transmission attempts to try a new power
                                 //!< or rate. The 'timer' threshold in the ARF algorithm.
    uint32_t m_successThreshold; //!< The minimum number of successful transmissions to try a new
                                 //!< power or rate.

    /**
     * Minimal power level.
     * In contrast to rate, power levels do not depend on the remote station.
     * The levels depend only on the physical layer of the device.
     */
    uint8_t m_minPower;

    /**
     * Maximal power level.
     */
    uint8_t m_maxPower;

    /**
     * The trace source fired when the transmission power changes.
     */
    TracedCallback<double, double, Mac48Address> m_powerChange;
    /**
     * The trace source fired when the transmission rate changes.
     */
    TracedCallback<DataRate, DataRate, Mac48Address> m_rateChange;
};

} // namespace ns3

#endif /* PARF_WIFI_MANAGER_H */
