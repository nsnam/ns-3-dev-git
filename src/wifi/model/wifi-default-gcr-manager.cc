/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-default-gcr-manager.h"

#include "ap-wifi-mac.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiDefaultGcrManager");

NS_OBJECT_ENSURE_REGISTERED(WifiDefaultGcrManager);

TypeId
WifiDefaultGcrManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiDefaultGcrManager")
                            .SetParent<GcrManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiDefaultGcrManager>();
    return tid;
}

WifiDefaultGcrManager::WifiDefaultGcrManager()
{
    NS_LOG_FUNCTION(this);
}

WifiDefaultGcrManager::~WifiDefaultGcrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

Mac48Address
WifiDefaultGcrManager::GetIndividuallyAddressedRecipient(
    const Mac48Address& /* groupAddress */) const
{
    /*
     * 802.11-2020: 10.23.2.12.2 Unsolicited retry procedure:
     * When using a protection mechanism that requires a response from another STA,
     * the AP should select a STA that is a member of the GCR group.
     * For now, we assume all associated GCR-capable STAs are part of the GCR group,
     * hence we pick the first STA from the list held by the AP.
     */
    NS_ASSERT(!m_staMembers.empty());
    return *m_staMembers.cbegin();
}

} // namespace ns3
