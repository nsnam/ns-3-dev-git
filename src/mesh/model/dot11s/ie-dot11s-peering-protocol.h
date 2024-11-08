/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef MESH_PEERING_PROTOCOL_H
#define MESH_PEERING_PROTOCOL_H

#include "ns3/mesh-information-element-vector.h"

namespace ns3
{
namespace dot11s
{

/**
 * @ingroup dot11s
 *
 * @brief Mesh Peering Protocol Identifier information element
 * Note that it does not permit to set any value besides zero
 * (corresponding to mesh peering management protocol)
 */
class IePeeringProtocol : public WifiInformationElement
{
  public:
    IePeeringProtocol();

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator i, uint16_t length) override;
    void Print(std::ostream& os) const override;

  private:
    uint8_t m_protocol; ///< the protocol
};
} // namespace dot11s
} // namespace ns3
#endif
