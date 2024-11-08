//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#ifndef FLOW_PROBE_H
#define FLOW_PROBE_H

#include "flow-classifier.h"

#include "ns3/nstime.h"
#include "ns3/object.h"

#include <map>
#include <vector>

namespace ns3
{

class FlowMonitor;

/// The FlowProbe class is responsible for listening for packet events
/// in a specific point of the simulated space, report those events to
/// the global FlowMonitor, and collect its own flow statistics
/// regarding only the packets that pass through that probe.
class FlowProbe : public Object
{
  protected:
    /// Constructor
    /// @param flowMonitor the FlowMonitor this probe is associated with
    FlowProbe(Ptr<FlowMonitor> flowMonitor);
    void DoDispose() override;

  public:
    ~FlowProbe() override;

    // Delete copy constructor and assignment operator to avoid misuse
    FlowProbe(const FlowProbe&) = delete;
    FlowProbe& operator=(const FlowProbe&) = delete;

    /// Register this type.
    /// @return The TypeId.
    static TypeId GetTypeId();

    /// Structure to hold the statistics of a flow
    struct FlowStats
    {
        FlowStats()
            : delayFromFirstProbeSum(),
              bytes(0),
              packets(0)
        {
        }

        /// packetsDropped[reasonCode] => number of dropped packets
        std::vector<uint32_t> packetsDropped;
        /// bytesDropped[reasonCode] => number of dropped bytes
        std::vector<uint64_t> bytesDropped;
        /// divide by 'packets' to get the average delay from the
        /// first (entry) probe up to this one (partial delay)
        Time delayFromFirstProbeSum;
        /// Number of bytes seen of this flow
        uint64_t bytes;
        /// Number of packets seen of this flow
        uint32_t packets;
    };

    /// Container to map FlowId -> FlowStats
    typedef std::map<FlowId, FlowStats> Stats;

    /// Add a packet data to the flow stats
    /// @param flowId the flow Identifier
    /// @param packetSize the packet size
    /// @param delayFromFirstProbe packet delay
    void AddPacketStats(FlowId flowId, uint32_t packetSize, Time delayFromFirstProbe);
    /// Add a packet drop data to the flow stats
    /// @param flowId the flow Identifier
    /// @param packetSize the packet size
    /// @param reasonCode reason code for the drop
    void AddPacketDropStats(FlowId flowId, uint32_t packetSize, uint32_t reasonCode);

    /// Get the partial flow statistics stored in this probe.  With this
    /// information you can, for example, find out what is the delay
    /// from the first probe to this one.
    /// @returns the partial flow statistics
    Stats GetStats() const;

    /// Serializes the results to an std::ostream in XML format
    /// @param os the output stream
    /// @param indent number of spaces to use as base indentation level
    /// @param index FlowProbe index
    void SerializeToXmlStream(std::ostream& os, uint16_t indent, uint32_t index) const;

  protected:
    Ptr<FlowMonitor> m_flowMonitor; //!< the FlowMonitor instance
    Stats m_stats;                  //!< The flow stats
};

} // namespace ns3

#endif /* FLOW_PROBE_H */
