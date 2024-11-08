/*
 * Copyright (c) 2007 Emmanuelle Laprise
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca
 * Derived from the p2p net device file
Transmi */

#ifndef BACKOFF_H
#define BACKOFF_H

#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup csma
 * @brief The backoff class is used for calculating backoff times
 * when many net devices can write to the same channel
 */

class Backoff
{
  public:
    /**
     * Minimum number of backoff slots (when multiplied by m_slotTime, determines minimum backoff
     * time)
     */
    uint32_t m_minSlots;

    /**
     * Maximum number of backoff slots (when multiplied by m_slotTime, determines maximum backoff
     * time)
     */
    uint32_t m_maxSlots;

    /**
     * Caps the exponential function when the number of retries reaches m_ceiling.
     */
    uint32_t m_ceiling;

    /**
     * Maximum number of transmission retries before the packet is dropped.
     */
    uint32_t m_maxRetries;

    /**
     * Length of one slot. A slot time, it usually the packet transmission time, if the packet size
     * is fixed.
     */
    Time m_slotTime;

    Backoff();
    /**
     * @brief Constructor
     * @param slotTime Length of one slot
     * @param minSlots Minimum number of backoff slots
     * @param maxSlots Maximum number of backoff slots
     * @param ceiling Cap to the exponential function
     * @param maxRetries Maximum number of transmission retries
     */
    Backoff(Time slotTime,
            uint32_t minSlots,
            uint32_t maxSlots,
            uint32_t ceiling,
            uint32_t maxRetries);

    /**
     * @return The amount of time that the net device should wait before
     * trying to retransmit the packet
     */
    Time GetBackoffTime();

    /**
     * Indicates to the backoff object that the last packet was
     * successfully transmitted and that the number of retries should be
     * reset to 0.
     */
    void ResetBackoffTime();

    /**
     * @return True if the maximum number of retries has been reached
     */
    bool MaxRetriesReached() const;

    /**
     * Increments the number of retries by 1.
     */
    void IncrNumRetries();

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  private:
    /**
     * Number of times that the transmitter has tried to unsuccessfuly transmit the current packet.
     */
    uint32_t m_numBackoffRetries;

    /**
     * Random number generator
     */
    Ptr<UniformRandomVariable> m_rng;
};

} // namespace ns3

#endif /* BACKOFF_H */
