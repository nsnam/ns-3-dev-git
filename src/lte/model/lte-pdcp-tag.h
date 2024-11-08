/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef PDCP_TAG_H
#define PDCP_TAG_H

#include "ns3/nstime.h"
#include "ns3/packet.h"

namespace ns3
{

class Tag;

/**
 * Tag to calculate the per-PDU delay from eNb PDCP to UE PDCP
 */

class PdcpTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Create an empty PDCP tag
     */
    PdcpTag();
    /**
     * Create an PDCP tag with the given senderTimestamp
     * @param senderTimestamp the time stamp
     */
    PdcpTag(Time senderTimestamp);

    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    /**
     * Get the instant when the PDCP delivers the PDU to the MAC SAP provider
     * @return the sender timestamp
     */
    Time GetSenderTimestamp() const;

    /**
     * Set the sender timestamp
     * @param senderTimestamp time stamp of the instant when the PDCP delivers the PDU to the MAC
     * SAP provider
     */
    void SetSenderTimestamp(Time senderTimestamp);

  private:
    Time m_senderTimestamp; ///< sender timestamp
};

} // namespace ns3

#endif /* PDCP_TAG_H */
