#ifndef SIMULATION_LOGGER_H
#define SIMULATION_LOGGER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <string>
#include <fstream>
#include <map>

namespace ns3 {

/**
 * @brief Unified chronological trace logger for simulation events (MDMKP, SUMO, 5G NR).
 */
class SimulationLogger {
public:
    static void Init(std::string filepath);
    static void Close();

    /**
     * @brief Writes a formatted entry to the unified CSV trace
     */
    static void Log(std::string category, std::string eventType, uint32_t nodeId, std::string entityId, std::string details);

    // Helpers to map IPv4 sockets back to UE Node IDs natively
    static void RegisterIp(Ipv4Address ip, uint32_t nodeId);
    static uint32_t GetNodeIdFromIp(Ipv4Address ip);

private:
    static std::ofstream m_file;
    static std::map<Ipv4Address, uint32_t> m_ipToNodeId;
};

} // namespace ns3

#endif // SIMULATION_LOGGER_H

