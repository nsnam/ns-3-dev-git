/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef CONSTANT_RATE_WIFI_MANAGER_H
#define CONSTANT_RATE_WIFI_MANAGER_H

#include "ns3/wifi-remote-station-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 * @brief use constant rates for data and RTS transmissions
 *
 * This class uses always the same transmission rate for every
 * packet sent.
 */
class ConstantRateWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ConstantRateWifiManager();
    ~ConstantRateWifiManager() override;

  private:
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

    WifiMode m_dataMode; //!< Wifi mode for unicast Data frames
    WifiMode m_ctlMode;  //!< Wifi mode for RTS frames
};

} // namespace ns3

#endif /* CONSTANT_RATE_WIFI_MANAGER_H */
