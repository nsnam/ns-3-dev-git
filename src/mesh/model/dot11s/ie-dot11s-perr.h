/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef PERR_INFORMATION_ELEMENT_H
#define PERR_INFORMATION_ELEMENT_H

#include "hwmp-protocol.h"

#include "ns3/mac48-address.h"
#include "ns3/mesh-information-element-vector.h"

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 * @brief See 7.3.2.98 of 802.11s draft 2.07
 */
class IePerr : public WifiInformationElement
{
  public:
    IePerr();
    ~IePerr() override;
    /**
     * Get number of destination function
     * @returns the number of destinations
     */
    uint8_t GetNumOfDest() const;
    /**
     * Add address unit function
     * @param unit the address of the failed destination
     */
    void AddAddressUnit(HwmpProtocol::FailedDestination unit);
    /**
     * Is full function
     * @returns true if full
     */
    bool IsFull() const;
    /**
     * Get address unit vector function
     * @returns the list of failed destinations
     */
    std::vector<HwmpProtocol::FailedDestination> GetAddressUnitVector() const;
    /**
     * Delete address unit function
     * @param address the MAC address of the deleted unit
     */
    void DeleteAddressUnit(Mac48Address address);
    /// Reset PERR
    void ResetPerr();

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;
    uint16_t GetInformationFieldSize() const override;

  private:
    std::vector<HwmpProtocol::FailedDestination> m_addressUnits; ///< address units
    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IePerr& a, const IePerr& b);
};

bool operator==(const IePerr& a, const IePerr& b);
std::ostream& operator<<(std::ostream& os, const IePerr& perr);
} // namespace dot11s
} // namespace ns3
#endif
