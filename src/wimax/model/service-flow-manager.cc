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
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "service-flow-manager.h"

#include "bs-net-device.h"
#include "bs-uplink-scheduler.h"
#include "connection-manager.h"
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

#include <stdint.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ServiceFlowManager");

NS_OBJECT_ENSURE_REGISTERED(ServiceFlowManager);

TypeId
ServiceFlowManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ServiceFlowManager").SetParent<Object>().SetGroupName("Wimax");
    return tid;
}

ServiceFlowManager::ServiceFlowManager()
{
    m_serviceFlows = new std::vector<ServiceFlow*>;
}

ServiceFlowManager::~ServiceFlowManager()
{
}

void
ServiceFlowManager::DoDispose()
{
    for (std::vector<ServiceFlow*>::iterator iter = m_serviceFlows->begin();
         iter != m_serviceFlows->end();
         ++iter)
    {
        delete (*iter);
    }
    m_serviceFlows->clear();
    delete m_serviceFlows;
}

void
ServiceFlowManager::AddServiceFlow(ServiceFlow* serviceFlow)
{
    m_serviceFlows->push_back(serviceFlow);
}

ServiceFlow*
ServiceFlowManager::DoClassify(Ipv4Address srcAddress,
                               Ipv4Address dstAddress,
                               uint16_t srcPort,
                               uint16_t dstPort,
                               uint8_t proto,
                               ServiceFlow::Direction dir) const
{
    for (std::vector<ServiceFlow*>::iterator iter = m_serviceFlows->begin();
         iter != m_serviceFlows->end();
         ++iter)
    {
        if ((*iter)->GetDirection() == dir)
        {
            if ((*iter)->CheckClassifierMatch(srcAddress, dstAddress, srcPort, dstPort, proto))
            {
                return (*iter);
            }
        }
    }
    return nullptr;
}

ServiceFlow*
ServiceFlowManager::GetServiceFlow(uint32_t sfid) const
{
    for (std::vector<ServiceFlow*>::iterator iter = m_serviceFlows->begin();
         iter != m_serviceFlows->end();
         ++iter)
    {
        if ((*iter)->GetSfid() == sfid)
        {
            return (*iter);
        }
    }

    NS_LOG_DEBUG("GetServiceFlow: service flow not found!");
    return nullptr;
}

ServiceFlow*
ServiceFlowManager::GetServiceFlow(Cid cid) const
{
    for (std::vector<ServiceFlow*>::iterator iter = m_serviceFlows->begin();
         iter != m_serviceFlows->end();
         ++iter)
    {
        if ((*iter)->GetCid() == cid.GetIdentifier())
        {
            return (*iter);
        }
    }

    NS_LOG_DEBUG("GetServiceFlow: service flow not found!");
    return nullptr;
}

std::vector<ServiceFlow*>
ServiceFlowManager::GetServiceFlows(ServiceFlow::SchedulingType schedulingType) const
{
    std::vector<ServiceFlow*> tmpServiceFlows;
    for (std::vector<ServiceFlow*>::iterator iter = m_serviceFlows->begin();
         iter != m_serviceFlows->end();
         ++iter)
    {
        if (((*iter)->GetSchedulingType() == schedulingType) ||
            (schedulingType == ServiceFlow::SF_TYPE_ALL))
        {
            tmpServiceFlows.push_back((*iter));
        }
    }
    return tmpServiceFlows;
}

bool
ServiceFlowManager::AreServiceFlowsAllocated()
{
    return AreServiceFlowsAllocated(m_serviceFlows);
}

bool
ServiceFlowManager::AreServiceFlowsAllocated(std::vector<ServiceFlow*>* serviceFlowVector)
{
    return AreServiceFlowsAllocated(*serviceFlowVector);
}

bool
ServiceFlowManager::AreServiceFlowsAllocated(std::vector<ServiceFlow*> serviceFlowVector)
{
    for (std::vector<ServiceFlow*>::const_iterator iter = serviceFlowVector.begin();
         iter != serviceFlowVector.end();
         ++iter)
    {
        if (!(*iter)->GetIsEnabled())
        {
            return false;
        }
    }
    return true;
}

ServiceFlow*
ServiceFlowManager::GetNextServiceFlowToAllocate()
{
    std::vector<ServiceFlow*>::iterator iter;
    for (iter = m_serviceFlows->begin(); iter != m_serviceFlows->end(); ++iter)
    {
        if (!(*iter)->GetIsEnabled())
        {
            return (*iter);
        }
    }
    return nullptr;
}

uint32_t
ServiceFlowManager::GetNrServiceFlows() const
{
    return m_serviceFlows->size();
}

} // namespace ns3
