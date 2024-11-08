/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef VHT_FRAME_EXCHANGE_MANAGER_H
#define VHT_FRAME_EXCHANGE_MANAGER_H

#include "ns3/ht-frame-exchange-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * VhtFrameExchangeManager handles the frame exchange sequences
 * for VHT stations.
 */
class VhtFrameExchangeManager : public HtFrameExchangeManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    VhtFrameExchangeManager();
    ~VhtFrameExchangeManager() override;

    Ptr<WifiPsdu> GetWifiPsdu(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector) const override;

  protected:
    uint32_t GetPsduSize(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector) const override;
};

} // namespace ns3

#endif /* VHT_FRAME_EXCHANGE_MANAGER_H */
