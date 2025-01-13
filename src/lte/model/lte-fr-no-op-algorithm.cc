/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#include "lte-fr-no-op-algorithm.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteFrNoOpAlgorithm");

NS_OBJECT_ENSURE_REGISTERED(LteFrNoOpAlgorithm);

LteFrNoOpAlgorithm::LteFrNoOpAlgorithm()
    : m_ffrSapUser(nullptr),
      m_ffrRrcSapUser(nullptr)
{
    NS_LOG_FUNCTION(this);
    m_ffrSapProvider = new MemberLteFfrSapProvider<LteFrNoOpAlgorithm>(this);
    m_ffrRrcSapProvider = new MemberLteFfrRrcSapProvider<LteFrNoOpAlgorithm>(this);
}

LteFrNoOpAlgorithm::~LteFrNoOpAlgorithm()
{
    NS_LOG_FUNCTION(this);
}

void
LteFrNoOpAlgorithm::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_ffrSapProvider;
    delete m_ffrRrcSapProvider;
}

TypeId
LteFrNoOpAlgorithm::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteFrNoOpAlgorithm")
                            .SetParent<LteFfrAlgorithm>()
                            .SetGroupName("Lte")
                            .AddConstructor<LteFrNoOpAlgorithm>();
    return tid;
}

void
LteFrNoOpAlgorithm::SetLteFfrSapUser(LteFfrSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_ffrSapUser = s;
}

LteFfrSapProvider*
LteFrNoOpAlgorithm::GetLteFfrSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_ffrSapProvider;
}

void
LteFrNoOpAlgorithm::SetLteFfrRrcSapUser(LteFfrRrcSapUser* s)
{
    NS_LOG_FUNCTION(this << s);
    m_ffrRrcSapUser = s;
}

LteFfrRrcSapProvider*
LteFrNoOpAlgorithm::GetLteFfrRrcSapProvider()
{
    NS_LOG_FUNCTION(this);
    return m_ffrRrcSapProvider;
}

void
LteFrNoOpAlgorithm::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    LteFfrAlgorithm::DoInitialize();
}

void
LteFrNoOpAlgorithm::Reconfigure()
{
    NS_LOG_FUNCTION(this);
}

std::vector<bool>
LteFrNoOpAlgorithm::DoGetAvailableDlRbg()
{
    NS_LOG_FUNCTION(this);
    std::vector<bool> rbgMap;
    int rbgSize = GetRbgSize(m_dlBandwidth);
    rbgMap.resize(m_dlBandwidth / rbgSize, false);
    return rbgMap;
}

bool
LteFrNoOpAlgorithm::DoIsDlRbgAvailableForUe(int i, uint16_t rnti)
{
    NS_LOG_FUNCTION(this);
    return true;
}

std::vector<bool>
LteFrNoOpAlgorithm::DoGetAvailableUlRbg()
{
    NS_LOG_FUNCTION(this);
    std::vector<bool> rbgMap;
    rbgMap.resize(m_ulBandwidth, false);
    return rbgMap;
}

bool
LteFrNoOpAlgorithm::DoIsUlRbgAvailableForUe(int i, uint16_t rnti)
{
    NS_LOG_FUNCTION(this);
    return true;
}

void
LteFrNoOpAlgorithm::DoReportDlCqiInfo(
    const FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("Method should not be called, because it is empty");
}

void
LteFrNoOpAlgorithm::DoReportUlCqiInfo(
    const FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("Method should not be called, because it is empty");
}

void
LteFrNoOpAlgorithm::DoReportUlCqiInfo(std::map<uint16_t, std::vector<double>> ulCqiMap)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("Method should not be called, because it is empty");
}

uint8_t
LteFrNoOpAlgorithm::DoGetTpc(uint16_t rnti)
{
    NS_LOG_FUNCTION(this);
    return 1; // 1 is mapped to 0 for Accumulated mode, and to -1 in Absolute mode TS36.213
              // Table 5.1.1.1-2
}

uint16_t
LteFrNoOpAlgorithm::DoGetMinContinuousUlBandwidth()
{
    NS_LOG_FUNCTION(this);
    return m_ulBandwidth;
}

void
LteFrNoOpAlgorithm::DoReportUeMeas(uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION(this << rnti << (uint16_t)measResults.measId);
    NS_LOG_WARN("Method should not be called, because it is empty");
}

void
LteFrNoOpAlgorithm::DoRecvLoadInformation(EpcX2Sap::LoadInformationParams params)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("Method should not be called, because it is empty");
}

} // end of namespace ns3
