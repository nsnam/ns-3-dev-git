/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "mq-queue-disc.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MqQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(MqQueueDisc);

TypeId
MqQueueDisc::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MqQueueDisc")
                            .SetParent<QueueDisc>()
                            .SetGroupName("TrafficControl")
                            .AddConstructor<MqQueueDisc>();
    return tid;
}

MqQueueDisc::MqQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::NO_LIMITS)
{
    NS_LOG_FUNCTION(this);
}

MqQueueDisc::~MqQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

MqQueueDisc::WakeMode
MqQueueDisc::GetWakeMode() const
{
    return WAKE_CHILD;
}

bool
MqQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_FATAL_ERROR("MqQueueDisc: DoEnqueue should never be called");
}

Ptr<QueueDiscItem>
MqQueueDisc::DoDequeue()
{
    NS_FATAL_ERROR("MqQueueDisc: DoDequeue should never be called");
}

Ptr<const QueueDiscItem>
MqQueueDisc::DoPeek()
{
    NS_FATAL_ERROR("MqQueueDisc: DoPeek should never be called");
}

bool
MqQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("MqQueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("MqQueueDisc cannot have internal queues");
        return false;
    }

    return true;
}

void
MqQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
