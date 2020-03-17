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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef YANS_WIFI_PHY_H
#define YANS_WIFI_PHY_H

#include "wifi-phy.h"

namespace ns3 {

class YansWifiChannel;
class WifiPpdu;

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a model of 802.11a. The model
 * implemented here is based on the model described
 * in "Yet Another Network Simulator",
 * (http://cutebugs.net/files/wns2-yans.pdf).
 *
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::PropagationLossModel
 * and ns3::PropagationDelayModel classes, both of which are
 * members of the ns3::YansWifiChannel class.
 */
class YansWifiPhy : public WifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  YansWifiPhy ();
  virtual ~YansWifiPhy ();

  // Implementation of pure virtual method.
  void StartTx (Ptr<WifiPpdu> ppdu);
  virtual Ptr<Channel> GetChannel (void) const;

  /**
   * Set the YansWifiChannel this YansWifiPhy is to be connected to.
   *
   * \param channel the YansWifiChannel this YansWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<YansWifiChannel> channel);

protected:
  // Inherited
  virtual void DoDispose (void);


private:
  Ptr<YansWifiChannel> m_channel; //!< YansWifiChannel that this YansWifiPhy is connected to
};

} //namespace ns3

#endif /* YANS_WIFI_PHY_H */
