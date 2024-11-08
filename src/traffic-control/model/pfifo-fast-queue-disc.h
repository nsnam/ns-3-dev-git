/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 */

#ifndef PFIFO_FAST_H
#define PFIFO_FAST_H

#include "queue-disc.h"

namespace ns3
{

/**
 * @ingroup traffic-control
 *
 * Linux pfifo_fast is the default priority queue enabled on Linux
 * systems. Packets are enqueued in three FIFO droptail queues according
 * to three priority bands based on the packet priority.
 *
 * The system behaves similar to three ns3::DropTail queues operating
 * together, in which packets from higher priority bands are always
 * dequeued before a packet from a lower priority band is dequeued.
 *
 * The queue disc capacity, i.e., the maximum number of packets that can
 * be enqueued in the queue disc, is set through the limit attribute, which
 * plays the same role as txqueuelen in Linux. If no internal queue is
 * provided, three DropTail queues having each a capacity equal to limit are
 * created by default. User is allowed to provide queues, but they must be
 * three, operate in packet mode and each have a capacity not less
 * than limit. No packet filter can be provided.
 */
class PfifoFastQueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * @brief PfifoFastQueueDisc constructor
     *
     * Creates a queue with a depth of 1000 packets per band by default
     */
    PfifoFastQueueDisc();

    ~PfifoFastQueueDisc() override;

    // Reasons for dropping packets
    static constexpr const char* LIMIT_EXCEEDED_DROP =
        "Queue disc limit exceeded"; //!< Packet dropped due to queue disc limit exceeded

  private:
    /**
     * Priority to band map. Values are taken from the prio2band array used by
     * the Linux pfifo_fast queue disc.
     */
    static const uint32_t prio2band[16];

    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
};

} // namespace ns3

#endif /* PFIFO_FAST_H */
