/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

/**
 * @file
 * @ingroup antenna-examples
 * Example program illustrating one application of symmetric adjacency matrices for routing
 */

#include "ns3/command-line.h"
#include "ns3/symmetric-adjacency-matrix.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>

int
main(int argc, char** argv)
{
    char srcNodeOpt = 'A'; // 0
    char dstNodeOpt = 'I'; // 8
    ns3::CommandLine cmd(__FILE__);
    cmd.AddValue("srcNode", "Source node [0-9]", srcNodeOpt);
    cmd.AddValue("dstNode", "Destination node [0-9]", dstNodeOpt);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(srcNodeOpt < 'A' || srcNodeOpt > 'J', "Invalid source node");
    NS_ABORT_MSG_IF(dstNodeOpt < 'A' || dstNodeOpt > 'J', "Invalid destination node");

    // -A(65) remove the skew from 0
    srcNodeOpt -= 'A';
    dstNodeOpt -= 'A';

    constexpr float maxFloat = std::numeric_limits<float>::max();
    // Create routing weight matrix for 10 nodes and initialize weights to infinity (disconnected)
    ns3::SymmetricAdjacencyMatrix<float> routeWeights(10, maxFloat);

    /* Let's add the entries of this network topology to the matrix
     *
     * Node | Corresponding matrix row
     *  A   | 0
     *  B   | 1
     *  C   | 2
     *  D   | 3
     *  E   | 4
     *  F   | 5
     *  G   | 6
     *  H   | 7
     *  I   | 8
     *  J   | 9
     *
     * A------5-------B-------------14-------C
     * \               \                   /1|
     *  \               3                 J  |
     *   \               \               /1  | 7
     *    4           E-2-F--4---G--3--H     |
     *     \       8 /                  \    |
     *      D--------                    10--I
     */

    // Distance from nodes to other nodes
    routeWeights.SetValue(0, 1, 5);  // A-B=5
    routeWeights.SetValue(1, 2, 14); // B-C=14
    routeWeights.SetValue(0, 3, 4);  // A-D=4
    routeWeights.SetValue(1, 5, 3);  // B-F=3
    routeWeights.SetValue(2, 9, 1);  // C-J=1
    routeWeights.SetValue(9, 7, 1);  // J-H=1
    routeWeights.SetValue(2, 8, 7);  // C-I=7
    routeWeights.SetValue(3, 4, 8);  // D-E=8
    routeWeights.SetValue(4, 5, 2);  // E-F=2
    routeWeights.SetValue(5, 6, 4);  // F-G=4
    routeWeights.SetValue(6, 7, 3);  // G-H=3
    routeWeights.SetValue(7, 8, 10); // H-I=10

    // Distance from nodes to themselves is zero
    for (size_t i = 0; i < routeWeights.GetRows(); i++)
    {
        routeWeights.SetValue(i, i, 0);
    }

    std::map<std::pair<int, int>, std::vector<int>> routeMap;
    // Initialize routes
    for (size_t i = 0; i < routeWeights.GetRows(); i++)
    {
        for (size_t j = 0; j < routeWeights.GetRows(); j++)
        {
            if (routeWeights.GetValue(i, j) != maxFloat)
            {
                if (i != j)
                {
                    routeMap[{i, j}] = {(int)i, (int)j};
                }
                else
                {
                    routeMap[{i, j}] = {(int)i};
                }
            }
        }
    }
    // Compute every single shortest route between the nodes of the graph (represented by the
    // adjacency matrix) We do this in multiple iterations, until we fill the entire matrix
    for (size_t bridgeNode = 0; bridgeNode < routeWeights.GetRows(); bridgeNode++)
    {
        for (size_t srcNode = 0; srcNode < routeWeights.GetRows(); srcNode++)
        {
            for (size_t dstNode = 0; dstNode < routeWeights.GetRows(); dstNode++)
            {
                auto weightA = routeWeights.GetValue(srcNode, bridgeNode);
                auto weightB = routeWeights.GetValue(bridgeNode, dstNode);
                // If there is a path between A and bridge, plus bridge and B
                if (std::max(weightA, weightB) == maxFloat)
                {
                    continue;
                }
                // Check if sum of weights is lower than existing path
                auto weightAB = routeWeights.GetValue(srcNode, dstNode);
                if (weightA + weightB < weightAB)
                {
                    // If it is, update adjacency matrix with the new weight of the shortest
                    // path
                    routeWeights.SetValue(srcNode, dstNode, weightA + weightB);

                    // Retrieve the partial routes A->bridge and bridge->C,
                    // and assemble the new route A->bridge->C
                    const auto srcToBridgeRoute = routeMap.at({srcNode, bridgeNode});
                    const auto bridgeToDstRoute = routeMap.at({bridgeNode, dstNode});
                    std::vector<int> dst;
                    dst.insert(dst.end(), srcToBridgeRoute.begin(), srcToBridgeRoute.end());
                    dst.insert(dst.end(), bridgeToDstRoute.begin() + 1, bridgeToDstRoute.end());
                    routeMap[{srcNode, dstNode}] = dst;

                    // We also include the reverse path, since the graph is bidirectional
                    std::vector<int> invDst(dst.rbegin(), dst.rend());
                    routeMap[{dstNode, srcNode}] = invDst;
                }
            }
        }
    }

    // Now we can print the shortest route between srcNode and dstNode
    std::cout << "shortest route between " << (char)(srcNodeOpt + 'A') << " and "
              << (char)(dstNodeOpt + 'A') << " (length "
              << routeWeights.GetValue(srcNodeOpt, dstNodeOpt) << "):";
    auto lastNodeNumber = srcNodeOpt;
    for (auto nodeNumber : routeMap.at({srcNodeOpt, dstNodeOpt}))
    {
        std::cout << "--" << routeWeights.GetValue(lastNodeNumber, nodeNumber) << "-->"
                  << (char)('A' + nodeNumber);
        lastNodeNumber = nodeNumber;
    }
    std::cout << std::endl;
    return 0;
}
