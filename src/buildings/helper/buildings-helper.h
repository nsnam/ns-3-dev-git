/*
 * Copyright (c) 2012 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef BUILDINGS_HELPER_H
#define BUILDINGS_HELPER_H

#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"

#include <string>

namespace ns3
{

class MobilityModel;
class Building;

/**
 * @ingroup buildings
 *
 * Helper used to install a MobilityBuildingInfo into a set of nodes.
 */
class BuildingsHelper
{
  public:
    /**
     * Install the MobilityBuildingInfo to a node
     *
     * @param node the mobility model of the node to be updated
     */
    static void Install(Ptr<Node> node); // for any nodes
    /**
     * Install the MobilityBuildingInfo to the set of nodes in a NodeContainer
     *
     * @param c the NodeContainer including the nodes to be updated
     */
    static void Install(NodeContainer c); // for any nodes
};

} // namespace ns3

#endif /* BUILDINGS_HELPER_H */
