/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Duy Nguyen
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
 * Authors: Duy Nguyen <duy@soe.ucsc.edu>
 *          Matías Richart <mrichart@fing.edu.uy>
 */

#ifndef MINSTREL_WIFI_MANAGER_H
#define MINSTREL_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "wifi-remote-station-manager.h"
#include "ns3/random-variable-stream.h"
#include <fstream>

namespace ns3 {

/**
 * A struct to contain all information related to a data rate
 */
struct RateInfo
{
  /**
   * Perfect transmission time calculation, or frame calculation
   * Given a bit rate and a packet length n bytes
   */
  Time perfectTxTime;

  uint32_t retryCount;          ///< retry limit
  uint32_t adjustedRetryCount;  ///< adjust the retry limit for this rate
  uint32_t numRateAttempt;      ///< how many number of attempts so far
  uint32_t numRateSuccess;      ///< number of successful pkts
  uint32_t prob;                ///< (# pkts success )/(# total pkts)
  /**
   * EWMA calculation
   * ewma_prob =[prob *(100 - ewma_level) + (ewma_prob_old * ewma_level)]/100
   */
  uint32_t ewmaProb;
  uint32_t throughput;  ///< throughput of a rate

  uint32_t prevNumRateAttempt;  //!< Number of transmission attempts with previous rate.
  uint32_t prevNumRateSuccess;  //!< Number of successful frames transmitted with previous rate.
  uint64_t successHist;         //!< Aggregate of all transmission successes.
  uint64_t attemptHist;         //!< Aggregate of all transmission attempts.

  uint8_t numSamplesSkipped; //!< number of samples skipped
  int sampleLimit; //!< sample limit
};

/**
 * Data structure for a Minstrel Rate table
 * A vector of a struct RateInfo
 */
typedef std::vector<RateInfo> MinstrelRate;
/**
 * Data structure for a Sample Rate table
 * A vector of a vector uint32_t
 */
typedef std::vector<std::vector<uint32_t> > SampleRate;

/**
 * \brief hold per-remote-station state for Minstrel Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Minstrel Wifi manager
 */
struct MinstrelWifiRemoteStation : public WifiRemoteStation
{
  Time m_nextStatsUpdate;  ///< 10 times every second

  /**
   * To keep track of the current position in the our random sample table
   * going row by row from 1st column until the 10th column(Minstrel defines 10)
   * then we wrap back to the row 1 col 1.
   * note: there are many other ways to do this.
   */
  uint32_t m_col, m_index;       ///< index
  uint32_t m_maxTpRate;          ///< the current throughput rate
  uint32_t m_maxTpRate2;         ///< second highest throughput rate
  uint32_t m_maxProbRate;        ///< rate with highest prob of success
  uint32_t m_nModes;             ///< number of modes supported
  int m_totalPacketsCount;       ///< total number of packets as of now
  int m_samplePacketsCount;      ///< how many packets we have sample so far
  int m_numSamplesDeferred;      ///< number samles deferred
  bool m_isSampling;             ///< a flag to indicate we are currently sampling
  uint32_t m_sampleRate;         ///< current sample rate
  bool  m_sampleDeferred;        ///< a flag to indicate sample rate is on the second stage
  uint32_t m_shortRetry;         ///< short retries such as control packts
  uint32_t m_longRetry;          ///< long retries such as data packets
  uint32_t m_retry;              ///< total retries short + long
  uint32_t m_txrate;             ///< current transmit rate
  bool m_initialized;            ///< for initializing tables
  MinstrelRate m_minstrelTable;  ///< minstrel table
  SampleRate m_sampleTable;      ///< sample table
  std::ofstream m_statsFile;     ///< stats file
};


/**
 * \brief Implementation of Minstrel Rate Control Algorithm
 * \ingroup wifi
 *
 * Minstrel is a rate control algorithm implemented in MadWifi and Linux.
 * The basic principle is to probe the environment and adapt the rate
 * based on statistics collected on the probability of successful
 * transmission.  The algorithm adapts the rate to the highest rate
 * that it considers successful, and spends a fraction of its time
 * doing 'look around' by trying other rates.
 *
 * Minstrel is appropriate for non-HT/VHT/HE configurations; for HT/VHT/HE
 * (i.e. 802.11n/ac/ax), users should use MinstrelHtWifiManager instead.
 * Minstrel will error exit if the user tries to configure it with a
 * Wi-Fi MAC that has VhtSupported, HtSupported or HeSupported set.
 *
 * Some notes on this implementation follow.  The implementation has
 * been adapted to bring it closer to the Linux implementation.
 * For each rate, a new parameter samplesSkipped is added. This parameter
 * is intended to solve an issue regarding the sampling of low rates when
 * a high rate is working well, which leads to outdated statistics.
 * This change makes throughput a bit lower in simple, stable scenarios,
 * but may help in dynamic scenarios to react faster and more accurately
 * to changes.
 *
 * Related to the previous, the logic for deciding when to sample random
 * rates is as follows.  When a sample rate is deffered to the second MRR
 * chain stage, a new parameter (numSamplesDeferred) is increased. This
 * paramters is used (jointly with sampleCount) to compare current
 * sample count with the lookaround rate.
 *
 * Also related with sampling, another parameter sampleLimit is added.
 * This parameter limits the number of times a very low or very high
 * probability rate is sampled, avoiding to try a poorly working sample
 * rate too often.
 *
 * When updating the EWMA probability of a rate for the first time, it does
 * not apply EWMA but instead assigns the entire probability.
 * Since the EWMA probability is initialized to zero, this generates
 * a more accurate EWMA.
 */
class MinstrelWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  MinstrelWifiManager ();
  virtual ~MinstrelWifiManager ();

  // Inherited from WifiRemoteStationManager
  void SetupPhy (const Ptr<WifiPhy> phy);
  void SetupMac (const Ptr<WifiMac> mac);
  void SetHtSupported (bool enable);
  void SetVhtSupported (bool enable);
  void SetHeSupported (bool enable);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   *
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  /**
   * updating the rate
   *
   * \param station the station object
   */
  void UpdateRate (MinstrelWifiRemoteStation *station);

  /**
   * updating the Minstrel Table every 1/10 seconds
   *
   * \param station the station object
   */
  void UpdateStats (MinstrelWifiRemoteStation *station);

  /**
   * find a rate to use from Minstrel Table
   *
   * \param station the station object
   * \returns the rate
   */
  uint32_t FindRate (MinstrelWifiRemoteStation *station);

  /**
   * Get data transmit vector
   *
   * \param station the station object
   * \returns WifiTxVector
   */
  WifiTxVector GetDataTxVector (MinstrelWifiRemoteStation *station);

  /**
   * Get RTS transmit vector
   *
   * \param station the station object
   * \returns WifiTxVector
   */
  WifiTxVector GetRtsTxVector (MinstrelWifiRemoteStation *station);

  /**
   * Count retries
   *
   * \param station the station object
   * \returns the number if retries
   */
  uint32_t CountRetries (MinstrelWifiRemoteStation *station);

  /**
   * Update packet counters
   *
   * \param station the station object
   */
  void UpdatePacketCounters (MinstrelWifiRemoteStation *station);

  /**
   * update the number of retries and reset accordingly
   *
   * \param station the station object
   */
  void UpdateRetry (MinstrelWifiRemoteStation *station);

  /**
   * check for initializations
   *
   * \param station the station object
   */
  void CheckInit (MinstrelWifiRemoteStation *station);

  /**
   * initialize Sample Table
   *
   * \param station the station object
   */
  void InitSampleTable (MinstrelWifiRemoteStation *station);

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

  bool DoNeedDataRetransmission (WifiRemoteStation *st,
                                 Ptr<const Packet> packet, bool normally);

  bool IsLowLatency (void) const;

  /// for estimating the TxTime of a packet with a given mode
  Time GetCalcTxTime (WifiMode mode) const;
  /**
   * Add transmission time for the given mode to an internal list.
   *
   * \param mode Wi-Fi mode
   * \param t transmission time
   */
  void AddCalcTxTime (WifiMode mode, Time t);

  /**
   * initialize Minstrel Table
   *
   * \param station the station object
   */
  void RateInit (MinstrelWifiRemoteStation *station);

  /**
   * getting the next sample from Sample Table
   *
   * \param station the station object
   * \returns the next sample
   */
  uint32_t GetNextSample (MinstrelWifiRemoteStation *station);

  /**
   * Estimate the time to transmit the given packet with the given number of retries.
   * This function is "roughly" the function "calc_usecs_unicast_packet" in minstrel.c
   * in the madwifi implementation.
   *
   * The basic idea is that, we try to estimate the "average" time used to transmit the
   * packet for the given number of retries while also accounting for the 802.11 congestion
   * window change. The original code in the madwifi seems to estimate the number of backoff
   * slots as the half of the current CW size.
   *
   * There are four main parts:
   *  - wait for DIFS (sense idle channel)
   *  - ACK timeouts
   *  - DATA transmission
   *  - backoffs according to CW
   *
   * \param dataTransmissionTime the data transmission time
   * \param shortRetries short retries
   * \param longRetries long retries
   * \returns the unicast packet time
   */
  Time CalculateTimeUnicastPacket (Time dataTransmissionTime, uint32_t shortRetries, uint32_t longRetries);

  /**
   * printing Sample Table
   *
   * \param station the station object
   */
  void PrintSampleTable (MinstrelWifiRemoteStation *station);

  /**
   * printing Minstrel Table
   *
   * \param station the station object
   */
  void PrintTable (MinstrelWifiRemoteStation *station);

  /**
   * typedef for a vector of a pair of Time, WifiMode.
   * (Essentially a list for WifiMode and its corresponding transmission time
   * to transmit a reference packet.
   */
  typedef std::vector<std::pair<Time,WifiMode> > TxTime;

  TxTime m_calcTxTime;      ///< to hold all the calculated TxTime for all modes
  Time m_updateStats;       ///< how frequent do we calculate the stats (1/10 seconds)
  double m_lookAroundRate;  ///< the % to try other rates than our current rate
  double m_ewmaLevel;       ///< exponential weighted moving average
  uint32_t m_sampleCol;     ///< number of sample columns
  uint32_t m_pktLen;        ///< packet length used for calculate mode TxTime
  bool m_printStats;        ///< If statistics table should be printed.

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;

  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} //namespace ns3

#endif /* MINSTREL_WIFI_MANAGER_H */
