/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 *
 */

#include "lte-enb-component-carrier-manager.h"

#include "lte-common.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteEnbComponentCarrierManager");
NS_OBJECT_ENSURE_REGISTERED(LteEnbComponentCarrierManager);

LteEnbComponentCarrierManager::LteEnbComponentCarrierManager()
{
}

LteEnbComponentCarrierManager::~LteEnbComponentCarrierManager()
{
}

TypeId
LteEnbComponentCarrierManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LteEnbComponentCarrierManager").SetParent<Object>().SetGroupName("Lte");
    return tid;
}

void
LteEnbComponentCarrierManager::DoDispose()
{
}

void
LteEnbComponentCarrierManager::SetLteCcmRrcSapUser(LteCcmRrcSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_ccmRrcSapUser = s;
}

LteCcmRrcSapProvider*
LteEnbComponentCarrierManager::GetLteCcmRrcSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_ccmRrcSapProvider;
}

LteMacSapProvider*
LteEnbComponentCarrierManager::GetLteMacSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_macSapProvider;
}

LteCcmMacSapUser*
LteEnbComponentCarrierManager::GetLteCcmMacSapUser()
{
    NS_LOG_FUNCTION(this);
    return m_ccmMacSapUser;
}

bool
LteEnbComponentCarrierManager::SetMacSapProvider(uint8_t componentCarrierId, LteMacSapProvider* sap)
{
    NS_LOG_FUNCTION(this);
    bool res = false;
    auto it = m_macSapProvidersMap.find(componentCarrierId);
    if ((uint16_t)componentCarrierId > m_noOfComponentCarriers)
    {
        NS_FATAL_ERROR("Inconsistent componentCarrierId or you didn't call "
                       "SetNumberOfComponentCarriers before calling this method");
    }
    if (it != m_macSapProvidersMap.end())
    {
        NS_FATAL_ERROR("Tried to allocated an existing componentCarrierId");
    }
    else
    {
        m_macSapProvidersMap.insert(
            std::pair<uint8_t, LteMacSapProvider*>(componentCarrierId, sap));
        res = true;
    }
    return res;
}

bool
LteEnbComponentCarrierManager::SetCcmMacSapProviders(uint8_t componentCarrierId,
                                                     LteCcmMacSapProvider* sap)
{
    NS_LOG_FUNCTION(this);
    bool res = false;
    auto it = m_ccmMacSapProviderMap.find(componentCarrierId);

    if (it == m_ccmMacSapProviderMap.end())
    {
        m_ccmMacSapProviderMap.insert(
            std::pair<uint8_t, LteCcmMacSapProvider*>(componentCarrierId, sap));
    }

    res = true;
    return res;
}

void
LteEnbComponentCarrierManager::SetNumberOfComponentCarriers(uint16_t noOfComponentCarriers)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(noOfComponentCarriers < MIN_NO_CC || noOfComponentCarriers > MAX_NO_CC,
                    "Number of component carriers should be greater than 0 and less than 6");
    m_noOfComponentCarriers = noOfComponentCarriers;
    // Set the number of component carriers in eNB RRC
    m_ccmRrcSapUser->SetNumberOfComponentCarriers(noOfComponentCarriers);
}

} // end of namespace ns3
