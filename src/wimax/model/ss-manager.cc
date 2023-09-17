/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
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
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#include "ss-manager.h"

#include "service-flow.h"

#include "ns3/log.h"

#include <stdint.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SSManager");

NS_OBJECT_ENSURE_REGISTERED(SSManager);

TypeId
SSManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SSManager").SetParent<Object>().SetGroupName("Wimax");
    return tid;
}

SSManager::SSManager()
{
    m_ssRecords = new std::vector<SSRecord*>();
}

SSManager::~SSManager()
{
    for (auto iter = m_ssRecords->begin(); iter != m_ssRecords->end(); ++iter)
    {
        delete *iter;
    }
    delete m_ssRecords;
    m_ssRecords = nullptr;
}

SSRecord*
SSManager::CreateSSRecord(const Mac48Address& macAddress)
{
    auto ssRecord = new SSRecord(macAddress);
    m_ssRecords->push_back(ssRecord);
    return ssRecord;
}

SSRecord*
SSManager::GetSSRecord(const Mac48Address& macAddress) const
{
    for (auto iter = m_ssRecords->begin(); iter != m_ssRecords->end(); ++iter)
    {
        if ((*iter)->GetMacAddress() == macAddress)
        {
            return *iter;
        }
    }

    NS_LOG_DEBUG("GetSSRecord: SSRecord not found!");
    return nullptr;
}

SSRecord*
SSManager::GetSSRecord(Cid cid) const
{
    for (auto iter1 = m_ssRecords->begin(); iter1 != m_ssRecords->end(); ++iter1)
    {
        SSRecord* ssRecord = *iter1;
        if (ssRecord->GetBasicCid() == cid || ssRecord->GetPrimaryCid() == cid)
        {
            return ssRecord;
        }
        else
        {
            std::vector<ServiceFlow*> sf = ssRecord->GetServiceFlows(ServiceFlow::SF_TYPE_ALL);
            for (auto iter2 = sf.begin(); iter2 != sf.end(); ++iter2)
            {
                if ((*iter2)->GetConnection()->GetCid() == cid)
                {
                    return ssRecord;
                }
            }
        }
    }

    NS_LOG_DEBUG("GetSSRecord: SSRecord not found!");
    return nullptr;
}

std::vector<SSRecord*>*
SSManager::GetSSRecords() const
{
    return m_ssRecords;
}

bool
SSManager::IsInRecord(const Mac48Address& macAddress) const
{
    for (auto iter = m_ssRecords->begin(); iter != m_ssRecords->end(); ++iter)
    {
        if ((*iter)->GetMacAddress() == macAddress)
        {
            return true;
        }
    }
    return false;
}

bool
SSManager::IsRegistered(const Mac48Address& macAddress) const
{
    SSRecord* ssRecord = GetSSRecord(macAddress);
    return ssRecord != nullptr &&
           ssRecord->GetRangingStatus() == WimaxNetDevice::RANGING_STATUS_SUCCESS;
}

void
SSManager::DeleteSSRecord(Cid cid)
{
    for (auto iter1 = m_ssRecords->begin(); iter1 != m_ssRecords->end(); ++iter1)
    {
        SSRecord* ssRecord = *iter1;
        if (ssRecord->GetBasicCid() == cid || ssRecord->GetPrimaryCid() == cid)
        {
            m_ssRecords->erase(iter1);
            return;
        }
        else
        {
            std::vector<ServiceFlow*> sf = ssRecord->GetServiceFlows(ServiceFlow::SF_TYPE_ALL);
            for (auto iter2 = sf.begin(); iter2 != sf.end(); ++iter2)
            {
                if ((*iter2)->GetConnection()->GetCid() == cid)
                {
                    m_ssRecords->erase(iter1);
                    return;
                }
            }
        }
    }
}

Mac48Address
SSManager::GetMacAddress(Cid cid) const
{
    return GetSSRecord(cid)->GetMacAddress();
}

uint32_t
SSManager::GetNSSs() const
{
    return m_ssRecords->size();
}

uint32_t
SSManager::GetNRegisteredSSs() const
{
    uint32_t nrSS = 0;
    for (auto iter = m_ssRecords->begin(); iter != m_ssRecords->end(); ++iter)
    {
        if ((*iter)->GetRangingStatus() == WimaxNetDevice::RANGING_STATUS_SUCCESS)
        {
            nrSS++;
        }
    }
    return nrSS;
}

} // namespace ns3
