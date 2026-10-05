#ifndef MEC_ORCHESTRATOR_H
#define MEC_ORCHESTRATOR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include <map>
#include <vector>

namespace ns3 {

/**
 * @brief Telemetry and limits for a MEC Server
 */
struct MecServerInfo {
    uint32_t nodeId;
    Ipv4Address ipv4Address;
    double cpuTotal;
    double ramTotal;
    double bwTotal;
    double cpuUsed;
    double ramUsed;
    double bwUsed;
    Ptr<Node> node;
};

/**
 * @brief Context and telemetry for a deployed UE Service
 */
struct UeServiceContext {
    uint32_t ueNodeId;
    Ipv4Address clientIp;
    uint32_t currentMecNodeId;
    double cpuReq;
    double ramReq;
    double bwReq;
    uint32_t stateSizeBytes;
    Ptr<Node> node;
    ApplicationContainer applications;
};

/**
 * @brief Represents an executed orchestration decision
 */
struct MigrationDecision {
    uint32_t ueNodeId;
    uint32_t targetMecNodeId;
    Ipv4Address targetMecIp;
};

/**
 * @brief MDMKP-Based MEC Service Orchestrator
 * Maximizes QoE (via latency minimization) under strict multidimensional capacity constraints.
 */
class MecOrchestrator : public Object {
public:
    static TypeId GetTypeId(void);
    MecOrchestrator();
    virtual ~MecOrchestrator();

    void RegisterMecServer(uint32_t nodeId, Ipv4Address ip, double cpu, double ram, double bw, Ptr<Node> node);
    void RegisterUe(uint32_t ueNodeId, Ipv4Address ip, double cpu, double ram, double bw, uint32_t stateSize, Ptr<Node> node, uint32_t initialMecId, ApplicationContainer apps);
    void RemoveUe(uint32_t ueNodeId);
    void UpdateUeApplications(uint32_t ueNodeId, ApplicationContainer apps);

    // Periodic hook for running the MDMKP algorithm
    void RunOptimizationCycle();

    void SetOptimizationInterval(double interval);

    // Callback used to cleanly restart applications with a new destination IP after T_down elapses
    using AppMigrationCallback = std::function<ApplicationContainer(Ptr<Node>, Ipv4Address, Time)>;
    void SetAppMigrationCallback(AppMigrationCallback cb);

private:
    double EstimateLatency(uint32_t ueNodeId, uint32_t mecNodeId);
    std::vector<MigrationDecision> SolveMDMKP();
    void ExecuteMigrations(const std::vector<MigrationDecision>& decisions, uint32_t cycleId);

    std::map<uint32_t, MecServerInfo> m_mecServers;
    std::map<uint32_t, UeServiceContext> m_activeUes;
    AppMigrationCallback m_migrationCallback;
    
    double m_betaPenalty; // Beta parameter for state migration latency penalty
    double m_backhaulBwMbps; // Baseline bandwidth for state transfer downtime calculations
    double m_optimizationInterval{1.0}; // Optimization cycle interval in seconds
};

} // namespace ns3

#endif // MEC_ORCHESTRATOR_H

