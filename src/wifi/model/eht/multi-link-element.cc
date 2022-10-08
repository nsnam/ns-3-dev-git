/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "multi-link-element.h"

#include "ns3/address-utils.h"
#include "ns3/mgt-headers.h"

#include <utility>

namespace ns3
{

/**
 * CommonInfoBasicMle
 */

uint16_t
CommonInfoBasicMle::GetPresenceBitmap() const
{
    // see Sec. 9.4.2.312.2.1 of 802.11be D1.5
    return (m_linkIdInfo.has_value() ? 0x0001 : 0x0) |
           (m_bssParamsChangeCount.has_value() ? 0x0002 : 0x0) |
           (m_mediumSyncDelayInfo.has_value() ? 0x0004 : 0x0) |
           (m_emlCapabilities.has_value() ? 0x0008 : 0x0) |
           (m_mldCapabilities.has_value() ? 0x0010 : 0x0);
}

uint8_t
CommonInfoBasicMle::GetSize() const
{
    uint8_t ret = 7; // Common Info Length (1) + MLD MAC Address (6)
    ret += (m_linkIdInfo.has_value() ? 1 : 0);
    ret += (m_bssParamsChangeCount.has_value() ? 1 : 0);
    ret += (m_mediumSyncDelayInfo.has_value() ? 2 : 0);
    // NOTE Fig. 9-1002h of 802.11be D1.5 reports that the size of the EML Capabilities
    // subfield is 3 octets, but this is likely a typo (the correct size is 2 octets)
    ret += (m_emlCapabilities.has_value() ? 2 : 0);
    ret += (m_mldCapabilities.has_value() ? 2 : 0);
    return ret;
}

void
CommonInfoBasicMle::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(GetSize()); // Common Info Length
    WriteTo(start, m_mldMacAddress);
    if (m_linkIdInfo.has_value())
    {
        start.WriteU8(*m_linkIdInfo & 0x0f);
    }
    if (m_bssParamsChangeCount.has_value())
    {
        start.WriteU8(*m_bssParamsChangeCount);
    }
    if (m_mediumSyncDelayInfo.has_value())
    {
        start.WriteU8(m_mediumSyncDelayInfo->mediumSyncDuration);
        uint8_t val = m_mediumSyncDelayInfo->mediumSyncOfdmEdThreshold |
                      (m_mediumSyncDelayInfo->mediumSyncMaxNTxops << 4);
        start.WriteU8(val);
    }
    if (m_emlCapabilities.has_value())
    {
        uint16_t val =
            m_emlCapabilities->emlsrSupport | (m_emlCapabilities->emlsrPaddingDelay << 1) |
            (m_emlCapabilities->emlsrTransitionDelay << 4) |
            (m_emlCapabilities->emlmrSupport << 7) | (m_emlCapabilities->emlmrDelay << 8) |
            (m_emlCapabilities->transitionTimeout << 11);
        start.WriteHtolsbU16(val);
    }
    if (m_mldCapabilities.has_value())
    {
        uint16_t val =
            m_mldCapabilities->maxNSimultaneousLinks | (m_mldCapabilities->srsSupport << 4) |
            (m_mldCapabilities->tidToLinkMappingSupport << 5) |
            (m_mldCapabilities->freqSepForStrApMld << 7) | (m_mldCapabilities->aarSupport << 12);
        start.WriteHtolsbU16(val);
    }
}

uint8_t
CommonInfoBasicMle::Deserialize(Buffer::Iterator start, uint16_t presence)
{
    Buffer::Iterator i = start;

    uint8_t length = i.ReadU8();
    ReadFrom(i, m_mldMacAddress);
    uint8_t count = 7;

    if ((presence & 0x0001) != 0)
    {
        m_linkIdInfo = i.ReadU8() & 0x0f;
        count++;
    }
    if ((presence & 0x0002) != 0)
    {
        m_bssParamsChangeCount = i.ReadU8();
        count++;
    }
    if ((presence & 0x0004) != 0)
    {
        m_mediumSyncDelayInfo = MediumSyncDelayInfo();
        m_mediumSyncDelayInfo->mediumSyncDuration = i.ReadU8();
        uint8_t val = i.ReadU8();
        m_mediumSyncDelayInfo->mediumSyncOfdmEdThreshold = val & 0x0f;
        m_mediumSyncDelayInfo->mediumSyncMaxNTxops = (val >> 4) & 0x0f;
        count += 2;
    }
    if ((presence & 0x0008) != 0)
    {
        m_emlCapabilities = EmlCapabilities();
        uint16_t val = i.ReadLsbtohU16();
        m_emlCapabilities->emlsrSupport = val & 0x0001;
        m_emlCapabilities->emlsrPaddingDelay = (val >> 1) & 0x0007;
        m_emlCapabilities->emlsrTransitionDelay = (val >> 4) & 0x0007;
        m_emlCapabilities->emlmrSupport = (val >> 7) & 0x0001;
        m_emlCapabilities->emlmrDelay = (val >> 8) & 0x0007;
        m_emlCapabilities->transitionTimeout = (val >> 11) & 0x000f;
        count += 2;
    }
    if ((presence & 0x0010) != 0)
    {
        m_mldCapabilities = MldCapabilities();
        uint16_t val = i.ReadLsbtohU16();
        m_mldCapabilities->maxNSimultaneousLinks = val & 0x000f;
        m_mldCapabilities->srsSupport = (val >> 4) & 0x0001;
        m_mldCapabilities->tidToLinkMappingSupport = (val >> 5) & 0x0003;
        m_mldCapabilities->freqSepForStrApMld = (val >> 7) & 0x001f;
        m_mldCapabilities->aarSupport = (val >> 12) & 0x0001;
        count += 2;
    }

    NS_ABORT_MSG_IF(count != length,
                    "Common Info Length (" << +length
                                           << ") differs "
                                              "from actual number of bytes read ("
                                           << +count << ")");
    return count;
}

/**
 * MultiLinkElement
 */
MultiLinkElement::MultiLinkElement(WifiMacType frameType)
    : m_frameType(frameType),
      m_commonInfo(std::in_place_type<std::monostate>) // initialize as UNSET
{
}

MultiLinkElement::MultiLinkElement(Variant variant, WifiMacType frameType)
    : MultiLinkElement(frameType)
{
    NS_ASSERT(variant != UNSET);
    SetVariant(variant);
}

WifiInformationElementId
MultiLinkElement::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
MultiLinkElement::ElementIdExt() const
{
    return IE_EXT_MULTI_LINK_ELEMENT;
}

MultiLinkElement::Variant
MultiLinkElement::GetVariant() const
{
    return static_cast<Variant>(m_commonInfo.index());
}

void
MultiLinkElement::SetVariant(Variant variant)
{
    NS_ABORT_MSG_IF(GetVariant() != UNSET, "Multi-Link Element variant already set");
    NS_ABORT_MSG_IF(variant == UNSET, "Invalid variant");

    switch (variant)
    {
    case BASIC_VARIANT:
        m_commonInfo = CommonInfoBasicMle();
        break;
    default:
        NS_ABORT_MSG("Unsupported variant: " << +variant);
    }
}

void
MultiLinkElement::SetMldMacAddress(Mac48Address address)
{
    std::get<BASIC_VARIANT>(m_commonInfo).m_mldMacAddress = address;
}

Mac48Address
MultiLinkElement::GetMldMacAddress() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_mldMacAddress;
}

void
MultiLinkElement::SetLinkIdInfo(uint8_t linkIdInfo)
{
    std::get<BASIC_VARIANT>(m_commonInfo).m_linkIdInfo = (linkIdInfo & 0x0f);
}

bool
MultiLinkElement::HasLinkIdInfo() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_linkIdInfo.has_value();
}

uint8_t
MultiLinkElement::GetLinkIdInfo() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_linkIdInfo.value();
}

void
MultiLinkElement::SetBssParamsChangeCount(uint8_t count)
{
    std::get<BASIC_VARIANT>(m_commonInfo).m_bssParamsChangeCount = count;
}

bool
MultiLinkElement::HasBssParamsChangeCount() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_bssParamsChangeCount.has_value();
}

uint8_t
MultiLinkElement::GetBssParamsChangeCount() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_bssParamsChangeCount.value();
}

void
MultiLinkElement::SetMediumSyncDelayTimer(Time delay)
{
    int64_t delayUs = delay.GetMicroSeconds();
    NS_ABORT_MSG_IF(delayUs % 32 != 0, "Delay must be a multiple of 32 microseconds");
    delayUs /= 32;

    auto& mediumSyncDelayInfo = std::get<BASIC_VARIANT>(m_commonInfo).m_mediumSyncDelayInfo;
    if (!mediumSyncDelayInfo.has_value())
    {
        mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }
    mediumSyncDelayInfo.value().mediumSyncDuration = (delayUs & 0xff);
}

Time
MultiLinkElement::GetMediumSyncDelayTimer() const
{
    return MicroSeconds(
        (std::get<BASIC_VARIANT>(m_commonInfo).m_mediumSyncDelayInfo.value().mediumSyncDuration) *
        32);
}

void
MultiLinkElement::SetMediumSyncOfdmEdThreshold(int8_t threshold)
{
    NS_ABORT_MSG_IF(threshold < -72 || threshold > -62, "Threshold may range from -72 to -62 dBm");
    uint8_t value = 72 + threshold;

    auto& mediumSyncDelayInfo = std::get<BASIC_VARIANT>(m_commonInfo).m_mediumSyncDelayInfo;
    if (!mediumSyncDelayInfo.has_value())
    {
        mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }
    mediumSyncDelayInfo.value().mediumSyncOfdmEdThreshold = value;
}

int8_t
MultiLinkElement::GetMediumSyncOfdmEdThreshold() const
{
    return (std::get<BASIC_VARIANT>(m_commonInfo)
                .m_mediumSyncDelayInfo.value()
                .mediumSyncOfdmEdThreshold) -
           72;
}

void
MultiLinkElement::SetMediumSyncMaxNTxops(uint8_t nTxops)
{
    NS_ASSERT(nTxops > 0);
    nTxops--;

    auto& mediumSyncDelayInfo = std::get<BASIC_VARIANT>(m_commonInfo).m_mediumSyncDelayInfo;
    if (!mediumSyncDelayInfo.has_value())
    {
        mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }
    mediumSyncDelayInfo.value().mediumSyncMaxNTxops = (nTxops & 0x0f);
}

uint8_t
MultiLinkElement::GetMediumSyncMaxNTxops() const
{
    return (std::get<BASIC_VARIANT>(m_commonInfo)
                .m_mediumSyncDelayInfo.value()
                .mediumSyncMaxNTxops) +
           1;
}

bool
MultiLinkElement::HasMediumSyncDelayInfo() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_mediumSyncDelayInfo.has_value();
}

MultiLinkElement::PerStaProfileSubelement::PerStaProfileSubelement(Variant variant,
                                                                   WifiMacType frameType)
    : m_variant(variant),
      m_frameType(frameType),
      m_staControl(0)
{
}

MultiLinkElement::PerStaProfileSubelement::PerStaProfileSubelement(
    const PerStaProfileSubelement& perStaProfile)
    : m_variant(perStaProfile.m_variant),
      m_frameType(perStaProfile.m_frameType),
      m_staControl(perStaProfile.m_staControl),
      m_staMacAddress(perStaProfile.m_staMacAddress)
{
    // deep copy of the STA Profile field
    if (perStaProfile.HasAssocRequest())
    {
        m_staProfile = std::make_unique<MgtAssocRequestHeader>(
            *static_cast<MgtAssocRequestHeader*>(perStaProfile.m_staProfile.get()));
    }
    else if (perStaProfile.HasReassocRequest())
    {
        m_staProfile = std::make_unique<MgtReassocRequestHeader>(
            *static_cast<MgtReassocRequestHeader*>(perStaProfile.m_staProfile.get()));
    }
    else if (perStaProfile.HasAssocResponse())
    {
        m_staProfile = std::make_unique<MgtAssocResponseHeader>(
            *static_cast<MgtAssocResponseHeader*>(perStaProfile.m_staProfile.get()));
    }
}

MultiLinkElement::PerStaProfileSubelement&
MultiLinkElement::PerStaProfileSubelement::operator=(const PerStaProfileSubelement& perStaProfile)
{
    // check for self-assignment
    if (&perStaProfile == this)
    {
        return *this;
    }

    m_variant = perStaProfile.m_variant;
    m_frameType = perStaProfile.m_frameType;
    m_staControl = perStaProfile.m_staControl;
    m_staMacAddress = perStaProfile.m_staMacAddress;

    // deep copy of the STA Profile field
    if (perStaProfile.HasAssocRequest())
    {
        m_staProfile = std::make_unique<MgtAssocRequestHeader>(
            *static_cast<MgtAssocRequestHeader*>(perStaProfile.m_staProfile.get()));
    }
    else if (perStaProfile.HasReassocRequest())
    {
        m_staProfile = std::make_unique<MgtReassocRequestHeader>(
            *static_cast<MgtReassocRequestHeader*>(perStaProfile.m_staProfile.get()));
    }
    else if (perStaProfile.HasAssocResponse())
    {
        m_staProfile = std::make_unique<MgtAssocResponseHeader>(
            *static_cast<MgtAssocResponseHeader*>(perStaProfile.m_staProfile.get()));
    }

    return *this;
}

void
MultiLinkElement::PerStaProfileSubelement::SetLinkId(uint8_t linkId)
{
    m_staControl &= 0xfff0; // reset Link ID subfield in the STA Control field
    m_staControl |= (linkId & 0x0f);
}

uint8_t
MultiLinkElement::PerStaProfileSubelement::GetLinkId() const
{
    return static_cast<uint8_t>(m_staControl & 0x000f);
}

void
MultiLinkElement::PerStaProfileSubelement::SetCompleteProfile()
{
    m_staControl |= 0x0010;
}

bool
MultiLinkElement::PerStaProfileSubelement::IsCompleteProfileSet() const
{
    return (m_staControl & 0x0010) != 0;
}

void
MultiLinkElement::PerStaProfileSubelement::SetStaMacAddress(Mac48Address address)
{
    NS_ABORT_IF(m_variant != BASIC_VARIANT);
    m_staMacAddress = address;
    m_staControl |= 0x0020;
}

bool
MultiLinkElement::PerStaProfileSubelement::HasStaMacAddress() const
{
    return (m_staControl & 0x0020) != 0;
}

Mac48Address
MultiLinkElement::PerStaProfileSubelement::GetStaMacAddress() const
{
    NS_ABORT_IF(!HasStaMacAddress());
    return m_staMacAddress;
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocRequest(
    const std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>& assoc)
{
    switch (m_frameType)
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        m_staProfile =
            std::make_unique<MgtAssocRequestHeader>(std::get<MgtAssocRequestHeader>(assoc));
        break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
        m_staProfile =
            std::make_unique<MgtReassocRequestHeader>(std::get<MgtReassocRequestHeader>(assoc));
        break;
    default:
        NS_ABORT_MSG("Invalid frame type: " << m_frameType);
    }
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocRequest(
    std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>&& assoc)
{
    switch (m_frameType)
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        m_staProfile = std::make_unique<MgtAssocRequestHeader>(
            std::move(std::get<MgtAssocRequestHeader>(assoc)));
        break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
        m_staProfile = std::make_unique<MgtReassocRequestHeader>(
            std::move(std::get<MgtReassocRequestHeader>(assoc)));
        break;
    default:
        NS_ABORT_MSG("Invalid frame type: " << m_frameType);
    }
}

bool
MultiLinkElement::PerStaProfileSubelement::HasAssocRequest() const
{
    return m_staProfile && m_frameType == WIFI_MAC_MGT_ASSOCIATION_REQUEST;
}

bool
MultiLinkElement::PerStaProfileSubelement::HasReassocRequest() const
{
    return m_staProfile && m_frameType == WIFI_MAC_MGT_REASSOCIATION_REQUEST;
}

AssocReqRefVariant
MultiLinkElement::PerStaProfileSubelement::GetAssocRequest() const
{
    if (HasAssocRequest())
    {
        return *static_cast<MgtAssocRequestHeader*>(m_staProfile.get());
    }
    NS_ABORT_UNLESS(HasReassocRequest());
    return *static_cast<MgtReassocRequestHeader*>(m_staProfile.get());
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocResponse(const MgtAssocResponseHeader& assoc)
{
    NS_ABORT_IF(m_frameType != WIFI_MAC_MGT_ASSOCIATION_RESPONSE &&
                m_frameType != WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
    m_staProfile = std::make_unique<MgtAssocResponseHeader>(assoc);
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocResponse(MgtAssocResponseHeader&& assoc)
{
    NS_ABORT_IF(m_frameType != WIFI_MAC_MGT_ASSOCIATION_RESPONSE &&
                m_frameType != WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
    m_staProfile = std::make_unique<MgtAssocResponseHeader>(std::move(assoc));
}

bool
MultiLinkElement::PerStaProfileSubelement::HasAssocResponse() const
{
    return m_staProfile && (m_frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE ||
                            m_frameType == WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}

MgtAssocResponseHeader&
MultiLinkElement::PerStaProfileSubelement::GetAssocResponse() const
{
    NS_ABORT_IF(!HasAssocResponse());
    return *static_cast<MgtAssocResponseHeader*>(m_staProfile.get());
}

uint8_t
MultiLinkElement::PerStaProfileSubelement::GetStaInfoLength() const
{
    uint8_t ret = 1; // STA Info Length

    if (HasStaMacAddress())
    {
        ret += 6;
    }
    // TODO add other subfields of the STA Info field
    return ret;
}

WifiInformationElementId
MultiLinkElement::PerStaProfileSubelement::ElementId() const
{
    return PER_STA_PROFILE_SUBELEMENT_ID;
}

uint16_t
MultiLinkElement::PerStaProfileSubelement::GetInformationFieldSize() const
{
    uint16_t ret = 2; // STA Control field

    ret += GetStaInfoLength();

    if (HasAssocRequest() || HasReassocRequest() || HasAssocResponse())
    {
        ret += m_staProfile->GetSerializedSize();
    }

    return ret;
}

void
MultiLinkElement::PerStaProfileSubelement::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteHtolsbU16(m_staControl);
    start.WriteU8(GetStaInfoLength());

    if (HasStaMacAddress())
    {
        WriteTo(start, m_staMacAddress);
    }
    // TODO add other subfields of the STA Info field
    if (HasAssocRequest() || HasReassocRequest() || HasAssocResponse())
    {
        m_staProfile->Serialize(start);
    }
}

uint16_t
MultiLinkElement::PerStaProfileSubelement::DeserializeInformationField(Buffer::Iterator start,
                                                                       uint16_t length)
{
    Buffer::Iterator i = start;
    uint16_t count = 0;

    m_staControl = i.ReadLsbtohU16();
    count += 2;

    i.ReadU8(); // STA Info Length
    count++;

    if (HasStaMacAddress())
    {
        ReadFrom(i, m_staMacAddress);
        count += 6;
    }

    // TODO add other subfields of the STA Info field

    if (count >= length)
    {
        return count;
    }

    if (m_frameType == WIFI_MAC_MGT_ASSOCIATION_REQUEST)
    {
        MgtAssocRequestHeader assoc;
        count += assoc.Deserialize(i);
        SetAssocRequest(std::move(assoc));
    }
    else if (m_frameType == WIFI_MAC_MGT_REASSOCIATION_REQUEST)
    {
        MgtReassocRequestHeader reassoc;
        count += reassoc.Deserialize(i);
        SetAssocRequest(std::move(reassoc));
    }
    else if (m_frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE ||
             m_frameType == WIFI_MAC_MGT_REASSOCIATION_RESPONSE)
    {
        MgtAssocResponseHeader assoc;
        count += assoc.Deserialize(i);
        SetAssocResponse(assoc);
    }

    return count;
}

void
MultiLinkElement::AddPerStaProfileSubelement()
{
    auto variant = GetVariant();
    NS_ABORT_IF(variant == UNSET);
    NS_ABORT_IF(m_frameType == WIFI_MAC_DATA);
    m_perStaProfileSubelements.emplace_back(variant, m_frameType);
}

std::size_t
MultiLinkElement::GetNPerStaProfileSubelements() const
{
    return m_perStaProfileSubelements.size();
}

MultiLinkElement::PerStaProfileSubelement&
MultiLinkElement::GetPerStaProfile(std::size_t i)
{
    return m_perStaProfileSubelements.at(i);
}

const MultiLinkElement::PerStaProfileSubelement&
MultiLinkElement::GetPerStaProfile(std::size_t i) const
{
    return m_perStaProfileSubelements.at(i);
}

uint16_t
MultiLinkElement::GetInformationFieldSize() const
{
    uint16_t ret = 3; // ElementIdExt (1) + Multi-Link Control (2)

    // add the Common Info field size (dependent on the Multi-Link Element variant)
    ret += std::visit(
        [](auto&& arg) -> uint8_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                NS_ABORT_MSG("Multi-Link Element variant not set");
                return 0;
            }
            else
            {
                return arg.GetSize();
            }
        },
        m_commonInfo);

    for (const auto& subelement : m_perStaProfileSubelements)
    {
        ret += subelement.GetSerializedSize();
    }

    return ret;
}

void
MultiLinkElement::SerializeInformationField(Buffer::Iterator start) const
{
    // serialize the Multi-Link Control and Common Info fields
    std::visit(
        [this, &start](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                NS_ABORT_MSG("Multi-Link Element variant not set");
            }
            else
            {
                uint16_t mlControl =
                    static_cast<uint8_t>(GetVariant()) + (arg.GetPresenceBitmap() << 4);
                start.WriteHtolsbU16(mlControl);
                arg.Serialize(start);
            }
        },
        m_commonInfo);

    for (const auto& subelement : m_perStaProfileSubelements)
    {
        start = subelement.Serialize(start);
    }
}

uint16_t
MultiLinkElement::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    uint16_t count = 0;

    uint16_t mlControl = i.ReadLsbtohU16();
    count += 2;

    SetVariant(static_cast<Variant>(mlControl & 0x0007));
    uint16_t presence = mlControl >> 4;

    uint8_t nBytes = std::visit(
        [&i, &presence](auto&& arg) -> uint8_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
                NS_ABORT_MSG("Multi-Link Element variant not set");
                return 0;
            }
            else
            {
                return arg.Deserialize(i, presence);
            }
        },
        m_commonInfo);
    i.Next(nBytes);
    count += nBytes;

    while (count < length)
    {
        switch (static_cast<SubElementId>(i.PeekU8()))
        {
        case PER_STA_PROFILE_SUBELEMENT_ID:
            AddPerStaProfileSubelement();
            i = GetPerStaProfile(GetNPerStaProfileSubelements() - 1).Deserialize(i);
            count = i.GetDistanceFrom(start);
            break;
        default:
            NS_ABORT_MSG("Unsupported Subelement ID: " << +i.PeekU8());
        }
    }

    return count;
}

} // namespace ns3
