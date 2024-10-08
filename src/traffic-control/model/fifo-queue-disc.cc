/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "fifo-queue-disc.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(FifoQueueDisc);

TypeId
FifoQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FifoQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<FifoQueueDisc>()
            .AddAttribute("MaxSize",
                          "The max queue size",
                          QueueSizeValue(QueueSize("1000p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker());
    return tid;
}

FifoQueueDisc::FifoQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
}

FifoQueueDisc::~FifoQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

bool
FifoQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    if (GetCurrentSize() + item > GetMaxSize())
    {
        NS_LOG_LOGIC("Queue full -- dropping pkt");
        DropBeforeEnqueue(item, LIMIT_EXCEEDED_DROP);
        return false;
    }

    bool retval = GetInternalQueue(0)->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
    // internal queue because QueueDisc::AddInternalQueue sets the trace callback

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return retval;
}

Ptr<QueueDiscItem>
FifoQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item = GetInternalQueue(0)->Dequeue();

    if (!item)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    return item;
}

Ptr<const QueueDiscItem>
FifoQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item = GetInternalQueue(0)->Peek();

    if (!item)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    return item;
}

bool
FifoQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("FifoQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("FifoQueueDisc needs no packet filter");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // add a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("FifoQueueDisc needs 1 internal queue");
        return false;
    }

    return true;
}

void
FifoQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
