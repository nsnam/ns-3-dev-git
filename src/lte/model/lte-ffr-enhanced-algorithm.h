/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_FFR_ENHANCED_ALGORITHM_H
#define LTE_FFR_ENHANCED_ALGORITHM_H

#include "lte-ffr-algorithm.h"
#include "lte-ffr-rrc-sap.h"
#include "lte-ffr-sap.h"
#include "lte-rrc-sap.h"

#include <map>

// value for SINR outside the range defined by FF-API, used to indicate that there
// is no CQI for this element
#define NO_SINR -5000

namespace ns3
{

/**
 * @brief Enhanced Fractional Frequency Reuse algorithm implementation
 */
class LteFfrEnhancedAlgorithm : public LteFfrAlgorithm
{
  public:
    /**
     * @brief Creates a trivial ffr algorithm instance.
     */
    LteFfrEnhancedAlgorithm();
    ~LteFfrEnhancedAlgorithm() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from LteFfrAlgorithm
    void SetLteFfrSapUser(LteFfrSapUser* s) override;
    LteFfrSapProvider* GetLteFfrSapProvider() override;

    void SetLteFfrRrcSapUser(LteFfrRrcSapUser* s) override;
    LteFfrRrcSapProvider* GetLteFfrRrcSapProvider() override;

    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrSapProvider<LteFfrEnhancedAlgorithm>;
    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrRrcSapProvider<LteFfrEnhancedAlgorithm>;

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
    /**
     * Set downlink configuration
     *
     * @param cellId the cell ID
     * @param bandwidth the bandwidth
     */
    void SetDownlinkConfiguration(uint16_t cellId, uint8_t bandwidth);
    /**
     * Set uplink configuration
     *
     * @param cellId the cell ID
     * @param bandwidth the bandwidth
     */
    void SetUplinkConfiguration(uint16_t cellId, uint8_t bandwidth);
    /**
     * Initialize downlink RBG maps
     */
    void InitializeDownlinkRbgMaps();
    /**
     * Initialize uplink RBG maps
     */
    void InitializeUplinkRbgMaps();

    /**
     * Initialize uplink RBG maps
     *
     * @param rnti the RNTI
     * @param rb RB
     * @param ulCqiMap UL CQI map
     * @returns UL SINR
     */
    double EstimateUlSinr(uint16_t rnti,
                          uint16_t rb,
                          std::map<uint16_t, std::vector<double>> ulCqiMap);
    /**
     * Get CQI from spectral efficiency
     *
     * @param s spectral efficiency
     * @returns CQI
     */
    int GetCqiFromSpectralEfficiency(double s);

    // FFR SAP
    LteFfrSapUser* m_ffrSapUser;         ///< FFR SAP user
    LteFfrSapProvider* m_ffrSapProvider; ///< FFR SAP provider

    // FFR RRF SAP
    LteFfrRrcSapUser* m_ffrRrcSapUser;         ///< FFR RRC SAP user
    LteFfrRrcSapProvider* m_ffrRrcSapProvider; ///< FFR RRC SAP provider

    uint8_t m_dlSubBandOffset;      ///< DL subband offset
    uint8_t m_dlReuse3SubBandwidth; ///< DL reuse 3 subband bandwidth
    uint8_t m_dlReuse1SubBandwidth; ///< DL reuse 1 subband bandwidth

    uint8_t m_ulSubBandOffset;      ///< UL subband offset
    uint8_t m_ulReuse3SubBandwidth; ///< UL reuse 3 subbandwidth
    uint8_t m_ulReuse1SubBandwidth; ///< UL reuse 1 subbandwidth

    std::vector<bool> m_dlRbgMap; ///< DL RBG map
    std::vector<bool> m_ulRbgMap; ///< UL RBG Map

    std::vector<bool> m_dlReuse3RbgMap;           ///< DL reuse 3 RBG map
    std::vector<bool> m_dlReuse1RbgMap;           ///< DL reuse 1 RBG map
    std::vector<bool> m_dlPrimarySegmentRbgMap;   ///< DL primary segment RBG map
    std::vector<bool> m_dlSecondarySegmentRbgMap; ///< DL secondary segment RBG map

    std::vector<bool> m_ulReuse3RbgMap;           ///< UL reuse 3 RBG map
    std::vector<bool> m_ulReuse1RbgMap;           ///< UL reuse 1 RBG map
    std::vector<bool> m_ulPrimarySegmentRbgMap;   ///< UL primary segment RBG map
    std::vector<bool> m_ulSecondarySegmentRbgMap; ///< UL secondary segment RBG map

    /// UE Position enumeration
    enum UePosition
    {
        AreaUnset,
        CenterArea,
        EdgeArea
    };

    std::map<uint16_t, uint8_t> m_ues; ///< UEs

    uint8_t m_rsrqThreshold; ///< RSRQ threshold

    uint8_t m_centerAreaPowerOffset; ///< Center area power offset
    uint8_t m_edgeAreaPowerOffset;   ///< Edge area power offset

    uint8_t m_centerAreaTpc; ///< Center area TPC
    uint8_t m_edgeAreaTpc;   ///< Edge are TPC

    uint8_t m_dlCqiThreshold; ///< DL CQI threshold
    /**
     * Map of UE's DL CQI A30 received
     */
    std::map<uint16_t, SbMeasResult_s> m_dlCqi;
    std::map<uint16_t, std::vector<bool>> m_dlRbgAvailableforUe; ///< DL RBG available for UE

    uint8_t m_ulCqiThreshold;                                   ///< UL CQI threshold
    std::map<uint16_t, std::vector<int>> m_ulCqi;               ///< UL CQI
    std::map<uint16_t, std::vector<bool>> m_ulRbAvailableforUe; ///< UL RB available for UE

    /// The expected measurement identity
    uint8_t m_measId;
};

} // end of namespace ns3

#endif /* LTE_FFR_ENHANCED_ALGORITHM_H */
