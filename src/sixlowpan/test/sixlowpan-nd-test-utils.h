/*
 * Utility functions for sixlowpan ND tests.
 * Extracted from sixlowpan-nd-reg-test.cc to reduce duplication and improve reuse.
 */
#ifndef SIXLOWPAN_ND_TEST_UTILS_H
#define SIXLOWPAN_ND_TEST_UTILS_H

#include "ns3/nstime.h"

#include <cstdint>
#include <list>
#include <string>
#include <utility>
#include <vector>

namespace ns3
{

/**
 * @brief Generate expected routing table output for a star topology.
 * @param numNodes total number of nodes (1 router + numNodes-1 leaf nodes)
 * @param time simulation time at which the table is printed
 * @return the expected routing table string
 */
std::string GenerateRoutingTableOutput(uint32_t numNodes, Time time);

/**
 * @brief Sort the host-route entries in node 0's routing table block numerically.
 * @param routingTable the raw routing table string to sort
 * @return the sorted routing table string
 */
std::string SortRoutingTableString(std::string routingTable);

/**
 * @brief Replace STALE with REACHABLE in an NDisc cache output string.
 * @param ndiscOutput the NDisc cache output to normalize
 * @return the normalized string
 */
std::string NormalizeNdiscCacheStates(const std::string& ndiscOutput);

/**
 * @brief Generate expected NDisc cache output for a star topology.
 * @param numNodes total number of nodes
 * @param time simulation time at which the cache is printed
 * @return the expected NDisc cache string
 */
std::string GenerateNdiscCacheOutput(uint32_t numNodes, Time time);

/**
 * @brief Generate expected binding table output for a star topology.
 * @param numNodes total number of nodes
 * @param time simulation time at which the table is printed
 * @return the expected binding table string
 */
std::string GenerateBindingTableOutput(uint32_t numNodes, Time time);

} // namespace ns3

#endif // SIXLOWPAN_ND_TEST_UTILS_H
