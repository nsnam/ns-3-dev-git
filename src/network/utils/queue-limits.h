/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
 */

#ifndef QUEUE_LIMITS_H
#define QUEUE_LIMITS_H

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup network
 *
 * @brief Abstract base class for NetDevice queue length controller
 *
 * QueueLimits is an abstract base class providing the interface to
 * the NetDevice queue length controller.
 *
 * Child classes need to implement the methods used for a byte-based
 * measure of the queue length.
 *
 * The design and implementation of this class is inspired by Linux.
 * For more details, see the queue limits Sphinx documentation.
 */
class QueueLimits : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ~QueueLimits() override;

    /**
     * @brief Reset queue limits state
     */
    virtual void Reset() = 0;

    /**
     * @brief Record number of completed bytes and recalculate the limit
     * @param count the number of completed bytes
     */
    virtual void Completed(uint32_t count) = 0;

    /**
     * Available is called from NotifyTransmittedBytes to calculate the
     * number of bytes that can be passed again to the NetDevice.
     * A negative value means that no packets can be passed to the NetDevice.
     * In this case, NotifyTransmittedBytes stops the transmission queue.
     * @brief Returns how many bytes can be queued
     * @return the number of bytes that can be queued
     */
    virtual int32_t Available() const = 0;

    /**
     * @brief Record the number of bytes queued
     * @param count the number of bytes queued
     */
    virtual void Queued(uint32_t count) = 0;
};

} // namespace ns3

#endif /* QUEUE_LIMITS_H */
