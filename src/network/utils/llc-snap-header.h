/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef LLC_SNAP_HEADER_H
#define LLC_SNAP_HEADER_H

#include "ns3/header.h"

#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * The length in octets of the LLC/SNAP header
 */
static const uint16_t LLC_SNAP_HEADER_LENGTH = 8;

/**
 * @ingroup network
 *
 * @brief Header for the LLC/SNAP encapsulation
 *
 * For a list of EtherTypes, see
 * http://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
 */
class LlcSnapHeader : public Header
{
  public:
    LlcSnapHeader();

    /**
     * @brief Set the Ethertype.
     * @param type the Ethertype
     */
    void SetType(uint16_t type);
    /**
     * @brief Return the Ethertype.
     * @return Ethertype
     */
    uint16_t GetType();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint16_t m_etherType; //!< the Ethertype
};

} // namespace ns3

#endif /* LLC_SNAP_HEADER_H */
