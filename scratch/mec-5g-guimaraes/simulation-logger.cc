#include "simulation-logger.h"

namespace ns3 {

std::ofstream SimulationLogger::m_file;
std::map<Ipv4Address, uint32_t> SimulationLogger::m_ipToNodeId;

void SimulationLogger::Init(std::string filepath) {
    m_file.open(filepath, std::ios::out);
    if (m_file.is_open()) {
        m_file << "timestamp_s,event_category,event_type,primary_node_id,secondary_entity_id,details\n";
    }
}

void SimulationLogger::Close() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void SimulationLogger::Log(std::string category, std::string eventType, uint32_t nodeId, std::string entityId, std::string details) {
    if (!m_file.is_open()) return;
    
    m_file << Simulator::Now().GetSeconds() << ","
           << category << "," 
           << eventType << ","
           << nodeId << "," 
           << entityId << "," 
           << "\"" << details << "\"\n";
}

void SimulationLogger::RegisterIp(Ipv4Address ip, uint32_t nodeId) {
    m_ipToNodeId[ip] = nodeId;
}

uint32_t SimulationLogger::GetNodeIdFromIp(Ipv4Address ip) {
    auto it = m_ipToNodeId.find(ip);
    return (it != m_ipToNodeId.end()) ? it->second : 0;
}

} // namespace ns3

