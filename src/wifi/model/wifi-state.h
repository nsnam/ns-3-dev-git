/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_STATE_H
#define WIFI_STATE_H

#include "ns3/simulator.h"
#include "ns3/time-span.h"

#include <optional>

namespace ns3
{

/**
 * @ingroup wifi
 * Track the current state of a variable and the most recent interval spent in each state.
 * It can be chosen whether an empty interval should be tracked or not.
 *
 * @tparam State the state type, whose values are unique and can be sorted with operator<
 */
template <class State>
class WifiState
{
  public:
    /**
     * Constructor.
     *
     * @param initial the initial state
     * @param ignoreEmptyInterval true to ignore empty intervals (i.e., two transitions A->B->A
     *                            occurring at the same simulation time do not update the most
     *                            recent interval spent in state B nor the start time of the current
     *                            state A), false to record empty intervals
     */
    WifiState(State initial, bool ignoreEmptyInterval = true);

    /**
     * Set the given state as the current state. The current state start time is set equal to the
     * current simulation time, unless:
     * - the given state is already the current state, OR
     * - empty intervals are ignored and the given state is the last previous state that has been
     *   set for a nonempty interval of time,
     * in which cases the current state start time is not modified.
     *
     * @param state the new current state
     */
    void SetState(State state);

    /**
     * @return the current state
     */
    State GetCurrent() const;

    /**
     * @param state a given state
     * @return the last time the given state became the current state (if it ever became the
     *         current state)
     */
    std::optional<Time> GetLastStart(State state) const;

    /**
     * @param state a given state
     * @return the end of the state's last completed interval, or the current simulation time
     *         if the given state is the current state; std::nullopt if no such interval exists
     */
    std::optional<Time> GetLastEnd(State state) const;

    /**
     * @param state a given state
     * @return the end of the state's last completed interval; std::nullopt if no such interval
     *         exists
     */
    std::optional<Time> GetLastCompletedEnd(State state) const;

  private:
    /**
     * Information about a state.
     */
    struct Elem
    {
        State state{}; ///< the current state
        Time start;    ///< the time when the state became the current state
    };

    /**
     * Record the given element in the list of completed intervals.
     *
     * @param elem the state and start time of the completed interval
     * @param end the end time of the completed interval
     */
    void MoveToCompleted(const Elem& elem, const Time& end);

    Elem m_current;                        ///< the current state
    std::map<State, TimeSpan> m_completed; ///< Last completed interval for each visited state
    bool m_ignoreEmptyInterval; ///< true to ignore empty intervals, false to record empty intervals

    // If empty intervals are ignored, we need a "pending" completed interval to record an interval
    // just completed, because the next state may be followed at the same simulation time by the
    // state associated with the pending interval, which becomes again the current state, whose
    // start time is taken from the pending interval. For example, while the current state is A, two
    // consecutive state transitions (A->B and B->A) occurring at the same simulation time are not
    // recorded, it is just like the transition to B never occurred:
    // current = (A, t0)
    // t1) Set(B) -> current = (B, t1), pending = (A, t0)
    // t1) Set(A) -> current = (A, t0)

    std::optional<Elem> m_pending; ///< a completed interval to be finalized once the current state
                                   ///< is held for a nonempty time interval
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <class State>
WifiState<State>::WifiState(State initial, bool ignoreEmptyInterval)
    : m_current({.state = initial, .start = Simulator::Now()}),
      m_ignoreEmptyInterval(ignoreEmptyInterval)
{
}

template <class State>
void
WifiState<State>::MoveToCompleted(const Elem& elem, const Time& end)
{
    m_completed[elem.state] = TimeSpan{elem.start, end};
}

template <class State>
void
WifiState<State>::SetState(State state)
{
    if (m_current.state == state)
    {
        return;
    }

    const auto now = Simulator::Now();
    if (!m_ignoreEmptyInterval)
    {
        MoveToCompleted(m_current, now);
        m_current = {state, now};
        return;
    }

    if (now > m_current.start)
    {
        // the pending interval, if any, can be considered completed
        if (m_pending.has_value())
        {
            MoveToCompleted(*m_pending, m_current.start);
        }
        m_pending = m_current;
        m_current = {state, now};
        return;
    }

    // we get here if the interval corresponding to the current state is empty, hence the current
    // state shall not be recorded

    if (m_pending.has_value() && (state == m_pending->state))
    {
        // the pending interval becomes the current interval again
        m_current = *m_pending;
        m_pending.reset();
    }
    else
    {
        m_current.state = state;
    }
}

template <class State>
State
WifiState<State>::GetCurrent() const
{
    return m_current.state;
}

template <class State>
std::optional<Time>
WifiState<State>::GetLastStart(State state) const
{
    if (m_current.state == state)
    {
        return m_current.start;
    }

    if (m_pending.has_value() && (m_pending->state == state))
    {
        return m_pending->start;
    }

    const auto it = m_completed.find(state);
    return (it != m_completed.cend()) ? std::make_optional(it->second.Begin()) : std::nullopt;
}

template <class State>
std::optional<Time>
WifiState<State>::GetLastEnd(State state) const
{
    return (m_current.state == state) ? Simulator::Now() : GetLastCompletedEnd(state);
}

template <class State>
std::optional<Time>
WifiState<State>::GetLastCompletedEnd(State state) const
{
    if (m_pending.has_value() && m_pending->state == state)
    {
        // the pending interval is a completed interval unless, after this call, at the same
        // simulation time, the pending state becomes again the current state; based on the
        // information available when this function is called, the pending interval shall be
        // considered completed and returned by this function
        return m_current.start;
    }

    const auto it = m_completed.find(state);
    return (it != m_completed.cend()) ? std::make_optional(it->second.End()) : std::nullopt;
}

} // namespace ns3

#endif /* WIFI_STATE_H */
