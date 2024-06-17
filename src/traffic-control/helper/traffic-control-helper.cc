/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "traffic-control-helper.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/pointer.h"
#include "ns3/queue-limits.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TrafficControlHelper");

QueueDiscFactory::QueueDiscFactory(ObjectFactory factory)
    : m_queueDiscFactory(factory)
{
}

void
QueueDiscFactory::AddInternalQueue(ObjectFactory factory)
{
    m_internalQueuesFactory.push_back(factory);
}

void
QueueDiscFactory::AddPacketFilter(ObjectFactory factory)
{
    m_packetFiltersFactory.push_back(factory);
}

uint16_t
QueueDiscFactory::AddQueueDiscClass(ObjectFactory factory)
{
    m_queueDiscClassesFactory.push_back(factory);
    return static_cast<uint16_t>(m_queueDiscClassesFactory.size() - 1);
}

void
QueueDiscFactory::SetChildQueueDisc(uint16_t classId, uint16_t handle)
{
    NS_ABORT_MSG_IF(classId >= m_queueDiscClassesFactory.size(),
                    "Cannot attach a queue disc to a non existing class");
    m_classIdChildHandleMap[classId] = handle;
}

Ptr<QueueDisc>
QueueDiscFactory::CreateQueueDisc(const std::vector<Ptr<QueueDisc>>& queueDiscs)
{
    // create the queue disc
    Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc>();

    // create and add the internal queues
    for (auto i = m_internalQueuesFactory.begin(); i != m_internalQueuesFactory.end(); i++)
    {
        qd->AddInternalQueue(i->Create<QueueDisc::InternalQueue>());
    }

    // create and add the packet filters
    for (auto i = m_packetFiltersFactory.begin(); i != m_packetFiltersFactory.end(); i++)
    {
        qd->AddPacketFilter(i->Create<PacketFilter>());
    }

    // create and add the queue disc classes
    for (std::size_t i = 0; i < m_queueDiscClassesFactory.size(); i++)
    {
        // the class ID is given by the index i of the vector
        NS_ABORT_MSG_IF(m_classIdChildHandleMap.find(i) == m_classIdChildHandleMap.end(),
                        "Cannot create a queue disc class with no attached queue disc");

        uint16_t handle = m_classIdChildHandleMap[i];
        NS_ABORT_MSG_IF(handle >= queueDiscs.size() || !queueDiscs[handle],
                        "A queue disc with handle " << handle << " has not been created yet");

        m_queueDiscClassesFactory[i].Set("QueueDisc", PointerValue(queueDiscs[handle]));
        qd->AddQueueDiscClass(m_queueDiscClassesFactory[i].Create<QueueDiscClass>());
    }

    return qd;
}

TrafficControlHelper::TrafficControlHelper()
{
}

TrafficControlHelper
TrafficControlHelper::Default(std::size_t nTxQueues)
{
    NS_LOG_FUNCTION(nTxQueues);
    NS_ABORT_MSG_IF(nTxQueues == 0, "The device must have at least one queue");
    TrafficControlHelper helper;

    if (nTxQueues == 1)
    {
        helper.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    }
    else
    {
        uint16_t handle = helper.SetRootQueueDisc("ns3::MqQueueDisc");
        ClassIdList cls = helper.AddQueueDiscClasses(handle, nTxQueues, "ns3::QueueDiscClass");
        helper.AddChildQueueDiscs(handle, cls, "ns3::FqCoDelQueueDisc");
    }
    return helper;
}

uint16_t
TrafficControlHelper::DoSetRootQueueDisc(ObjectFactory factory)
{
    NS_ABORT_MSG_UNLESS(m_queueDiscFactory.empty(),
                        "A root queue disc has been already added to this factory");

    m_queueDiscFactory.emplace_back(factory);
    return 0;
}

void
TrafficControlHelper::DoAddInternalQueues(uint16_t handle, uint16_t count, ObjectFactory factory)
{
    NS_ABORT_MSG_IF(handle >= m_queueDiscFactory.size(),
                    "A queue disc with handle " << handle << " does not exist");

    for (int i = 0; i < count; i++)
    {
        m_queueDiscFactory[handle].AddInternalQueue(factory);
    }
}

void
TrafficControlHelper::DoAddPacketFilter(uint16_t handle, ObjectFactory factory)
{
    NS_ABORT_MSG_IF(handle >= m_queueDiscFactory.size(),
                    "A queue disc with handle " << handle << " does not exist");

    m_queueDiscFactory[handle].AddPacketFilter(factory);
}

TrafficControlHelper::ClassIdList
TrafficControlHelper::DoAddQueueDiscClasses(uint16_t handle, uint16_t count, ObjectFactory factory)
{
    NS_ABORT_MSG_IF(handle >= m_queueDiscFactory.size(),
                    "A queue disc with handle " << handle << " does not exist");

    ClassIdList list;
    uint16_t classId;

    for (int i = 0; i < count; i++)
    {
        classId = m_queueDiscFactory[handle].AddQueueDiscClass(factory);
        list.push_back(classId);
    }
    return list;
}

uint16_t
TrafficControlHelper::DoAddChildQueueDisc(uint16_t handle, uint16_t classId, ObjectFactory factory)
{
    NS_ABORT_MSG_IF(handle >= m_queueDiscFactory.size(),
                    "A queue disc with handle " << handle << " does not exist");

    auto childHandle = static_cast<uint16_t>(m_queueDiscFactory.size());
    m_queueDiscFactory.emplace_back(factory);
    m_queueDiscFactory[handle].SetChildQueueDisc(classId, childHandle);

    return childHandle;
}

TrafficControlHelper::HandleList
TrafficControlHelper::DoAddChildQueueDiscs(uint16_t handle,
                                           const TrafficControlHelper::ClassIdList& classes,
                                           ObjectFactory factory)
{
    HandleList list;
    for (auto c = classes.begin(); c != classes.end(); c++)
    {
        uint16_t childHandle = DoAddChildQueueDisc(handle, *c, factory);
        list.push_back(childHandle);
    }
    return list;
}

QueueDiscContainer
TrafficControlHelper::Install(Ptr<NetDevice> d)
{
    QueueDiscContainer container;

    // A TrafficControlLayer object is aggregated by the InternetStackHelper, but check
    // anyway because a queue disc has no effect without a TrafficControlLayer object
    Ptr<TrafficControlLayer> tc = d->GetNode()->GetObject<TrafficControlLayer>();
    NS_ASSERT(tc);

    // Start from an empty vector of queue discs
    m_queueDiscs.clear();
    m_queueDiscs.resize(m_queueDiscFactory.size());

    // Create queue discs (from leaves to root)
    for (auto i = m_queueDiscFactory.size(); i-- > 0;)
    {
        m_queueDiscs[i] = m_queueDiscFactory[i].CreateQueueDisc(m_queueDiscs);
    }

    // Set the root queue disc (if any has been created) on the device
    if (!m_queueDiscs.empty() && m_queueDiscs[0])
    {
        tc->SetRootQueueDiscOnDevice(d, m_queueDiscs[0]);
        container.Add(m_queueDiscs[0]);
    }

    // Queue limits objects can only be installed if a netdevice queue interface
    // has been aggregated to the netdevice. This is normally the case if the
    // netdevice has been created via helpers. Abort the simulation if not.
    if (m_queueLimitsFactory.GetTypeId().GetUid())
    {
        Ptr<NetDeviceQueueInterface> ndqi = d->GetObject<NetDeviceQueueInterface>();
        NS_ABORT_MSG_IF(!ndqi,
                        "A NetDeviceQueueInterface object has not been"
                        "aggregated to the NetDevice");
        for (std::size_t i = 0; i < ndqi->GetNTxQueues(); i++)
        {
            Ptr<QueueLimits> ql = m_queueLimitsFactory.Create<QueueLimits>();
            ndqi->GetTxQueue(i)->SetQueueLimits(ql);
        }
    }

    return container;
}

QueueDiscContainer
TrafficControlHelper::Install(NetDeviceContainer c)
{
    QueueDiscContainer container;

    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        container.Add(Install(*i));
    }

    return container;
}

void
TrafficControlHelper::Uninstall(Ptr<NetDevice> d)
{
    Ptr<TrafficControlLayer> tc = d->GetNode()->GetObject<TrafficControlLayer>();
    NS_ASSERT(tc);

    tc->DeleteRootQueueDiscOnDevice(d);
    // remove the queue limits objects installed on the device transmission queues
    Ptr<NetDeviceQueueInterface> ndqi = d->GetObject<NetDeviceQueueInterface>();
    // if a queue disc has been installed on the device, a netdevice queue interface
    // must have been aggregated to the device
    NS_ASSERT(ndqi);
    for (std::size_t i = 0; i < ndqi->GetNTxQueues(); i++)
    {
        ndqi->GetTxQueue(i)->SetQueueLimits(nullptr);
    }
}

void
TrafficControlHelper::Uninstall(NetDeviceContainer c)
{
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Uninstall(*i);
    }
}

} // namespace ns3
