/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#ifndef ORBIS_TOPOLOGY_READER_H
#define ORBIS_TOPOLOGY_READER_H

#include "topology-reader.h"

/**
 * @file
 * @ingroup topology
 * ns3::OrbisTopologyReader declaration.
 */

namespace ns3
{

// ------------------------------------------------------------
// --------------------------------------------
/**
 * @ingroup topology
 *
 * @brief Topology file reader (Orbis-format type).
 *
 * This class takes an input file in Orbis format and extracts all
 * the information needed to build the topology
 * (i.e.number of nodes, links and links structure).
 * It have been tested with Orbis 0.70
 * https://web.archive.org/web/20181102004219/http://sysnet.ucsd.edu/~pmahadevan/topo_research/topo.html
 */
class OrbisTopologyReader : public TopologyReader
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId.
     */
    static TypeId GetTypeId();

    OrbisTopologyReader();
    ~OrbisTopologyReader() override;

    // Delete copy constructor and assignment operator to avoid misuse
    OrbisTopologyReader(const OrbisTopologyReader&) = delete;
    OrbisTopologyReader& operator=(const OrbisTopologyReader&) = delete;

    /**
     * @brief Main topology reading function.
     *
     * This method opens an input stream and reads the Orbis-format file.
     * Every row represents a topology link (the ids of a couple of nodes),
     * so the input file is read line by line to figure out how many links
     * and nodes are in the topology.
     *
     * @return The container of the nodes created (or empty container if there was an error)
     */
    NodeContainer Read() override;

    // end class OrbisTopologyReader
};

// end namespace ns3
}; // namespace ns3

#endif /* ORBIS_TOPOLOGY_READER_H */
