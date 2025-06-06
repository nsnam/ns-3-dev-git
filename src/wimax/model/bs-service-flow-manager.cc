/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "bs-net-device.h"
#include "bs-uplink-scheduler.h"
#include "connection-manager.h"
#include "service-flow-manager.h"
#include "service-flow-record.h"
#include "service-flow.h"
#include "ss-manager.h"
#include "ss-record.h"
#include "ss-scheduler.h"
#include "wimax-connection.h"

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

NS_LOG_COMPONENT_DEFINE("BsServiceFlowManager");

BsServiceFlowManager::BsServiceFlowManager(Ptr<BaseStationNetDevice> device)
    : m_device(device),
      m_sfidIndex(100),
      m_maxDsaRspRetries(100) // default value
{
    m_inuseScheduleDsaRspCid = Cid::InitialRanging();
}

BsServiceFlowManager::~BsServiceFlowManager()
{
}

/* static */
TypeId
BsServiceFlowManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BsServiceFlowManager").SetParent<ServiceFlowManager>().SetGroupName("Wifi")
        // No AddConstructor because this class has no default constructor.
        ;
    return tid;
}

void
BsServiceFlowManager::DoDispose()
{
    ServiceFlowManager::DoDispose();
}

void
BsServiceFlowManager::SetMaxDsaRspRetries(uint8_t maxDsaRspRetries)
{
    m_maxDsaRspRetries = maxDsaRspRetries;
}

uint8_t
BsServiceFlowManager::GetMaxDsaRspRetries() const
{
    return m_maxDsaRspRetries;
}

EventId
BsServiceFlowManager::GetDsaAckTimeoutEvent() const
{
    return m_dsaAckTimeoutEvent;
}

void
BsServiceFlowManager::AddServiceFlow(ServiceFlow* serviceFlow)
{
    ServiceFlowManager::AddServiceFlow(serviceFlow);
}

ServiceFlow*
BsServiceFlowManager::GetServiceFlow(uint32_t sfid) const
{
    return ServiceFlowManager::GetServiceFlow(sfid);
}

ServiceFlow*
BsServiceFlowManager::GetServiceFlow(Cid cid) const
{
    return ServiceFlowManager::GetServiceFlow(cid);
}

std::vector<ServiceFlow*>
BsServiceFlowManager::GetServiceFlows(ServiceFlow::SchedulingType schedulingType) const
{
    return ServiceFlowManager::GetServiceFlows(schedulingType);
}

DsaRsp
BsServiceFlowManager::CreateDsaRsp(const ServiceFlow* serviceFlow, uint16_t transactionId)
{
    DsaRsp dsaRsp;
    dsaRsp.SetTransactionId(transactionId);
    dsaRsp.SetServiceFlow(*serviceFlow);
    // assuming SS can supports all of the service flow parameters
    dsaRsp.SetConfirmationCode(CONFIRMATION_CODE_SUCCESS);

    return dsaRsp;
}

void
BsServiceFlowManager::ScheduleDsaRsp(ServiceFlow* serviceFlow, Cid cid)
{
    Ptr<BaseStationNetDevice> bs = m_device->GetObject<BaseStationNetDevice>();

    SSRecord* ssRecord = bs->GetSSManager()->GetSSRecord(cid);
    if (ssRecord == nullptr)
    {
        NS_LOG_INFO("SS not registered with the BS CID:" << cid);
        return;
    }

    serviceFlow->SetIsEnabled(true);
    serviceFlow->SetType(ServiceFlow::SF_TYPE_ACTIVE);
    ssRecord->AddServiceFlow(serviceFlow);

    bs->GetUplinkScheduler()->SetupServiceFlow(ssRecord, serviceFlow);

    Ptr<Packet> p = Create<Packet>();
    DsaRsp dsaRsp;

    if (ssRecord->GetDsaRspRetries() == 0)
    {
        dsaRsp = CreateDsaRsp(serviceFlow, ssRecord->GetSfTransactionId());
        p->AddHeader(dsaRsp);
        ssRecord->SetDsaRsp(dsaRsp);
    }
    else
    {
        if (ssRecord->GetDsaRspRetries() < m_maxDsaRspRetries)
        {
            p->AddHeader(ssRecord->GetDsaRsp());
        }
        else
        {
            NS_LOG_DEBUG("Service flows could not be initialized!");
        }
    }

    ssRecord->IncrementDsaRspRetries();
    p->AddHeader(ManagementMessageType(ManagementMessageType::MESSAGE_TYPE_DSA_RSP));

    if (m_dsaAckTimeoutEvent.IsPending())
    {
        Simulator::Cancel(m_dsaAckTimeoutEvent);
    }

    m_inuseScheduleDsaRspCid = cid;

    m_dsaAckTimeoutEvent = Simulator::Schedule(bs->GetIntervalT8(),
                                               &BsServiceFlowManager::ScheduleDsaRsp,
                                               this,
                                               serviceFlow,
                                               cid);
    m_device->Enqueue(p, MacHeaderType(), bs->GetConnection(ssRecord->GetPrimaryCid()));
}

ServiceFlow*
BsServiceFlowManager::ProcessDsaReq(const DsaReq& dsaReq, Cid cid)
{
    ServiceFlow* serviceFlow;
    Ptr<BaseStationNetDevice> bs = m_device->GetObject<BaseStationNetDevice>();
    SSRecord* ssRecord = bs->GetSSManager()->GetSSRecord(cid);

    NS_LOG_INFO("BsServiceFlowManager: Processing DSA-REQ...");
    if (ssRecord->GetSfTransactionId() != 0)
    {
        // had already received DSA-REQ. DSA-RSP was lost
        NS_ASSERT_MSG(
            dsaReq.GetTransactionId() == ssRecord->GetSfTransactionId(),
            "Error while processing DSA request:the received transaction ID is not expected");
        serviceFlow = GetServiceFlow(ssRecord->GetDsaRsp().GetSfid());
    }
    else
    {
        ServiceFlow sf = dsaReq.GetServiceFlow();
        Ptr<WimaxConnection> transportConnection;
        Ptr<ConnectionManager> BsConManager = bs->GetConnectionManager();
        transportConnection = BsConManager->CreateConnection(Cid::TRANSPORT);
        serviceFlow = new ServiceFlow(m_sfidIndex++, sf.GetDirection(), transportConnection);
        transportConnection->SetServiceFlow(serviceFlow);
        serviceFlow->CopyParametersFrom(sf);
        serviceFlow->SetUnsolicitedGrantInterval(1);
        serviceFlow->SetUnsolicitedPollingInterval(1);
        serviceFlow->SetConvergenceSublayerParam(sf.GetConvergenceSublayerParam());
        AddServiceFlow(serviceFlow);
        ssRecord->SetSfTransactionId(dsaReq.GetTransactionId());
        NS_LOG_INFO("BsServiceFlowManager: Creating a new Service flow: SFID = "
                    << serviceFlow->GetSfid() << " CID = " << serviceFlow->GetCid());
    }
    return serviceFlow;
}

void
BsServiceFlowManager::AddMulticastServiceFlow(ServiceFlow sf, WimaxPhy::ModulationType modulation)
{
    auto serviceFlow = new ServiceFlow();
    serviceFlow->CopyParametersFrom(sf);
    Ptr<BaseStationNetDevice> bs = m_device->GetObject<BaseStationNetDevice>();
    Ptr<WimaxConnection> multicastConnection =
        bs->GetConnectionManager()->CreateConnection(Cid::MULTICAST);
    serviceFlow->SetConnection(multicastConnection);
    AddServiceFlow(serviceFlow);
    serviceFlow->SetIsEnabled(true);
    serviceFlow->SetType(ServiceFlow::SF_TYPE_ACTIVE);
    serviceFlow->SetIsMulticast(true);
    serviceFlow->SetModulation(modulation);
    bs->GetUplinkScheduler()->SetupServiceFlow(nullptr, serviceFlow);
}

void
BsServiceFlowManager::AllocateServiceFlows(const DsaReq& dsaReq, Cid cid)
{
    ServiceFlow* serviceFlow = ProcessDsaReq(dsaReq, cid);
    if (serviceFlow)
    {
        ScheduleDsaRsp(serviceFlow, cid);
    }
    else
    {
        NS_LOG_INFO("No service Flow. Could not connect.");
    }
}

void
BsServiceFlowManager::ProcessDsaAck(const DsaAck& dsaAck, Cid cid)
{
    Ptr<BaseStationNetDevice> bs = m_device->GetObject<BaseStationNetDevice>();
    SSRecord* ssRecord = bs->GetSSManager()->GetSSRecord(cid);

    if (dsaAck.GetTransactionId() != ssRecord->GetSfTransactionId())
    {
        return;
    }

    ssRecord->SetDsaRspRetries(0);
    ssRecord->SetSfTransactionId(0);

    // check if all service flow have been initiated
    if (AreServiceFlowsAllocated(ssRecord->GetServiceFlows(ServiceFlow::SF_TYPE_ALL)))
    {
        ssRecord->SetAreServiceFlowsAllocated(true);
    }
}
} // namespace ns3
