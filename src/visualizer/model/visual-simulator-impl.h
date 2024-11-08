/*
 * Copyright (c) 2010 Gustavo Carneiro
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro  <gjcarneiro@gmail.com>
 */

#ifndef VISUAL_SIMULATOR_IMPL_H
#define VISUAL_SIMULATOR_IMPL_H

#include "ns3/simulator-impl.h"

namespace ns3
{

/**
 * @defgroup  visualizer Visualizer
 *
 */

/**
 * @ingroup visualizer
 * @ingroup simulator
 *
 * @brief A replacement simulator that starts the visualizer
 *
 * To use this class, run any ns-3 simulation with the command-line
 * argument --SimulatorImplementationType=ns3::VisualSimulatorImpl.
 * This causes the visualizer (PyViz) to start automatically.
 **/
class VisualSimulatorImpl : public SimulatorImpl
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    VisualSimulatorImpl();
    ~VisualSimulatorImpl() override;

    // Inherited
    void Destroy() override;
    bool IsFinished() const override;
    void Stop() override;
    EventId Stop(const Time& delay) override;
    EventId Schedule(const Time& delay, EventImpl* event) override;
    void ScheduleWithContext(uint32_t context, const Time& delay, EventImpl* event) override;
    EventId ScheduleNow(EventImpl* event) override;
    EventId ScheduleDestroy(EventImpl* event) override;
    void Remove(const EventId& id) override;
    void Cancel(const EventId& id) override;
    bool IsExpired(const EventId& id) const override;
    void Run() override;
    Time Now() const override;
    Time GetDelayLeft(const EventId& id) const override;
    Time GetMaximumSimulationTime() const override;
    void SetScheduler(ObjectFactory schedulerFactory) override;
    uint32_t GetSystemId() const override;
    uint32_t GetContext() const override;
    uint64_t GetEventCount() const override;

    /// calls Run() in the wrapped simulator
    void RunRealSimulator();

  protected:
    void DoDispose() override;
    void NotifyConstructionCompleted() override;

  private:
    Ptr<SimulatorImpl> m_simulator;       ///< the simulator implementation
    ObjectFactory m_simulatorImplFactory; ///< simulator implementation factory
};

} // namespace ns3

#endif /* DEFAULT_SIMULATOR_IMPL_H */
