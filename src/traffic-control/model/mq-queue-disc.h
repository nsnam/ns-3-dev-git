/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef MQ_QUEUE_DISC_H
#define MQ_QUEUE_DISC_H

#include "queue-disc.h"

namespace ns3
{

/**
 * @ingroup traffic-control
 *
 * mq is a classful multi-queue aware dummy scheduler. It has as many child
 * queue discs as the number of device transmission queues. Packets are
 * directly enqueued into and dequeued from child queue discs.
 */
class MqQueueDisc : public QueueDisc
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * @brief MqQueueDisc constructor
     */
    MqQueueDisc();

    ~MqQueueDisc() override;

    /**
     * @brief Return the wake mode adopted by this queue disc.
     * @return the wake mode adopted by this queue disc.
     */
    WakeMode GetWakeMode() const override;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
};

} // namespace ns3

#endif /* MQ_QUEUE_DISC_H */
