/*
 * Copyright (c) 2024 Huazhong University of Science and Technology
 * Copyright (c) 2024 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Muyuan Shen <muyuan@uw.edu>
 *           Hao Yin <haoyin@uw.edu>
 */

#ifndef WIFI_TX_STATS_HELPER_H
#define WIFI_TX_STATS_HELPER_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/qos-utils.h"
#include "ns3/type-id.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-types.h"
#include "ns3/wifi-utils.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace ns3
{

class NetDeviceContainer;
class NodeContainer;
class Time;
class Packet;
class WifiMpdu;

/**
 * @brief Statistics helper for tracking outcomes of data MPDU transmissions.
 *
 * This helper may be used to track statistics of all data MPDU transmissions on a given Node,
 * WifiNetDevice, or even link granularity (for MLO operation), including counts of the
 * number of successfully acknowledged MPDUs, the number of retransmissions (if any) of
 * those successfully acknowledged MPDUs, and the number of failed MPDUs (by drop reason).
 * The helper can also be used to access timestamped data about how long the MPDU was enqueued
 * and how long it took to complete acknowledgement (or failure) once the first transmission
 * of it was attempted. For failed MPDUs, their WifiMacDropReason can be accessed.
 */
class WifiTxStatsHelper
{
  public:
    /**
     * When Multi-Link Operation (MLO) is used, it is possible for MPDUs to be sent on
     * multiple links concurrently.  When such an MPDU is acked, a question arises as to how
     * to count the number of successes (only one success, or success on each link).  The
     * ACK does not convey which transmission on which link triggered the event.  Therefore,
     * two policies are defined for how to count a successful event.
     * 1) FIRST_LINK_IN_SET: Count the success on the first link of the set of in-flight links
     * 2) ALL_LINKS: Count the success on all links in the in-flight link set
     */
    enum MultiLinkSuccessType : uint8_t
    {
        FIRST_LINK_IN_SET,
        ALL_LINKS
    };

    /**
     * Structure for recording information about an individual data MPDU transmission.
     */
    struct MpduRecord
    {
        uint32_t m_nodeId{std::numeric_limits<uint32_t>::max()};   //!< The sending node ID
        uint32_t m_deviceId{std::numeric_limits<uint32_t>::max()}; //!< The sending device ID
        Time m_enqueueTime; //!< The enqueue time (time that the packet was added to a WifiMacQueue)
        Time m_txStartTime; //!< The time at which the MPDU was first transmitted
        std::optional<Time> m_dropTime;    //!< The time of removal from the WifiMacQueue, if failed
        Time m_ackTime;                    //!< The time at which the MPDU was acknowledged
        uint64_t m_mpduSeqNum{0};          //!< The MPDU sequence number
        uint32_t m_retransmissions{0};     //!< A count of the number of retransmissions of the MPDU
        uint8_t m_tid{WIFI_TID_UNDEFINED}; //!< The TID for the MPDU
        std::set<uint8_t> m_successLinkIdSet; //!< If acked, the set of in-flight link ID(s)
        std::optional<WifiMacDropReason> m_dropReason; //!< If failed, the drop reason
    };

    /**
     *  Default constructor; start time initialized to zero and stop time to Time::Max()
     */
    WifiTxStatsHelper();
    /**
     * Construct a helper with start and stop times. Only those MPDUs
     * enqueued between the start and stop times will be counted.
     *
     * @param startTime The measurement start time
     * @param stopTime The measurement stop time
     */
    WifiTxStatsHelper(Time startTime, Time stopTime);

    /**
     * Enables trace collection for all nodes and WifiNetDevices in the specified NodeContainer.
     * @param nodes The NodeContainer to which traces are to be connected.
     */
    void Enable(const NodeContainer& nodes);
    /**
     * Enables trace collection for all WifiNetDevices in the specified NetDeviceContainer.
     * @param devices The NetDeviceContainer to which traces are to be connected.
     */
    void Enable(const NetDeviceContainer& devices);
    /**
     * @brief Set the start time for statistics collection
     *
     * No MPDUs enqueued before startTime will be included
     * in the statistics.
     *
     * @param startTime The measurement start time
     */
    void Start(Time startTime);
    /**
     * @brief Set the stop time for statistics collection
     *
     * No MPDUs enqueued after stopTime elapses will be included in
     * statistics. However, MPDUs that were enqueued before stopTime
     * (and after startTime) will be included in the statistics if
     * they are dequeued before the statistics are queried by the user.
     *
     * @param stopTime The measurement stop time
     */
    void Stop(Time stopTime);
    /**
     * @brief Clear the data structures for all completed successful and failed MpduRecords
     */
    void Reset();

    //
    // Hash function and unordered maps
    //

    /**
     * Hash function for the tuples used in unordered_maps
     */
    struct TupleHash
    {
        /**
         * Combines two hash values into one (similar to boost::hash_combine)
         * @param seed the starting hash point
         * @param value the value to combine to the hash
         */
        template <typename T>
        void hash_combine(size_t& seed, const T& value) const
        {
            seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        /**
         * Hash function for tuples of arbitrary size
         * @param tuple the input Tuple
         * @return the hash of the tuple elements
         */
        template <typename Tuple, size_t Index = 0>
        size_t tuple_hash(const Tuple& tuple) const
        {
            if constexpr (Index < std::tuple_size<Tuple>::value)
            {
                size_t seed = tuple_hash<Tuple, Index + 1>(tuple);
                hash_combine(seed, std::get<Index>(tuple));
                return seed;
            }
            else
            {
                return 0; // Base case
            }
        }

        /**
         * Operator overload for calculating hash of tuple
         * @param tuple the input Tuple
         * @return the hash of the tuple elements
         */
        template <typename... Args>
        size_t operator()(const std::tuple<Args...>& tuple) const
        {
            return tuple_hash(tuple);
        }
    };

    using CountPerNodeDeviceLink_t =
        std::unordered_map<std::tuple<uint32_t, uint32_t, uint8_t>,
                           uint64_t,
                           TupleHash>; //!< std::unordered_map of {nodeId, deviceId, linkId} tuple
                                       //!< to counter value
    using CountPerNodeDevice_t =
        std::unordered_map<std::tuple<uint32_t, uint32_t>,
                           uint64_t,
                           TupleHash>; //!< std::unordered_map of {nodeId, deviceId} tuple to
                                       //!< counter value
    using MpduRecordsPerNodeDeviceLink_t =
        std::unordered_map<std::tuple<uint32_t, uint32_t, uint8_t>,
                           std::list<MpduRecord>,
                           TupleHash>; //!< std::unordered_map of {nodeId, deviceId, linkId} tuple
                                       //!< to a list of MPDU records
    using MpduRecordsPerNodeDevice_t =
        std::unordered_map<std::tuple<uint32_t, uint32_t>,
                           std::list<MpduRecord>,
                           TupleHash>; //!< std::unordered_map of {nodeId, deviceId} tuple to a list
                                       //!< of MPDU records

    //
    // Methods to retrieve the counts tabulated by this helper
    //

    /**
     * @brief Return the counts of successful MPDU transmissions in a hash map
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID}. The value returned by indexing the tuple is the count of
     * successful MPDU transmissions.
     *
     * If Multi-Link Operation (MLO) is configured, a variant of this method is available
     * that returns an unordered_map indexed by a tuple that includes the Link ID, for
     * more granular statistics reporting.
     * @see GetSuccessesByNodeDeviceLink
     *
     * If Multi-Link Operation (MLO) is configured, and MPDUs are sent on multiple
     * links in parallel, this method will count one success for any such acknowledged
     * MPDU (even if it was successfully received on more than one link).
     *
     * @return A hash map with counts of MPDU successful transmissions
     */
    CountPerNodeDevice_t GetSuccessesByNodeDevice() const;
    /**
     * @brief Return the counts of successful MPDU transmissions in a hash map
     * @param type how the statistics handle a successful MPDU transmitted on multiple links
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID, link ID}. The value returned by indexing the tuple is the count of
     * successful MPDU transmissions.
     *
     * The link ID corresponds to the link for which the MPDU was successfully
     * acknowledged, regardless of the links that may have been used in
     * earlier transmissions of the MPDU.
     *
     * For a similar method that does not use Link IDs in the key,
     * @see GetSuccessesByNodeDevice
     *
     * @return A hash map with counts of MPDU successful transmissions
     */
    CountPerNodeDeviceLink_t GetSuccessesByNodeDeviceLink(
        WifiTxStatsHelper::MultiLinkSuccessType type = FIRST_LINK_IN_SET) const;
    /**
     * @brief Return the counts of failed MPDU transmissions in a hash map
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID}. The value returned by indexing the tuple is the count of
     * failed MPDU transmissions.
     *
     * The number of failures are only stored with the granularity
     * of per-device, since in a multi-link scenario, some retransmissions
     * (that lead to eventual failure) may occur on different links.
     *
     * @return A hash map with counts of MPDU failed transmissions
     */
    CountPerNodeDevice_t GetFailuresByNodeDevice() const;
    /**
     * @brief Return the counts of failed MPDU transmissions due to given drop reason in a hash map
     * @param reason Reason for dropping the MPDU
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID}. The value returned by indexing the tuple is the count of
     * failed MPDU transmissions due to the reason given.
     *
     * The number of failures are only stored with the granularity
     * of per-device, since in a multi-link scenario, some retransmissions
     * (that lead to eventual failure) may occur on different links.
     *
     * @return A hash map with counts of MPDU failed transmissions
     */
    CountPerNodeDevice_t GetFailuresByNodeDevice(WifiMacDropReason reason) const;
    /**
     * @brief Return the counts of MPDU retransmissions in a hash map
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID}. The value returned by indexing the tuple is the count of
     * MPDU retransmissions of those MPDUs that were successfully acked.
     *
     * The number of retransmissions are only stored with the granularity
     * of per-device, since in a multi-link scenario, some retransmissions
     * may be sent on different links.
     *
     * @return A hash map with counts of MPDU retransmissions for successful MPDUs
     */
    CountPerNodeDevice_t GetRetransmissionsByNodeDevice() const;
    /**
     * @brief Return the count of successful MPDU transmissions across all enabled devices
     *
     * @return The count of all successful MPDU transmissions
     */
    uint64_t GetSuccesses() const;
    /**
     * @brief Return the count of failed MPDU transmissions across all enabled devices
     *
     * @return The count of all failed MPDU transmissions
     */
    uint64_t GetFailures() const;
    /**
     * @brief Return the count of failed MPDU transmissions due to given drop reason across
     * all enabled devices
     * @param reason Reason for dropping the MPDU
     *
     * @return The count of all failed MPDU transmissions for the specified reason
     */
    uint64_t GetFailures(WifiMacDropReason reason) const;
    /**
     * @brief Return the count of MPDU retransmissions across all enabled devices
     *
     * @return The count of all MPDU retransmissions for acked MPDUs
     */
    uint64_t GetRetransmissions() const;
    /**
     * @brief Return the duration since the helper was started or reset
     *
     * @return The duration since the helper was started or reset
     */
    Time GetDuration() const;
    /**
     * @brief Return a hash map of successful MPDU records
     * @param type how the statistics handle a successful MPDU transmitted on multiple links
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID, link ID}. The value returned by indexing the tuple is the list of MPDU
     * records of the MPDUs that were successfully acked.
     *
     * @return A const reference to the hash map of successful MPDU records
     */
    const MpduRecordsPerNodeDeviceLink_t GetSuccessRecords(
        WifiTxStatsHelper::MultiLinkSuccessType type = FIRST_LINK_IN_SET) const;
    /**
     * @brief Return a hash map of MPDU records for failed transmissions
     *
     * The return type is an std::unordered_map, and the keys are tuples of {node ID,
     * device ID}. The value returned by indexing the tuple is the list of MPDU
     * records of the MPDUs that failed.
     *
     * @return A const reference to the hash map of MPDU records corresponding to failure
     */
    const MpduRecordsPerNodeDevice_t& GetFailureRecords() const;

  private:
    /**
     * @brief Callback for the WifiMacQueue::Enqueue trace
     * @param nodeId the Node ID triggering the trace
     * @param deviceId the Node ID triggering the trace
     * @param mpdu The MPDU enqueued
     */
    void NotifyMacEnqueue(uint32_t nodeId, uint32_t deviceId, Ptr<const WifiMpdu> mpdu);
    /**
     * @brief Callback for the WifiPhy::PhyTxBegin trace
     * @param nodeId the Node ID triggering the trace
     * @param deviceId the Node ID triggering the trace
     * @param pkt The frame being sent
     */
    void NotifyTxStart(uint32_t nodeId, uint32_t deviceId, Ptr<const Packet> pkt, Watt_u);
    /**
     * @brief Callback for the WifiMac::AckedMpdu trace
     * @param nodeId the Node ID triggering the trace
     * @param deviceId the Node ID triggering the trace
     * @param mpdu The MPDU acked
     */
    void NotifyAcked(uint32_t nodeId, uint32_t deviceId, Ptr<const WifiMpdu> mpdu);
    /**
     * @brief Callback for the WifiMac::DroppedMpdu trace
     * @param nodeId the Node ID triggering the trace
     * @param deviceId the Node ID triggering the trace
     * @param reason Reason for dropping the MPDU
     * @param mpdu The MPDU dropped
     */
    void NotifyMacDropped(uint32_t nodeId,
                          uint32_t deviceId,
                          WifiMacDropReason reason,
                          Ptr<const WifiMpdu> mpdu);

    MpduRecordsPerNodeDevice_t m_successMap;      //!< The nested map of successful MPDUs
    MpduRecordsPerNodeDevice_t m_failureMap;      //!< The nested map of failed MPDUs
    std::map<uint64_t, MpduRecord> m_inflightMap; //!< In-flight MPDUs; key is a Packet UID
    Time m_startTime;                             //!< The start time for recording statistics
    Time m_stopTime{Time::Max()};                 //!< The stop time for recording statistics
};

} // namespace ns3

#endif // WIFI_TX_STATS_HELPER_H
