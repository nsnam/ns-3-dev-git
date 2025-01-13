/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Lluis Parcerisa <lparcerisa@cttc.cat>
 */

#include "lte-rrc-protocol-real.h"

#include "lte-enb-net-device.h"
#include "lte-enb-rrc.h"
#include "lte-rrc-header.h"
#include "lte-ue-net-device.h"
#include "lte-ue-rrc.h"

#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteRrcProtocolReal");

/// RRC real message delay
const Time RRC_REAL_MSG_DELAY;

NS_OBJECT_ENSURE_REGISTERED(LteUeRrcProtocolReal);

LteUeRrcProtocolReal::LteUeRrcProtocolReal()
    : m_ueRrcSapProvider(nullptr),
      m_enbRrcSapProvider(nullptr)
{
    m_ueRrcSapUser = new MemberLteUeRrcSapUser<LteUeRrcProtocolReal>(this);
    m_completeSetupParameters.srb0SapUser =
        new LteRlcSpecificLteRlcSapUser<LteUeRrcProtocolReal>(this);
    m_completeSetupParameters.srb1SapUser =
        new LtePdcpSpecificLtePdcpSapUser<LteUeRrcProtocolReal>(this);
}

LteUeRrcProtocolReal::~LteUeRrcProtocolReal()
{
}

void
LteUeRrcProtocolReal::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_ueRrcSapUser;
    delete m_completeSetupParameters.srb0SapUser;
    delete m_completeSetupParameters.srb1SapUser;
    m_rrc = nullptr;
}

TypeId
LteUeRrcProtocolReal::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteUeRrcProtocolReal")
                            .SetParent<Object>()
                            .SetGroupName("Lte")
                            .AddConstructor<LteUeRrcProtocolReal>();
    return tid;
}

void
LteUeRrcProtocolReal::SetLteUeRrcSapProvider(LteUeRrcSapProvider* p)
{
    m_ueRrcSapProvider = p;
}

LteUeRrcSapUser*
LteUeRrcProtocolReal::GetLteUeRrcSapUser()
{
    return m_ueRrcSapUser;
}

void
LteUeRrcProtocolReal::SetUeRrc(Ptr<LteUeRrc> rrc)
{
    m_rrc = rrc;
}

void
LteUeRrcProtocolReal::DoSetup(LteUeRrcSapUser::SetupParameters params)
{
    NS_LOG_FUNCTION(this);

    m_setupParameters.srb0SapProvider = params.srb0SapProvider;
    m_setupParameters.srb1SapProvider = params.srb1SapProvider;
    m_ueRrcSapProvider->CompleteSetup(m_completeSetupParameters);
}

void
LteUeRrcProtocolReal::DoSendRrcConnectionRequest(LteRrcSap::RrcConnectionRequest msg)
{
    // initialize the RNTI and get the EnbLteRrcSapProvider for the
    // eNB we are currently attached to
    m_rnti = m_rrc->GetRnti();
    SetEnbRrcSapProvider();

    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionRequestHeader rrcConnectionRequestHeader;
    rrcConnectionRequestHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionRequestHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = m_rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupParameters.srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteUeRrcProtocolReal::DoSendRrcConnectionSetupCompleted(
    LteRrcSap::RrcConnectionSetupCompleted msg) const
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;
    rrcConnectionSetupCompleteHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionSetupCompleteHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = m_rnti;
    transmitPdcpSduParameters.lcid = 1;

    if (m_setupParameters.srb1SapProvider)
    {
        m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
    }
}

void
LteUeRrcProtocolReal::DoSendRrcConnectionReconfigurationCompleted(
    LteRrcSap::RrcConnectionReconfigurationCompleted msg)
{
    // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
    // eNB we are currently attached to
    m_rnti = m_rrc->GetRnti();
    SetEnbRrcSapProvider();

    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
    rrcConnectionReconfigurationCompleteHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReconfigurationCompleteHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = m_rnti;
    transmitPdcpSduParameters.lcid = 1;

    m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void
LteUeRrcProtocolReal::DoSendMeasurementReport(LteRrcSap::MeasurementReport msg)
{
    // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
    // eNB we are currently attached to
    m_rnti = m_rrc->GetRnti();
    SetEnbRrcSapProvider();

    Ptr<Packet> packet = Create<Packet>();

    MeasurementReportHeader measurementReportHeader;
    measurementReportHeader.SetMessage(msg);

    packet->AddHeader(measurementReportHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = m_rnti;
    transmitPdcpSduParameters.lcid = 1;

    m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void
LteUeRrcProtocolReal::DoSendIdealUeContextRemoveRequest(uint16_t rnti)
{
    NS_LOG_FUNCTION(this << rnti);
    uint16_t cellId = m_rrc->GetCellId();
    // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
    // eNB we are currently attached to or attempting random access to
    // a target eNB
    m_rnti = m_rrc->GetRnti();

    NS_LOG_DEBUG("RNTI " << rnti << " sending UE context remove request to cell id " << cellId);
    NS_ABORT_MSG_IF(m_rnti != rnti, "RNTI mismatch");

    SetEnbRrcSapProvider(); // the provider has to be reset since the cell might
                            //  have changed due to handover
    // ideally informing eNB
    Simulator::Schedule(RRC_REAL_MSG_DELAY,
                        &LteEnbRrcSapProvider::RecvIdealUeContextRemoveRequest,
                        m_enbRrcSapProvider,
                        rnti);
}

void
LteUeRrcProtocolReal::DoSendRrcConnectionReestablishmentRequest(
    LteRrcSap::RrcConnectionReestablishmentRequest msg) const
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
    rrcConnectionReestablishmentRequestHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReestablishmentRequestHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = m_rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupParameters.srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteUeRrcProtocolReal::DoSendRrcConnectionReestablishmentComplete(
    LteRrcSap::RrcConnectionReestablishmentComplete msg) const
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
    rrcConnectionReestablishmentCompleteHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReestablishmentCompleteHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = m_rnti;
    transmitPdcpSduParameters.lcid = 1;

    m_setupParameters.srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void
LteUeRrcProtocolReal::SetEnbRrcSapProvider()
{
    NS_LOG_FUNCTION(this);

    uint16_t cellId = m_rrc->GetCellId();

    NS_LOG_DEBUG("RNTI " << m_rnti << " connected to cell " << cellId);

    // walk list of all nodes to get the peer eNB
    Ptr<LteEnbNetDevice> enbDev;
    auto listEnd = NodeList::End();
    bool found = false;
    for (auto i = NodeList::Begin(); (i != listEnd) && (!found); ++i)
    {
        Ptr<Node> node = *i;
        int nDevs = node->GetNDevices();
        for (int j = 0; (j < nDevs) && (!found); j++)
        {
            enbDev = node->GetDevice(j)->GetObject<LteEnbNetDevice>();
            if (!enbDev)
            {
                continue;
            }
            else
            {
                if (enbDev->HasCellId(cellId))
                {
                    found = true;
                    break;
                }
            }
        }
    }
    NS_ASSERT_MSG(found, " Unable to find eNB with CellId =" << cellId);
    m_enbRrcSapProvider = enbDev->GetRrc()->GetLteEnbRrcSapProvider();
    Ptr<LteEnbRrcProtocolReal> enbRrcProtocolReal =
        enbDev->GetRrc()->GetObject<LteEnbRrcProtocolReal>();
    enbRrcProtocolReal->SetUeRrcSapProvider(m_rnti, m_ueRrcSapProvider);
}

void
LteUeRrcProtocolReal::DoReceivePdcpPdu(Ptr<Packet> p)
{
    // Get type of message received
    RrcDlCcchMessage rrcDlCcchMessage;
    p->PeekHeader(rrcDlCcchMessage);

    // Declare possible headers to receive
    RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
    RrcConnectionReestablishmentRejectHeader rrcConnectionReestablishmentRejectHeader;
    RrcConnectionSetupHeader rrcConnectionSetupHeader;
    RrcConnectionRejectHeader rrcConnectionRejectHeader;

    // Declare possible messages
    LteRrcSap::RrcConnectionReestablishment rrcConnectionReestablishmentMsg;
    LteRrcSap::RrcConnectionReestablishmentReject rrcConnectionReestablishmentRejectMsg;
    LteRrcSap::RrcConnectionSetup rrcConnectionSetupMsg;
    LteRrcSap::RrcConnectionReject rrcConnectionRejectMsg;

    // Deserialize packet and call member recv function with appropriate structure
    switch (rrcDlCcchMessage.GetMessageType())
    {
    case 0:
        // RrcConnectionReestablishment
        p->RemoveHeader(rrcConnectionReestablishmentHeader);
        rrcConnectionReestablishmentMsg = rrcConnectionReestablishmentHeader.GetMessage();
        m_ueRrcSapProvider->RecvRrcConnectionReestablishment(rrcConnectionReestablishmentMsg);
        break;
    case 1:
        // RrcConnectionReestablishmentReject
        p->RemoveHeader(rrcConnectionReestablishmentRejectHeader);
        rrcConnectionReestablishmentRejectMsg =
            rrcConnectionReestablishmentRejectHeader.GetMessage();
        // m_ueRrcSapProvider->RecvRrcConnectionReestablishmentReject
        // (rrcConnectionReestablishmentRejectMsg);
        break;
    case 2:
        // RrcConnectionReject
        p->RemoveHeader(rrcConnectionRejectHeader);
        rrcConnectionRejectMsg = rrcConnectionRejectHeader.GetMessage();
        m_ueRrcSapProvider->RecvRrcConnectionReject(rrcConnectionRejectMsg);
        break;
    case 3:
        // RrcConnectionSetup
        p->RemoveHeader(rrcConnectionSetupHeader);
        rrcConnectionSetupMsg = rrcConnectionSetupHeader.GetMessage();
        m_ueRrcSapProvider->RecvRrcConnectionSetup(rrcConnectionSetupMsg);
        break;
    }
}

void
LteUeRrcProtocolReal::DoReceivePdcpSdu(LtePdcpSapUser::ReceivePdcpSduParameters params)
{
    // Get type of message received
    RrcDlDcchMessage rrcDlDcchMessage;
    params.pdcpSdu->PeekHeader(rrcDlDcchMessage);

    // Declare possible headers to receive
    RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
    RrcConnectionReleaseHeader rrcConnectionReleaseHeader;

    // Declare possible messages to receive
    LteRrcSap::RrcConnectionReconfiguration rrcConnectionReconfigurationMsg;
    LteRrcSap::RrcConnectionRelease rrcConnectionReleaseMsg;

    // Deserialize packet and call member recv function with appropriate structure
    switch (rrcDlDcchMessage.GetMessageType())
    {
    case 4:
        params.pdcpSdu->RemoveHeader(rrcConnectionReconfigurationHeader);
        rrcConnectionReconfigurationMsg = rrcConnectionReconfigurationHeader.GetMessage();
        m_ueRrcSapProvider->RecvRrcConnectionReconfiguration(rrcConnectionReconfigurationMsg);
        break;
    case 5:
        params.pdcpSdu->RemoveHeader(rrcConnectionReleaseHeader);
        rrcConnectionReleaseMsg = rrcConnectionReleaseHeader.GetMessage();
        // m_ueRrcSapProvider->RecvRrcConnectionRelease (rrcConnectionReleaseMsg);
        break;
    }
}

NS_OBJECT_ENSURE_REGISTERED(LteEnbRrcProtocolReal);

LteEnbRrcProtocolReal::LteEnbRrcProtocolReal()
    : m_enbRrcSapProvider(nullptr)
{
    NS_LOG_FUNCTION(this);
    m_enbRrcSapUser = new MemberLteEnbRrcSapUser<LteEnbRrcProtocolReal>(this);
}

LteEnbRrcProtocolReal::~LteEnbRrcProtocolReal()
{
    NS_LOG_FUNCTION(this);
}

void
LteEnbRrcProtocolReal::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_enbRrcSapUser;
    for (auto it = m_completeSetupUeParametersMap.begin();
         it != m_completeSetupUeParametersMap.end();
         ++it)
    {
        delete it->second.srb0SapUser;
        delete it->second.srb1SapUser;
    }
    m_completeSetupUeParametersMap.clear();
}

TypeId
LteEnbRrcProtocolReal::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteEnbRrcProtocolReal")
                            .SetParent<Object>()
                            .SetGroupName("Lte")
                            .AddConstructor<LteEnbRrcProtocolReal>();
    return tid;
}

void
LteEnbRrcProtocolReal::SetLteEnbRrcSapProvider(LteEnbRrcSapProvider* p)
{
    m_enbRrcSapProvider = p;
}

LteEnbRrcSapUser*
LteEnbRrcProtocolReal::GetLteEnbRrcSapUser()
{
    return m_enbRrcSapUser;
}

void
LteEnbRrcProtocolReal::SetCellId(uint16_t cellId)
{
    m_cellId = cellId;
}

LteUeRrcSapProvider*
LteEnbRrcProtocolReal::GetUeRrcSapProvider(uint16_t rnti)
{
    auto it = m_enbRrcSapProviderMap.find(rnti);
    NS_ASSERT_MSG(it != m_enbRrcSapProviderMap.end(), "could not find RNTI = " << rnti);
    return it->second;
}

void
LteEnbRrcProtocolReal::SetUeRrcSapProvider(uint16_t rnti, LteUeRrcSapProvider* p)
{
    auto it = m_enbRrcSapProviderMap.find(rnti);
    // assign UE RRC only if the RNTI is found at eNB
    if (it != m_enbRrcSapProviderMap.end())
    {
        it->second = p;
    }
}

void
LteEnbRrcProtocolReal::DoSetupUe(uint16_t rnti, LteEnbRrcSapUser::SetupUeParameters params)
{
    NS_LOG_FUNCTION(this << rnti);

    // // walk list of all nodes to get the peer UE RRC SAP Provider
    // Ptr<LteUeRrc> ueRrc;
    // NodeList::Iterator listEnd = NodeList::End ();
    // bool found = false;
    // for (NodeList::Iterator i = NodeList::Begin (); (i != listEnd) && (found == false); i++)
    //   {
    //     Ptr<Node> node = *i;
    //     int nDevs = node->GetNDevices ();
    //     for (int j = 0; j < nDevs; j++)
    //       {
    //         Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
    //         if (!ueDev)
    //           {
    //             continue;
    //           }
    //         else
    //           {
    //             ueRrc = ueDev->GetRrc ();
    //             if ((ueRrc->GetRnti () == rnti) && (ueRrc->GetCellId () == m_cellId))
    //               {
    //              found = true;
    //              break;
    //               }
    //           }
    //       }
    //   }
    // NS_ASSERT_MSG (found , " Unable to find UE with RNTI=" << rnti << " cellId=" << m_cellId);
    // m_enbRrcSapProviderMap[rnti] = ueRrc->GetLteUeRrcSapProvider ();

    // just create empty entry, the UeRrcSapProvider will be set by the
    // ue upon connection request or connection reconfiguration
    // completed
    m_enbRrcSapProviderMap[rnti] = nullptr;

    // Store SetupUeParameters
    m_setupUeParametersMap[rnti] = params;

    LteEnbRrcSapProvider::CompleteSetupUeParameters completeSetupUeParameters;
    auto csupIt = m_completeSetupUeParametersMap.find(rnti);
    if (csupIt == m_completeSetupUeParametersMap.end())
    {
        // Create LteRlcSapUser, LtePdcpSapUser
        LteRlcSapUser* srb0SapUser = new RealProtocolRlcSapUser(this, rnti);
        LtePdcpSapUser* srb1SapUser =
            new LtePdcpSpecificLtePdcpSapUser<LteEnbRrcProtocolReal>(this);
        completeSetupUeParameters.srb0SapUser = srb0SapUser;
        completeSetupUeParameters.srb1SapUser = srb1SapUser;
        // Store LteRlcSapUser, LtePdcpSapUser
        m_completeSetupUeParametersMap[rnti] = completeSetupUeParameters;
    }
    else
    {
        completeSetupUeParameters = csupIt->second;
    }
    m_enbRrcSapProvider->CompleteSetupUe(rnti, completeSetupUeParameters);
}

void
LteEnbRrcProtocolReal::DoRemoveUe(uint16_t rnti)
{
    NS_LOG_FUNCTION(this << rnti);
    auto it = m_completeSetupUeParametersMap.find(rnti);
    NS_ASSERT(it != m_completeSetupUeParametersMap.end());
    delete it->second.srb0SapUser;
    delete it->second.srb1SapUser;
    m_completeSetupUeParametersMap.erase(it);
    m_enbRrcSapProviderMap.erase(rnti);
    m_setupUeParametersMap.erase(rnti);
}

void
LteEnbRrcProtocolReal::DoSendSystemInformation(uint16_t cellId, LteRrcSap::SystemInformation msg)
{
    NS_LOG_FUNCTION(this << cellId);
    // walk list of all nodes to get UEs with this cellId
    Ptr<LteUeRrc> ueRrc;
    for (auto i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; ++j)
        {
            Ptr<LteUeNetDevice> ueDev = node->GetDevice(j)->GetObject<LteUeNetDevice>();
            if (ueDev)
            {
                Ptr<LteUeRrc> ueRrc = ueDev->GetRrc();
                NS_LOG_LOGIC("considering UE IMSI " << ueDev->GetImsi() << " that has cellId "
                                                    << ueRrc->GetCellId());
                if (ueRrc->GetCellId() == cellId)
                {
                    NS_LOG_LOGIC("sending SI to IMSI " << ueDev->GetImsi());

                    Simulator::ScheduleWithContext(node->GetId(),
                                                   RRC_REAL_MSG_DELAY,
                                                   &LteUeRrcSapProvider::RecvSystemInformation,
                                                   ueRrc->GetLteUeRrcSapProvider(),
                                                   msg);
                }
            }
        }
    }
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionSetup(uint16_t rnti, LteRrcSap::RrcConnectionSetup msg)
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionSetupHeader rrcConnectionSetupHeader;
    rrcConnectionSetupHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionSetupHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupUeParametersMap.at(rnti).srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionReject(uint16_t rnti, LteRrcSap::RrcConnectionReject msg)
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionRejectHeader rrcConnectionRejectHeader;
    rrcConnectionRejectHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionRejectHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionReconfiguration(
    uint16_t rnti,
    LteRrcSap::RrcConnectionReconfiguration msg)
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
    rrcConnectionReconfigurationHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReconfigurationHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = rnti;
    transmitPdcpSduParameters.lcid = 1;

    m_setupUeParametersMap[rnti].srb1SapProvider->TransmitPdcpSdu(transmitPdcpSduParameters);
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionReestablishment(
    uint16_t rnti,
    LteRrcSap::RrcConnectionReestablishment msg)
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
    rrcConnectionReestablishmentHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReestablishmentHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionReestablishmentReject(
    uint16_t rnti,
    LteRrcSap::RrcConnectionReestablishmentReject msg)
{
    Ptr<Packet> packet = Create<Packet>();

    RrcConnectionReestablishmentRejectHeader rrcConnectionReestablishmentRejectHeader;
    rrcConnectionReestablishmentRejectHeader.SetMessage(msg);

    packet->AddHeader(rrcConnectionReestablishmentRejectHeader);

    LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
    transmitPdcpPduParameters.pdcpPdu = packet;
    transmitPdcpPduParameters.rnti = rnti;
    transmitPdcpPduParameters.lcid = 0;

    m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu(transmitPdcpPduParameters);
}

void
LteEnbRrcProtocolReal::DoSendRrcConnectionRelease(uint16_t rnti,
                                                  LteRrcSap::RrcConnectionRelease msg)
{
    // The code below is commented so RRC connection release can be sent in an ideal way
    /*
    Ptr<Packet> packet = Create<Packet> ();

    RrcConnectionReleaseHeader rrcConnectionReleaseHeader;
    rrcConnectionReleaseHeader.SetMessage (msg);

    packet->AddHeader (rrcConnectionReleaseHeader);

    LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
    transmitPdcpSduParameters.pdcpSdu = packet;
    transmitPdcpSduParameters.rnti = rnti;
    transmitPdcpSduParameters.lcid = 1;

    m_setupUeParametersMap[rnti].srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
    */
    /**
     * Send RRC connection release in an idle way to ensure UE goes
     * to idle mode during handover failure and connection setup timeout.
     * Implemented to avoid unnecessary triggering of assert msgs due to reception of
     * msgs (SRS CQI reports) from UE after UE context is deleted at eNodeB.
     * TODO: Detection of handover failure and connection setup timeout at UE,
     * so that the RRC connection release can be sent through the physical channel again.
     */
    NS_LOG_FUNCTION(this << rnti);
    Simulator::Schedule(RRC_REAL_MSG_DELAY,
                        &LteUeRrcSapProvider::RecvRrcConnectionRelease,
                        GetUeRrcSapProvider(rnti),
                        msg);
}

void
LteEnbRrcProtocolReal::DoReceivePdcpPdu(uint16_t rnti, Ptr<Packet> p)
{
    // Get type of message received
    RrcUlCcchMessage rrcUlCcchMessage;
    p->PeekHeader(rrcUlCcchMessage);

    // Declare possible headers to receive
    RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
    RrcConnectionRequestHeader rrcConnectionRequestHeader;

    // Deserialize packet and call member recv function with appropriate structure
    switch (rrcUlCcchMessage.GetMessageType())
    {
    case 0:
        p->RemoveHeader(rrcConnectionReestablishmentRequestHeader);
        LteRrcSap::RrcConnectionReestablishmentRequest rrcConnectionReestablishmentRequestMsg;
        rrcConnectionReestablishmentRequestMsg =
            rrcConnectionReestablishmentRequestHeader.GetMessage();
        m_enbRrcSapProvider->RecvRrcConnectionReestablishmentRequest(
            rnti,
            rrcConnectionReestablishmentRequestMsg);
        break;
    case 1:
        p->RemoveHeader(rrcConnectionRequestHeader);
        LteRrcSap::RrcConnectionRequest rrcConnectionRequestMsg;
        rrcConnectionRequestMsg = rrcConnectionRequestHeader.GetMessage();
        m_enbRrcSapProvider->RecvRrcConnectionRequest(rnti, rrcConnectionRequestMsg);
        break;
    }
}

void
LteEnbRrcProtocolReal::DoReceivePdcpSdu(LtePdcpSapUser::ReceivePdcpSduParameters params)
{
    // Get type of message received
    RrcUlDcchMessage rrcUlDcchMessage;
    params.pdcpSdu->PeekHeader(rrcUlDcchMessage);

    // Declare possible headers to receive
    MeasurementReportHeader measurementReportHeader;
    RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
    RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
    RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;

    // Declare possible messages to receive
    LteRrcSap::MeasurementReport measurementReportMsg;
    LteRrcSap::RrcConnectionReconfigurationCompleted rrcConnectionReconfigurationCompleteMsg;
    LteRrcSap::RrcConnectionReestablishmentComplete rrcConnectionReestablishmentCompleteMsg;
    LteRrcSap::RrcConnectionSetupCompleted rrcConnectionSetupCompletedMsg;

    // Deserialize packet and call member recv function with appropriate structure
    switch (rrcUlDcchMessage.GetMessageType())
    {
    case 1:
        params.pdcpSdu->RemoveHeader(measurementReportHeader);
        measurementReportMsg = measurementReportHeader.GetMessage();
        m_enbRrcSapProvider->RecvMeasurementReport(params.rnti, measurementReportMsg);
        break;
    case 2:
        params.pdcpSdu->RemoveHeader(rrcConnectionReconfigurationCompleteHeader);
        rrcConnectionReconfigurationCompleteMsg =
            rrcConnectionReconfigurationCompleteHeader.GetMessage();
        m_enbRrcSapProvider->RecvRrcConnectionReconfigurationCompleted(
            params.rnti,
            rrcConnectionReconfigurationCompleteMsg);
        break;
    case 3:
        params.pdcpSdu->RemoveHeader(rrcConnectionReestablishmentCompleteHeader);
        rrcConnectionReestablishmentCompleteMsg =
            rrcConnectionReestablishmentCompleteHeader.GetMessage();
        m_enbRrcSapProvider->RecvRrcConnectionReestablishmentComplete(
            params.rnti,
            rrcConnectionReestablishmentCompleteMsg);
        break;
    case 4:
        params.pdcpSdu->RemoveHeader(rrcConnectionSetupCompleteHeader);
        rrcConnectionSetupCompletedMsg = rrcConnectionSetupCompleteHeader.GetMessage();
        m_enbRrcSapProvider->RecvRrcConnectionSetupCompleted(params.rnti,
                                                             rrcConnectionSetupCompletedMsg);
        break;
    }
}

Ptr<Packet>
LteEnbRrcProtocolReal::DoEncodeHandoverPreparationInformation(
    LteRrcSap::HandoverPreparationInfo msg)
{
    HandoverPreparationInfoHeader h;
    h.SetMessage(msg);

    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(h);
    return p;
}

LteRrcSap::HandoverPreparationInfo
LteEnbRrcProtocolReal::DoDecodeHandoverPreparationInformation(Ptr<Packet> p)
{
    HandoverPreparationInfoHeader h;
    p->RemoveHeader(h);
    LteRrcSap::HandoverPreparationInfo msg = h.GetMessage();
    return msg;
}

Ptr<Packet>
LteEnbRrcProtocolReal::DoEncodeHandoverCommand(LteRrcSap::RrcConnectionReconfiguration msg)
{
    RrcConnectionReconfigurationHeader h;
    h.SetMessage(msg);
    Ptr<Packet> p = Create<Packet>();
    p->AddHeader(h);
    return p;
}

LteRrcSap::RrcConnectionReconfiguration
LteEnbRrcProtocolReal::DoDecodeHandoverCommand(Ptr<Packet> p)
{
    RrcConnectionReconfigurationHeader h;
    p->RemoveHeader(h);
    LteRrcSap::RrcConnectionReconfiguration msg = h.GetMessage();
    return msg;
}

//////////////////////////////////////////////////////

RealProtocolRlcSapUser::RealProtocolRlcSapUser(LteEnbRrcProtocolReal* pdcp, uint16_t rnti)
    : m_pdcp(pdcp),
      m_rnti(rnti)
{
}

RealProtocolRlcSapUser::RealProtocolRlcSapUser()
{
}

void
RealProtocolRlcSapUser::ReceivePdcpPdu(Ptr<Packet> p)
{
    m_pdcp->DoReceivePdcpPdu(m_rnti, p);
}

} // namespace ns3
