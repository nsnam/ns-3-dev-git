/*
 * Copyright (c) 2009, 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef ALOHA_NOACK_MAC_HEADER_H
#define ALOHA_NOACK_MAC_HEADER_H

#include "ns3/address-utils.h"
#include "ns3/header.h"
#include "ns3/mac48-address.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *  Header for the AlohaNoack NetDevice
 *
 */
class AlohaNoackMacHeader : public Header
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the source address
     * @param source the source address
     */
    void SetSource(Mac48Address source);
    /**
     * Set the destination address
     * @param destination the destination address
     */
    void SetDestination(Mac48Address destination);

    /**
     * Get the source address
     * @returns the source address
     */
    Mac48Address GetSource() const;
    /**
     * Get the destination address
     * @returns the destination address
     */
    Mac48Address GetDestination() const;

  private:
    Mac48Address m_source;      //!< source address
    Mac48Address m_destination; //!< destination address
};

} // namespace ns3

#endif /* ALOHA_NOACK_MAC_HEADER_H */
