/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef FLOW_ID_TAG_H
#define FLOW_ID_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class FlowIdTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer buf) const override;
    void Deserialize(TagBuffer buf) override;
    void Print(std::ostream& os) const override;
    FlowIdTag();

    /**
     * Constructs a FlowIdTag with the given flow id
     *
     * @param flowId Id to use for the tag
     */
    FlowIdTag(uint32_t flowId);
    /**
     * Sets the flow id for the tag
     * @param flowId Id to assign to the tag
     */
    void SetFlowId(uint32_t flowId);
    /**
     * Gets the flow id for the tag
     * @returns current flow id for this tag
     */
    uint32_t GetFlowId() const;
    /**
     * Uses a static variable to generate sequential flow id
     * @returns flow id allocated
     */
    static uint32_t AllocateFlowId();

  private:
    uint32_t m_flowId; //!< Flow ID
};

} // namespace ns3

#endif /* FLOW_ID_TAG_H */
