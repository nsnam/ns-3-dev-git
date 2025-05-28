/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#ifndef COEX_ARBITRATOR_H
#define COEX_ARBITRATOR_H

#include "coex-types.h"

#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <map>
#include <optional>
#include <tuple>

namespace ns3
{
class Node;

namespace coex
{
class DeviceManager;

/**
 * @defgroup coex Framework to handle coexistence events
 *
 * @brief an arbitrator for Coex Events
 *
 */

/// typedef for event ID type
using eventId_t = uint32_t;

/**
 * @ingroup coex
 * @brief Coex Arbitrator base class
 *
 * A Coex Arbitrator handles requests for using a transmitting resource (antenna) shared among
 * multiple NetDevices.
 */
class Arbitrator : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    Arbitrator();
    ~Arbitrator() override;

    /**
     * Set the Coex Manager for the given Radio Access Technology (RAT).
     *
     * @param rat the given RAT
     * @param coexManager the Coex Manager
     */
    void SetDeviceCoexManager(Rat rat, Ptr<DeviceManager> coexManager);

    /**
     * Get the Coex Manager for the given RAT.
     *
     * @param rat the given RAT
     * @return the Coex Manager for the given RAT
     */
    Ptr<DeviceManager> GetDeviceCoexManager(Rat rat) const;

    /**
     * This method is used by a device to inform the Coex Arbitrator of a scheduled event that
     * requires Coex protection.
     *
     * @param coexEvent the given coex event
     * @return the ID the coex arbitrator assigns to the given coex event, if the request is
     *         accepted
     */
    std::optional<eventId_t> RegisterProtection(const Event& coexEvent);

    /**
     * This method is used by a device to cancel the (periodic) coex event that was previously
     * notified.
     *
     * @param coexEvId the ID of the coex event to cancel
     */
    void CancelCoexEvent(eventId_t coexEvId);

    /**
     * @return the start time of the current coex event, if any is ongoing, or the start time of the
     *         next coex event, if any is scheduled
     */
    std::optional<Time> GetCurrOrNextEventStart();

  protected:
    void NotifyNewAggregate() override;
    void DoDispose() override;

    /**
     * Stored information about a coex event.
     */
    struct CoexEventElem
    {
        eventId_t id;    ///< the coex event ID
        Event coexEvent; ///< the coex event
    };

    /**
     * Struct providing a function call operator to compare two CoexEventElem objects.
     * This struct is used as the Compare template type parameter of the set of CoexEventElem
     * objects to sort them in increasing order of start time.
     */
    struct CoexEventCompare
    {
        /**
         * Function call operator.
         *
         * @param lhs left hand side CoexEventElem object
         * @param rhs right hand side CoexEventElem object
         * @return true if the left hand side CoexEventElem object starts before the right hand
         *         side CoexEventElem object
         */
        bool operator()(const CoexEventElem& lhs, const CoexEventElem& rhs) const
        {
            const auto& lEvent = lhs.coexEvent;
            const auto& rEvent = rhs.coexEvent;

            return std::tie(lEvent.start, lEvent.duration, lhs.id) <
                   std::tie(rEvent.start, rEvent.duration, rhs.id);
        }
    };

    /// @brief Coex events sorted in increasing order of start time
    using Events = std::set<CoexEventElem, CoexEventCompare>;

    /// @return the set of coex events
    const Events& GetCoexEvents() const;

    /**
     * Update the set of coex events by removing past coex events and replacing periodic coex events
     * with their next occurrence.
     */
    void UpdateCoexEvents();

    /**
     * This method is used by subclasses to determine whether a request from a device to use the
     * shared resource should be accepted.
     *
     * @param coexEvent the requested coex event
     * @return whether the request should be accepted
     */
    virtual bool IsRequestAccepted(const Event& coexEvent) = 0;

  private:
    Ptr<Node> m_node; ///< the node which this arbitrator is aggregated to
    std::map<Rat, Ptr<DeviceManager>>
        m_devCoexManagers; ///< a map of CoexManagers for the NetDevices on this node, indexed by
                           ///< the RAT corresponding to the NetDevice
    Events m_coexEvents;   ///< set of coex events sorted in increasing order of start time
    eventId_t m_nextCoexEvId{0}; ///< ID to assign to the next coex event

    /// signature of callbacks for the coex event trace
    using CoexEventTraceCb = void (*)(const Event&);

    TracedCallback<const Event&> m_coexEventTrace; ///< coex event trace source
};

} // namespace coex
} // namespace ns3

#endif /* COEX_ARBITRATOR_H */
