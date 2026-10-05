#include "mec-orchestrator.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "simulation-logger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MecOrchestrator");
NS_OBJECT_ENSURE_REGISTERED(MecOrchestrator);

TypeId MecOrchestrator::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MecOrchestrator")
        .SetParent<Object>()
        .SetGroupName("Applications")
        .AddConstructor<MecOrchestrator>();
    return tid;
}

MecOrchestrator::MecOrchestrator() : m_betaPenalty(20.0), m_backhaulBwMbps(10000.0) {}
MecOrchestrator::~MecOrchestrator() {}

void MecOrchestrator::RegisterMecServer(uint32_t nodeId, Ipv4Address ip, double cpu, double ram, double bw, Ptr<Node> node) {
    MecServerInfo info{nodeId, ip, cpu, ram, bw, 0.0, 0.0, 0.0, node};
    m_mecServers[nodeId] = info;
    NS_LOG_UNCOND("Orchestrator: Registered MEC Server " << nodeId << " (CPU: " << cpu << ", RAM: " << ram << "GB, BW: " << bw << "Mbps)");
}

void MecOrchestrator::RegisterUe(uint32_t ueNodeId, Ipv4Address ip, double cpu, double ram, double bw, uint32_t stateSize, Ptr<Node> node, uint32_t initialMecId, ApplicationContainer apps) {
    UeServiceContext ctx{ueNodeId, ip, initialMecId, cpu, ram, bw, stateSize, node, apps};
    m_activeUes[ueNodeId] = ctx;
    
    // Assign initial resource consumption to the target MEC
    if (m_mecServers.find(initialMecId) != m_mecServers.end()) {
        m_mecServers[initialMecId].cpuUsed += cpu;
        m_mecServers[initialMecId].ramUsed += ram;
        m_mecServers[initialMecId].bwUsed += bw;
    }
}

void MecOrchestrator::RemoveUe(uint32_t ueNodeId) {
    auto it = m_activeUes.find(ueNodeId);
    if (it != m_activeUes.end()) {
        uint32_t mecId = it->second.currentMecNodeId;
        if (m_mecServers.find(mecId) != m_mecServers.end()) {
            m_mecServers[mecId].cpuUsed -= it->second.cpuReq;
            m_mecServers[mecId].ramUsed -= it->second.ramReq;
            m_mecServers[mecId].bwUsed -= it->second.bwReq;
        }
        m_activeUes.erase(it);
    }
}

void MecOrchestrator::UpdateUeApplications(uint32_t ueNodeId, ApplicationContainer apps) {
    if (m_activeUes.find(ueNodeId) != m_activeUes.end()) {
        m_activeUes[ueNodeId].applications = apps;
    }
}

void MecOrchestrator::SetAppMigrationCallback(AppMigrationCallback cb) {
    m_migrationCallback = cb;
}

double MecOrchestrator::EstimateLatency(uint32_t ueNodeId, uint32_t mecNodeId) {
    Ptr<MobilityModel> ueMob = m_activeUes[ueNodeId].node->GetObject<MobilityModel>();
    Ptr<MobilityModel> mecMob = m_mecServers[mecNodeId].node->GetObject<MobilityModel>();
    
    if (!ueMob || !mecMob) return 10.0; // Fallback latency
    
    // 1. Calculate physical Euclidean distance
    double distance = ueMob->GetDistanceFrom(mecMob);
    
    // 2. Propagation delay (Distance / Speed of Light) in milliseconds
    double propDelayMs = (distance / 3e8) * 1000.0;
    
    // 3. Historical / Base RAN & Transport Delay (Approximating standard 5G RLC/MAC scheduler constraints)
    // If we had a trace sink wired to DlDataSinr, we would map it here. For now we use standard heuristic constants.
    double baseRanDelayMs = 4.0;
    double backhaulQueuingDelayMs = 1.0; 
    
    return propDelayMs + baseRanDelayMs + backhaulQueuingDelayMs;
}

std::vector<MigrationDecision> MecOrchestrator::SolveMDMKP() {
    std::vector<MigrationDecision> decisions;
    
    // Reset MEC resource counters (re-calculating the entire state)
    for (auto& mec : m_mecServers) {
        mec.second.cpuUsed = 0.0;
        mec.second.ramUsed = 0.0;
        mec.second.bwUsed = 0.0;
    }

    struct CandidateAssignment {
        uint32_t ueId;
        uint32_t mecId;
        double efficiency;
        double utility;
        double resourceCost;
    };
    std::vector<CandidateAssignment> candidates;

    const double EPSILON = 1e-6;

    // Build the pseudo-utility and efficiency tables for all UE-MEC pairs
    for (auto& uePair : m_activeUes) {
        uint32_t ueId = uePair.first;
        UeServiceContext& ue = uePair.second;
        
        for (auto& mecPair : m_mecServers) {
            uint32_t mecId = mecPair.first;
            MecServerInfo& mec = mecPair.second;
            
            // Objective Function: max( 1000 / (L_ij + 1) - beta * Migration_Indicator )
            double L_ij = EstimateLatency(ueId, mecId);
            double migrationPenalty = (mecId != ue.currentMecNodeId) ? m_betaPenalty : 0.0;
            double U_ij = (1000.0 / (L_ij + 1.0)) - migrationPenalty;
            
            // Resource Fraction: Aggregate Multidimensional Resource Impact (Toyoda's concept)
            double availCpu = std::max(EPSILON, mec.cpuTotal - mec.cpuUsed);
            double availRam = std::max(EPSILON, mec.ramTotal - mec.ramUsed);
            double availBw  = std::max(EPSILON, mec.bwTotal - mec.bwUsed);
            
            double R_ij = (ue.cpuReq / availCpu) + (ue.ramReq / availRam) + (ue.bwReq / availBw);
            double E_ij = U_ij / R_ij;
            
            candidates.push_back({ueId, mecId, E_ij, U_ij, R_ij});
        }
    }

    // Sort combinations primarily by Efficiency Score (Greedy Heuristic)
    std::sort(candidates.begin(), candidates.end(), [](const CandidateAssignment& a, const CandidateAssignment& b) {
        return a.efficiency > b.efficiency;
    });

    std::map<uint32_t, bool> assignedUes;
    for (auto& uePair : m_activeUes) assignedUes[uePair.first] = false;

    // Sequentially lock in the best choices that don't violate constraints
    for (const auto& cand : candidates) {
        if (assignedUes[cand.ueId]) continue; // Already mapped this cycle
        
        MecServerInfo& mec = m_mecServers[cand.mecId];
        UeServiceContext& ue = m_activeUes[cand.ueId];
        
        // Ensure hard multidimensional capacity limits are respected
        if (mec.cpuUsed + ue.cpuReq <= mec.cpuTotal &&
            mec.ramUsed + ue.ramReq <= mec.ramTotal &&
            mec.bwUsed + ue.bwReq <= mec.bwTotal) 
        {
            mec.cpuUsed += ue.cpuReq;
            mec.ramUsed += ue.ramReq;
            mec.bwUsed += ue.bwReq;
            assignedUes[cand.ueId] = true;
            
            // Output a migration decision if it's deviating from the current host
            if (cand.mecId != ue.currentMecNodeId) {
                decisions.push_back({cand.ueId, cand.mecId, mec.ipv4Address});
            }
        }
    }
    
    // Note: Any UEs not assigned would technically drop service if capacity completely capped out.
    // In our implementation, we'll let them forcefully idle or stay wherever they were last.
    
    return decisions;
}

void MecOrchestrator::ExecuteMigrations(const std::vector<MigrationDecision>& decisions, uint32_t cycleId) {
    std::ofstream eventLog("results/raw_data/vehicle_events.csv", std::ios::app);

    for (const auto& dec : decisions) {
        UeServiceContext& ue = m_activeUes[dec.ueNodeId];
        
        // Calculate Migration State Transfer Downtime Penalty
        double tDownSecs = (ue.stateSizeBytes * 8.0) / (m_backhaulBwMbps * 1000000.0);
        Time tDown = Seconds(tDownSecs);
        
        // SimulationLogger Integration
        std::ostringstream decisionDetails;
        decisionDetails << "action=MIGRATE, src_mec=" << ue.currentMecNodeId << ", tgt_mec=" << dec.targetMecNodeId << ", downtime_ms=" << (tDownSecs * 1000.0) << ", utility_gain=0.0";
        SimulationLogger::Log("ORCHESTRATOR", "ORCHESTRATOR_DECISION_OUTPUT", dec.ueNodeId, "cycle_" + std::to_string(cycleId), decisionDetails.str());
        
        std::ostringstream bindingDetails;
        bindingDetails << "old_mec=" << ue.currentMecNodeId << ", new_mec=" << dec.targetMecNodeId << ", new_ip=" << dec.targetMecIp;
        SimulationLogger::Log("MEC_STATE", "MEC_BINDING_UPDATED", dec.ueNodeId, "mec_node_" + std::to_string(dec.targetMecNodeId), bindingDetails.str());

        NS_LOG_UNCOND("MDMKP_MIGRATION time=" << Simulator::Now().GetSeconds() 
                      << " ueNode=" << dec.ueNodeId 
                      << " fromMec=" << ue.currentMecNodeId 
                      << " toMec=" << dec.targetMecNodeId 
                      << " targetIp=" << dec.targetMecIp 
                      << " downtime=" << tDownSecs << "s");
                      
        if (eventLog.is_open()) {
            eventLog << Simulator::Now().GetSeconds() << ",MDMKP_MIGRATION," << dec.ueNodeId 
                     << "," << ue.currentMecNodeId << "->" << dec.targetMecNodeId << "," << tDownSecs << "\n";
        }

        // Pause active application transmission
        if (ue.applications.GetN() > 0) {
            ue.applications.Stop(Simulator::Now());
            ue.applications = ApplicationContainer();
        }
        
        // Schedule callback to resume traffic on new destination IP after the downtime penalty
        if (m_migrationCallback) {
            Time resumeTime = Simulator::Now() + tDown;
            ApplicationContainer newApps = m_migrationCallback(ue.node, dec.targetMecIp, resumeTime);
            ue.applications = newApps;
        }
        
        ue.currentMecNodeId = dec.targetMecNodeId;
    }
}

void MecOrchestrator::RunOptimizationCycle() {
    NS_LOG_LOGIC("Running MDMKP Service Optimization Cycle...");

    static uint32_t cycleId = 1;
    
    std::ostringstream snapshotDetails;
    snapshotDetails << "active_ues=" << m_activeUes.size();
    for (auto& mecPair : m_mecServers) {
        auto& mec = mecPair.second;
        double cpuRemPct = (mec.cpuTotal > 0) ? ((mec.cpuTotal - mec.cpuUsed) / mec.cpuTotal * 100.0) : 0;
        snapshotDetails << ", mec" << mecPair.first << "_cpu_rem=" << std::round(cpuRemPct) << "%";
    }
    
    SimulationLogger::Log("ORCHESTRATOR", "ORCHESTRATOR_INPUT_SNAPSHOT", 0, "cycle_" + std::to_string(cycleId), snapshotDetails.str());

    std::vector<MigrationDecision> decisions = SolveMDMKP();
    
    ExecuteMigrations(decisions, cycleId);
    
    cycleId++;

    // Re-schedule the orchestrator execution
    Simulator::Schedule(Seconds(m_optimizationInterval), &MecOrchestrator::RunOptimizationCycle, this);
}

void MecOrchestrator::SetOptimizationInterval(double interval) {
    m_optimizationInterval = interval;
}

} // namespace ns3

