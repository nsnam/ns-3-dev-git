/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_ACKNOWLEDGMENT_H
#define WIFI_ACKNOWLEDGMENT_H

#include "ns3/nstime.h"
#include "wifi-tx-vector.h"
#include "wifi-mac-header.h"
#include "ctrl-headers.h"
#include <map>


namespace ns3 {

class Mac48Address;

/**
 * \ingroup wifi
 *
 * WifiAcknowledgment is an abstract base struct. Each derived struct defines an acknowledgment
 * method and stores the information needed to perform acknowledgment according to
 * that method.
 */
struct WifiAcknowledgment
{
  /**
   * \enum Method
   * \brief Available acknowledgment methods
   */
  enum Method
    {
      NONE = 0,
      NORMAL_ACK
    };

  /**
   * Constructor.
   * \param m the acknowledgment method for this object
   */
  WifiAcknowledgment (Method m);
  virtual ~WifiAcknowledgment ();

  /**
   * Get the QoS Ack policy to use for the MPDUs addressed to the given receiver
   * and belonging to the given TID.
   *
   * \param receiver the MAC address of the receiver
   * \param tid the TID
   * \return the QoS Ack policy to use
   */
  WifiMacHeader::QosAckPolicy GetQosAckPolicy (Mac48Address receiver, uint8_t tid) const;

  /**
   * Set the QoS Ack policy to use for the MPDUs addressed to the given receiver
   * and belonging to the given TID. If the pair (receiver, TID) already exists,
   * it is overwritten with the given QoS Ack policy.
   *
   * \param receiver the MAC address of the receiver
   * \param tid the TID
   * \param ackPolicy the QoS Ack policy to use
   */
  void SetQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy);

  /**
   * \brief Print the object contents.
   * \param os output stream in which the data should be printed.
   */
  virtual void Print (std::ostream &os) const = 0;

  const Method method;       //!< acknowledgment method
  Time acknowledgmentTime;   //!< time required by the acknowledgment method

private:
  /**
   * Check whether the given QoS Ack policy can be used for the MPDUs addressed
   * to the given receiver and belonging to the given TID.
   *
   * \param receiver the MAC address of the receiver
   * \param tid the TID
   * \param ackPolicy the QoS Ack policy to use
   * \return true if the given QoS Ack policy can be used, false otherwise
   */
  virtual bool CheckQosAckPolicy (Mac48Address receiver, uint8_t tid,
                                  WifiMacHeader::QosAckPolicy ackPolicy) const = 0;

  /// Qos Ack Policy to set for MPDUs addressed to a given receiver and having a given TID
  std::map<std::pair<Mac48Address, uint8_t>, WifiMacHeader::QosAckPolicy> m_ackPolicy;
};


/**
 * \ingroup wifi
 *
 * WifiNoAck specifies that no acknowledgment is required.
 */
struct WifiNoAck : public WifiAcknowledgment
{
  WifiNoAck ();

  // Overridden from WifiAcknowledgment
  bool CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const override;
  void Print (std::ostream &os) const override;
};


/**
 * \ingroup wifi
 *
 * WifiNormalAck specifies that acknowledgment via Normal Ack is required.
 */
struct WifiNormalAck : public WifiAcknowledgment
{
  WifiNormalAck ();

  // Overridden from WifiAcknowledgment
  bool CheckQosAckPolicy (Mac48Address receiver, uint8_t tid, WifiMacHeader::QosAckPolicy ackPolicy) const override;
  void Print (std::ostream &os) const override;

  WifiTxVector ackTxVector;       //!< Ack TXVECTOR
};


/**
 * \brief Stream insertion operator.
 *
 * \param os the output stream
 * \param acknowledgment the acknowledgment method
 * \returns a reference to the stream
 */
std::ostream& operator<< (std::ostream& os, const WifiAcknowledgment* acknowledgment);

} //namespace ns3

#endif /* WIFI_ACKNOWLEDGMENT_H */
