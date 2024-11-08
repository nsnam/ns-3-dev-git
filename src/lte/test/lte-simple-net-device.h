/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_SIMPLE_NET_DEVICE_H
#define LTE_SIMPLE_NET_DEVICE_H

#include "ns3/error-model.h"
#include "ns3/event-id.h"
#include "ns3/lte-rlc.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"

namespace ns3
{

/**
 * @ingroup lte
 * The LteSimpleNetDevice class implements the LTE simple net device.
 * This class is used to provide a limited LteNetDevice functionalities that
 * are necessary for testing purposes.
 */
class LteSimpleNetDevice : public SimpleNetDevice
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    LteSimpleNetDevice();
    /**
     * Constructor
     *
     * @param node the Node
     */
    LteSimpleNetDevice(Ptr<Node> node);

    ~LteSimpleNetDevice() override;
    void DoDispose() override;

    // inherited from NetDevice
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;

  protected:
    // inherited from Object
    void DoInitialize() override;
};

} // namespace ns3

#endif // LTE_SIMPLE_NET_DEVICE_H
