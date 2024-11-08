/*
 *  Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *
 */

#ifndef IPCS_CLASSIFIER_H
#define IPCS_CLASSIFIER_H

#include "ss-service-flow-manager.h"

#include "ns3/packet.h"
#include "ns3/ptr.h"

#include <stdint.h>
#include <vector>

namespace ns3
{
class SsServiceFlowManager;

/**
 * @ingroup wimax
 *
 * IPCS classifier
 */
class IpcsClassifier : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    IpcsClassifier();
    ~IpcsClassifier() override;
    /**
     * @brief classify a packet in a service flow
     * @param packet the packet to classify
     * @param sfm the service flow manager to be used to classify packets
     * @param dir The direction on which the packet should be sent (UP or DOWN)
     * @return The service flow that should be used to send this packet
     */
    ServiceFlow* Classify(Ptr<const Packet> packet,
                          Ptr<ServiceFlowManager> sfm,
                          ServiceFlow::Direction dir);
};
} // namespace ns3

#endif /* IPCS_CLASSIFIER_H */
