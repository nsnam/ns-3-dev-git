/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universidad de la República - Uruguay
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
 * Author: Matias Richart <mrichart@fing.edu.uy>
 */

#ifndef PARF_WIFI_MANAGER_H
#define PARF_WIFI_MANAGER_H

#include "wifi-remote-station-manager.h"

namespace ns3 {

struct ParfWifiRemoteStation;
/**
 * \ingroup wifi
 * PARF Rate control algorithm
 *
 * This class implements the PARF algorithm as described in
 * <i>Self-management in chaotic wireless deployments</i>, by
 * Akella, A.; Judd, G.; Seshan, S. and Steenkiste, P. in
 * Wireless Networks, Kluwer Academic Publishers, 2007, 13, 737-755
 * http://www.cs.odu.edu/~nadeem/classes/cs795-WNS-S13/papers/enter-006.pdf
 *
 * This RAA does not support HT, VHT nor HE modes and will error
 * exit if the user tries to configure this RAA with a Wi-Fi MAC
 * that has VhtSupported, HtSupported or HeSupported set.
 */
class ParfWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  ParfWifiManager ();
  virtual ~ParfWifiManager ();

  // Inherited from WifiRemoteStationManager
  void SetupPhy (const Ptr<WifiPhy> phy);
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
  bool IsLowLatency (void) const;

  /** Check for initializations.
   *
   * \param station The remote station.
   */
  void CheckInit (ParfWifiRemoteStation *station);

  uint32_t m_attemptThreshold; //!< The minimum number of transmission attempts to try a new power or rate. The 'timer' threshold in the ARF algorithm.
  uint32_t m_successThreshold; //!< The minimum number of successful transmissions to try a new power or rate.

  /**
   * Minimal power level.
   * In contrast to rate, power levels do not depend on the remote station.
   * The levels depend only on the physical layer of the device.
   */
  uint32_t m_minPower;

  /**
   * Maximal power level.
   */
  uint32_t m_maxPower;

  /**
   * The trace source fired when the transmission power changes.
   */
  TracedCallback<double, double, Mac48Address> m_powerChange;
  /**
   * The trace source fired when the transmission rate changes.
   */
  TracedCallback<DataRate, DataRate, Mac48Address> m_rateChange;
};

} //namespace ns3

#endif /* PARF_WIFI_MANAGER_H */
