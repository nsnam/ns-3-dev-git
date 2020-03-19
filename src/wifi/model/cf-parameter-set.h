/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015
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
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef CF_PARAMETER_SET_H
#define CF_PARAMETER_SET_H

#include "ns3/wifi-information-element.h"

namespace ns3 {

/**
 * \brief The CF Parameter Set
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the CF Parameter Set.
 */
class CfParameterSet : public WifiInformationElement
{
public:
  CfParameterSet ();

  // Implementations of pure virtual methods of WifiInformationElement
  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
  /* This information element is a bit special in that it is only
     included if the STA is a QoS STA. To support this we
     override the Serialize and GetSerializedSize methods of
     WifiInformationElement. */
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

  /**
   * Set PCF supported function
   * \param pcfSupported the PCF supported indicator
   */
  void SetPcfSupported (uint8_t pcfSupported);

  /**
   * Return the CFP Count in DTIM frames unit.
   *
   * \return the CFP Count in DTIM frames unit
   */
  uint8_t GetCFPCount (void) const;
  /**
   * Return the CFP Period in DTIM frames unit.
   *
   * \return the CFP Period in DTIM frames unit
   */
  uint8_t GetCFPPeriod (void) const;
  /**
   * Return the maximum CFP duration in microseconds.
   *
   * \return the maximum CFP duration in microseconds.
   */
  uint64_t GetCFPMaxDurationUs (void) const;
  /**
   * Return the remaining CFP duration in microseconds.
   *
   * \return the remaining CFP duration in microseconds
   */
  uint64_t GetCFPDurRemainingUs (void) const;

  /**
   * Set the CFP Count in DTIM frames unit.
   *
   * \param count the CFP Count in DTIM frames unit
   */
  void SetCFPCount (uint8_t count);
  /**
   * Set the CFP Period in DTIM frames unit.
   *
   * \param period the CFP Period in DTIM frames unit
   */
  void SetCFPPeriod (uint8_t period);
  /**
   * Set the maximum CFP duration in microseconds.
   *
   * \param maxDuration the maximum CFP duration in microseconds
   */
  void SetCFPMaxDurationUs (uint64_t maxDuration);
  /**
   * Set the remaining CFP duration in microseconds.
   *
   * \param durRemaining the remaining CFP duration in microseconds
   */
  void SetCFPDurRemainingUs (uint64_t durRemaining);


private:
  uint8_t m_CFPCount;                 ///< CFP Count
  uint8_t m_CFPPeriod;                ///< CFP Period
  uint64_t m_CFPMaxDuration;          ///< CFP maximum duration in microseconds
  uint64_t m_CFPDurRemaining;         ///< CFP remaining duration in microseconds

  /// This is used to decide if this element should be added to the frame or not
  uint8_t m_pcfSupported;
};

/**
 * output operator
 *
 * \param os output stream
 * \param cfParameterSet the CF Parameter Set
 *
 * \return output stream
 */
std::ostream &operator << (std::ostream &os, const CfParameterSet &cfParameterSet);

} //namespace ns3

#endif /* CF_PARAMETER_SET_H */
