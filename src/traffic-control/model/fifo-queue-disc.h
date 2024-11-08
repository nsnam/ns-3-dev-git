/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#ifndef FIFO_QUEUE_DISC_H
#define FIFO_QUEUE_DISC_H

#include "queue-disc.h"

namespace ns3
{

/**
 * @ingroup traffic-control
 *
 * Simple queue disc implementing the FIFO (First-In First-Out) policy.
 *
 */
class FifoQueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * @brief FifoQueueDisc constructor
     *
     * Creates a queue with a depth of 1000 packets by default
     */
    FifoQueueDisc();

    ~FifoQueueDisc() override;

    // Reasons for dropping packets
    static constexpr const char* LIMIT_EXCEEDED_DROP =
        "Queue disc limit exceeded"; //!< Packet dropped due to queue disc limit exceeded

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
};

} // namespace ns3

#endif /* FIFO_QUEUE_DISC_H */
