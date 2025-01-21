/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_FR_NO_OP_ALGORITHM_H
#define LTE_FR_NO_OP_ALGORITHM_H

#include "lte-ffr-algorithm.h"
#include "lte-ffr-rrc-sap.h"
#include "lte-ffr-sap.h"
#include "lte-rrc-sap.h"

namespace ns3
{

/**
 * @brief FR algorithm implementation which simply does nothing.
 *
 * Selecting this FR algorithm is equivalent to disabling FFR.
 * This is the default choice.
 *
 * To enable FR, please select another FR algorithm, i.e.,
 * another child class of LteFfrAlgorithm.
 */
class LteFrNoOpAlgorithm : public LteFfrAlgorithm
{
  public:
    /**
     * @brief Creates a NoOP FR algorithm instance.
     */
    LteFrNoOpAlgorithm();

    ~LteFrNoOpAlgorithm() override;

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
    friend class MemberLteFfrSapProvider<LteFrNoOpAlgorithm>;
    /// let the forwarder class access the protected and private members
    friend class MemberLteFfrRrcSapProvider<LteFrNoOpAlgorithm>;

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
    // FFR SAP
    LteFfrSapUser* m_ffrSapUser;         ///< FFR SAP user
    LteFfrSapProvider* m_ffrSapProvider; ///< FFR SAP provider

    // FFR RRF SAP
    LteFfrRrcSapUser* m_ffrRrcSapUser;         ///< FFR RRC SAP user
    LteFfrRrcSapProvider* m_ffrRrcSapProvider; ///< FFR RRC SAP provider
};

} // end of namespace ns3

#endif /* LTE_FFR_NO_OP_ALGORITHM_H */
