/*
 * Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "bs-net-device.h"
#include "bs-uplink-scheduler.h"
#include "connection-manager.h"
#include "service-flow-manager.h"
#include "service-flow-record.h"
#include "service-flow.h"
#include "ss-manager.h"
#include "ss-net-device.h"
#include "ss-record.h"
#include "ss-scheduler.h"
#include "wimax-connection.h"
#include "wimax-net-device.h"

#include "ns3/buffer.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include <cstdint>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SsServiceFlowManager");

SsServiceFlowManager::SsServiceFlowManager(Ptr<SubscriberStationNetDevice> device)
    : m_device(device),
      m_maxDsaReqRetries(100),
      m_dsaReq(DsaReq()),
      m_dsaAck(DsaAck()),
      m_currentTransactionId(0),
      m_transactionIdIndex(1),
      m_dsaReqRetries(0),
      m_pendingServiceFlow(nullptr)
{
}

SsServiceFlowManager::~SsServiceFlowManager()
{
}

/* static */
TypeId
SsServiceFlowManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SsServiceFlowManager").SetParent<ServiceFlowManager>().SetGroupName("Wimax")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

void
SsServiceFlowManager::DoDispose()
{
    ServiceFlowManager::DoDispose();
}

void
SsServiceFlowManager::SetMaxDsaReqRetries(uint8_t maxDsaReqRetries)
{
    m_maxDsaReqRetries = maxDsaReqRetries;
}

uint8_t
SsServiceFlowManager::GetMaxDsaReqRetries() const
{
    return m_maxDsaReqRetries;
}

EventId
SsServiceFlowManager::GetDsaRspTimeoutEvent() const
{
    return m_dsaRspTimeoutEvent;
}

EventId
SsServiceFlowManager::GetDsaAckTimeoutEvent() const
{
    return m_dsaAckTimeoutEvent;
}

void
SsServiceFlowManager::AddServiceFlow(ServiceFlow serviceFlow)
{
    auto sf = new ServiceFlow();
    sf->CopyParametersFrom(serviceFlow);
    ServiceFlowManager::AddServiceFlow(sf);
}

void
SsServiceFlowManager::AddServiceFlow(ServiceFlow* serviceFlow)
{
    ServiceFlowManager::AddServiceFlow(serviceFlow);
}

void
SsServiceFlowManager::InitiateServiceFlows()
{
    ServiceFlow* serviceFlow = GetNextServiceFlowToAllocate();
    NS_ASSERT_MSG(
        serviceFlow != nullptr,
        "Error while initiating a new service flow: All service flows have been initiated");
    m_pendingServiceFlow = serviceFlow;
    ScheduleDsaReq(m_pendingServiceFlow);
}

DsaReq
SsServiceFlowManager::CreateDsaReq(const ServiceFlow* serviceFlow)
{
    DsaReq dsaReq;
    dsaReq.SetTransactionId(m_transactionIdIndex);
    m_currentTransactionId = m_transactionIdIndex++;

    /*as it is SS-initiated DSA therefore SFID and CID will
     not be included, see 6.3.2.3.10.1 and 6.3.2.3.11.1*/
    dsaReq.SetServiceFlow(*serviceFlow);
    // dsaReq.SetParameterSet (*serviceFlow->GetParameterSet ());
    return dsaReq;
}

Ptr<Packet>
SsServiceFlowManager::CreateDsaAck()
{
    DsaAck dsaAck;
    dsaAck.SetTransactionId(m_dsaReq.GetTransactionId());
    dsaAck.SetConfirmationCode(CONFIRMATION_CODE_SUCCESS);
    m_dsaAck = dsaAck;
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(dsaAck);
    p->AddHeader(ManagementMessageType(ManagementMessageType::MESSAGE_TYPE_DSA_ACK));
    return p;
}

void
SsServiceFlowManager::ScheduleDsaReq(const ServiceFlow* serviceFlow)
{
    Ptr<Packet> p = Create<Packet>();
    DsaReq dsaReq;
    Ptr<SubscriberStationNetDevice> ss = m_device->GetObject<SubscriberStationNetDevice>();

    if (m_dsaReqRetries == 0)
    {
        dsaReq = CreateDsaReq(serviceFlow);
        p->AddHeader(dsaReq);
        m_dsaReq = dsaReq;
    }
    else
    {
        if (m_dsaReqRetries <= m_maxDsaReqRetries)
        {
            p->AddHeader(m_dsaReq);
        }
        else
        {
            NS_LOG_DEBUG("Service flows could not be initialized!");
        }
    }

    m_dsaReqRetries++;
    p->AddHeader(ManagementMessageType(ManagementMessageType::MESSAGE_TYPE_DSA_REQ));

    if (m_dsaRspTimeoutEvent.IsPending())
    {
        Simulator::Cancel(m_dsaRspTimeoutEvent);
    }

    m_dsaRspTimeoutEvent = Simulator::Schedule(ss->GetIntervalT7(),
                                               &SsServiceFlowManager::ScheduleDsaReq,
                                               this,
                                               serviceFlow);

    m_device->Enqueue(p, MacHeaderType(), ss->GetPrimaryConnection());
}

void
SsServiceFlowManager::ProcessDsaRsp(const DsaRsp& dsaRsp)
{
    Ptr<SubscriberStationNetDevice> ss = m_device->GetObject<SubscriberStationNetDevice>();

    // already received DSA-RSP for that particular DSA-REQ
    if (dsaRsp.GetTransactionId() != m_currentTransactionId)
    {
        return;
    }

    Ptr<Packet> dsaAck = CreateDsaAck();
    m_device->Enqueue(dsaAck, MacHeaderType(), ss->GetPrimaryConnection());

    m_dsaReqRetries = 0;
    if (m_pendingServiceFlow == nullptr)
    {
        // May be the DSA-ACK was not received by the SS
        return;
    }
    ServiceFlow sf = dsaRsp.GetServiceFlow();
    (*m_pendingServiceFlow) = sf;
    m_pendingServiceFlow->SetUnsolicitedGrantInterval(1);
    m_pendingServiceFlow->SetUnsolicitedPollingInterval(1);
    Ptr<WimaxConnection> transportConnection =
        CreateObject<WimaxConnection>(sf.GetCid(), Cid::TRANSPORT);

    m_pendingServiceFlow->SetConnection(transportConnection);
    transportConnection->SetServiceFlow(m_pendingServiceFlow);
    ss->GetConnectionManager()->AddConnection(transportConnection, Cid::TRANSPORT);
    m_pendingServiceFlow->SetIsEnabled(true);
    m_pendingServiceFlow = nullptr;
    // check if all service flow have been initiated
    ServiceFlow* serviceFlow = GetNextServiceFlowToAllocate();
    if (serviceFlow == nullptr)
    {
        ss->SetAreServiceFlowsAllocated(true);
    }
    else
    {
        m_pendingServiceFlow = serviceFlow;
        ScheduleDsaReq(m_pendingServiceFlow);
    }
}

} // namespace ns3
