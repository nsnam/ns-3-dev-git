/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_FRAME_EXCHANGE_MANAGER_H
#define UHR_FRAME_EXCHANGE_MANAGER_H

#include "ns3/eht-frame-exchange-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * UhrFrameExchangeManager handles the frame exchange sequences
 * for UHR stations.
 */
class UhrFrameExchangeManager : public EhtFrameExchangeManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    UhrFrameExchangeManager();
    ~UhrFrameExchangeManager() override;
};

} // namespace ns3

#endif /* UHR_FRAME_EXCHANGE_MANAGER_H */
