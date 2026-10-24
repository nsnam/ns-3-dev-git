/*
 * Copyright (c) 2008 INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * C++ helper functions for use by the python visualizer (for things
 * Python is too slow at).
 *
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#ifndef NS3_PYVIZ_H
#define NS3_PYVIZ_H

#include "ns3/address.h"
#include "ns3/channel.h"
#include "ns3/event-id.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

#include <deque>
#include <map>
#include <set>

namespace ns3
{
namespace visualizer
{

/**
 * TransmissionSample structure
 */
struct TransmissionSample
{
    Ptr<Node> transmitter; //!< The source node of the transmission
    Ptr<Node> receiver;    //!< The destination node of the transmission (Null if broadcast)
    Ptr<Channel> channel;  //!< The channel reference used for the transmission
    uint32_t bytes;        //!< The number of bytes transmitted
};

/**
 * One observed packet interval between the visualizer's device Tx and Rx traces.
 */
struct PacketFlowSample
{
    Time startTx;          //!< Time at the transmitting device trace
    Time startRx;          //!< Time at the receiving device trace
    Ptr<Node> transmitter; //!< The source node
    Ptr<Node> receiver;    //!< The destination node
    uint32_t bytes;        //!< Packet size in bytes
    uint32_t packetUid;    //!< Packet UID recorded by the Tx trace
};

/**
 * PacketDropSample structure
 */
struct PacketDropSample
{
    Ptr<Node> transmitter; //!< The transmitter node where the drop was registered
    uint32_t bytes;        //!< The number of bytes dropped
    uint32_t packets;      //!< The number of packets dropped
    std::string kind;      //!< The cause: "queue", "channel", "ttl" or "other"
};

/**
 * Bytes a node transmitted during one simulation step, received or not.
 */
struct NodeTxSample
{
    Ptr<Node> transmitter; //!< The transmitting node
    uint32_t bytes;        //!< The number of bytes transmitted
};

/**
 * PacketSample structure.
 */
struct PacketSample
{
    Time time;             //!< The received or transmitted time of the sample.
    Ptr<Packet> packet;    //!< The packet reference
    Ptr<NetDevice> device; //!< The NetDevice reference
};

/**
 * TxPacketSample structure
 */
struct TxPacketSample : PacketSample
{
    /// Address of the device that received the packet, the broadcast address
    /// of the transmitting device if several did, or empty if none did (yet).
    Address to;
};

/**
 *  RxPacketSample structure
 */
struct RxPacketSample : PacketSample
{
    Address from; //!< Address of the transmitting device, or empty if unknown
};

/**
 *  Structure to handle a sample of the last received, transmitted or drop packets
 *  in a node.
 */
struct LastPacketsSample
{
    std::vector<RxPacketSample> lastReceivedPackets;    //!< Last received packets
    std::vector<TxPacketSample> lastTransmittedPackets; //!< Last transmitted packets
    std::vector<PacketSample> lastDroppedPackets;       //!< Last dropped packets
};

/**
 * The NetDeviceStatistics structure
 */
struct NetDeviceStatistics
{
    NetDeviceStatistics()
        : transmittedBytes(0),
          receivedBytes(0),
          transmittedPackets(0),
          receivedPackets(0)
    {
    }

    uint64_t transmittedBytes;   //!< Transmitted bytes
    uint64_t receivedBytes;      //!< Received bytes
    uint32_t transmittedPackets; //!< Transmitted packets
    uint32_t receivedPackets;    //!< Received packets
};

/**
 * The NodeStatistics structure
 */
struct NodeStatistics
{
    uint32_t nodeId;                             //!<  Node ID
    std::vector<NetDeviceStatistics> statistics; //!<  Statistics
};

/**
 * The PacketCaptureMode enumeration
 */
enum PacketCaptureMode
{
    PACKET_CAPTURE_DISABLED = 1,       //!< Packet capture is disabled
    PACKET_CAPTURE_FILTER_HEADERS_OR,  //!< Packet capture if any of the indicated headers is
                                       //!< present
    PACKET_CAPTURE_FILTER_HEADERS_AND, //!< Packet capture if all of the indicated headers are
                                       //!< present
};

/**
 * The PacketCaptureOptions structure
 */
struct PacketCaptureOptions
{
    std::set<TypeId> headers; //!< The headers
    uint32_t numLastPackets;  //!< Number of last packets
    PacketCaptureMode mode;   //!< The packet capture node
};

using TransmissionSampleList = std::vector<TransmissionSample>; //!< The transmission sample list
using PacketDropSampleList = std::vector<PacketDropSample>;     //!< The packet drop list

/**
 * @ingroup visualizer
 *
 * @brief helper class to be used by the visualizer
 * @internal
 *
 * This class is not meant to be used by simulations.  It is only
 * meant to be used by the visualizer tool (PyViz).  The only reason
 * it is public is because Python bindings for it are needed,
 * otherwise it should be considered private.
 **/
class PyViz
{
  public:
    PyViz();
    ~PyViz();

    /**
     * Run simulation until a given (simulated, absolute) time is reached.
     *
     * @param time the run time
     * @return True if the simulation reached the given time; false if it
     *         stopped earlier, because no events remain or Simulator::Stop()
     *         was called by the simulation itself.
     */
    bool SimulatorRunUntil(Time time);

    /**
     * Run the rest of the simulation, until no events remain or
     * Simulator::Stop() is called.
     */
    void SimulatorRun();

    /**
     * Get the stop time of the underlying visual simulator implementation.
     *
     * @return The stop time of the visual simulator implementation.
     */
    Time GetSimulatorStopTime();

    /**
     * Pause function.
     *
     * @param message the pause message
     */
    static void Pause(const std::string& message);

    /**
     * Get pause message function.
     *
     * @returns the pause message
     */
    std::vector<std::string> GetPauseMessages() const;

    /**
     * Get transmission samples.
     *
     * @returns the transmission sample list
     */
    TransmissionSampleList GetTransmissionSamples() const;

    /**
     * Get recently completed packet Tx-to-Rx intervals.
     *
     * @return Recent packet flow samples.
     */
    std::vector<PacketFlowSample> GetPacketFlowSamples() const;

    /**
     * Get packet drop samples.
     *
     * @returns the packet drop sample list
     */
    PacketDropSampleList GetPacketDropSamples() const;

    /**
     * Get the bytes each node transmitted during the last step, whether or not
     * any device received them.
     *
     * @return The per-node transmission samples.
     */
    std::vector<NodeTxSample> GetNodeTxSamples() const;

    /**
     * Get last packets function.
     *
     * @param nodeId the node ID
     * @returns the last packets
     */
    LastPacketsSample GetLastPackets(uint32_t nodeId) const;

    /**
     * Set nodes of interest function.
     *
     * @param nodes the collection of nodes
     */
    void SetNodesOfInterest(std::set<uint32_t> nodes);

    /**
     * Get node statistics.
     *
     * @returns the node statistics
     */
    std::vector<NodeStatistics> GetNodesStatistics() const;

    /**
     * Set packet capture options function.
     *
     * @param nodeId the node ID
     * @param options the capture options
     */
    void SetPacketCaptureOptions(uint32_t nodeId, PacketCaptureOptions options);

    /**
     * Register drop trace path function.
     *
     * @param tracePath the path to trace
     */
    void RegisterDropTracePath(const std::string& tracePath);

    /**
     * Connect a generic visualizer trace handler to a Config path.
     *
     * The trace source must have the signature void (Ptr<const Packet>) and
     * its context must name the node ("/NodeList/N/...") and, for "Tx" and
     * "Rx", the device ("/NodeList/N/DeviceList/D/..."). Which trace sources
     * of which device types to connect is decided by the Python visualizer;
     * sources with other signatures are adapted there and use RecordTx(),
     * RecordRx() and RecordDrop().
     *
     * The caller must make sure every TypeId named in the path is registered,
     * since Config path resolution aborts on unknown type names.
     *
     * @param handler "Tx" (a device sends a packet), "Rx" (a device receives a
     *        packet) or "Drop" (a node drops a packet).
     * @param path Config path of the trace source.
     * @return True if the handler exists and at least one trace was connected.
     */
    bool ConnectTrace(const std::string& handler, const std::string& path);

    /**
     * Record a packet sent by a device.
     *
     * @param nodeIndex the node index
     * @param devIndex the device index in the node
     * @param packet the packet
     */
    void RecordTx(uint32_t nodeIndex, uint32_t devIndex, Ptr<const Packet> packet);

    /**
     * Record a packet received by a device; matched with the transmission
     * of the same packet on the same channel.
     *
     * @param nodeIndex the node index
     * @param devIndex the device index in the node
     * @param packet the packet
     */
    void RecordRx(uint32_t nodeIndex, uint32_t devIndex, Ptr<const Packet> packet);

    /**
     * Record a packet dropped by a node.
     *
     * @param nodeIndex the node index
     * @param packet the packet
     * @param kind the cause: "queue", "channel", "ttl" or "other"
     */
    void RecordDrop(uint32_t nodeIndex,
                    Ptr<const Packet> packet,
                    const std::string& kind = "other");

    /**
     * Register CSMA like device function.
     *
     * @param deviceTypeName the device type name
     */
    void RegisterCsmaLikeDevice(const std::string& deviceTypeName);

    /**
     * Register WIFI like device function.
     *
     * @param deviceTypeName the device type name
     */
    void RegisterWifiLikeDevice(const std::string& deviceTypeName);

    /**
     * Register point to point like device function.
     *
     * @param deviceTypeName the device type name
     */
    void RegisterPointToPointLikeDevice(const std::string& deviceTypeName);

    // Yes, I know, this is just a utility function, not really related to the class in any way.
    /**
     * Utility function - clips a line to a bounding box.
     * @param [in] boundsX1 Bounding box, minimum X coord
     * @param [in] boundsY1 Bounding box, minimum Y coord
     * @param [in] boundsX2 Bounding box, maximum X coord
     * @param [in] boundsY2 Bounding box, maximum Y coord
     * @param [in,out] lineX1 Line, minimum X coord (any on input, clipped to the bounding box
     * on output)
     * @param [in,out] lineY1 Line, minimum Y coord (any on input, clipped to the bounding box
     * on output)
     * @param [in,out] lineX2 Line, maximum X coord (any on input, clipped to the bounding box
     * on output)
     * @param [in,out] lineY2 Line, maximum Y coord (any on input, clipped to the bounding box
     * on output)
     */
    // -#- @lineX1(direction=inout); @lineY1(direction=inout); @lineX2(direction=inout);
    // @lineY2(direction=inout) -#-
    static void LineClipping(double boundsX1,
                             double boundsY1,
                             double boundsX2,
                             double boundsY2,
                             double& lineX1,
                             double& lineY1,
                             double& lineX2,
                             double& lineY2);

  private:
    /**
     * Get packet capture options function.
     *
     * @param nodeId The node ID
     * @param outOptions The packet capture options
     * @returns True if successful
     */
    bool GetPacketCaptureOptions(uint32_t nodeId, const PacketCaptureOptions** outOptions) const;

    /**
     * Filter packet function.
     *
     * @param packet The packet
     * @param options The capture options
     * @returns True if successful
     */
    static bool FilterPacket(Ptr<const Packet> packet, const PacketCaptureOptions& options);

    /**
     * Find net device statistics function.
     *
     * @param node The node
     * @param interface The interface number
     * @returns The device statistics
     */
    inline NetDeviceStatistics& FindNetDeviceStatistics(int node, int interface);

    /**
     * Do pause function.
     *
     * @param message the pause message
     */
    void DoPause(const std::string& message);

    /// Run the simulator implementation wrapped by the visualizer, or the simulator itself
    void RunSimulator();

    /**
     * Stop simulation callback function.
     */
    void CallbackStopSimulation();

    /////////////////////
    // Trace callbacks //
    /////////////////////

    /**
     * Generic transmit trace callback.
     *
     * @param context the context ("/NodeList/N/DeviceList/D/...")
     * @param packet the packet
     */
    void TraceTx(std::string context, Ptr<const Packet> packet);

    /**
     * Generic receive trace callback.
     *
     * @param context the context ("/NodeList/N/DeviceList/D/...")
     * @param packet the packet
     */
    void TraceRx(std::string context, Ptr<const Packet> packet);

    /**
     * Generic drop trace callback.
     *
     * @param context the context ("/NodeList/N/...")
     * @param packet the packet
     */
    void TraceDrop(std::string context, Ptr<const Packet> packet);

    /**
     * Trace a packet a full queue discarded.
     *
     * @param context the context
     * @param packet the packet
     */
    void TraceQueueDrop(std::string context, Ptr<const Packet> packet);

    /**
     * Trace a packet the channel corrupted, discarded by its receiver.
     *
     * @param context the context
     * @param packet the packet
     */
    void TraceChannelDrop(std::string context, Ptr<const Packet> packet);

    /**
     * The TxRecordValue structure
     */
    struct TxRecordValue
    {
        Time time;         //!< The transmission time
        Ptr<Node> srcNode; //!< The source node of the transmission
        bool isBroadcast;  //!< Broadcast flag
    };

    /**
     * The TransmissionSampleKey structure
     */
    struct TransmissionSampleKey
    {
        /**
         * Less than operator
         *
         * @param other Object to compare
         * @return True if less than
         */
        bool operator<(const TransmissionSampleKey& other) const;
        /**
         * Equality operator
         *
         * @param other Object to compare
         * @return True if equal
         */
        bool operator==(const TransmissionSampleKey& other) const;
        Ptr<Node> transmitter; //!< transmitter
        Ptr<Node> receiver;    //!< NULL if broadcast
        Ptr<Channel> channel;  //!< channel
    };

    /**
     * The TransmissionSampleValue structure
     */
    struct TransmissionSampleValue
    {
        uint32_t bytes; //!< The nuumber of bytes of the transmission sample
    };

    using TxRecordKey = std::pair<Ptr<Channel>, uint32_t>; //!< TxRecordKey type definition

    std::map<uint32_t, PacketCaptureOptions> m_packetCaptureOptions; //!< Packet capture options
    std::vector<std::string> m_pauseMessages;                        //!< Pause message
    std::map<TxRecordKey, TxRecordValue> m_txRecords;                //!< Transmit records
    /// Transmitting device of the recent transmissions, by channel and packet
    /// UID, to fill in the addresses of the last packets samples
    std::map<TxRecordKey, Ptr<NetDevice>> m_recentTransmitters;
    std::deque<TxRecordKey>
        m_recentTransmissionOrder; //!< Keys of m_recentTransmitters, oldest first
    std::map<TransmissionSampleKey,
             TransmissionSampleValue>
        m_transmissionSamples;                         //!< Transmission samples
    std::vector<PacketFlowSample> m_packetFlowSamples; //!< Recent Tx-to-Rx intervals
    using DropKey = std::pair<Ptr<Node>, std::string>; //!< Dropping node and cause
    std::map<DropKey, uint32_t> m_packetDrops;         //!< Bytes dropped
    std::map<DropKey, uint32_t> m_packetDropCounts;    //!< Packets dropped
    std::map<Ptr<Node>, uint32_t> m_nodeTxBytes;       //!< Bytes transmitted per node
    std::set<uint32_t> m_nodesOfInterest;              //!< List of node IDs whose transmissions
                                                       //!< will be monitored
    std::map<uint32_t, Time> m_packetsOfInterest; //!< List of packet UIDs that will be monitored
    std::map<uint32_t, LastPacketsSample> m_lastPackets;                    //!< Last packets
    std::map<uint32_t, std::vector<NetDeviceStatistics>> m_nodesStatistics; //!< Node statistics
    bool m_stop;     //!< Stop simulation flag
    Time m_runUntil; //!< Indicates until when the simulation should run for its next step
};

} // namespace visualizer
} // namespace ns3

#endif /* NS3_PYVIZ_H */
