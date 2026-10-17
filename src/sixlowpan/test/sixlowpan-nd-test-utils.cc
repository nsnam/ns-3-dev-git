/*
 * Utility implementations for sixlowpan ND tests.
 */

#include "sixlowpan-nd-test-utils.h"

#include <algorithm>
#include <iomanip>
#include <regex>
#include <sstream>
#include <vector>

namespace ns3
{

// ---------------------------------------------------------------------------
// Address formatting helpers
// Node indices are 1-based (i=1 is the first LN, with link-layer addr 0x02).
// ---------------------------------------------------------------------------

/**
 * @brief Global unicast address for leaf node index i.
 * @param i node index (1-based)
 * @return address string "2001::200:ff:fe00:<i+1>"
 */
static std::string
GlobalAddr(uint32_t i)
{
    std::ostringstream oss;
    oss << "2001::200:ff:fe00:" << std::hex << (i + 1);
    return oss.str();
}

/**
 * @brief Link-local address for leaf node index i.
 * @param i node index (1-based)
 * @return address string "fe80::200:ff:fe00:<i+1>"
 */
static std::string
LinkLocalAddr(uint32_t i)
{
    std::ostringstream oss;
    oss << "fe80::200:ff:fe00:" << std::hex << (i + 1);
    return oss.str();
}

/**
 * @brief Link-layer address for leaf node index i.
 * @param i node index (1-based)
 * @return address string "03-06-00:00:00:00:00:<i+1>"
 */
static std::string
LLAddr(uint32_t i)
{
    std::ostringstream oss;
    oss << "03-06-00:00:00:00:00:" << std::setfill('0') << std::setw(2) << std::hex << (i + 1);
    return oss.str();
}

/**
 * @brief Format a node/time/protocol header line as printed by ns-3 routing helpers.
 * @param nodeId the node identifier
 * @param time the simulation time
 * @param protocol the protocol name string
 * @return the formatted header string
 */
static std::string
NodeHeader(uint32_t nodeId, Time time, const std::string& protocol)
{
    std::ostringstream oss;
    oss << "Node: " << nodeId << ", Time: +" << time.GetSeconds() << "s"
        << ", Local time: +" << time.GetSeconds() << "s, " << protocol;
    return oss.str();
}

std::string
GenerateRoutingTableOutput(uint32_t numNodes, Time time)
{
    std::ostringstream oss;
    oss << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    for (uint32_t nodeId = 0; nodeId < numNodes; ++nodeId)
    {
        oss << NodeHeader(nodeId, time, "Ipv6ListRouting table") << std::endl;

        // ---- Static routing (priority 0) ----
        oss << "  Priority: 0 Protocol: ns3::Ipv6StaticRouting" << std::endl;
        oss << NodeHeader(nodeId, time, "Ipv6StaticRouting table") << std::endl;
        oss << "Destination                    Next Hop                   Flag Met Ref Use If"
            << std::endl;

        oss << std::setw(31) << "::1/128" << std::setw(27) << "::" << std::setw(5) << "UH"
            << std::setw(4) << "0"
            << "-   -   0" << std::endl;
        oss << std::setw(31) << "fe80::/64" << std::setw(27) << "::" << std::setw(5) << "U"
            << std::setw(4) << "0"
            << "-   -   1" << std::endl;

        if (nodeId == 0)
        {
            for (uint32_t i = 1; i < numNodes; ++i)
            {
                oss << std::setw(31) << (GlobalAddr(i) + "/128") << std::setw(27)
                    << LinkLocalAddr(i) << std::setw(5) << "UH" << std::setw(4) << "0"
                    << "-   -   1" << std::endl;
            }
        }
        else
        {
            oss << std::setw(31) << "::/0" << std::setw(27) << "fe80::200:ff:fe00:1" << std::setw(5)
                << "UG" << std::setw(4) << "0"
                << "-   -   1" << std::endl;
        }

        oss << std::endl;

        // ---- Global routing (priority -10, empty) ----
        oss << "  Priority: -10 Protocol: ns3::Ipv6GlobalRouting" << std::endl;
        oss << NodeHeader(nodeId, time, "Ipv6GlobalRouting table") << std::endl;
        oss << "Destination                    Next Hop                   Flag Met Ref Use Iface"
            << std::endl
            << std::endl;
    }

    return oss.str();
}

std::string
SortRoutingTableString(std::string routingTable)
{
    std::istringstream iss(routingTable);
    std::ostringstream oss;
    std::string line;

    while (std::getline(iss, line))
    {
        if (line.find("Node: 0") != std::string::npos)
        {
            oss << line << std::endl;
            std::getline(iss, line);
            oss << line << std::endl;

            std::vector<std::string> standardRoutes;
            std::vector<std::pair<int, std::string>> hostRoutes;

            while (std::getline(iss, line) && !line.empty())
            {
                if (line.find("2001::200:ff:fe00:") != std::string::npos)
                {
                    std::regex hexPattern("2001::200:ff:fe00:([0-9a-f]+)/128");
                    std::smatch match;

                    if (std::regex_search(line, match, hexPattern))
                    {
                        int hexValue = std::stoi(match[1].str(), nullptr, 16);
                        hostRoutes.emplace_back(hexValue, line);
                    }
                }
                else
                {
                    standardRoutes.push_back(line);
                }
            }

            for (const auto& route : standardRoutes)
            {
                oss << route << std::endl;
            }

            std::sort(hostRoutes.begin(),
                      hostRoutes.end(),
                      [](const std::pair<int, std::string>& a,
                         const std::pair<int, std::string>& b) { return a.first < b.first; });

            for (const auto& route : hostRoutes)
            {
                oss << route.second << std::endl;
            }

            oss << std::endl;
        }
        else
        {
            oss << line << std::endl;
            if (line.find("Node:") != std::string::npos)
            {
                while (std::getline(iss, line))
                {
                    oss << line << std::endl;
                    if (line.empty())
                    {
                        break;
                    }
                }
            }
        }
    }

    return oss.str();
}

std::string
NormalizeNdiscCacheStates(const std::string& ndiscOutput)
{
    std::string result = ndiscOutput;
    result = std::regex_replace(result, std::regex("STALE"), "REACHABLE");
    return result;
}

std::string
GenerateNdiscCacheOutput(uint32_t numNodes, Time time)
{
    std::ostringstream oss;

    for (uint32_t nodeId = 0; nodeId < numNodes; ++nodeId)
    {
        oss << "NDISC Cache of node " << std::dec << nodeId << " at time +" << time.GetSeconds()
            << "s" << std::endl;

        if (nodeId == 0)
        {
            for (uint32_t i = 1; i < numNodes; ++i)
            {
                oss << GlobalAddr(i) << " dev 2 lladdr " << LLAddr(i) << " REACHABLE" << std::endl;
            }
            for (uint32_t i = 1; i < numNodes; ++i)
            {
                oss << LinkLocalAddr(i) << " dev 2 lladdr " << LLAddr(i) << " REACHABLE"
                    << std::endl;
            }
        }
        else
        {
            oss << "fe80::200:ff:fe00:1 dev 2 lladdr 03-06-00:00:00:00:00:01 REACHABLE"
                << std::endl;
        }
    }

    return oss.str();
}

std::string
GenerateBindingTableOutput(uint32_t numNodes, Time time)
{
    std::ostringstream oss;

    for (uint32_t nodeId = 0; nodeId < numNodes; ++nodeId)
    {
        oss << "6LoWPAN-ND Binding Table of node " << std::dec << nodeId << " at time +"
            << time.GetSeconds() << "s" << std::endl;

        if (nodeId == 0)
        {
            oss << "Interface 1:" << std::endl;
            // Global addresses sort numerically before link-local (2001:: < fe80::),
            // so emit all globals first, then all link-locals, both in node order.
            for (uint32_t i = 1; i < numNodes; ++i)
            {
                oss << GlobalAddr(i) << " REACHABLE" << std::endl;
            }
            for (uint32_t i = 1; i < numNodes; ++i)
            {
                oss << LinkLocalAddr(i) << " REACHABLE" << std::endl;
            }
        }
    }

    return oss.str();
}

} // namespace ns3
