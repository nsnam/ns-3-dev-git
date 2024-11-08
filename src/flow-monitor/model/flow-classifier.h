//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#ifndef FLOW_CLASSIFIER_H
#define FLOW_CLASSIFIER_H

#include "ns3/simple-ref-count.h"

#include <ostream>

namespace ns3
{

/**
 * @ingroup flow-monitor
 * @brief Abstract identifier of a packet flow
 */
typedef uint32_t FlowId;

/**
 * @ingroup flow-monitor
 * @brief Abstract identifier of a packet within a flow
 */
typedef uint32_t FlowPacketId;

/// @ingroup flow-monitor
/// Provides a method to translate raw packet data into abstract
/// `flow identifier` and `packet identifier` parameters.  These
/// identifiers are unsigned 32-bit integers that uniquely identify a
/// flow and a packet within that flow, respectively, for the whole
/// simulation, regardless of the point in which the packet was
/// captured.  These abstract identifiers are used in the
/// communication between FlowProbe and FlowMonitor, and all collected
/// statistics reference only those abstract identifiers in order to
/// keep the core architecture generic and not tied down to any
/// particular flow capture method or classification system.
class FlowClassifier : public SimpleRefCount<FlowClassifier>
{
  private:
    FlowId m_lastNewFlowId; //!< Last known Flow ID

  public:
    FlowClassifier();
    virtual ~FlowClassifier();

    // Delete copy constructor and assignment operator to avoid misuse
    FlowClassifier(const FlowClassifier&) = delete;
    FlowClassifier& operator=(const FlowClassifier&) = delete;

    /// Serializes the results to an std::ostream in XML format
    /// @param os the output stream
    /// @param indent number of spaces to use as base indentation level
    virtual void SerializeToXmlStream(std::ostream& os, uint16_t indent) const = 0;

  protected:
    /// Returns a new, unique Flow Identifier
    /// @returns a new FlowId
    FlowId GetNewFlowId();

    ///
    /// @brief Add a number of spaces for indentation purposes.
    /// @param os The stream to write to.
    /// @param level The number of spaces to add.
    void Indent(std::ostream& os, uint16_t level) const;
};

inline void
FlowClassifier::Indent(std::ostream& os, uint16_t level) const
{
    for (uint16_t __xpto = 0; __xpto < level; __xpto++)
    {
        os << ' ';
    }
}

} // namespace ns3

#endif /* FLOW_CLASSIFIER_H */
