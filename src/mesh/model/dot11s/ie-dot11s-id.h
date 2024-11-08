/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef MESH_ID_H
#define MESH_ID_H

#include "ns3/buffer.h"
#include "ns3/mesh-information-element-vector.h"

#include <stdint.h>

namespace ns3
{
namespace dot11s
{

/**
 * @brief a IEEE 802.11 Mesh ID element (Section 8.4.2.101 of IEEE 802.11-2012)
 */
class IeMeshId : public WifiInformationElement
{
  public:
    // broadcast meshId
    IeMeshId();
    /**
     * Constructor
     *
     * @param s reference id
     */
    IeMeshId(std::string s);

    /**
     * Equality test
     * @param o another IeMeshId
     * @returns true if equal
     */
    bool IsEqual(const IeMeshId& o) const;
    /**
     * Return true if broadcast (if first octet of Mesh ID is zero)
     * @returns true if broadcast
     */
    bool IsBroadcast() const;
    // uint32_t GetLength () const;
    /**
     * Peek the IeMeshId as a string value
     * @returns the mesh ID as a string
     */
    char* PeekString() const;

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;
    uint16_t GetInformationFieldSize() const override;

  private:
    uint8_t m_meshId[33]; ///< mesh ID
    /**
     * equality operator
     *
     * @param a lhs
     * @param b lhs
     * @returns true if equal
     */
    friend bool operator==(const IeMeshId& a, const IeMeshId& b);
};

/**
 * @brief Stream insertion operator.
 *
 * @param [in] os The reference to the output stream.
 * @param [in] meshId The IeMeshId object.
 * @returns The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const IeMeshId& meshId);

} // namespace dot11s
} // namespace ns3
#endif /* MESH_ID_H */
