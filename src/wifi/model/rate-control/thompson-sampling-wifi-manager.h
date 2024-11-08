/*
 * Copyright (c) 2021 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alexander Krotov <krotov@iitp.ru>
 */

#ifndef THOMPSON_SAMPLING_WIFI_MANAGER_H
#define THOMPSON_SAMPLING_WIFI_MANAGER_H

#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"
#include "ns3/wifi-remote-station-manager.h"

namespace ns3
{

/**
 * @brief Thompson Sampling rate control algorithm
 * @ingroup wifi
 *
 * This class implements Thompson Sampling rate control algorithm.
 *
 * It was implemented for use as a baseline in
 * https://doi.org/10.1109/ACCESS.2020.3023552
 */
class ThompsonSamplingWifiManager : public WifiRemoteStationManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ThompsonSamplingWifiManager();
    ~ThompsonSamplingWifiManager() override;

    int64_t AssignStreams(int64_t stream) override;

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
    void DoReportAmpduTxStatus(WifiRemoteStation* station,
                               uint16_t nSuccessfulMpdus,
                               uint16_t nFailedMpdus,
                               double rxSnr,
                               double dataSnr,
                               MHz_u dataChannelWidth,
                               uint8_t dataNss) override;
    void DoReportFinalRtsFailed(WifiRemoteStation* station) override;
    void DoReportFinalDataFailed(WifiRemoteStation* station) override;
    WifiTxVector DoGetDataTxVector(WifiRemoteStation* station, MHz_u allowedWidth) override;
    WifiTxVector DoGetRtsTxVector(WifiRemoteStation* station) override;

    /**
     * Initializes station rate tables. If station is already initialized,
     * nothing is done.
     *
     * @param station Station which should be initialized.
     */
    void InitializeStation(WifiRemoteStation* station) const;

    /**
     * Draws a new MCS and related parameters to try next time for this
     * station.
     *
     * This method should only be called between TXOPs to avoid sending
     * multiple frames using different modes. Otherwise it is impossible
     * to tell which mode was used for succeeded/failed frame when
     * feedback is received.
     *
     * @param station Station for which a new mode should be drawn.
     */
    void UpdateNextMode(WifiRemoteStation* station) const;

    /**
     * Applies exponential decay to MCS statistics.
     *
     * @param st Remote STA for which MCS statistics is to be updated.
     * @param i MCS index.
     */
    void Decay(WifiRemoteStation* st, size_t i) const;

    /**
     * Returns guard interval for the given mode.
     *
     * @param st Remote STA
     * @param mode The WifiMode
     * @return the guard interval
     */
    Time GetModeGuardInterval(WifiRemoteStation* st, WifiMode mode) const;

    /**
     * Sample beta random variable with given parameters
     * @param alpha first parameter of beta distribution
     * @param beta second parameter of beta distribution
     * @return beta random variable sample
     */
    double SampleBetaVariable(uint64_t alpha, uint64_t beta) const;

    Ptr<GammaRandomVariable>
        m_gammaRandomVariable; //!< Variable used to sample beta-distributed random variables

    double m_decay; //!< Exponential decay coefficient, Hz

    TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} // namespace ns3

#endif /* THOMPSON_SAMPLING_WIFI_MANAGER_H */
