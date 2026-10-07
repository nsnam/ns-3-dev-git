/*
 * Copyright (c) 2025 Arnav Kapoor
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Arnav Kapoor <kapoorarnav43@gmail.com>
 */

#ifndef HWMP_CONTROL_RECORDER_H
#define HWMP_CONTROL_RECORDER_H

#include "ns3/hwmp-protocol.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device-container.h"
#include "ns3/nstime.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace ns3
{
namespace dot11s
{
class IePrep;
class IePreq;
} // namespace dot11s
} // namespace ns3

using namespace ns3;

/**
 * @ingroup dot11s-test
 *
 * @brief Record the HWMP path selection elements received by a set of mesh points.
 *
 * The regression tests use the records to check which mesh point sent which
 * PREQ, PREP or PERR to which neighbor, and in which order, without relying on
 * the exact content or timing of the frames on the air.
 */
class HwmpControlRecorder
{
  public:
    /// HWMP path selection element type
    enum class Type
    {
        PREQ,
        PREP,
        PERR
    };

    /// A path selection element received by a mesh point
    struct Event
    {
        Time time;               ///< reception time
        Type type;               ///< element type
        uint32_t receiver;       ///< index of the receiving mesh point
        uint32_t transmitter;    ///< index of the transmitting neighbor
        Mac48Address originator; ///< PREQ or PREP originator (unused for PERR)
        Mac48Address target;     ///< first PREQ target, PREP destination or PERR destination
        bool doFlag;             ///< DO flag of the first PREQ target
        bool rfFlag;             ///< RF flag of the first PREQ target
    };

    /**
     * Connect to the HWMP trace sources of the given mesh points.  The index of
     * a mesh point in the container identifies it in the recorded events.
     *
     * @param meshPoints the mesh point devices
     */
    void Connect(const NetDeviceContainer& meshPoints);

    /**
     * @param index the index of a mesh point
     * @return the MAC address of that mesh point
     */
    Mac48Address GetAddress(uint32_t index) const;

    /// @return all recorded events, in order of reception
    const std::vector<Event>& GetEvents() const;

    /**
     * @param predicate the condition to check
     * @return the number of recorded events matching the predicate
     */
    std::size_t Count(const std::function<bool(const Event&)>& predicate) const;

    /**
     * @param predicate the condition to check
     * @return the first recorded event matching the predicate, or nullptr
     */
    const Event* FindFirst(const std::function<bool(const Event&)>& predicate) const;

  private:
    /**
     * @param address the MAC address of a mesh point
     * @return the index of that mesh point, or the number of mesh points if not found
     */
    uint32_t GetIndex(Mac48Address address) const;

    /**
     * Record a received PREQ
     * @param receiver index of the receiving mesh point
     * @param preq the PREQ element
     * @param from the transmitting neighbor
     * @param interface the receiving interface
     */
    void PreqReceived(uint32_t receiver,
                      const dot11s::IePreq& preq,
                      Mac48Address from,
                      uint32_t interface);

    /**
     * Record a received PREP
     * @param receiver index of the receiving mesh point
     * @param prep the PREP element
     * @param from the transmitting neighbor
     * @param interface the receiving interface
     */
    void PrepReceived(uint32_t receiver,
                      const dot11s::IePrep& prep,
                      Mac48Address from,
                      uint32_t interface);

    /**
     * Record a received PERR
     * @param receiver index of the receiving mesh point
     * @param destinations the unreachable destinations
     * @param from the transmitting neighbor
     * @param interface the receiving interface
     */
    void PerrReceived(uint32_t receiver,
                      const std::vector<dot11s::HwmpProtocol::FailedDestination>& destinations,
                      Mac48Address from,
                      uint32_t interface);

    std::vector<Mac48Address> m_addresses; ///< mesh point addresses, by index
    std::vector<Event> m_events;           ///< recorded events
};

#endif // HWMP_CONTROL_RECORDER_H
