/*
 * Copyright (c) 2023 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-ap-emlsr-manager.h"

#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultApEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultApEmlsrManager);

TypeId
DefaultApEmlsrManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DefaultApEmlsrManager")
                            .SetParent<ApEmlsrManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultApEmlsrManager>();
    return tid;
}

DefaultApEmlsrManager::DefaultApEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultApEmlsrManager::~DefaultApEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

Time
DefaultApEmlsrManager::GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                                   const WifiTxVector& txVector,
                                                   WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << psdu << txVector << band);
    // EMLSR clients switch back to listening operation at the end of the PPDU
    return WifiPhy::CalculateTxDuration(psdu, txVector, band);
}

bool
DefaultApEmlsrManager::UpdateCwAfterFailedIcf()
{
    return true;
}

} // namespace ns3
