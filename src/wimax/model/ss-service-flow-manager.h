/*
 * Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *  Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#ifndef SS_SERVICE_FLOW_MANAGER_H
#define SS_SERVICE_FLOW_MANAGER_H

#include "mac-messages.h"
#include "service-flow-manager.h"
#include "ss-net-device.h"

#include "ns3/buffer.h"
#include "ns3/event-id.h"

#include <stdint.h>

namespace ns3
{

class Packet;
class ServiceFlow;
class WimaxNetDevice;
class WimaxConnection;
class SubscriberStationNetDevice;

/**
 * @ingroup wimax
 * @brief SsServiceFlowManager class
 */
class SsServiceFlowManager : public ServiceFlowManager
{
  public:
    /// Confirmation code enumeration
    enum ConfirmationCode // as per Table 384 (not all codes implemented)
    {
        CONFIRMATION_CODE_SUCCESS,
        CONFIRMATION_CODE_REJECT
    };

    /**
     * Constructor
     *
     * Creates a service flow manager and attaches it to a device
     *
     * @param device the device to which the service flow manager will be attached
     */
    SsServiceFlowManager(Ptr<SubscriberStationNetDevice> device);
    ~SsServiceFlowManager() override;
    void DoDispose() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief add a service flow to the list
     * @param serviceFlow the service flow to add
     */
    void AddServiceFlow(ServiceFlow* serviceFlow);
    /**
     * @brief add a service flow to the list
     * @param serviceFlow the service flow to add
     */
    void AddServiceFlow(ServiceFlow serviceFlow);
    /**
     * @brief sets the maximum retries on DSA request message
     * @param maxDsaReqRetries the maximum retries on DSA request message
     */
    void SetMaxDsaReqRetries(uint8_t maxDsaReqRetries);
    /**
     * @return the maximum retries on DSA request message
     */
    uint8_t GetMaxDsaReqRetries() const;

    /**
     * Get DSA response timeout event
     * @returns the DSA response timeout event
     */
    EventId GetDsaRspTimeoutEvent() const;
    /**
     * Get DSA ack timeout event
     * @returns the DSA ack timeout event
     */
    EventId GetDsaAckTimeoutEvent() const;

    /// Initiate service flows
    void InitiateServiceFlows();

    /**
     * Create DSA request
     * @param serviceFlow the service flow
     * @returns the DSA request
     */
    DsaReq CreateDsaReq(const ServiceFlow* serviceFlow);

    /**
     * Create DSA ack
     * @returns the packet
     */
    Ptr<Packet> CreateDsaAck();

    /**
     * Schedule DSA response
     * @param serviceFlow the service flow
     */
    void ScheduleDsaReq(const ServiceFlow* serviceFlow);

    /**
     * Process DSA response
     * @param dsaRsp the DSA response
     */
    void ProcessDsaRsp(const DsaRsp& dsaRsp);

  private:
    Ptr<SubscriberStationNetDevice> m_device; ///< the device

    uint8_t m_maxDsaReqRetries; ///< maximum DSA request retries

    EventId m_dsaRspTimeoutEvent; ///< DSA response timeout event
    EventId m_dsaAckTimeoutEvent; ///< DSA ack timeout event

    DsaReq m_dsaReq; ///< DSA request
    DsaAck m_dsaAck; ///< DSA ack

    uint16_t m_currentTransactionId; ///< current transaction ID
    uint16_t m_transactionIdIndex;   ///< transaction ID index
    uint8_t m_dsaReqRetries;         ///< DSA request retries

    // pointer to the service flow currently being configured
    ServiceFlow* m_pendingServiceFlow; ///< pending service flow
};

} // namespace ns3

#endif /* SS_SERVICE_FLOW_MANAGER_H */
