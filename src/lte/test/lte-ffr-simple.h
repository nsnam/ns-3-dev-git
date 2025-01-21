/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_FFR_SIMPLE_H
#define LTE_FFR_SIMPLE_H

#include "ns3/lte-ffr-algorithm.h"
#include "ns3/lte-ffr-rrc-sap.h"
#include "ns3/lte-ffr-sap.h"
#include "ns3/lte-rrc-sap.h"
#include "ns3/traced-callback.h"

#include <map>

namespace ns3
{

/**
 * @ingroup lte-test
 *
 * @brief Simple Frequency Reuse algorithm implementation which uses only 1 sub-band.
 *                Used to test Downlink Power Allocation. When Simple FR receives UE measurements
 *                it immediately call functions to change PdschConfigDedicated (i.e. P_A) value for
 *                this UE.
 */
class LteFfrSimple : public LteFfrAlgorithm
{
  public:
    /**
     * @brief Creates a trivial ffr algorithm instance.
     */
    LteFfrSimple();

    ~LteFfrSimple() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Callback function that is used to be connected to trace ChangePdschConfigDedicated
     * @param change trace fired upon change of PdschConfigDedicated if true
     */
    void ChangePdschConfigDedicated(bool change);
    /**
     * @brief Set PDSCH config dedicated function
     * @param pdschConfigDedicated LteRrcSap::PdschConfigDedicated object
     */
    void SetPdschConfigDedicated(LteRrcSap::PdschConfigDedicated pdschConfigDedicated);

    /**
     * @brief Set transmission power control
     * @param tpc TPC
     * @param num number of TPC configurations in the test case
     * @param accumulatedMode whether TPC accumulated mode is used
     */
    void SetTpc(uint32_t tpc, uint32_t num, bool accumulatedMode);

    // inherited from LteFfrAlgorithm
    void SetLteFfrSapUser(LteFfrSapUser* s) override;
    LteFfrSapProvider* GetLteFfrSapProvider() override;

    void SetLteFfrRrcSapUser(LteFfrRrcSapUser* s) override;
    LteFfrRrcSapProvider* GetLteFfrRrcSapProvider() override;

    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrSapProvider<LteFfrSimple>;
    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrRrcSapProvider<LteFfrSimple>;

    /**
     * TracedCallback signature for change of PdschConfigDedicated.
     *
     * @param [in] rnti
     * @param [in] pdschPa PdschConfiDedicated.pa
     */
    typedef void (*PdschTracedCallback)(uint16_t rnti, uint8_t pdschPa);

  protected:
    // inherited from Object
    void DoInitialize() override;
    void DoDispose() override;

    void Reconfigure() override;

    // FFR SAP PROVIDER IMPLEMENTATION
    std::vector<bool> DoGetAvailableDlRbg() override;
    bool DoIsDlRbgAvailableForUe(int i, uint16_t rnti) override;
    std::vector<bool> DoGetAvailableUlRbg() override;
    bool DoIsUlRbgAvailableForUe(int i, uint16_t rnti) override;
    void DoReportDlCqiInfo(
        const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params) override;
    void DoReportUlCqiInfo(
        const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params) override;
    void DoReportUlCqiInfo(std::map<uint16_t, std::vector<double>> ulCqiMap) override;
    uint8_t DoGetTpc(uint16_t rnti) override;
    uint16_t DoGetMinContinuousUlBandwidth() override;

    // FFR SAP RRC PROVIDER IMPLEMENTATION
    void DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults) override;
    void DoRecvLoadInformation(EpcX2Sap::LoadInformationParams params) override;

  private:
    /// Update PDSCH config dedicated function
    void UpdatePdschConfigDedicated();

    // FFR SAP
    LteFfrSapUser* m_ffrSapUser;         ///< FFR SAP user
    LteFfrSapProvider* m_ffrSapProvider; ///< FFR SAP provider

    // FFR RRF SAP
    LteFfrRrcSapUser* m_ffrRrcSapUser;         ///< FFR RRC SAP user
    LteFfrRrcSapProvider* m_ffrRrcSapProvider; ///< FFR RRC SAP provider

    uint8_t m_dlOffset;  ///< DL offset
    uint8_t m_dlSubBand; ///< DL subband

    uint8_t m_ulOffset;  ///< UL offset
    uint8_t m_ulSubBand; ///< UL subband

    std::vector<bool> m_dlRbgMap; ///< DL RBG map
    std::vector<bool> m_ulRbgMap; ///< UL RBG map

    std::map<uint16_t, LteRrcSap::PdschConfigDedicated> m_ues; ///< UEs

    // The expected measurement identity
    uint8_t m_measId; ///< measure ID

    bool m_changePdschConfigDedicated; ///< PDSCH config dedicate changed?

    LteRrcSap::PdschConfigDedicated m_pdschConfigDedicated; ///< PDSCH config dedicated

    TracedCallback<uint16_t, uint8_t>
        m_changePdschConfigDedicatedTrace; ///< PDSCH config dedicated change trace callback

    // Uplink Power Control
    uint32_t m_tpc;         ///< transmission power control to be used
    uint32_t m_tpcNum;      ///< number of TPC configurations
    bool m_accumulatedMode; ///< whether to use the TPC accumulated mode
};

} // end of namespace ns3

#endif /* LTE_FFR_SIMPLE_H */
