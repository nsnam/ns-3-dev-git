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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef INTERFERENCE_HELPER_H
#define INTERFERENCE_HELPER_H

#include "ns3/nstime.h"
#include "wifi-tx-vector.h"
#include <map>

namespace ns3 {

class WifiPpdu;
class WifiPsdu;
class ErrorRateModel;

/**
 * \ingroup wifi
 * \brief handles interference calculations
 * \brief signal event for a PPDU.
 */
class Event : public SimpleRefCount<Event>
{
public:
  /**
   * Create an Event with the given parameters.
   *
   * \param ppdu the PPDU
   * \param txVector the TXVECTOR
   * \param duration duration of the PPDU
   * \param rxPower the received power (w)
   */
  Event (Ptr<const WifiPpdu> ppdu, WifiTxVector txVector, Time duration, double rxPower);
  ~Event ();

  /**
   * Return the PSDU in the PPDU.
   *
   * \return the PSDU in the PPDU
   */
  Ptr<const WifiPsdu> GetPsdu (void) const;
  /**
   * Return the PPDU.
   *
   * \return the PPDU
   */
  Ptr<const WifiPpdu> GetPpdu (void) const;
  /**
   * Return the start time of the signal.
   *
   * \return the start time of the signal
   */
  Time GetStartTime (void) const;
  /**
   * Return the end time of the signal.
   *
   * \return the end time of the signal
   */
  Time GetEndTime (void) const;
  /**
   * Return the duration of the signal.
   *
   * \return the duration of the signal
   */
  Time GetDuration (void) const;
  /**
   * Return the received power (w).
   *
   * \return the received power (w)
   */
  double GetRxPowerW (void) const;
  /**
   * Return the TXVECTOR of the PPDU.
   *
   * \return the TXVECTOR of the PPDU
   */
  WifiTxVector GetTxVector (void) const;


private:
  Ptr<const WifiPpdu> m_ppdu; ///< PPDU
  WifiTxVector m_txVector; ///< TXVECTOR
  Time m_startTime; ///< start time
  Time m_endTime; ///< end time
  double m_rxPowerW; ///< received power in watts
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param event the event
 * \returns a reference to the stream
 */
std::ostream& operator<< (std::ostream& os, const Event &event);


/**
 * \ingroup wifi
 * \brief handles interference calculations
 */
class InterferenceHelper
{
public:
  /**
   * Signal event for a PPDU.
   */

  /**
   * A struct for both SNR and PER
   */
  struct SnrPer
  {
    double snr; ///< SNR in linear scale
    double per; ///< PER
  };

  InterferenceHelper ();
  ~InterferenceHelper ();

  /**
   * Set the noise figure.
   *
   * \param value noise figure in linear scale
   */
  void SetNoiseFigure (double value);
  /**
   * Set the error rate model for this interference helper.
   *
   * \param rate Error rate model
   */
  void SetErrorRateModel (const Ptr<ErrorRateModel> rate);

  /**
   * Return the error rate model.
   *
   * \return Error rate model
   */
  Ptr<ErrorRateModel> GetErrorRateModel (void) const;
  /**
   * Set the number of RX antennas in the receiver corresponding to this
   * interference helper.
   *
   * \param rx the number of RX antennas
   */
  void SetNumberOfReceiveAntennas (uint8_t rx);

  /**
   * \param energyW the minimum energy (W) requested
   *
   * \returns the expected amount of time the observed
   *          energy on the medium will be higher than
   *          the requested threshold.
   */
  Time GetEnergyDuration (double energyW) const;

  /**
   * Add the PPDU-related signal to interference helper.
   *
   * \param ppdu the PPDU
   * \param txVector the TXVECTOR
   * \param rxPower received power (W)
   *
   * \return Event
   */
  Ptr<Event> Add (Ptr<const WifiPpdu> ppdu, WifiTxVector txVector, Time duration, double rxPower);

  /**
   * Add a non-Wifi signal to interference helper.
   * \param duration the duration of the signal
   * \param rxPower received power (W)
   */
  void AddForeignSignal (Time duration, double rxPower);
  /**
   * Calculate the SNIR at the start of the payload and accumulate
   * all SNIR changes in the SNIR vector for each MPDU of an A-MPDU.
   * This workaround is required in order to provide one PER per MPDU, for
   * reception success/failure evaluation, while hiding aggregation details from
   * this class.
   *
   * \param event the event corresponding to the first time the corresponding PPDU arrives
   * \param relativeMpduStartStop the time window (pair of start and end times) of PHY payload to focus on
   *
   * \return struct of SNR and PER (with PER being evaluated over the provided time window)
   */
  struct InterferenceHelper::SnrPer CalculatePayloadSnrPer (Ptr<Event> event, std::pair<Time, Time> relativeMpduStartStop) const;
  /**
   * Calculate the SNIR for the event (starting from now until the event end).
   *
   * \param event the event corresponding to the first time the corresponding PPDU arrives
   *
   * \return the SNR for the PPDU in linear scale
   */
  double CalculateSnr (Ptr<Event> event) const;
  /**
   * Calculate the SNIR at the start of the non-HT PHY header and accumulate
   * all SNIR changes in the SNIR vector.
   *
   * \param event the event corresponding to the first time the corresponding PPDU arrives
   *
   * \return struct of SNR and PER
   */
  struct InterferenceHelper::SnrPer CalculateNonHtPhyHeaderSnrPer (Ptr<Event> event) const;
  /**
   * Calculate the SNIR at the start of the HT PHY header and accumulate
   * all SNIR changes in the SNIR vector.
   *
   * \param event the event corresponding to the first time the corresponding PPDU arrives
   *
   * \return struct of SNR and PER
   */
  struct InterferenceHelper::SnrPer CalculateHtPhyHeaderSnrPer (Ptr<Event> event) const;

  /**
   * Notify that RX has started.
   */
  void NotifyRxStart ();
  /**
   * Notify that RX has ended.
   */
  void NotifyRxEnd ();
  /**
   * Erase all events.
   */
  void EraseEvents (void);


protected:
  /**
   * Calculate SNR (linear ratio) from the given signal power and noise+interference power.
   *
   * \param signal signal power, W
   * \param noiseInterference noise and interference power, W
   * \param txVector the TXVECTOR
   *
   * \return SNR in linear scale
   */
  double CalculateSnr (double signal, double noiseInterference, WifiTxVector txVector) const;
  /**
   * Calculate the success rate of the chunk given the SINR, duration, and Wi-Fi mode.
   * The duration and mode are used to calculate how many bits are present in the chunk.
   *
   * \param snir the SINR
   * \param duration the duration of the chunk
   * \param mode the WifiMode
   * \param txVector the TXVECTOR
   *
   * \return the success rate
   */
  double CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode, WifiTxVector txVector) const;


private:
  /**
   * Noise and Interference (thus Ni) event.
   */
  class NiChange
  {
public:
    /**
     * Create a NiChange at the given time and the amount of NI change.
     *
     * \param power the power in watts
     * \param event causes this NI change
     */
    NiChange (double power, Ptr<Event> event);
    /**
     * Return the power
     *
     * \return the power in watts
     */
    double GetPower (void) const;
    /**
     * Add a given amount of power.
     *
     * \param power the power to be added to the existing value in watts
     */
    void AddPower (double power);
    /**
     * Return the event causes the corresponding NI change
     *
     * \return the event
     */
    Ptr<Event> GetEvent (void) const;


private:
    double m_power; ///< power in watts
    Ptr<Event> m_event; ///< event
  };

  /**
   * typedef for a multimap of NiChanges
   */
  typedef std::multimap<Time, NiChange> NiChanges;

  /**
   * Append the given Event.
   *
   * \param event
   */
  void AppendEvent (Ptr<Event> event);
  /**
   * Calculate noise and interference power in W.
   *
   * \param event the event
   * \param ni the NiChanges
   *
   * \return noise and interference power
   */
  double CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const;
  /**
   * Calculate the success rate of the payload chunk given the SINR, duration, and Wi-Fi mode.
   * The duration and mode are used to calculate how many bits are present in the chunk.
   *
   * \param snir the SINR
   * \param duration the duration of the chunk
   * \param txVector the TXVECTOR
   *
   * \return the success rate
   */
  double CalculatePayloadChunkSuccessRate (double snir, Time duration, WifiTxVector txVector) const;
  /**
   * Calculate the error rate of the given PHY payload only in the provided time
   * window (thus enabling per MPDU PER information). The PHY payload can be divided into
   * multiple chunks (e.g. due to interference from other transmissions).
   *
   * \param event the event
   * \param ni the NiChanges
   * \param window time window (pair of start and end times) of PHY payload to focus on
   *
   * \return the error rate of the payload
   */
  double CalculatePayloadPer (Ptr<const Event> event, NiChanges *ni, std::pair<Time, Time> window) const;
  /**
   * Calculate the error rate of the non-HT PHY header. The non-HT PHY header
   * can be divided into multiple chunks (e.g. due to interference from other transmissions).
   *
   * \param event the event
   * \param ni the NiChanges
   *
   * \return the error rate of the non-HT PHY header
   */
  double CalculateNonHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
  /**
   * Calculate the error rate of the HT PHY header. TheHT PHY header
   * can be divided into multiple chunks (e.g. due to interference from other transmissions).
   *
   * \param event the event
   * \param ni the NiChanges
   *
   * \return the error rate of the HT PHY header
   */
  double CalculateHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;

  double m_noiseFigure; /**< noise figure (linear) */
  Ptr<ErrorRateModel> m_errorRateModel; ///< error rate model
  uint8_t m_numRxAntennas; /**< the number of RX antennas in the corresponding receiver */
  /// Experimental: needed for energy duration calculation
  NiChanges m_niChanges;
  double m_firstPower; ///< first power in watts
  bool m_rxing; ///< flag whether it is in receiving state

  /**
   * Returns an iterator to the first NiChange that is later than moment
   *
   * \param moment time to check from
   * \returns an iterator to the list of NiChanges
   */
  NiChanges::const_iterator GetNextPosition (Time moment) const;
  /**
   * Returns an iterator to the first NiChange that is later than moment
   *
   * \param moment time to check from
   * \returns an iterator to the list of NiChanges
   */
  //NiChanges::iterator GetNextPosition (Time moment);
  /**
   * Returns an iterator to the last NiChange that is before than moment
   *
   * \param moment time to check from
   * \returns an iterator to the list of NiChanges
   */
  NiChanges::const_iterator GetPreviousPosition (Time moment) const;

  /**
   * Add NiChange to the list at the appropriate position and
   * return the iterator of the new event.
   *
   * \param moment time to check from
   * \param change the NiChange to add
   * \returns the iterator of the new event
   */
  NiChanges::iterator AddNiChangeEvent (Time moment, NiChange change);
};

} //namespace ns3

#endif /* INTERFERENCE_HELPER_H */
