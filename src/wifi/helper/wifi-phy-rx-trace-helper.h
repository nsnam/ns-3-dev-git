/*
 * Copyright (c) 2023 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef WIFI_PHY_RX_TRACE_HELPER_H
#define WIFI_PHY_RX_TRACE_HELPER_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/phy-entity.h"
#include "ns3/ptr.h"
#include "ns3/wifi-phy-common.h"
#include "ns3/wifi-phy-state.h"
#include "ns3/wifi-ppdu.h"

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

// Forward declare some friend test classes that access private methods
class TestWifiPhyRxTraceHelper;
class TestWifiPhyRxTraceHelperMloStr;
class TestWifiPhyRxTraceHelperYans;

namespace ns3
{

class Mac48Address;
class NetDeviceContainer;
class Node;
class NodeContainer;
struct SpectrumSignalParameters;
class WifiPhyRxTraceSink;
class WifiTxVector;

/**
 * @struct WifiPhyTraceStatistics
 * @brief Keeps track of PHY layer trace statistics.
 *
 * This structure stores various statistics related to the Physical Layer (PHY) of the Wi-Fi
 * communication, including the number of successful and failed PPDUs containing unicast data,
 * and unicast data MPDU receptions.
 */
struct WifiPhyTraceStatistics
{
    uint64_t m_overlappingPpdus{
        0}; ///< Number of PPDUs that overlapped in time with at least one other PPDU.
    uint64_t m_nonOverlappingPpdus{
        0}; ///< Number of PPDUs that did not overlap in time with any other PPDU.
    uint64_t m_receivedPpdus{0}; ///< Number of successfully received PPDUs (with unicast data).
    uint64_t m_failedPpdus{0};   ///< Number of failed PPDU receptions (with unicast data).
    uint64_t m_receivedMpdus{0}; ///< Number of successfully received unicast data MPDUs.
    uint64_t m_failedMpdus{0};   ///< Number of failed unicast data MPDU receptions.
    std::map<WifiPhyRxfailureReason, uint64_t> m_ppduDropReasons; ///< Counts of the drop reasons
};

/**
 * @struct WifiPpduRxRecord
 * @brief Structure recording a received PPDU (Physical Protocol Data Unit) in a Wi-Fi network.
 *
 * This structure contains various details about the received PPDU, such as signal strength,
 * identifiers for the sender and receiver, timing information, and reception status.
 */
struct WifiPpduRxRecord
{
    Ptr<const WifiPpdu> m_ppdu{nullptr}; ///< Pointer to the received PPDU.
    double m_rssi{0};                    ///< Received Signal Strength Indicator (RSSI) in dBm.
    uint64_t m_rxTag{
        std::numeric_limits<uint64_t>::max()}; ///< Unique tag for the reception of this PPDU.
    uint32_t m_receiverId{std::numeric_limits<uint32_t>::max()}; ///< Node ID of the receiver.
    Time m_startTime; ///< Start time of the PPDU reception.
    Time m_endTime;   ///< End time of the PPDU reception.
    WifiPhyRxfailureReason m_reason{
        WifiPhyRxfailureReason::UNKNOWN}; ///< Reason for reception failure, if any.
    std::vector<WifiPpduRxRecord>
        m_overlappingPpdu; ///< List of PPDUs that overlapped in time with this reception.
    std::vector<bool> m_statusPerMpdu; ///< Reception status for each MPDU within the PPDU.
    uint8_t m_linkId{std::numeric_limits<uint8_t>::max()}; ///< The link ID belonging to this record
    uint32_t m_senderId{std::numeric_limits<uint32_t>::max()};       ///< Node ID of the sender.
    uint32_t m_senderDeviceId{std::numeric_limits<uint32_t>::max()}; ///< Device ID of Sender
};

/**
 * @class WifiPhyRxTraceHelper
 * @brief Assists in tracing and analyzing Wi-Fi Physical Layer (PHY) receptions.
 *
 * The WifiPhyRxTraceHelper class can be used to instrument Wi-Fi nodes (or devices, or
 * links) to keep track of the reception of Wi-Fi signals, and in particular, whether they overlap
 * (collide) with one another.  The implementation maintains reception records within internal
 * data structures, and statistics or full reception records can be queried.
 *
 * The class provides functionality to connect traces to all nodes and WifiNetDevices within scope,
 * enabling the capture of all Physical Protocol Data Units (PPDUs) received. It also
 * allows for the collection and retrieval of statistics related to successful and failed receptions
 * of PPDUs containing unicast data, and their corresponding MAC Protocol Data Units (MPDUs).
 *
 * Key features include:
 * - Enabling trace connections to capture reception data.
 * - Starting and stopping the collection of statistics at specified times.
 * - Resetting the collected data for fresh starts in data collection.
 * - Accessing detailed reception records for further analysis.
 *
 * Usage involves connecting to desired nodes or devices, (optionally) managing the
 * collection period with start, stop, and reset methods, and finally, accessing the collected
 * statistics or reception records.
 *
 * Statistics are only compiled for unicast data (WIFI_MAC_DATA and WIFI_MAC_QOSDATA), although
 * PPDU records are kept for all frame types because it is possible for non-data frames to
 * collide with data frames.
 */
class WifiPhyRxTraceHelper
{
    friend class ::TestWifiPhyRxTraceHelper;
    friend class ::TestWifiPhyRxTraceHelperMloStr;
    friend class ::TestWifiPhyRxTraceHelperYans;

  public:
    /**
     * Constructor.
     */
    WifiPhyRxTraceHelper();

    /**
     * Enables trace collection for all nodes and WifiNetDevices in the specified NodeContainer.
     * @param nodes The NodeContainer to which traces are to be connected.
     */
    void Enable(NodeContainer nodes);

    /**
     * Enables trace collection for all nodes corresponding to the devices in the specified
     * NetDeviceContainer.
     * @param devices The NetDeviceContainer containing nodes to which traces are to be connected.
     */
    void Enable(NetDeviceContainer devices);

    /**
     * Retrieves current statistics of successful and failed data PPDUs and MPDUs receptions,
     * for all nodes, devices, and links that have been enabled.
     *
     * Statistics are compiled for the current observation window, which extends to the
     * trace helper start time or last reset time (whichever is later) until the last
     * stop time or the current time (whichever is earlier).
     *
     * @return A WifiPhyTraceStatistics object containing current reception statistics.
     */
    WifiPhyTraceStatistics GetStatistics() const;

    /**
     * Retrieves reception statistics for a given node, device, and link.
     * @param node Ptr to Node.
     * @param deviceId Device identifier (optional, defaults to 0).
     * @param linkId Link identifier (optional, defaults to 0).
     * @return WifiPhyTraceStatistics object with current reception stats.
     */
    WifiPhyTraceStatistics GetStatistics(Ptr<Node> node,
                                         uint32_t deviceId = 0,
                                         uint8_t linkId = 0) const;

    /**
     * Retrieves reception statistics for a given node, device, and link.
     * @param nodeId Node identifier.
     * @param deviceId Device identifier (optional, defaults to 0).
     * @param linkId Link identifier (optional, defaults to 0).
     * @return WifiPhyTraceStatistics object with current reception stats.
     */
    WifiPhyTraceStatistics GetStatistics(uint32_t nodeId,
                                         uint32_t deviceId = 0,
                                         uint8_t linkId = 0) const;

    /**
     * Starts the collection of statistics from a specified start time.
     * @param startTime The time to start collecting statistics.
     */
    void Start(Time startTime);

    /**
     * Stops the collection of statistics at a specified time.
     * @param stopTime The time to stop collecting statistics.
     */
    void Stop(Time stopTime);

    /**
     * Resets the current statistics, clearing all counts and PPDU records.
     */
    void Reset();

    /**
     * Accesses a vector of saved and completed PPDU reception records.
     * @return A constant reference to a vector of WifiPpduRxRecord instances.
     */
    const std::vector<WifiPpduRxRecord>& GetPpduRecords() const;

    /**
     * Accesses PPDU reception records for a specific node, device, and link.
     *
     * Because the return value could be null if the specified node/device/link do not have any
     * records, this method returns a std::optional object, and because we are passing the
     * underlying vector by const reference to avoid a copy, it must be wrapped by
     * std::reference_wrapper. See the example program wifi-phy-reception-trace-example.cc for an
     * example of how to use the value returned.
     *
     * @param nodeId The ID of the node.
     * @param deviceId The ID of the WifiNetDevice (optional, defaults to 0).
     * @param linkId The ID of the link (optional, defaults to 0).
     * @return A reference to a vector of WifiPpduRxRecord instances.
     */
    std::optional<std::reference_wrapper<const std::vector<WifiPpduRxRecord>>> GetPpduRecords(
        uint32_t nodeId,
        uint32_t deviceId = 0,
        uint8_t linkId = 0) const;

    /**
     * Print statistics for all nodes, devices, and links during the collection period.
     */
    void PrintStatistics() const;

    /**
     * Prints statistics collected in the period for a specific node, device, and link.
     * @param node Ptr to Node.
     * @param deviceId Device identifier (optional, defaults to 0).
     * @param linkId Link identifier (optional, defaults to 0).
     */
    void PrintStatistics(Ptr<Node> node, uint32_t deviceId = 0, uint8_t linkId = 0) const;

    /**
     * Prints statistics collected in the period for a specific node, device, and link.
     * @param nodeId Node identifier.
     * @param deviceId Device identifier (optional, defaults to 0).
     * @param linkId Link identifier (optional, defaults to 0).
     */
    void PrintStatistics(uint32_t nodeId, uint32_t deviceId = 0, uint8_t linkId = 0) const;

  private:
    /**
     * Enables trace collection for all nodes and WifiNetDevices in the specified NodeContainer.
     * @param nodes The NodeContainer to which traces are to be connected.
     * @param MacToNodeMap A mapping from MAC address to node ID.
     */
    void Enable(NodeContainer nodes, const std::map<Mac48Address, uint32_t>& MacToNodeMap);

    /**
     * Populates the mapping of MAC addresses to node IDs for a given set of nodes.
     * @param nodes The container of nodes for which to populate the MAC address to node ID mapping.
     * @return a copy of the std::map container
     */
    std::map<Mac48Address, uint32_t> MapMacAddressesToNodeIds(NodeContainer nodes) const;

    Ptr<WifiPhyRxTraceSink> m_traceSink; ///< Pointer to the current trace sink object.
};

/**
 * @class WifiPhyRxTraceSink
 * @brief Sink class for capturing and analyzing PHY layer reception events in Wi-Fi networks.
 *
 * The WifiPhyRxTraceSink class acts as a comprehensive sink for events related to the
 * reception of signals at the Physical Layer (PHY) of Wi-Fi networks. It is designed to facilitate
 * the detailed analysis and tracing of reception activities, and the management of reception data
 * across nodes, devices, and links. This class supports the capture and analysis of detailed
 * reception records, including successful and failed receptions, and offers capabilities to
 * manage the collection of reception statistics.
 *
 * Features include:
 * - Unique tag generation for tracking individual reception events.
 * - Access to detailed PPDU reception records for analysis.
 * - Management of reception statistics collection through start, stop, and reset operations.
 * - Mapping of MAC addresses to node IDs for network element identification.
 * - Handling of PHY signal transmission and reception events for detailed outcome logging.
 * - Tracking and analysis of overlapping reception events for interference assessment.
 */
class WifiPhyRxTraceSink : public Object
{
  public:
    /**
     * Constructor.
     */
    WifiPhyRxTraceSink();

    /**
     * Retrieves the TypeId of the class.
     * @return The TypeId of the WifiPhyRxTraceSink class.
     */
    static TypeId GetTypeId();

    /**
     * @brief Generating unique tags for more than one instance of a WifiPpdu object
     *
     * This class is responsible for generating unique identifiers for each received
     * WifiPpdu.  The WifiPpdu UID is not sufficient because there can be more than one
     * record per WifiPpdu.
     */
    class UniqueTagGenerator
    {
      public:
        /**
         * Generates a unique tag for a WifiPpdu. Uses an internal counter and a set of
         * previously used tags to ensure that each generated tag is unique, allowing accurate
         * tracking of received frames.
         * @param ppduUid The PPDU UID to generate a unique tag for.
         * @return uint64_t The generated unique tag.
         */
        uint64_t GenerateUniqueTag(uint64_t ppduUid);

      private:
        uint64_t counter{0};         ///< Counter to help generate unique tags.
        std::set<uint64_t> usedTags; ///< Set of already used tags.
    };

    UniqueTagGenerator m_tagGenerator; ///< Instance of UniqueTagGenerator used for generating
                                       ///< unique reception tags.

    /**
     * Provides access to all saved and completed PPDU reception records across all nodes,
     * WifiNetDevices, and links.
     * @return A constant reference to a vector of WifiPpduRxRecord instances.
     */
    const std::vector<WifiPpduRxRecord>& GetPpduRecords() const;

    /**
     * Provides access to PPDU reception records for a specific node, device, and link.
     * @param nodeId The ID of the node.
     * @param deviceId The ID of the WifiNetDevice (optional, defaults to 0).
     * @param linkId The ID of the link (optional, defaults to 0).
     * @return A constant reference to a vector of WifiPpduRxRecord instances.
     */
    std::optional<std::reference_wrapper<const std::vector<WifiPpduRxRecord>>> GetPpduRecords(
        uint32_t nodeId,
        uint32_t deviceId = 0,
        uint8_t linkId = 0) const;

    /**
     * Creates a vector with all completed WiFi reception records to be returned by the
     * GetPpduRecords(). This is only called if no specific node, device or link is
     * provided.
     */
    void CreateVectorFromRecords();

    /**
     * Provides a custom mapping of MAC addresses to intended receiver node IDs. This mapping is
     * crucial for identifying the nodes involved in reception events based on their MAC addresses,
     * facilitating the analysis of network dynamics at the PHY layer.
     * @param MacAddressToNodeIdMap A mapping from MAC addresses to node IDs.
     */
    void SetMapMacAddressToNodeId(const std::map<Mac48Address, uint32_t>& MacAddressToNodeIdMap);

    /**
     * Retrieves the frequency and channel number used by a specific link.
     * @param node The node ID.
     * @param deviceId The device ID of the WifiNetDevice.
     * @param link The link ID.
     * @return A pair containing the channel number (uint8_t) and frequency (uint16_t).
     */
    std::optional<std::pair<uint8_t, uint16_t>> GetChannelInfo(uint32_t node,
                                                               uint32_t deviceId,
                                                               int link) const;

    /**
     * Starts the statistics collection period from a specified start time. Ongoing PPDUs when this
     * method is called will be counted towards this statistics collection period.
     */
    void Start();

    /**
     * Stops the statistics collection period at a specified time. This method ensures that
     * incomplete PPDUs at the end of the collection period are not included in the current
     * statistics count.
     */
    void Stop();

    /**
     * Resets the statistics collection, clearing all counts and discarding all fully completed PPDU
     * records. This allows for a fresh start in data collection and analysis.
     */
    void Reset();

    /**
     * Translates a context string to a link ID, facilitating the association of events with
     * specific links in the network.
     * @param context The context string.
     * @return The link ID associated with the context.
     */
    int ContextToLinkId(std::string context) const;

    /**
     * Translates a context string to a device ID, enabling the identification of devices involved
     * in reception events based on context information.
     * @param context The context string.
     * @return The device ID associated with the context.
     */
    int ContextToDeviceId(std::string context) const;

    /**
     * Translates a context string to a node ID.
     * @param context The context string to translate.
     * @return The corresponding node ID, facilitating the identification of the node involved in a
     * reception event.
     */
    uint32_t ContextToNodeId(std::string context) const;

    /**
     * Translate a context string to a colon-delimited tuple "0:0:0" where the first element
     * is a node ID, second is a device ID, third is a link ID
     * @param context The context string to translate.
     * @return The tuple of node, device, and link IDs
     */
    std::string ContextToTuple(std::string context) const;

    /**
     * Handles the event of a PHY signal transmission.
     * @param context The context string, providing situational information about the transmission.
     * @param ppdu The Wi-Fi PPDU being transmitted.
     * @param txVector The transmission vector, detailing the parameters of the transmission.
     */
    void PhySignalTransmission(std::string context,
                               Ptr<const WifiPpdu> ppdu,
                               const WifiTxVector& txVector);

    /**
     * @brief Handles the event of a Wi-Fi PPDU arrival.
     *
     * @param context The context string, offering contextual information about the signal
     * reception.
     * @param ppdu The received WifiPpdu
     * @param rxPower The power level at which the signal was received, indicating the signal's
     * strength.
     * @param duration The duration of the signal, offering temporal information about the reception
     * event.
     */
    void PhySignalArrival(std::string context,
                          Ptr<const WifiPpdu> ppdu,
                          double rxPower,
                          Time duration);

    /**
     * @brief Handles the event of a PHY signal arrival from a SpectrumChannel.
     *
     * @param context The context string, offering contextual information about the signal
     * reception.
     * @param signal The received signal parameters, detailing the characteristics of the arriving
     * signal.
     * @param senderNodeId The ID of the node sending the signal, providing insight into the source
     * of the signal.
     * @param rxPower The power level at which the signal was received, indicating the signal's
     * strength.
     * @param duration The duration of the signal, offering temporal information about the reception
     * event.
     */
    void SpectrumPhySignalArrival(std::string context,
                                  Ptr<const SpectrumSignalParameters> signal,
                                  uint32_t senderNodeId,
                                  double rxPower,
                                  Time duration);

    /**
     * Maps nodes to links and channels, creating a structured association between network
     * elements and their communication channels. This mapping is essential for understanding
     * what frequency and channel number is used for a given link.
     * @param nodes NodeContainer of all nodes whose MAC address is going to be link to their
     * NodeID.
     */
    void MapNodeToLinkToChannel(NodeContainer nodes);

    /**
     * Returns statistics on the count of successful and failed PPDUs with unicast data, and their
     * MPDUs, across all nodes, devices, and links.
     * @return A WifiPhyTraceStatistics object containing the current statistics. This method does
     * not reset the collected statistics, allowing for continuous accumulation of data over time.
     */
    WifiPhyTraceStatistics GetStatistics() const;

    /**
     * Returns statistics on the count of successful and failed PPDUs with unicast data, and their
     * MPDUs, for a specific node, device, and link.
     * @param nodeId The node ID from which to get the statistics.
     * @param deviceId The device ID from which to get the statistics.
     * @param linkId The link ID from which to get the statistics.
     * @return A WifiPhyTraceStatistics object containing the current statistics. This method does
     * not reset the collected statistics, allowing for continuous accumulation of data over time.
     */
    WifiPhyTraceStatistics GetStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const;

    /**
     * Updates the information for signals currently being received by a node.
     * @param nodeId The node ID for which to update the reception information.
     * @param deviceId The device ID on the node for which to update the information.
     * @param linkId The link ID associated with the ongoing reception.
     */
    void UpdateCurrentlyReceivedSignal(uint32_t nodeId, uint32_t deviceId, uint8_t linkId);

    /**
     * Records the outcome of a PPDU transmission, including the signal info, and the success or
     * failure status of each MPDU within the PPDU.
     * @param context Descriptive context for the PPDU outcome.
     * @param ppdu Pointer to the PPDU whose outcome is being recorded.
     * @param signal Information about the received signal.
     * @param txVector The transmission vector used for the PPDU.
     * @param statusPerMpdu A vector indicating the success (true) or failure (false) of each MPDU.
     */
    void PpduOutcome(std::string context,
                     Ptr<const WifiPpdu> ppdu,
                     RxSignalInfo signal,
                     const WifiTxVector& txVector,
                     const std::vector<bool>& statusPerMpdu);

    /**
     * Logs the drop of a PPDU at the PHY layer, detailing the context and reason for the drop.
     * @param context Descriptive context for the PPDU drop.
     * @param ppdu Pointer to the PPDU that was dropped.
     * @param reason The reason for the PPDU drop, represented as a WifiPhyRxfailureReason.
     */
    void PhyPpduDrop(std::string context, Ptr<const WifiPpdu> ppdu, WifiPhyRxfailureReason reason);

    /**
     * Handles the end of a PHY reception event, logging the conclusion of a reception and its
     * associated details.
     * @param nodeId The node ID where the reception ended.
     * @param deviceId The device ID on the node where the reception took place.
     * @param rxTag The unique reception tag associated with the reception event.
     * @param ppduUid The PPDU UID of the received WifiPpdu
     */
    void PhyRxEnd(uint32_t nodeId, uint32_t deviceId, uint64_t rxTag, uint64_t ppduUid);

    /**
     * Handles the conclusion of a transmission event, facilitating the logging of transmission
     * outcomes.
     * @param nodeId The node ID from which the transmission originated.
     * @param deviceId The device ID on the node that performed the transmission.
     * @param ppduRecord The WifiPpduRxRecord representing the concluded transmission.
     */
    void EndTx(uint32_t nodeId, uint32_t deviceId, WifiPpduRxRecord ppduRecord);

    /**
     * Counts and aggregates PHY layer statistics including receptions, transmissions, and
     * performance metrics (Success, Failure, Overlap) of unicast data PPDUs and MPDUs across all
     * nodes.
     * @return WifiPhyTraceStatistics object containing aggregated statistics for all nodes.
     */
    WifiPhyTraceStatistics CountStatistics() const;

    /**
     * Counts statistics related to PHY layer receptions, transmissions, and performance metrics
     * such as Success, Failure or Overlap of a PPDU containing unicast data, and the MPDUs within,
     * for the specific node,
     * device and link.
     * @param nodeId Node identifier.
     * @param deviceId Device identifier.
     * @param linkId Link identifier.
     * @return WifiPhyTraceStatistics object with current reception stats.
     */
    WifiPhyTraceStatistics CountStatistics(uint32_t nodeId,
                                           uint32_t deviceId,
                                           uint8_t linkId) const;

    /**
     * Prints a summary of the statistics collected, offering a concise overview of PHY layer
     * performance and reception outcomes for all nodes, devices and links.
     */
    void PrintStatistics() const;

    /**
     * Prints statistics collected in the period for a node, device, and link.
     * @param nodeId Node identifier.
     * @param deviceId Device identifier.
     * @param linkId Link identifier.
     */
    void PrintStatistics(uint32_t nodeId, uint32_t deviceId, uint8_t linkId) const;

    /**
     * @brief Returns whether the collection period is active
     * @return Whether the collection period is active
     */
    bool IsCollectionPeriodActive() const;

    /**
     * @brief Maps a reception tag to the corresponding WifiPpduRxRecord.
     *
     * This map links each unique reception tag generated upon receiving a
     * frame to the corresponding detailed reception record (WifiPpduRxRecord). It is
     * refreshed every time a new reception record is created.
     */
    std::map<uint64_t, WifiPpduRxRecord> m_rxTagToPpduRecord;

    /**
     * @brief Stores records of PPDUs that have completed reception, organized by node, device, and
     * link.
     *
     * This nested map structure is updated each time a WifiPpduRxRecord is finalized,
     * keeping a history of all completed packet receptions across different nodes,
     * devices, and links.
     */
    std::map<uint32_t, std::map<uint32_t, std::map<uint8_t, std::vector<WifiPpduRxRecord>>>>
        m_completedRecords;

    /**
     * @brief Stores a flat vector of all records of PPDUs that have completed reception.
     *
     * This vector is updated upon the request of GetPpduRecords(),
     * maintaining all packet receptions.
     */
    std::vector<WifiPpduRxRecord> m_records;

    /**
     * @brief Tracks ongoing frames being transmitted or received per node, device, and link.
     *
     * This map associates each node (by its ID) with nested maps keyed by device and link IDs, each
     * containing vectors of WifiPpduRxRecords. It enables the tracking of current packet
     * transmissions and receptions.
     */
    std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<WifiPpduRxRecord>>>>
        m_nodeDeviceLinkRxRecords;

    /**
     * @brief Maps each reception tag to a list of overlapping WifiPpduRxRecords.
     *
     * For each unique reception tag, this map stores a vector of WifiPpduRxRecords that
     * overlap with the record corresponding to the tag.
     */
    std::map<uint64_t, std::vector<WifiPpduRxRecord>> m_rxTagToListOfOverlappingPpduRecords;

    /**
     * @brief Aids in correlating PHY reception drops and outcomes with specific reception tags.
     *
     * This complex map structure is used to navigate from node and device IDs, and link and packet
     * IDs (PIDs), to the corresponding reception tag.
     */
    std::map<uint64_t, std::map<uint64_t, std::map<uint64_t, std::map<uint64_t, uint64_t>>>>
        m_nodeDeviceLinkPidToRxId;

    /**
     * @brief Maps WifiPpdu UIDs to WifiPpduRxRecord tags stored by transmitter.
     *
     * This map allows a transmit-side PPDU record to be fetched based on the WifiPpdu UID.
     */
    std::map<uint64_t, uint64_t> m_ppduUidToTxTag;

    /**
     * @brief Maps MAC addresses to node IDs.
     *
     * This mapping facilitates the identification of nodes within the network based on the MAC
     * addresses observed in reception events. By translating MAC addresses to node IDs, this map
     * enables the association of reception data with specific network nodes.
     */
    std::map<Mac48Address, uint32_t> m_macAddressToNodeId;

    /**
     * Maps node IDs to device IDs and further to link IDs, associating each with a pair consisting
     * of the channel number and frequency. This structure is vital for understanding the channel
     * and frequency utilization across different links.
     */
    std::map<uint32_t, std::map<uint32_t, std::map<int, std::pair<uint8_t, uint16_t>>>>
        m_nodeToDeviceToLinkToChannelInfo;

  private:
    /**
     * Flag indicating whether to keep a record of certain statistics or events for analysis. This
     * boolean flag can be toggled to control the granularity and scope of data collection, enabling
     * customization of the analysis to focus on specific times of interest.
     */
    bool m_statisticsCollectionPeriodStarted{false};

    /**
     * Update the passed-in statistics object with statistics from the passed-in record.
     * @param statistics The WifiPhyTraceStatistics object to modify
     * @param record The WifiPpduRxRecord to examine
     */
    void CountStatisticsForRecord(WifiPhyTraceStatistics& statistics,
                                  const WifiPpduRxRecord& record) const;
};

// Non-member function declarations

/**
 * @brief Checks if two WifiPpduRxRecord objects are equal.
 *
 * Compares two WifiPpduRxRecord objects to determine if they are equal. Two objects
 * are considered equal if all their corresponding counts have the same values.
 *
 * @param lhs The left-hand side WifiPpduRxRecord object in the comparison.
 * @param rhs The right-hand side WifiPpduRxRecord object in the comparison.
 * @return true if the objects are equal, false otherwise.
 */
bool operator==(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs);

/**
 * @brief Checks if two WifiPpduRxRecord objects are not equal.
 *
 * Compares two WifiPpduRxRecord objects to determine if they are not equal. Two objects
 * are considered not equal if at least one of their corresponding counts have different values.
 *
 * @param lhs The left-hand side WifiPpduRxRecord object in the comparison.
 * @param rhs The right-hand side WifiPpduRxRecord object in the comparison.
 * @return true if the objects are not equal, false otherwise.
 */
bool operator!=(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs);

/**
 * @brief Determines if one WifiPpduRxRecord object is less than another.
 *
 * Compares two WifiPpduRxRecord objects to determine if the left-hand side object is
 * considered less than the right-hand side object based on a specific criteria of comparison,
 * such as a key property value.
 *
 * @note The specific criteria for comparison should be defined and consistent.
 *
 * @param lhs The left-hand side WifiPpduRxRecord object in the comparison.
 * @param rhs The right-hand side WifiPpduRxRecord object in the comparison.
 * @return true if lhs is considered less than rhs, false otherwise.
 */
bool operator<(const WifiPpduRxRecord& lhs, const WifiPpduRxRecord& rhs);

/**
 * @brief Checks if two WifiPhyTraceStatistics objects are equal.
 *
 * Determines if two WifiPhyTraceStatistics objects are equal by comparing their counts.
 * Equality is based on the values of all relevant statistical counts being identical.
 *
 * @param lhs The left-hand side WifiPhyTraceStatistics object in the comparison.
 * @param rhs The right-hand side WifiPhyTraceStatistics object in the comparison.
 * @return true if all counts are equal, false otherwise.
 */
bool operator==(const WifiPhyTraceStatistics& lhs, const WifiPhyTraceStatistics& rhs);

/**
 * @brief Checks if two WifiPhyTraceStatistics objects are not equal.
 *
 * Determines if two WifiPhyTraceStatistics objects are not equal by comparing their counts.
 * Non-equality is based on any of the relevant statistical counts having different values.
 *
 * @param lhs The left-hand side WifiPhyTraceStatistics object in the comparison.
 * @param rhs The right-hand side WifiPhyTraceStatistics object in the comparison.
 * @return true if any property is different, false otherwise.
 */
bool operator!=(const WifiPhyTraceStatistics& lhs, const WifiPhyTraceStatistics& rhs);

/**
 * @brief Adds two WifiPhyTraceStatistics objects.
 *
 * Combines two WifiPhyTraceStatistics objects by summing their counts.
 * This can be useful for aggregating statistics from multiple sources or time periods.
 *
 * @param lhs The left-hand side WifiPhyTraceStatistics object to be added.
 * @param rhs The right-hand side WifiPhyTraceStatistics object to be added.
 * @return A new WifiPhyTraceStatistics object that is the result of the addition.
 */
WifiPhyTraceStatistics operator+(const WifiPhyTraceStatistics& lhs,
                                 const WifiPhyTraceStatistics& rhs);

} // namespace ns3

#endif /* WIFI_PHY_RX_TRACE_HELPER_H */
