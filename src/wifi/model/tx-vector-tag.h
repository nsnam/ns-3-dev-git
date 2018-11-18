/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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

#ifndef TX_VECTOR_TAG_H
#define TX_VECTOR_TAG_H

#include "ns3/tag.h"
#include "wifi-tx-vector.h"

/***************************************************************
 *           Packet Mode Tagger
 ***************************************************************/

namespace ns3 {

/**
 * HighLatencyDataTxVectorTag class
 */
class HighLatencyDataTxVectorTag : public Tag
{
public:
  HighLatencyDataTxVectorTag ();
  /**
   * Constructor
   *
   * \param dataTxVector TXVECTOR for data frames
   */
  HighLatencyDataTxVectorTag (WifiTxVector dataTxVector);
  /**
   * \returns the transmission mode to use to send this packet
   */
  WifiTxVector GetDataTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_dataTxVector; ///< TXVECTOR for data frames
};

/**
 * HighLatencyRtsTxVectorTag class
 */
class HighLatencyRtsTxVectorTag : public Tag
{
public:
  HighLatencyRtsTxVectorTag ();
  /**
   * Constructor
   *
   * \param rtsTxVector TXVECTOR for RTS frames
   */
  HighLatencyRtsTxVectorTag (WifiTxVector rtsTxVector);
  /**
   * \returns the transmission mode to use to send the RTS prior to the
   *          transmission of the data packet itself.
   */
  WifiTxVector GetRtsTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_rtsTxVector; ///< TXVECTOR for data frames
};

/**
 * HighLatencyCtsToSelfTxVectorTag class
 */
class HighLatencyCtsToSelfTxVectorTag : public Tag
{
public:
  HighLatencyCtsToSelfTxVectorTag ();
  /**
   * Constructor
   *
   * \param ctsToSelfTxVector TXVECTOR for CTS-to-self frames
   */
  HighLatencyCtsToSelfTxVectorTag (WifiTxVector ctsToSelfTxVector);
  /**
   * \returns the transmission mode to use for the CTS-to-self.
   */
  WifiTxVector GetCtsToSelfTxVector (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  WifiTxVector m_ctsToSelfTxVector; ///< TXVECTOR for CTS-to-self frames
};

} //namespace ns3

#endif /* TX_VECTOR_TAG_H */
