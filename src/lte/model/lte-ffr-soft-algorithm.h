/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_FFR_SOFT_ALGORITHM_H
#define LTE_FFR_SOFT_ALGORITHM_H

#include "lte-ffr-algorithm.h"
#include "lte-ffr-rrc-sap.h"
#include "lte-ffr-sap.h"
#include "lte-rrc-sap.h"

#include <map>

namespace ns3
{

/**
 * @brief Soft Fractional Frequency Reuse algorithm implementation
 */
class LteFfrSoftAlgorithm : public LteFfrAlgorithm
{
  public:
    /**
     * @brief Creates a trivial ffr algorithm instance.
     */
    LteFfrSoftAlgorithm();

    ~LteFfrSoftAlgorithm() override;

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
    friend class MemberLteFfrSapProvider<LteFfrSoftAlgorithm>;
    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrRrcSapProvider<LteFfrSoftAlgorithm>;

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
     * Set downlink configuration function
     *
     * @param cellId the cell ID
     * @param bandwidth the bandwidth
     */
    void SetDownlinkConfiguration(uint16_t cellId, uint8_t bandwidth);
    /**
     * Set uplink configuration function
     *
     * @param cellId the cell ID
     * @param bandwidth the bandwidth
     */
    void SetUplinkConfiguration(uint16_t cellId, uint8_t bandwidth);
    /**
     * Initialize downlink RBG maps function
     */
    void InitializeDownlinkRbgMaps();
    /**
     * Initialize uplink RBG maps function
     */
    void InitializeUplinkRbgMaps();

    // FFR SAP
    LteFfrSapUser* m_ffrSapUser;         ///< FFR SAP user
    LteFfrSapProvider* m_ffrSapProvider; ///< FFR SAP provider

    // FFR RRF SAP
    LteFfrRrcSapUser* m_ffrRrcSapUser;         ///< FFR RRC SAP user
    LteFfrRrcSapProvider* m_ffrRrcSapProvider; ///< FFR RRC SAP provider

    uint8_t m_dlCommonSubBandwidth; ///< DL common subbandwidth
    uint8_t m_dlEdgeSubBandOffset;  ///< DL edge subband offset
    uint8_t m_dlEdgeSubBandwidth;   ///< DL edge subbandwidth

    uint8_t m_ulCommonSubBandwidth; ///< UL common subbandwidth
    uint8_t m_ulEdgeSubBandOffset;  ///< UL edge subband offset
    uint8_t m_ulEdgeSubBandwidth;   ///< UL edge subbandwidth

    std::vector<bool> m_dlRbgMap; ///< DL RBG Map
    std::vector<bool> m_ulRbgMap; ///< UL RBG map

    std::vector<bool> m_dlCenterRbgMap; ///< DL center RBG map
    std::vector<bool> m_ulCenterRbgMap; ///< UL center RBG map

    std::vector<bool> m_dlMediumRbgMap; ///< DL medium RBG map
    std::vector<bool> m_ulMediumRbgMap; ///< UL medium RBG map

    std::vector<bool> m_dlEdgeRbgMap; ///< DL edge RBG map
    std::vector<bool> m_ulEdgeRbgMap; ///< UL edge RBG map

    /// UE position enumeration
    enum UePosition
    {
        AreaUnset,
        CenterArea,
        MediumArea,
        EdgeArea
    };

    std::map<uint16_t, uint8_t> m_ues; ///< UEs

    uint8_t m_centerSubBandThreshold; ///< center subband threshold
    uint8_t m_edgeSubBandThreshold;   ///< edge subband threshold

    uint8_t m_centerAreaPowerOffset; ///< center area power offset
    uint8_t m_mediumAreaPowerOffset; ///< medium area power offset
    uint8_t m_edgeAreaPowerOffset;   ///< edge area power offset

    uint8_t m_centerAreaTpc; ///< center area tpc
    uint8_t m_mediumAreaTpc; ///< medium area tpc
    uint8_t m_edgeAreaTpc;   ///< edge area tpc

    /// The expected measurement identity
    uint8_t m_measId;
};

} // end of namespace ns3

#endif /* LTE_FFR_SOFT_ALGORITHM_H */
