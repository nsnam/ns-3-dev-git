/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EHT_CAPABILITIES_H
#define EHT_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11be EHT Capabilities
 */
class EhtCapabilities : public WifiInformationElement
{
public:
  EhtCapabilities ();
  /**
   * Set EHT supported
   * \param ehtSupported the EHT supported indicator
   */
  void SetEhtSupported (uint8_t ehtSupported);

  // Implementations of pure virtual methods, or overridden from base class.
  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExt () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /*
   * This information element is a bit special in that it is only
   * included if the STA is an EHT STA. To support this we
   * override the Serialize and GetSerializedSize methods of
   * WifiInformationElement.
   */
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;


private:
  //TODO: add fields

  /// This is used to decide if this element should be added to the frame or not
  uint8_t m_ehtSupported;
};

/**
 * output stream output operator
 * \param os the output stream
 * \param ehtCapabilities the EHT capabilities
 * \returns the output stream
 */
std::ostream &operator << (std::ostream &os, const EhtCapabilities &ehtCapabilities);

} //namespace ns3

#endif /* HE_CAPABILITY_H */
