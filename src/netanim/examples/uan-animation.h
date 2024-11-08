/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#ifndef UAN_CW_EXAMPLE_H
#define UAN_CW_EXAMPLE_H

#include "ns3/network-module.h"
#include "ns3/uan-module.h"

using namespace ns3;

/**
 * @class NetAnimExperiment
 * @brief Helper class for UAN CW MAC example
 *
 */
class NetAnimExperiment
{
  public:
    /**
     * Run function
     * @param uan the UAN helper
     */
    void Run(UanHelper& uan);
    /**
     * Receive packet function
     * @param socket the socket to receive from
     */
    void ReceivePacket(Ptr<Socket> socket);
    /**
     * Update positions function
     * @param nodes the collection of nodes
     */
    void UpdatePositions(NodeContainer& nodes) const;
    /// Reset data function
    void ResetData();
    /**
     * Increment CW function
     * @param cw the CW
     */
    void IncrementCw(uint32_t cw);
    uint32_t m_numNodes;   ///< number of nodes
    uint32_t m_dataRate;   ///< data rate
    double m_depth;        ///< depth
    double m_boundary;     ///< boundary
    uint32_t m_packetSize; ///< packet size
    uint32_t m_bytesTotal; ///< bytes total
    uint32_t m_cwMin;      ///< CW minimum
    uint32_t m_cwMax;      ///< CW maximum
    uint32_t m_cwStep;     ///< CW step
    uint32_t m_avgs;       ///< averages

    Time m_slotTime; ///< slot time
    Time m_simTime;  ///< simulation time

    std::vector<double> m_throughputs; ///< throughputs

    /// the experiment
    NetAnimExperiment();
};

#endif /* UAN_CW_EXAMPLE_H */
