/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef WIFI_PHY_TAG_H
#define WIFI_PHY_TAG_H

#include "ns3/tag.h"
#include "wifi-preamble.h"
#include "wifi-mode.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * Tag for WifiTxVector and WifiPreamble information to be embedded in outgoing
 * transmissions as a PacketTag
 */
class WifiPhyTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  /**
   * Constructor
   */
  WifiPhyTag ();
  /**
   * Constructor
   * \param preamble the preamble type
   * \param modulation the modulation
   * \param frameComplete the frameComplete
   */
  WifiPhyTag (WifiPreamble preamble, WifiModulationClass modulation, uint8_t frameComplete);
  /**
   * Getter for preamble parameter
   * \return the preamble type
   */
  WifiPreamble GetPreambleType (void) const;
  /**
   * Getter for modulation parameter
   * \return the modulation
   */
  WifiModulationClass GetModulation (void) const;
  /**
   * Getter for frameComplete parameter
   * \return the frameComplete parameter, i.e. 0 if the frame is not complete, 1 otherwise.
   */
  uint8_t GetFrameComplete (void) const;

  // From class Tag
  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;


private:
  WifiPreamble m_preamble;          ///< preamble type
  WifiModulationClass m_modulation; ///< modulation used for transmission
  uint8_t m_frameComplete;          ///< Used to indicate that TX stopped sending before the end of the frame
};

} // namespace ns3

#endif /* WIFI_PHY_TAG_H */
