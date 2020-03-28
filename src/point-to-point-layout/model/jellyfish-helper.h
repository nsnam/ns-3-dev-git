/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Define an object to create a jellyfish topology.


#ifndef JELLY_FISH_HELPER_H
#define JELLY_FISH_HELPER_H

#include <string>
#include "ns3/point-to-point-module.h"
#include "ipv4-address-helper.h"
#include "internet-stack-helper.h"
#include "ipv4-interface-container.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

namespace ns3 {

class JellyFishHelper
{
public:
  /**
   * Create a JellyFishHelper in order to easily create
   * jellyfish topologies using Point-To-Point links
   * \param nTopOfRackSwitch is the total number of switches

   * \param nPort is the number of ports in a switch
   *
   * \param nServer is the number of servers connected to one switch

   * \param switchToSwitch is the Point-To-Point links between two different switches

   * \param switchToServer the Point-To-Point links between switch and server
   */
  JellyFishHelper(uint32_t nTopOfRackSwitch,
                  uint32_t nPort,
                  uint32_t nServer,
                  PointToPointHelper switchToSwitch,
                  PointToPointHelper switchToServer);

  ~JellyFishHelper ();

  /**
   * \returns pointer to the j'th server of i'th switch
   *
   */
  Ptr<Node>  GetServer (uint32_t i, uint32_t j);

  /**
   * \returns the number of Server present in the topology
   *
   */
  uint32_t  ServerCount ();

  /**
   * \param stack an InternetStackHelper which is used to install 
   *              on every node in the jellyfish topology
   */
  void InstallStack (InternetStackHelper stack);

  /**
   * \param address to assign Ipv4 addresses to the
   *               interfaces on the all nodes of topology
   *
   */
  void AssignIpv4Addresses (Ipv4AddressHelper address);

  /**
   * \returns an Ipv4Address of the j'th server of i'th switch
   * \param i node number
   */
  Ipv4Address GetIpv4Address (uint32_t i, uint32_t j);


/**

*
* \param nTopOfRackSwitch total number of switches
*
* \param nPort the number of ports in a switch

* \param switchPorts the number of ports on one switch which connects to other switches

*/
std::vector<std::pair<int,int>> edgeSet(int nTopOfRackSwitch, int nPort, int switchPorts);
private:
  uint32_t switchToSwitchLinks; // total number of Point_To_Point links between two switches 
  uint32_t nServersOnASwitch;   // number of servers linked to one switch
  NodeContainer          m_servers;
  NodeContainer          m_switches;
  NetDeviceContainer     m_switchToSwitchDevices1;
  NetDeviceContainer     m_switchToSwitchDevices2;
  NetDeviceContainer     m_switchDevices;
  NetDeviceContainer     m_serverDevices;
  Ipv4InterfaceContainer m_serverInterfaces;
};

} // namespace ns3
#endif /* JELLY_FISH_HELPER_H */
