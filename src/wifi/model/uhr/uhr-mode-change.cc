/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-mode-change.h"

#include "ns3/assert.h"

namespace ns3
{

void
UhrModeChange::Print(std::ostream& os) const
{
    os << "UHR Mode Change=[]"; // TODO
}

void
UhrModeChange::NpcaParams::Serialize(Buffer::Iterator& start) const
{
    const uint16_t val = switchingDelay | (static_cast<uint16_t>(switchBackDelay) << 6);
    start.WriteHtolsbU16(val);
}

uint16_t
UhrModeChange::NpcaParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadLsbtohU16();
    switchingDelay = val & 0x003f;
    switchBackDelay = (val >> 6) & 0x003f;
    return start.GetDistanceFrom(i);
}

uint16_t
UhrModeChange::NpcaParams::GetSerializedSize() const
{
    return WIFI_UHR_MODE_CHANGE_NPCA_PARAMS_SIZE_B;
}

void
UhrModeChange::EmlsrParams::Serialize(Buffer::Iterator& start) const
{
    const uint32_t val = linkBm | (static_cast<uint32_t>(paddingDelay) << 16) |
                         (static_cast<uint32_t>(transitionDelay) << 22) |
                         (static_cast<uint32_t>(inDevCoexAct) << 28);
    start.WriteHtolsbU32(val);
}

uint16_t
UhrModeChange::EmlsrParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadLsbtohU32();
    linkBm = val & 0xffff;
    paddingDelay = (val >> 16) & 0x003f;
    transitionDelay = (val >> 22) & 0x003f;
    inDevCoexAct = (val >> 28) & 0x0001;
    return start.GetDistanceFrom(i);
}

uint16_t
UhrModeChange::EmlsrParams::GetSerializedSize() const
{
    return WIFI_UHR_MODE_CHANGE_EMLSR_PARAMS_SIZE_B;
}

void
UhrModeChange::ModeTuple::Serialize(Buffer::Iterator& start) const
{
    uint8_t val = modeId | (modeEnable << 6) | (modeUpdate << 7);
    start.WriteU8(val);
    if (!modeLength.has_value())
    {
        return;
    }
    NS_ASSERT_MSG(modeParams.has_value(),
                  "Mode Specific Parameters must be set if Mode Length is set");
    val = modeLength.value();
    // TODO: Mode Specific Control field is reserved for all modes in latest draft
    start.WriteU8(val);
    auto serializeModeParams = [&](auto&& modeParams) { modeParams.Serialize(start); };
    std::visit(serializeModeParams, modeParams.value());
}

uint16_t
UhrModeChange::ModeTuple::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    auto val = start.ReadU8();
    modeId = val & 0x003f;
    modeEnable = (val >> 6) & 0x01;
    modeUpdate = (val >> 7) & 0x01;
    // Mode Specific Parameters field not implemented yet for all modes or not present/defined in
    // latest draft
    const auto hasModeParams = (modeId == static_cast<uint8_t>(UhrModes::NPCA)) ||
                               (modeId == static_cast<uint8_t>(UhrModes::EMLSR));
    if (!hasModeParams || modeEnable == 0)
    {
        // IEEE 802.11be D1.0 9.4.2.aa7 UHR Mode Change element: The Mode Length, Mode Specific
        // Control, and Mode Specific Parameters fields are not included if the Mode Enable field is
        // set to 0 or the value in the Mode ID field corresponds to a mode without parameters
        modeLength = std::nullopt;
        modeParams = std::nullopt;
        return start.GetDistanceFrom(i);
    }

    val = start.ReadU8();
    modeLength = val & 0x0f;
    // TODO: Mode Specific Control field is reserved for all modes in latest draft
    uint16_t count = start.GetDistanceFrom(i);

    auto deserializeModeParams = [&](auto&& modeParams) -> uint16_t {
        return modeParams.Deserialize(start);
    };

    switch (const auto mode = static_cast<UhrModes>(modeId); mode)
    {
    case UhrModes::NPCA:
        modeParams = NpcaParams();
        break;
    case UhrModes::EMLSR:
        modeParams = EmlsrParams();
        break;
    default:
        NS_ASSERT_MSG(false, "Unexpected Mode ID: " << +modeId);
        break;
    }
    const auto sizeParams = std::visit(deserializeModeParams, *modeParams);
    count += sizeParams;
    NS_ASSERT_MSG(modeLength.value() == sizeParams,
                  "Mode Length field (" << +modeLength.value() << " bytes) does not match actual "
                                        << sizeParams << " bytes of Mode Specific Parameters");
    return count;
}

uint16_t
UhrModeChange::ModeTuple::GetSerializedSize() const
{
    return modeLength.has_value() ? (WIFI_UHR_MODE_TUPLE_WITH_PARAMS_SIZE_B + modeLength.value())
                                  : WIFI_UHR_MODE_TUPLE_COMMON_SIZE_B;
}

WifiInformationElementId
UhrModeChange::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
UhrModeChange::ElementIdExt() const
{
    return IE_EXT_UHR_MODE_CHANGE_ELEMENT;
}

uint16_t
UhrModeChange::GetInformationFieldSize() const
{
    auto ret = WIFI_IE_ELEMENT_ID_EXT_SIZE;
    for (const auto& tuple : m_modeTuples)
    {
        ret += tuple.GetSerializedSize();
    }
    return ret;
}

void
UhrModeChange::SerializeInformationField(Buffer::Iterator start) const
{
    for (const auto& tuple : m_modeTuples)
    {
        tuple.Serialize(start);
    }
}

uint16_t
UhrModeChange::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    uint16_t count = 0;
    while (count < length)
    {
        ModeTuple tuple;
        const auto tupleSize = tuple.Deserialize(i);
        m_modeTuples.push_back(tuple);
        i.Next(tupleSize);
        count += tupleSize;
    }
    return count;
}

} // namespace ns3
