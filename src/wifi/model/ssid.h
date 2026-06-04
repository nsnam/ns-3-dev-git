/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef SSID_H
#define SSID_H

#include "wifi-information-element.h"

#include "ns3/wifi-export.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11 SSID Information Element
 *
 * @see attribute_Ssid
 */
class WIFI_EXPORT Ssid : public WifiInformationElement
{
  public:
    /**
     * Create SSID with broadcast SSID
     */
    Ssid();
    /**
     * Create SSID from a given string
     *
     * @param s SSID in string
     */
    Ssid(std::string s);

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Check if the two SSIDs are equal.
     *
     * @param o SSID to compare to
     *
     * @return true if the two SSIDs are equal,
     *         false otherwise
     */
    bool IsEqual(const Ssid& o) const;
    /**
     * Check if the SSID is broadcast.
     *
     * @return true if the SSID is broadcast,
     *         false otherwise
     */
    bool IsBroadcast() const;

    /**
     * Peek the SSID.
     *
     * @return a pointer to SSID string
     */
    char* PeekString() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    uint8_t m_ssid[33]; //!< Raw SSID value
    uint8_t m_length;   //!< Length of the SSID
};

/**
 * Serialize from the given istream to this SSID.
 *
 * @param is the input stream
 * @param ssid the SSID
 *
 * @return std::istream
 */
WIFI_EXPORT std::istream& operator>>(std::istream& is, Ssid& ssid);

// Hand-expanded ATTRIBUTE_HELPER_HEADER(Ssid): the attribute value class is
// spelled out so it can carry WIFI_EXPORT. The wifi library is built with
// hidden symbol visibility on MinGW (see src/wifi/CMakeLists.txt), and SsidValue
// is constructed by user code outside the library, so it must be exported. The
// accessor and checker remain macro-generated because they are used only within
// the wifi library.
/**
 * @ingroup attribute_Ssid
 * AttributeValue implementation for Ssid.
 */
class WIFI_EXPORT SsidValue : public AttributeValue
{
  public:
    SsidValue() = default;
    SsidValue(const Ssid& value); //!< Constructor
    void Set(const Ssid& value);  //!< Set the value
    Ssid Get() const;             //!< @return the value

    template <typename T>
    bool GetAccessor(T& value) const
    {
        value = T(m_value);
        return true;
    }

    Ptr<AttributeValue> Copy() const override;
    std::string SerializeToString(Ptr<const AttributeChecker> checker) const override;
    bool DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker) override;

  private:
    Ssid m_value; //!< the value
};

ATTRIBUTE_ACCESSOR_DEFINE(Ssid);
ATTRIBUTE_CHECKER_DEFINE(Ssid);

} // namespace ns3

#endif /* SSID_H */
