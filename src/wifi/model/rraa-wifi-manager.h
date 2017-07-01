/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Federico Maguolo <maguolof@dei.unipd.it>
 */

#ifndef RRAA_WIFI_MANAGER_H
#define RRAA_WIFI_MANAGER_H

#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "wifi-remote-station-manager.h"

namespace ns3 {

struct RraaWifiRemoteStation;

/// WifiRraaThresholds structure
struct WifiRraaThresholds
  {
    double m_ori; ///< Opportunistic Rate Increase threshold
    double m_mtl; ///< Maximum Tolerable Loss threshold
    uint32_t m_ewnd; ///< Evaluation Window
  };

/**
 * List of thresholds for each mode.
 */
typedef std::vector<std::pair<WifiRraaThresholds, WifiMode> > RraaThresholdsTable;

/**
 * \brief Robust Rate Adaptation Algorithm
 * \ingroup wifi
 *
 * This is an implementation of RRAA as described in
 * "Robust rate adaptation for 802.11 wireless networks"
 * by "Starsky H. Y. Wong", "Hao Yang", "Songwu Lu", and,
 * "Vaduvur Bharghavan" published in Mobicom 06.
 *
 * This RAA does not support HT, VHT nor HE modes and will error
 * exit if the user tries to configure this RAA with a Wi-Fi MAC
 * that has VhtSupported, HtSupported or HeSupported set.
 */
class RraaWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RraaWifiManager ();
  virtual ~RraaWifiManager ();

  // Inherited from WifiRemoteStationManager
  virtual void SetupPhy (const Ptr<WifiPhy> phy);
  virtual void SetupMac (const Ptr<WifiMac> mac);
  void SetHtSupported (bool enable);
  void SetVhtSupported (bool enable);
  void SetHeSupported (bool enable);


private:
  //overriden from base class
  WifiRemoteStation * DoCreateStation (void) const;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode);
  void DoReportRtsFailed (WifiRemoteStation *station);
  void DoReportDataFailed (WifiRemoteStation *station);
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr);
  void DoReportDataOk (WifiRemoteStation *station,
                       double ackSnr, WifiMode ackMode, double dataSnr);
  void DoReportFinalRtsFailed (WifiRemoteStation *station);
  void DoReportFinalDataFailed (WifiRemoteStation *station);
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station);
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  bool DoNeedRts (WifiRemoteStation *st,
                  Ptr<const Packet> packet, bool normally);
  bool IsLowLatency (void) const;

  /**
   * Check for initializations.
   * \param station The remote station.
   */
  void CheckInit (RraaWifiRemoteStation *station);
  /**
   * Return the index for the maximum transmission rate for
   * the given station.
   *
   * \param station
   *
   * \return the index for the maximum transmission rate
   */
  uint32_t GetMaxRate (RraaWifiRemoteStation *station);
  /**
   * Return the index for the minimum transmission rate for
   * the given station.
   *
   * \param station
   *
   * \return the index for the minimum transmission rate
   */
  uint32_t GetMinRate (RraaWifiRemoteStation *station);
  /**
   * Check if the counter should be resetted.
   *
   * \param station
   */
  void CheckTimeout (RraaWifiRemoteStation *station);
  /**
   * Find an appropriate rate for the given station, using
   * a basic algorithm.
   *
   * \param station
   */
  void RunBasicAlgorithm (RraaWifiRemoteStation *station);
  /**
   * Activate the use of RTS for the given station if the conditions are met.
   *
   * \param station
   */
  void ARts (RraaWifiRemoteStation *station);
  /**
   * Reset the counters of the given station.
   *
   * \param station
   */
  void ResetCountersBasic (RraaWifiRemoteStation *station);
  /**
   * Initialize the thresholds internal list for the given station.
   *
   * \param station
   */
  void InitThresholds (RraaWifiRemoteStation *station);
  /**
   * Get the thresholds for the given station and mode.
   *
   * \param station
   * \param mode
   *
   * \return threshold
   */
  WifiRraaThresholds GetThresholds (RraaWifiRemoteStation *station, WifiMode mode) const;
  /**
   * Get the thresholds for the given station and mode index.
   *
   * \param station
   * \param rate
   *
   * \return threshold
   */
  WifiRraaThresholds GetThresholds (RraaWifiRemoteStation *station, uint32_t rate) const;
  /**
   * Get the estimated TxTime of a packet with a given mode.
   *
   * \param mode
   *
   * \return time
   */
  Time GetCalcTxTime (WifiMode mode) const;
  /**
   * Add transmission time for the given mode to an internal list.
   *
   * \param mode Wi-Fi mode
   * \param t transmission time
   */
  void AddCalcTxTime (WifiMode mode, Time t);
  /**
   * typedef for a vector of a pair of Time, WifiMode.
   * Essentially a list for WifiMode and its corresponding transmission time
   * to transmit a reference packet.
   */
  typedef std::vector<std::pair<Time,WifiMode> > TxTime;

  TxTime m_calcTxTime;     //!< To hold all the calculated TxTime for all modes.
  Time m_sifs;             //!< Value of SIFS configured in the device.
  Time m_difs;             //!< Value of DIFS configured in the device.

  uint32_t m_frameLength;  //!< Data frame length used for calculate mode TxTime.
  uint32_t m_ackLength;    //!< Ack frame length used for calculate mode TxTime.

  bool m_basic;    ///< basic
  Time m_timeout;  ///< timeout
  double m_alpha;  //!< Alpha value for RRAA (value for calculating MTL threshold)
  double m_beta;   //!< Beta value for RRAA (value for calculating ORI threshold).
  double m_tau;    //!< Tau value for RRAA (value for calculating EWND size).

  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} //namespace ns3

#endif /* RRAA_WIFI_MANAGER_H */
