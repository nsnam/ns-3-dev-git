/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-zdo.h"

#include "zigbee-zdo-client-header.h"
#include "zigbee-zdo-server-header.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/zigbee-profiles.h"

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeZdo");
NS_OBJECT_ENSURE_REGISTERED(ZigbeeZdo);

TypeId
ZigbeeZdo::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeeZdo")
                            .SetParent<Object>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeZdo>();
    return tid;
}

ZigbeeZdo::ZigbeeZdo()
{
    NS_LOG_FUNCTION(this);
}

void
ZigbeeZdo::NotifyConstructionCompleted()
{
    NS_LOG_FUNCTION(this);
}

ZigbeeZdo::~ZigbeeZdo()
{
    NS_LOG_FUNCTION(this);
}

void
ZigbeeZdo::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Object::DoInitialize();
}

void
ZigbeeZdo::DoDispose()
{
    Object::DoDispose();
}

void
ZigbeeZdo::SetAps(Ptr<ZigbeeAps> aps)
{
    m_aps = aps;
}

Ptr<ZigbeeAps>
ZigbeeZdo::GetAps() const
{
    return m_aps;
}

void
ZigbeeZdo::SetNwk(Ptr<ZigbeeNwk> nwk)
{
    m_nwk = nwk;
}

Ptr<ZigbeeNwk>
ZigbeeZdo::GetNwk() const
{
    return m_nwk;
}

void
ZigbeeZdo::ApsDataIndication(ApsdeDataIndicationParams params, Ptr<Packet> asdu)
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_IF(params.m_profileId != ZigbeeProfiles::ZDP_PROFILE,
                    "Error, in ZDO the profile must be 0");
    NS_ABORT_MSG_IF(params.m_dstEndPoint != 0, "Error, in ZDO the endpoint must be 0");

    // All response commands use cluster identifiers greater than or equal to 0x8000,
    // while all request commands use cluster identifiers less than 0x8000.
    if (params.m_clusterId < ZdpClustersServerCmds::NWK_ADDR_RSP)
    {
        //  CLIENT REQUESTS
        switch (params.m_clusterId)
        {
        case ZdpClustersClientCmds::NWK_ADDR_REQ:
            HandleNwkAddrRequest(asdu, params);
            break;

        default:
            break;
        }
    }
    else
    {
        // SERVER RESPONSES
        switch (params.m_clusterId)
        {
        case ZdpClustersServerCmds::NWK_ADDR_RSP:
            HandleNwkAddrResponse(asdu, params);
            break;

        default:
            break;
        }
    }
}

void
ZigbeeZdo::NwkAddrRequest(NwkAddrRequestParams params)
{
    NS_LOG_FUNCTION(this);

    if (params.m_requestType == NwkAddrRequestType::EXTENDED_RESPONSE)
    {
        // TODO: Add support to requests associated devices.
        return;
    }

    Ptr<Packet> zdpFrame = Create<Packet>();

    // The ZDP frame
    NwkAddrRequestCmd nwkAddrRequestCmd;
    nwkAddrRequestCmd.SetTsn(m_tsn.GetValue());
    m_tsn++;
    nwkAddrRequestCmd.SetIeeeAddress(params.m_ieeeAddr);
    nwkAddrRequestCmd.SetRequestType(static_cast<uint8_t>(params.m_requestType));
    nwkAddrRequestCmd.SetStartIndex(params.m_startIndex);
    zdpFrame->AddHeader(nwkAddrRequestCmd);

    ApsdeDataRequestParams apsdeParams;
    apsdeParams.m_dstAddrMode = ApsDstAddressMode::DST_ADDR16_DST_ENDPOINT_PRESENT;
    apsdeParams.m_dstAddr16 = params.m_dstAddr;
    apsdeParams.m_dstEndPoint = 0x00; // ZDO endpoint
    apsdeParams.m_profileId = ZigbeeProfiles::ZDP_PROFILE;
    apsdeParams.m_clusterId = ZdpClustersClientCmds::NWK_ADDR_REQ;
    apsdeParams.m_srcEndPoint = 0x00; // ZDO endpoint
    apsdeParams.m_asduLength = zdpFrame->GetSize();
    apsdeParams.m_txOptions = 0;    // No special transmission options
    apsdeParams.m_useAlias = false; // Not using alias
    apsdeParams.m_radius = 0;       // Default radius

    m_aps->ApsdeDataRequest(apsdeParams, zdpFrame);
}

void
ZigbeeZdo::NwkAddrResponse(NwkAddrResponseParams params)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> zdpFrame = Create<Packet>();

    // The ZDP frame
    NwkAddrResponseCmd nwkAddrResponseCmd;
    nwkAddrResponseCmd.SetTsn(params.m_tsn);
    nwkAddrResponseCmd.SetStatus(static_cast<uint8_t>(params.m_status));
    nwkAddrResponseCmd.SetIeeeAddrRemoteDev(params.m_ieeeAddrRemoteDev);
    nwkAddrResponseCmd.SetNwkAddrRemoteDev(params.m_nwkAddrRemoteDev);
    zdpFrame->AddHeader(nwkAddrResponseCmd);

    ApsdeDataRequestParams apsdeParams;
    apsdeParams.m_dstAddrMode = ApsDstAddressMode::DST_ADDR16_DST_ENDPOINT_PRESENT;
    apsdeParams.m_dstAddr16 = params.m_dstAddr;
    apsdeParams.m_dstEndPoint = 0x00; // ZDO endpoint
    apsdeParams.m_profileId = ZigbeeProfiles::ZDP_PROFILE;
    apsdeParams.m_clusterId = ZdpClustersServerCmds::NWK_ADDR_RSP;
    apsdeParams.m_srcEndPoint = 0x00; // ZDO endpoint
    apsdeParams.m_asduLength = zdpFrame->GetSize();
    apsdeParams.m_txOptions = 0;    // No special transmission options
    apsdeParams.m_useAlias = false; // Not using alias
    apsdeParams.m_radius = 0;       // Default radius

    m_aps->ApsdeDataRequest(apsdeParams, zdpFrame);
}

void
ZigbeeZdo::HandleNwkAddrRequest(Ptr<Packet> asdu, ApsdeDataIndicationParams dataParams)
{
    NS_LOG_FUNCTION(this);

    // Get the information from the request command
    NwkAddrRequestCmd requestCmd;
    asdu->RemoveHeader(requestCmd);

    Mac64Address requestedIeeeAddr = requestCmd.GetIeeeAddress();

    // Zigbee broadcast addresses.
    bool isBroadcast = dataParams.m_dstAddr16 == "FF:FF" || // All devices
                       dataParams.m_dstAddr16 == "FF:FD" || // macRxOnWhenIdle = TRUE devices
                       dataParams.m_dstAddr16 == "FF:FC" || // All routers and coordinator
                       dataParams.m_dstAddr16 == "FF:FB";   // Low power routers

    // Compare the requested IEEE address to our own IEEE address
    // or to an address known to the NWK address map. The address map
    // is used instead of the neighbor table because the latter might not always
    // contain the necessary information and some stacks use it instead
    // of the neighbor table (opossed to what is described in the specification)
    Mac16Address matchedNwkAddr;
    bool matchFound = false;

    if (m_nwk->GetIeeeAddress() == requestedIeeeAddr)
    {
        // I am the device the NWK_addr_req is looking for.
        matchedNwkAddr = m_nwk->GetNetworkAddress();
        matchFound = true;
    }
    else if (m_nwk->GetNwkAddrByIeeeAddr(requestedIeeeAddr, matchedNwkAddr))
    {
        matchFound = true;
    }

    NwkAddrResponseParams responseParams;
    responseParams.m_dstAddr = dataParams.m_srcAddress16;
    responseParams.m_tsn = requestCmd.GetTsn();
    responseParams.m_ieeeAddrRemoteDev = requestedIeeeAddr;

    if (!matchFound)
    {
        // No match: a broadcast request is silently discarded, a unicast request
        // is answered with a DEVICE_NOT_FOUND status and an unknown short address.
        if (isBroadcast)
        {
            return;
        }

        responseParams.m_status = ZdpStatus::DEVICE_NOT_FOUND;
        responseParams.m_nwkAddrRemoteDev = Mac16Address("FF:FF");
        NwkAddrResponse(responseParams);
        return;
    }

    // A match was found: the request type determines the response.
    responseParams.m_nwkAddrRemoteDev = matchedNwkAddr;

    switch (static_cast<NwkAddrRequestType>(requestCmd.GetRequestType()))
    {
    case NwkAddrRequestType::SINGLE_DEVICE_RESPONSE:
        responseParams.m_status = ZdpStatus::SUCCESS;
        NwkAddrResponse(responseParams);
        break;

    case NwkAddrRequestType::EXTENDED_RESPONSE:
        // TODO: An extended response must also include the list of devices
        // associated to the matched device (NumAssocDev, StartIndex and
        // NWKAddrAssocDevList fields).
        break;

    default:
        // Reserved request type value: a unicast request is answered with an
        // INV_REQUESTTYPE status, a broadcast request is discarded.
        if (!isBroadcast)
        {
            responseParams.m_status = ZdpStatus::INV_REQUESTTYPE;
            NwkAddrResponse(responseParams);
        }
        break;
    }
}

void
ZigbeeZdo::HandleNwkAddrResponse(Ptr<Packet> asdu, ApsdeDataIndicationParams dataParams)
{
    NS_LOG_FUNCTION(this);

    // Get the information from the response command
    NwkAddrResponseCmd responseCmd;
    asdu->RemoveHeader(responseCmd);

    if (static_cast<ZdpStatus>(responseCmd.GetStatus()) == ZdpStatus::SUCCESS)
    {
        // If the response was successful,
        // update the NWK address map with the information received in the response.
        m_nwk->UpdateNwkAddrMap(responseCmd.GetNwkAddrRemoteDev(),
                                responseCmd.GetIeeeAddrRemoteDev());
    }

    // Notify the application framework (AF) that a NWK_addr_rsp was received.
    if (!m_zdoNwkAddrRspIndCallback.IsNull())
    {
        NwkAddrRespIndParams responseIndParams;
        responseIndParams.m_status = static_cast<ZdpStatus>(responseCmd.GetStatus());
        responseIndParams.m_ieeeAddrRemoteDev = responseCmd.GetIeeeAddrRemoteDev();
        responseIndParams.m_nwkAddrRemoteDev = responseCmd.GetNwkAddrRemoteDev();
        m_zdoNwkAddrRspIndCallback(responseIndParams);
    }
}

void
ZigbeeZdo::SetZdoNwkAddrRspIndCallback(ZdoNwkAddrRspIndCallback c)
{
    m_zdoNwkAddrRspIndCallback = c;
}

} // namespace zigbee
} // namespace ns3
