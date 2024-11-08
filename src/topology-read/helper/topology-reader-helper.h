/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#ifndef TOPOLOGY_READER_HELPER_H
#define TOPOLOGY_READER_HELPER_H

#include "ns3/topology-reader.h"

#include <string>

/**
 * @file
 * @ingroup topology
 * ns3::TopologyHelper declaration.
 */

namespace ns3
{

/**
 * @ingroup topology
 *
 * @brief Helper class which makes it easier to configure and use a generic TopologyReader.
 */
class TopologyReaderHelper
{
  public:
    TopologyReaderHelper();

    /**
     * @brief Sets the input file name.
     * @param [in] fileName The input file name.
     */
    void SetFileName(const std::string fileName);

    /**
     * @brief Sets the input file type. Supported file types are "Orbis", "Inet", "Rocketfuel".
     * @param [in] fileType The input file type.
     */
    void SetFileType(const std::string fileType);

    /**
     * @brief Gets a Ptr<TopologyReader> to the actual TopologyReader.
     * @return The created Topology Reader (or null if there was an error).
     */
    Ptr<TopologyReader> GetTopologyReader();

  private:
    Ptr<TopologyReader> m_inputModel; //!< Smart pointer to the actual topology model.
    std::string m_fileName;           //!< Name of the input file.
    std::string m_fileType;           //!< Type of the input file (e.g., "Inet", "Orbis", etc.).
};

} // namespace ns3

#endif /* TOPOLOGY_READER_HELPER_H */
