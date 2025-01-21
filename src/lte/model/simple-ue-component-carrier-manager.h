/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 *
 */

#ifndef SIMPLE_UE_COMPONENT_CARRIER_MANAGER_H
#define SIMPLE_UE_COMPONENT_CARRIER_MANAGER_H

#include "lte-rrc-sap.h"
#include "lte-ue-ccm-rrc-sap.h"
#include "lte-ue-component-carrier-manager.h"

#include <map>

namespace ns3
{
class LteUeCcmRrcSapProvider;

/**
 * @brief Component carrier manager implementation which simply does nothing.
 *
 * Selecting this component carrier selection algorithm is equivalent to disabling automatic
 * triggering of component carrier selection. This is the default choice.
 *
 */
class SimpleUeComponentCarrierManager : public LteUeComponentCarrierManager
{
  public:
    /// Creates a No-op CCS algorithm instance.
    SimpleUeComponentCarrierManager();

    ~SimpleUeComponentCarrierManager() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from LteComponentCarrierManager
    LteMacSapProvider* GetLteMacSapProvider() override;

    /// let the forwarder class access the protected and private members
    friend class MemberLteUeCcmRrcSapProvider<SimpleUeComponentCarrierManager>;
    // friend class MemberLteUeCcmRrcSapUser<SimpleUeComponentCarrierManager>;

    /// allow SimpleUeCcmMacSapProvider class friend access
    friend class SimpleUeCcmMacSapProvider;
    /// allow SimpleUeCcmMacSapUser class friend access
    friend class SimpleUeCcmMacSapUser;

  protected:
    // inherited from Object
    void DoInitialize() override;
    void DoDispose() override;
    // inherited from LteCcsAlgorithm as a Component Carrier Management SAP implementation
    /**
     * @brief Report Ue Measure function
     * @param rnti the RNTI
     * @param measResults the measure results
     */
    void DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults);
    // forwarded from LteMacSapProvider
    /**
     * @brief Transmit PDU function
     * @param params LteMacSapProvider::TransmitPduParameters
     */
    void DoTransmitPdu(LteMacSapProvider::TransmitPduParameters params);
    /**
     * @brief Report buffer status function
     * @param params LteMacSapProvider::ReportBufferStatusParameters
     */
    virtual void DoReportBufferStatus(LteMacSapProvider::ReportBufferStatusParameters params);
    /// Notify HARQ deliver failure
    void DoNotifyHarqDeliveryFailure();
    // forwarded from LteMacSapUser
    /**
     * @brief Notify TX opportunity function
     *
     * @param txOpParams the LteMacSapUser::TxOpportunityParameters
     */
    void DoNotifyTxOpportunity(LteMacSapUser::TxOpportunityParameters txOpParams);
    /**
     * @brief Receive PDU function
     *
     * @param rxPduParams the LteMacSapUser::ReceivePduParameters
     */
    void DoReceivePdu(LteMacSapUser::ReceivePduParameters rxPduParams);
    // forwarded from LteUeCcmRrcSapProvider
    /**
     * @brief Add LC function
     * @param lcId the LCID
     * @param lcConfig the logical channel config
     * @param msu the MSU
     * @returns updated LC config list
     */
    virtual std::vector<LteUeCcmRrcSapProvider::LcsConfig> DoAddLc(
        uint8_t lcId,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu);
    /**
     * @brief Remove LC function
     * @param lcid the LCID
     * @returns updated LC list
     */
    std::vector<uint16_t> DoRemoveLc(uint8_t lcid);
    /**
     * @brief Configure signal bearer function
     * @param lcId the LCID
     * @param lcConfig the logical channel config
     * @param msu the MSU
     * @returns LteMacSapUser *
     */
    virtual LteMacSapUser* DoConfigureSignalBearer(
        uint8_t lcId,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu);
    /**
     * @brief Reset LC map
     *
     */
    void DoReset();

  protected:
    LteMacSapUser* m_ccmMacSapUser;         //!< Interface to the UE RLC instance.
    LteMacSapProvider* m_ccmMacSapProvider; //!< Receive API calls from the UE RLC instance
};

} // end of namespace ns3

#endif /* SIMPLE_UE_COMPONENT_CARRIER_MANAGER_H */
