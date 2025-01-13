/*
 * Copyright (c) 2013 Budiarto Herman
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "no-op-handover-algorithm.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NoOpHandoverAlgorithm");

NS_OBJECT_ENSURE_REGISTERED(NoOpHandoverAlgorithm);

NoOpHandoverAlgorithm::NoOpHandoverAlgorithm()
    : m_handoverManagementSapUser(nullptr)
{
    NS_LOG_FUNCTION(this);
    m_handoverManagementSapProvider =
        new MemberLteHandoverManagementSapProvider<NoOpHandoverAlgorithm>(this);
}

NoOpHandoverAlgorithm::~NoOpHandoverAlgorithm()
{
    NS_LOG_FUNCTION(this);
}

void
NoOpHandoverAlgorithm::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_handoverManagementSapProvider;
}

TypeId
NoOpHandoverAlgorithm::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NoOpHandoverAlgorithm")
                            .SetParent<LteHandoverAlgorithm>()
                            .SetGroupName("Lte")
                            .AddConstructor<NoOpHandoverAlgorithm>();
    return tid;
}

void
NoOpHandoverAlgorithm::SetLteHandoverManagementSapUser(LteHandoverManagementSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_handoverManagementSapUser = s;
}

LteHandoverManagementSapProvider*
NoOpHandoverAlgorithm::GetLteHandoverManagementSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_handoverManagementSapProvider;
}

void
NoOpHandoverAlgorithm::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    LteHandoverAlgorithm::DoInitialize();
}

void
NoOpHandoverAlgorithm::DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)measResults.measId);
}

} // end of namespace ns3
