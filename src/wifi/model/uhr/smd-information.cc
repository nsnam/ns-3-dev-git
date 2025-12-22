/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "smd-information.h"

#include "ns3/address-utils.h"
#include "ns3/wifi-standard-constants.h"

namespace
{
/// Size in bytes of SMD Identifier field in SMD Information element, IEEE 802.11bn D2.0 Figure
/// 9-aa43
constexpr uint16_t WIFI_SMD_INFORMATION_SMD_IDENTIFIER_SIZE_B = 6;
/// Size in bytes of SMD Capabilities field in SMD Information element, IEEE 802.11bn D2.0 Figure
/// 9-aa43
constexpr uint16_t WIFI_SMD_INFORMATION_SMD_CAPABILITIES_SIZE_B = 1;
/// Size in bytes of Preparation Timeout field in SMD Information element, IEEE 802.11bn D2.0 Figure
/// 9-aa43
constexpr uint16_t WIFI_SMD_INFORMATION_PREPARATION_TIMEOUT_SIZE_B = 1;
/// Preparation Timeout field unit in TUs. Code points 0 and 1 are reserved.
constexpr uint16_t WIFI_SMD_INFORMATION_PREP_TIMEOUT_UNIT = 64;
} // namespace

namespace ns3
{

void
SmdInformation::Print(std::ostream& os) const
{
    os << "SMD Information=["
       << "SMD Identifier: " << m_smdIdentifier << ", "
       << "SMD Capabilities: {"
       << "DL Data Forwarding: " << +m_smdCapabilities.dlDataForwarding << ", "
       << "Max Number of Prepared Target AP MLDs: " << +m_smdCapabilities.maxNumOfPrepTargetApMlds
       << ", "
       << "SMD Type: " << +m_smdCapabilities.smdType << ", "
       << "PTK Mode: " << +m_smdCapabilities.ptkMode << ", "
       << "Neighboring AP Probing Support: " << +m_smdCapabilities.neighboringApProbingSupport
       << "}, Preparation Timeout: " << m_prepTimeout.As(Time::US) << "]";
}

void
SmdInformation::SmdCapabilities::Serialize(Buffer::Iterator& start) const
{
    const uint8_t val = dlDataForwarding | (maxNumOfPrepTargetApMlds << 1) | (smdType << 4) |
                        (ptkMode << 5) | (neighboringApProbingSupport << 6);
    start.WriteU8(val);
}

uint16_t
SmdInformation::SmdCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadU8();
    dlDataForwarding = val & 0x01;
    maxNumOfPrepTargetApMlds = (val >> 1) & 0x07;
    smdType = (val >> 4) & 0x01;
    ptkMode = (val >> 5) & 0x01;
    neighboringApProbingSupport = (val >> 6) & 0x01;
    return start.GetDistanceFrom(i);
}

uint16_t
SmdInformation::SmdCapabilities::GetSerializedSize() const
{
    return WIFI_SMD_INFORMATION_SMD_CAPABILITIES_SIZE_B;
}

WifiInformationElementId
SmdInformation::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
SmdInformation::ElementIdExt() const
{
    return IE_EXT_SMD_INFORMATION;
}

uint16_t
SmdInformation::GetInformationFieldSize() const
{
    return WIFI_IE_ELEMENT_ID_EXT_SIZE + WIFI_SMD_INFORMATION_SMD_IDENTIFIER_SIZE_B +
           m_smdCapabilities.GetSerializedSize() + WIFI_SMD_INFORMATION_PREPARATION_TIMEOUT_SIZE_B;
}

void
SmdInformation::SerializeInformationField(Buffer::Iterator start) const
{
    WriteTo(start, m_smdIdentifier);
    m_smdCapabilities.Serialize(start);
    const auto units =
        static_cast<uint8_t>(Div(m_prepTimeout, WIFI_TU * WIFI_SMD_INFORMATION_PREP_TIMEOUT_UNIT));
    start.WriteU8(units);
}

uint16_t
SmdInformation::DeserializeInformationField(Buffer::Iterator start, uint16_t /* length */)
{
    auto i = start;
    ReadFrom(i, m_smdIdentifier);
    i.Next(m_smdCapabilities.Deserialize(i));
    m_prepTimeout = i.ReadU8() * WIFI_TU * WIFI_SMD_INFORMATION_PREP_TIMEOUT_UNIT;
    return i.GetDistanceFrom(start);
}

void
SmdInformation::SetPrepTimeout(const Time& prepTimeout)
{
    const auto unit = WIFI_TU * WIFI_SMD_INFORMATION_PREP_TIMEOUT_UNIT;
    NS_ASSERT_MSG((prepTimeout % unit).IsZero(),
                  "The preparation timeout must be a multiple of 64 TUs");
    const auto units = Div(prepTimeout, unit);
    NS_ASSERT_MSG(units >= 2 && units <= 255,
                  "The preparation timeout code point must be in the range 2..255");
    m_prepTimeout = prepTimeout;
}

Time
SmdInformation::GetPrepTimeout() const
{
    return m_prepTimeout;
}

} // namespace ns3
