/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef MOCK_EVENT_GENERATOR_H
#define MOCK_EVENT_GENERATOR_H

#include "coex-types.h"
#include "device-coex-manager.h"

#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace coex
{

/**
 * @ingroup coex
 * @brief A mock Coex events generator.
 *
 * A device coex manager subclass that generates coex events according to a given probability
 * distribution. This coex manager shall be registered for the UNDEFINED coex RAT.
 */
class MockEventGenerator : public coex::DeviceManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    MockEventGenerator();
    ~MockEventGenerator() override;

    void ResourceBusyStart(const Event& coexEvent) override;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * Set the random variable used to derive the interval between consecutive coex events.
     *
     * @param rv the random variable used to derive the interval between consecutive coex events
     */
    void SetInterEventTimeRv(Ptr<RandomVariableStream> rv);

    /// @return the random variable used to derive the interval between consecutive coex events
    Ptr<RandomVariableStream> GetInterEventTimeRv() const;

  protected:
    void DoDispose() override;

    /// Generate a coex event and schedule the next one.
    void GenerateCoexEvent();

  private:
    Ptr<RandomVariableStream> m_interEventTime;  //!< interval between consecutive coex events
    Ptr<RandomVariableStream> m_eventStartDelay; //!< delay between notification and event start
    Ptr<RandomVariableStream> m_eventDuration;   //!< duration of a coex event
    EventId m_nextCoexEvent;                     //!< next coex event generation
};

} // namespace coex
} // namespace ns3

#endif /* MOCK_EVENT_GENERATOR_H */
