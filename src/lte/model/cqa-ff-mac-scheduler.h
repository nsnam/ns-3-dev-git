/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Biljana Bojovic <bbojovic@cttc.es>, Nicola Baldo<nbaldo@cttc.es>.
 *
 * Note: Implementation is using many common scheduler functionalities in its original version
 * implemented by Marco Miozzo<mmiozzo@cttc.es> in PF and RR schedulers. *
 */

#ifndef CQA_FF_MAC_SCHEDULER_H
#define CQA_FF_MAC_SCHEDULER_H

#include "ff-mac-csched-sap.h"
#include "ff-mac-sched-sap.h"
#include "ff-mac-scheduler.h"
#include "lte-amc.h"
#include "lte-common.h"
#include "lte-ffr-sap.h"

#include "ns3/nstime.h"

#include <map>
#include <set>
#include <vector>

namespace ns3
{

/// CGA Flow Performance structure
struct CqasFlowPerf_t
{
    Time flowStart;                       ///< flow start time
    unsigned long totalBytesTransmitted;  ///< Total bytes send by eNb for this UE
    unsigned int lastTtiBytesTransmitted; ///< Total bytes send by eNB in last tti for this UE
    double lastAveragedThroughput;        ///< Past average throughput
    double secondLastAveragedThroughput;  ///< Second last average throughput
    double targetThroughput;              ///< Target throughput
};

/**
 * @ingroup ff-api
 * @brief Implements the SCHED SAP and CSCHED SAP for the Channel and QoS Aware Scheduler
 *
 * This class implements the interface defined by the FfMacScheduler abstract class
 */

class CqaFfMacScheduler : public FfMacScheduler
{
  public:
    /**
     * @brief Constructor
     *
     * Creates the MAC Scheduler interface implementation
     */
    CqaFfMacScheduler();

    /**
     * Destructor
     */
    ~CqaFfMacScheduler() override;

    // inherited from Object
    void DoDispose() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from FfMacScheduler
    void SetFfMacCschedSapUser(FfMacCschedSapUser* s) override;
    void SetFfMacSchedSapUser(FfMacSchedSapUser* s) override;
    FfMacCschedSapProvider* GetFfMacCschedSapProvider() override;
    FfMacSchedSapProvider* GetFfMacSchedSapProvider() override;

    // FFR SAPs
    void SetLteFfrSapProvider(LteFfrSapProvider* s) override;
    LteFfrSapUser* GetLteFfrSapUser() override;

    /// allow MemberCschedSapProvider<CqaFfMacScheduler> class friend access
    friend class MemberCschedSapProvider<CqaFfMacScheduler>;
    /// allow MemberSchedSapProvider<CqaFfMacScheduler> class friend access
    friend class MemberSchedSapProvider<CqaFfMacScheduler>;

    /**
     * Trans mode config update
     * @param rnti the RNTI
     * @param txMode the transmit mode
     */
    void TransmissionModeConfigurationUpdate(uint16_t rnti, uint8_t txMode);

  private:
    //
    // Implementation of the CSCHED API primitives
    // (See 4.1 for description of the primitives)
    //

    /**
     * Csched Cell Config Request
     * @param params CschedCellConfigReqParameters&
     */
    void DoCschedCellConfigReq(const FfMacCschedSapProvider::CschedCellConfigReqParameters& params);

    /**
     * Csched UE Config Request
     * @param params CschedUeConfigReqParameters&
     */
    void DoCschedUeConfigReq(const FfMacCschedSapProvider::CschedUeConfigReqParameters& params);

    /**
     * Csched LC Config Request
     * @param params CschedLcConfigReqParameters&
     */
    void DoCschedLcConfigReq(const FfMacCschedSapProvider::CschedLcConfigReqParameters& params);

    /**
     * Csched LC Release Request
     * @param params CschedLcReleaseReqParameters&
     */
    void DoCschedLcReleaseReq(const FfMacCschedSapProvider::CschedLcReleaseReqParameters& params);

    /**
     * Csched UE Release Request
     * @param params CschedUeReleaseReqParameters&
     */
    void DoCschedUeReleaseReq(const FfMacCschedSapProvider::CschedUeReleaseReqParameters& params);

    //
    // Implementation of the SCHED API primitives
    // (See 4.2 for description of the primitives)
    //

    /**
     * Sched DL RLC Buffer Request
     * @param params SchedDlRlcBufferReqParameters&
     */
    void DoSchedDlRlcBufferReq(const FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params);

    /**
     * Sched DL Paging Buffer Request
     * @param params SchedDlPagingBufferReqParameters&
     */
    void DoSchedDlPagingBufferReq(
        const FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params);

    /**
     * Sched DL MAC Buffer Request
     * @param params SchedDlMacBufferReqParameters&
     */
    void DoSchedDlMacBufferReq(const FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params);

    /**
     * Sched DL RLC Buffer Request
     * @param params SchedDlTriggerReqParameters&
     */
    void DoSchedDlTriggerReq(const FfMacSchedSapProvider::SchedDlTriggerReqParameters& params);

    /**
     * Sched DL RACH Info Request
     * @param params SchedDlRachInfoReqParameters&
     */
    void DoSchedDlRachInfoReq(const FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params);

    /**
     * Sched DL CGI Info Request
     * @param params SchedDlCqiInfoReqParameters&
     */
    void DoSchedDlCqiInfoReq(const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params);

    /**
     * Sched UL Trigger Request
     * @param params SchedUlTriggerReqParameters&
     */
    void DoSchedUlTriggerReq(const FfMacSchedSapProvider::SchedUlTriggerReqParameters& params);

    /**
     * Sched UL Noise InterferenceRequest
     * @param params SchedUlNoiseInterferenceReqParameters&
     */
    void DoSchedUlNoiseInterferenceReq(
        const FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params);

    /**
     * Sched UL Sr Info Request
     * @param params SchedUlSrInfoReqParameters&
     */
    void DoSchedUlSrInfoReq(const FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params);

    /**
     * Sched UL MAC Control Info Request
     * @param params SchedUlMacCtrlInfoReqParameters&
     */
    void DoSchedUlMacCtrlInfoReq(
        const FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params);

    /**
     * Sched UL CGI Info Request
     * @param params SchedUlCqiInfoReqParameters&
     */
    void DoSchedUlCqiInfoReq(const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params);

    /**
     * Get RBG Size
     * @param dlbandwidth the DL bandwidth
     * @returns the size
     */
    int GetRbgSize(int dlbandwidth);

    /**
     * LC Active per flow
     * @param rnti the RNTI
     * @returns the LC active per flow
     */
    unsigned int LcActivePerFlow(uint16_t rnti);

    /**
     * Estimate UL Sinr
     * @param rnti the RNTI
     * @param rb the RB
     * @returns the UL SINR
     */
    double EstimateUlSinr(uint16_t rnti, uint16_t rb);

    /// Refresh DL CGI maps
    void RefreshDlCqiMaps();
    /// Refresh UL CGI maps
    void RefreshUlCqiMaps();

    /**
     * Update DL RLC buffer info
     * @param rnti the RNTI
     * @param lcid the LCID
     * @param size the size
     */
    void UpdateDlRlcBufferInfo(uint16_t rnti, uint8_t lcid, uint16_t size);
    /**
     * Update UL RLC buffer info
     * @param rnti the RNTI
     * @param size the size
     */
    void UpdateUlRlcBufferInfo(uint16_t rnti, uint16_t size);

    /**
     * @brief Update and return a new process Id for the RNTI specified
     *
     * @param rnti the RNTI of the UE to be updated
     * @return the process id  value
     */
    uint8_t UpdateHarqProcessId(uint16_t rnti);

    /**
     * @brief Return the availability of free process for the RNTI specified
     *
     * @param rnti the RNTI of the UE to be updated
     * @return the availability
     */
    bool HarqProcessAvailability(uint16_t rnti);

    /**
     * @brief Refresh HARQ processes according to the timers
     *
     */
    void RefreshHarqProcesses();

    Ptr<LteAmc> m_amc; ///< LTE AMC object

    /**
     * Vectors of UE's LC info
     */
    std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> m_rlcBufferReq;

    /**
     * Map of UE statistics (per RNTI basis) in downlink
     */
    std::map<uint16_t, CqasFlowPerf_t> m_flowStatsDl;

    /**
     * Map of UE statistics (per RNTI basis)
     */
    std::map<uint16_t, CqasFlowPerf_t> m_flowStatsUl;

    /**
     * Map of UE logical channel config list
     */
    std::map<LteFlowId_t, LogicalChannelConfigListElement_s> m_ueLogicalChannelsConfigList;

    /**
     * Map of UE's DL CQI P01 received
     */
    std::map<uint16_t, uint8_t> m_p10CqiRxed;

    /**
     * Map of UE's timers on DL CQI P01 received
     */
    std::map<uint16_t, uint32_t> m_p10CqiTimers;

    /**
     * Map of UE's DL CQI A30 received
     */
    std::map<uint16_t, SbMeasResult_s> m_a30CqiRxed;

    /**
     * Map of UE's timers on DL CQI A30 received
     */
    std::map<uint16_t, uint32_t> m_a30CqiTimers;

    /**
     * Map of previous allocated UE per RBG
     * (used to retrieve info from UL-CQI)
     */
    std::map<uint16_t, std::vector<uint16_t>> m_allocationMaps;

    /**
     * Map of UEs' UL-CQI per RBG
     */
    std::map<uint16_t, std::vector<double>> m_ueCqi;

    /**
     * Map of UEs' timers on UL-CQI per RBG
     */
    std::map<uint16_t, uint32_t> m_ueCqiTimers;

    /**
     * Map of UE's buffer status reports received
     */
    std::map<uint16_t, uint32_t> m_ceBsrRxed;

    // MAC SAPs
    FfMacCschedSapUser* m_cschedSapUser;         ///< MAC Csched SAP user
    FfMacSchedSapUser* m_schedSapUser;           ///< MAC Sched SAP user
    FfMacCschedSapProvider* m_cschedSapProvider; ///< Csched SAP provider
    FfMacSchedSapProvider* m_schedSapProvider;   ///< Sched SAP provider

    // FFR SAPs
    LteFfrSapUser* m_ffrSapUser;         ///< FFR SAP user
    LteFfrSapProvider* m_ffrSapProvider; ///< FFR SAP provider

    /// Internal parameters
    FfMacCschedSapProvider::CschedCellConfigReqParameters m_cschedCellConfig;

    double m_timeWindow; ///< time window

    uint16_t m_nextRntiUl; ///< RNTI of the next user to be served next scheduling in UL

    uint32_t m_cqiTimersThreshold; ///< # of TTIs for which a CQI can be considered valid

    std::map<uint16_t, uint8_t> m_uesTxMode; ///< txMode of the UEs

    // HARQ attributes
    bool m_harqOn; ///< m_harqOn when false inhibit the HARQ mechanisms (by default active)
    std::map<uint16_t, uint8_t> m_dlHarqCurrentProcessId; ///< DL HARQ process ID
    // HARQ status
    //  0: process Id available
    //  x>0: process Id equal to `x` transmission count
    std::map<uint16_t, DlHarqProcessesStatus_t>
        m_dlHarqProcessesStatus;                                       ///< DL HARQ process statuses
    std::map<uint16_t, DlHarqProcessesTimer_t> m_dlHarqProcessesTimer; ///< DL HARQ process timers
    std::map<uint16_t, DlHarqProcessesDciBuffer_t>
        m_dlHarqProcessesDciBuffer; ///< DL HARQ process DCI buffer
    std::map<uint16_t, DlHarqRlcPduListBuffer_t>
        m_dlHarqProcessesRlcPduListBuffer;                 ///< DL HARQ process RLC PDU list buffer
    std::vector<DlInfoListElement_s> m_dlInfoListBuffered; ///< DL HARQ retx buffered

    std::map<uint16_t, uint8_t> m_ulHarqCurrentProcessId; ///< UL HARQ current process ID
    // HARQ status
    //  0: process Id available
    //  x>0: process Id equal to `x` transmission count
    std::map<uint16_t, UlHarqProcessesStatus_t> m_ulHarqProcessesStatus; ///< UL HARQ process status
    std::map<uint16_t, UlHarqProcessesDciBuffer_t>
        m_ulHarqProcessesDciBuffer; ///< UL HARQ process DCI buffer

    // RACH attributes
    std::vector<RachListElement_s> m_rachList; ///< RACH list
    std::vector<uint16_t> m_rachAllocationMap; ///< RACH allocation map
    uint8_t m_ulGrantMcs;                      ///< MCS for UL grant (default 0)

    std::string m_CqaMetric; ///< CQA metric name
};

} // namespace ns3

#endif /* QOS_FF_MAC_SCHEDULER_H */
