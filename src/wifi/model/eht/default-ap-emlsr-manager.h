/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef DEFAULT_AP_EMLSR_MANAGER_H
#define DEFAULT_AP_EMLSR_MANAGER_H

#include "ap-emlsr-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * DefaultApEmlsrManager is the default AP EMLSR manager.
 */
class DefaultApEmlsrManager : public ApEmlsrManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultApEmlsrManager();
    ~DefaultApEmlsrManager() override;

    Time GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                     const WifiTxVector& txVector,
                                     WifiPhyBand band) override;
    bool UpdateCwAfterFailedIcf() override;
};

} // namespace ns3

#endif /* DEFAULT_AP_EMLSR_MANAGER_H */
