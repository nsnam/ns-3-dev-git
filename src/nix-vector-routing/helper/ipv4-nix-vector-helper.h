/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Georgia Institute of Technology 
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
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 */

#ifndef IPV4_NIX_VECTOR_HELPER_H
#define IPV4_NIX_VECTOR_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

/**
 * \ingroup nix-vector-routing
 *
 * \brief Helper class that adds Nix-vector routing to nodes.
 *
 * This class is expected to be used in conjunction with 
 * ns3::InternetStackHelper::SetRoutingHelper
 *
 */
class Ipv4NixVectorHelper : public Ipv4RoutingHelper
{
public:
  /**
   * Construct an Ipv4NixVectorHelper to make life easier while adding Nix-vector
   * routing to nodes.
   */
  Ipv4NixVectorHelper ();

  /**
   * \brief Construct an Ipv4NixVectorHelper from another previously 
   * initialized instance (Copy Constructor).
   */
  Ipv4NixVectorHelper (const Ipv4NixVectorHelper &);

  /**
   * \returns pointer to clone of this Ipv4NixVectorHelper 
   * 
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv4NixVectorHelper* Copy (void) const;

  /**
  * \param node the node on which the routing protocol will run
  * \returns a newly-created routing protocol
  *
  * This method will be called by ns3::InternetStackHelper::Install
  */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  /**
   * \brief prints the routing path for a source and destination at a particular time.
   * If the routing path does not exist, it prints that the path does not exist between
   * the nodes in the ostream.
   * \param printTime the time at which the routing path is supposed to be printed.
   * \param source the source node pointer to start traversing
   * \param dest the IPv4 destination address
   * \param stream the output stream object to use
   * \param unit the time unit to be used in the report
   *
   * This method calls the PrintRoutingPath() method of the
   * Ipv4NixVectorRouting for the source and destination to provide
   * the routing path at the specified time.
   */
  void PrintRoutingPathAt (Time printTime, Ptr<Node> source, Ipv4Address dest, Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S);

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   * \return Nothing useful.
   */
  Ipv4NixVectorHelper &operator = (const Ipv4NixVectorHelper &);

  ObjectFactory m_agentFactory; //!< Object factory

  /**
   * \brief prints the routing path for the source and destination. If the routing path
   * does not exist, it prints that the path does not exist between the nodes in the ostream.
   * \param source the source node pointer to start traversing
   * \param dest the IPv4 destination address
   * \param stream the output stream object to use
   * \param unit the time unit to be used in the report
   *
   * This method calls the PrintRoutingPath() method of the
   * Ipv4NixVectorRouting for the source and destination to provide
   * the routing path.
   */
  static void PrintRoute (Ptr<Node> source, Ipv4Address dest, Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S);
};
} // namespace ns3

#endif /* IPV4_NIX_VECTOR_HELPER_H */
