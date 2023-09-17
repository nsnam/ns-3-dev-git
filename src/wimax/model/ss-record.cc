/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "ss-record.h"

#include "service-flow.h"

#include <stdint.h>

namespace ns3
{

SSRecord::SSRecord()
{
    Initialize();
}

SSRecord::SSRecord(Mac48Address macAddress)
{
    m_macAddress = macAddress;
    Initialize();
}

SSRecord::SSRecord(Mac48Address macAddress, Ipv4Address IPaddress)
{
    m_macAddress = macAddress;
    m_IPAddress = IPaddress;
    Initialize();
}

void
SSRecord::Initialize()
{
    m_basicCid = Cid();
    m_primaryCid = Cid();

    m_rangingCorrectionRetries = 0;
    m_invitedRangingRetries = 0;
    m_modulationType = WimaxPhy::MODULATION_TYPE_BPSK_12;
    m_rangingStatus = WimaxNetDevice::RANGING_STATUS_EXPIRED;
    m_pollForRanging = false;
    m_areServiceFlowsAllocated = false;
    m_pollMeBit = false;

    m_sfTransactionId = 0;
    m_dsaRspRetries = 0;

    m_serviceFlows = new std::vector<ServiceFlow*>();
    m_dsaRsp = DsaRsp();
    m_broadcast = false;
}

SSRecord::~SSRecord()
{
    delete m_serviceFlows;
    m_serviceFlows = nullptr;
}

void
SSRecord::SetIPAddress(Ipv4Address IPAddress)
{
    m_IPAddress = IPAddress;
}

Ipv4Address
SSRecord::GetIPAddress()
{
    return m_IPAddress;
}

void
SSRecord::SetBasicCid(Cid basicCid)
{
    m_basicCid = basicCid;
}

Cid
SSRecord::GetBasicCid() const
{
    return m_basicCid;
}

void
SSRecord::SetPrimaryCid(Cid primaryCid)
{
    m_primaryCid = primaryCid;
}

Cid
SSRecord::GetPrimaryCid() const
{
    return m_primaryCid;
}

void
SSRecord::SetMacAddress(Mac48Address macAddress)
{
    m_macAddress = macAddress;
}

Mac48Address
SSRecord::GetMacAddress() const
{
    return m_macAddress;
}

uint8_t
SSRecord::GetRangingCorrectionRetries() const
{
    return m_rangingCorrectionRetries;
}

void
SSRecord::ResetRangingCorrectionRetries()
{
    m_rangingCorrectionRetries = 0;
}

void
SSRecord::IncrementRangingCorrectionRetries()
{
    m_rangingCorrectionRetries++;
}

uint8_t
SSRecord::GetInvitedRangRetries() const
{
    return m_invitedRangingRetries;
}

void
SSRecord::ResetInvitedRangingRetries()
{
    m_invitedRangingRetries = 0;
}

void
SSRecord::IncrementInvitedRangingRetries()
{
    m_invitedRangingRetries++;
}

void
SSRecord::SetModulationType(WimaxPhy::ModulationType modulationType)
{
    m_modulationType = modulationType;
}

WimaxPhy::ModulationType
SSRecord::GetModulationType() const
{
    return m_modulationType;
}

void
SSRecord::SetRangingStatus(WimaxNetDevice::RangingStatus rangingStatus)
{
    m_rangingStatus = rangingStatus;
}

WimaxNetDevice::RangingStatus
SSRecord::GetRangingStatus() const
{
    return m_rangingStatus;
}

void
SSRecord::EnablePollForRanging()
{
    m_pollForRanging = true;
}

void
SSRecord::DisablePollForRanging()
{
    m_pollForRanging = false;
}

bool
SSRecord::GetPollForRanging() const
{
    return m_pollForRanging;
}

void
SSRecord::SetAreServiceFlowsAllocated(bool val)
{
    m_areServiceFlowsAllocated = val;
}

bool
SSRecord::GetAreServiceFlowsAllocated() const
{
    return m_areServiceFlowsAllocated;
}

void
SSRecord::SetPollMeBit(bool pollMeBit)
{
    m_pollMeBit = pollMeBit;
}

bool
SSRecord::GetPollMeBit() const
{
    return m_pollMeBit;
}

void
SSRecord::AddServiceFlow(ServiceFlow* serviceFlow)
{
    m_serviceFlows->push_back(serviceFlow);
}

std::vector<ServiceFlow*>
SSRecord::GetServiceFlows(ServiceFlow::SchedulingType schedulingType) const
{
    std::vector<ServiceFlow*> tmpServiceFlows;
    for (auto iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if (((*iter)->GetSchedulingType() == schedulingType) ||
            (schedulingType == ServiceFlow::SF_TYPE_ALL))
        {
            tmpServiceFlows.push_back((*iter));
        }
    }
    return tmpServiceFlows;
}

void
SSRecord::SetIsBroadcastSS(bool broadcast_enable)
{
    m_broadcast = broadcast_enable;
}

bool
SSRecord::GetIsBroadcastSS() const
{
    return m_broadcast;
}

bool
SSRecord::GetHasServiceFlowUgs() const
{
    for (auto iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if ((*iter)->GetSchedulingType() == ServiceFlow::SF_TYPE_UGS)
        {
            return true;
        }
    }
    return false;
}

bool
SSRecord::GetHasServiceFlowRtps() const
{
    for (auto iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if ((*iter)->GetSchedulingType() == ServiceFlow::SF_TYPE_RTPS)
        {
            return true;
        }
    }
    return false;
}

bool
SSRecord::GetHasServiceFlowNrtps() const
{
    for (auto iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if ((*iter)->GetSchedulingType() == ServiceFlow::SF_TYPE_NRTPS)
        {
            return true;
        }
    }
    return false;
}

bool
SSRecord::GetHasServiceFlowBe() const
{
    for (auto iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if ((*iter)->GetSchedulingType() == ServiceFlow::SF_TYPE_BE)
        {
            return true;
        }
    }
    return false;
}

void
SSRecord::SetSfTransactionId(uint16_t sfTransactionId)
{
    m_sfTransactionId = sfTransactionId;
}

uint16_t
SSRecord::GetSfTransactionId() const
{
    return m_sfTransactionId;
}

void
SSRecord::SetDsaRspRetries(uint8_t dsaRspRetries)
{
    m_dsaRspRetries = dsaRspRetries;
}

void
SSRecord::IncrementDsaRspRetries()
{
    m_dsaRspRetries++;
}

uint8_t
SSRecord::GetDsaRspRetries() const
{
    return m_dsaRspRetries;
}

void
SSRecord::SetDsaRsp(DsaRsp dsaRsp)
{
    m_dsaRsp = dsaRsp;
}

DsaRsp
SSRecord::GetDsaRsp() const
{
    return m_dsaRsp;
}

} // namespace ns3
