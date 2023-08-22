/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef GCR_GROUP_ADDRESS_H
#define GCR_GROUP_ADDRESS_H

#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11 GCR Group Address Element (Sec. 9.4.2.125 of 802.11-2020)
 */
class GcrGroupAddress : public WifiInformationElement
{
  public:
    GcrGroupAddress() = default;

    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    Mac48Address m_gcrGroupAddress; //!< GCR Group Address field

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* GCR_GROUP_ADDRESS_H */
