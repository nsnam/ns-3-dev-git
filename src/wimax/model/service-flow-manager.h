/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#ifndef SERVICE_FLOW_MANAGER_H
#define SERVICE_FLOW_MANAGER_H

#include "mac-messages.h"

#include "ns3/buffer.h"
#include "ns3/event-id.h"

#include <stdint.h>

namespace ns3
{

class Packet;
class ServiceFlow;
class WimaxNetDevice;
class SSRecord;
class WimaxConnection;

/**
 * @ingroup wimax
 * The same service flow manager class serves both for BS and SS though some functions are exclusive
 * to only one of them.
 */
class ServiceFlowManager : public Object
{
  public:
    /// confirmation code enumeration as per Table 384 (not all codes implemented)
    enum ConfirmationCode
    {
        CONFIRMATION_CODE_SUCCESS,
        CONFIRMATION_CODE_REJECT
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ServiceFlowManager();
    ~ServiceFlowManager() override;
    void DoDispose() override;

    /**
     * Add service flow function
     * @param serviceFlow the service flow
     */
    void AddServiceFlow(ServiceFlow* serviceFlow);
    /**
     * Get service flow by flow id
     * @param sfid the service flow id
     * @returns pointer to the service flow object corresponding to the flow id
     */
    ServiceFlow* GetServiceFlow(uint32_t sfid) const;
    /**
     * Get service flow by CID
     * @param cid the CID
     * @returns pointer to the service flow object corresponding to the CID
     */
    ServiceFlow* GetServiceFlow(Cid cid) const;
    /**
     * Get service flows function
     * @param schedulingType the scheduling type
     * @returns vector of pointers to service flows corresponding to the scheduling type
     */
    std::vector<ServiceFlow*> GetServiceFlows(ServiceFlow::SchedulingType schedulingType) const;

    /**
     * @return true if all service flows are allocated, false otherwise
     */
    bool AreServiceFlowsAllocated();
    /**
     * @param  serviceFlows vector of pointers to service flows to be checked
     * @return true if all service flows are allocated, false otherwise
     */
    bool AreServiceFlowsAllocated(std::vector<ServiceFlow*>* serviceFlows);
    /**
     * @param  serviceFlows vector of pointers to service flows to be checked
     * @return true if all service flows are allocated, false otherwise
     */
    bool AreServiceFlowsAllocated(std::vector<ServiceFlow*> serviceFlows);
    /**
     * @return pointer to the next service flow to be allocated
     */
    ServiceFlow* GetNextServiceFlowToAllocate();

    /**
     * @return the number of all service flows
     */
    uint32_t GetNrServiceFlows() const;

    /**
     * @param SrcAddress the source ip address
     * @param DstAddress the destination ip address
     * @param SrcPort the source port
     * @param DstPort the destination port
     * @param Proto the protocol
     * @param dir the direction of the service flow
     * @return the service flow to which this ip flow is associated
     */
    ServiceFlow* DoClassify(Ipv4Address SrcAddress,
                            Ipv4Address DstAddress,
                            uint16_t SrcPort,
                            uint16_t DstPort,
                            uint8_t Proto,
                            ServiceFlow::Direction dir) const;

  private:
    std::vector<ServiceFlow*>* m_serviceFlows; ///< the service flows
};

} // namespace ns3

#endif /* SERVICE_FLOW_MANAGER_H */
