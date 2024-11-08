/*
 *  Copyright (c) 2010 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *
 */
#ifndef WIMAX_MAC_TO_MAC_HEADER_H
#define WIMAX_MAC_TO_MAC_HEADER_H

#include "ns3/header.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup wimax
 * @brief this class implements the mac to mac header needed to dump a wimax pcap file
 * The header format was reverse-engineered by looking  at existing live pcap traces which
 * could be opened with wireshark  i.e., we have no idea where this is coming from.
 */
class WimaxMacToMacHeader : public Header
{
  public:
    WimaxMacToMacHeader();
    ~WimaxMacToMacHeader() override;
    /**
     * Constructor
     *
     * @param len length
     */
    WimaxMacToMacHeader(uint32_t len);

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    /**
     * Get size of length field
     * @returns the size of length field
     */
    uint8_t GetSizeOfLen() const;
    void Print(std::ostream& os) const override;

  private:
    uint32_t m_len; ///< length
};
}; // namespace ns3
#endif
