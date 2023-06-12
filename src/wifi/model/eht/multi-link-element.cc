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

uint8_t
CommonInfoBasicMle::EncodeEmlsrPaddingDelay(Time delay)
{
    auto delayUs = delay.GetMicroSeconds();

    if (delayUs == 0)
    {
        return 0;
    }

    for (uint8_t i = 1; i <= 4; i++)
    {
        if (1 << (i + 4) == delayUs)
        {
            return i;
        }
    }

    NS_ABORT_MSG("Value not allowed (" << delay.As(Time::US) << ")");
    return 0;
}

Time
CommonInfoBasicMle::DecodeEmlsrPaddingDelay(uint8_t value)
{
    NS_ABORT_MSG_IF(value > 4, "Value not allowed (" << +value << ")");
    if (value == 0)
    {
        return MicroSeconds(0);
    }
    return MicroSeconds(1 << (4 + value));
}

uint8_t
CommonInfoBasicMle::EncodeEmlsrTransitionDelay(Time delay)
{
    auto delayUs = delay.GetMicroSeconds();

    if (delayUs == 0)
    {
        return 0;
    }

    for (uint8_t i = 1; i <= 5; i++)
    {
        if (1 << (i + 3) == delayUs)
        {
            return i;
        }
    }

    NS_ABORT_MSG("Value not allowed (" << delay.As(Time::US) << ")");
    return 0;
}

Time
CommonInfoBasicMle::DecodeEmlsrTransitionDelay(uint8_t value)
{
    NS_ABORT_MSG_IF(value > 5, "Value not allowed (" << +value << ")");
    if (value == 0)
    {
        return MicroSeconds(0);
    }
    return MicroSeconds(1 << (3 + value));
}

void
CommonInfoBasicMle::SetMediumSyncDelayTimer(Time delay)
{
    int64_t delayUs = delay.GetMicroSeconds();
    NS_ABORT_MSG_IF(delayUs % 32 != 0, "Delay must be a multiple of 32 microseconds");
    delayUs /= 32;

    if (!m_mediumSyncDelayInfo.has_value())
    {
        m_mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }
    m_mediumSyncDelayInfo->mediumSyncDuration = (delayUs & 0xff);
}

Time
CommonInfoBasicMle::GetMediumSyncDelayTimer() const
{
    NS_ASSERT(m_mediumSyncDelayInfo);
    return MicroSeconds(m_mediumSyncDelayInfo->mediumSyncDuration * 32);
}

void
CommonInfoBasicMle::SetMediumSyncOfdmEdThreshold(int8_t threshold)
{
    NS_ABORT_MSG_IF(threshold < -72 || threshold > -62, "Threshold may range from -72 to -62 dBm");
    uint8_t value = 72 + threshold;

    if (!m_mediumSyncDelayInfo.has_value())
    {
        m_mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }
    m_mediumSyncDelayInfo->mediumSyncOfdmEdThreshold = value;
}

int8_t
CommonInfoBasicMle::GetMediumSyncOfdmEdThreshold() const
{
    NS_ASSERT(m_mediumSyncDelayInfo);
    return (m_mediumSyncDelayInfo->mediumSyncOfdmEdThreshold) - 72;
}

void
CommonInfoBasicMle::SetMediumSyncMaxNTxops(uint8_t nTxops)
{
    NS_ASSERT_MSG(nTxops < 16, "Value " << +nTxops << "cannot be encoded in 4 bits");

    if (!m_mediumSyncDelayInfo.has_value())
    {
        m_mediumSyncDelayInfo = CommonInfoBasicMle::MediumSyncDelayInfo{};
    }

    if (nTxops == 0)
    {
        // no limit on max number of TXOPs
        m_mediumSyncDelayInfo->mediumSyncMaxNTxops = 15;
        return;
    }

    m_mediumSyncDelayInfo->mediumSyncMaxNTxops = --nTxops;
}

std::optional<uint8_t>
CommonInfoBasicMle::GetMediumSyncMaxNTxops() const
{
    NS_ASSERT(m_mediumSyncDelayInfo);
    uint8_t nTxops = m_mediumSyncDelayInfo->mediumSyncMaxNTxops;
    if (nTxops == 15)
    {
        return std::nullopt;
    }
    return nTxops + 1;
}

/**
 * MultiLinkElement
 */
MultiLinkElement::MultiLinkElement(ContainingFrame frame)
    : m_containingFrame(frame),
      m_commonInfo(std::in_place_type<std::monostate>) // initialize as UNSET
{
}

MultiLinkElement::MultiLinkElement(Variant variant, ContainingFrame frame)
    : MultiLinkElement(frame)
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

CommonInfoBasicMle&
MultiLinkElement::GetCommonInfoBasic()
{
    return std::get<BASIC_VARIANT>(m_commonInfo);
}

const CommonInfoBasicMle&
MultiLinkElement::GetCommonInfoBasic() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo);
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
MultiLinkElement::SetEmlsrSupported(bool supported)
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    if (!emlCapabilities.has_value())
    {
        emlCapabilities = CommonInfoBasicMle::EmlCapabilities{};
    }
    emlCapabilities->emlsrSupport = supported ? 1 : 0;
}

void
MultiLinkElement::SetEmlsrPaddingDelay(Time delay)
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    if (!emlCapabilities.has_value())
    {
        emlCapabilities = CommonInfoBasicMle::EmlCapabilities{};
    }
    emlCapabilities->emlsrPaddingDelay = CommonInfoBasicMle::EncodeEmlsrPaddingDelay(delay);
}

void
MultiLinkElement::SetEmlsrTransitionDelay(Time delay)
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    if (!emlCapabilities.has_value())
    {
        emlCapabilities = CommonInfoBasicMle::EmlCapabilities{};
    }
    emlCapabilities->emlsrTransitionDelay = CommonInfoBasicMle::EncodeEmlsrTransitionDelay(delay);
}

void
MultiLinkElement::SetTransitionTimeout(Time timeout)
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    if (!emlCapabilities.has_value())
    {
        emlCapabilities = CommonInfoBasicMle::EmlCapabilities{};
    }
    auto timeoutUs = timeout.GetMicroSeconds();

    if (timeoutUs == 0)
    {
        emlCapabilities->transitionTimeout = 0;
    }
    else
    {
        uint8_t i;
        for (i = 1; i <= 10; i++)
        {
            if (1 << (i + 6) == timeoutUs)
            {
                emlCapabilities->transitionTimeout = i;
                break;
            }
        }
        NS_ABORT_MSG_IF(i > 10, "Value not allowed (" << timeout.As(Time::US) << ")");
    }
}

bool
MultiLinkElement::HasEmlCapabilities() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities.has_value();
}

bool
MultiLinkElement::IsEmlsrSupported() const
{
    return std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities->emlsrSupport;
}

Time
MultiLinkElement::GetEmlsrPaddingDelay() const
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    NS_ASSERT(emlCapabilities);
    return CommonInfoBasicMle::DecodeEmlsrPaddingDelay(emlCapabilities->emlsrPaddingDelay);
}

Time
MultiLinkElement::GetEmlsrTransitionDelay() const
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    NS_ASSERT(emlCapabilities);
    return CommonInfoBasicMle::DecodeEmlsrTransitionDelay(emlCapabilities->emlsrTransitionDelay);
}

Time
MultiLinkElement::GetTransitionTimeout() const
{
    auto& emlCapabilities = std::get<BASIC_VARIANT>(m_commonInfo).m_emlCapabilities;
    NS_ASSERT(emlCapabilities);
    if (emlCapabilities->transitionTimeout == 0)
    {
        return MicroSeconds(0);
    }
    return MicroSeconds(1 << (6 + emlCapabilities->transitionTimeout));
}

MultiLinkElement::PerStaProfileSubelement::PerStaProfileSubelement(Variant variant)
    : m_variant(variant),
      m_staControl(0)
{
}

MultiLinkElement::PerStaProfileSubelement::PerStaProfileSubelement(
    const PerStaProfileSubelement& perStaProfile)
    : m_variant(perStaProfile.m_variant),
      m_staControl(perStaProfile.m_staControl),
      m_staMacAddress(perStaProfile.m_staMacAddress)
{
    // deep copy of the STA Profile field
    auto staProfileCopy = [&](auto&& frame) {
        using Ptr = std::decay_t<decltype(frame)>;
        if constexpr (std::is_same_v<Ptr, std::monostate>)
        {
            return;
        }
        else
        {
            using T = std::decay_t<decltype(*frame.get())>;
            m_staProfile = std::make_unique<T>(*frame.get());
        }
    };
    std::visit(staProfileCopy, perStaProfile.m_staProfile);
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
    m_staControl = perStaProfile.m_staControl;
    m_staMacAddress = perStaProfile.m_staMacAddress;

    // deep copy of the STA Profile field
    auto staProfileCopy = [&](auto&& frame) {
        using Ptr = std::decay_t<decltype(frame)>;
        if constexpr (std::is_same_v<Ptr, std::monostate>)
        {
            return;
        }
        else
        {
            using T = std::decay_t<decltype(*frame.get())>;
            m_staProfile = std::make_unique<T>(*frame.get());
        }
    };
    std::visit(staProfileCopy, perStaProfile.m_staProfile);

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
    std::visit(
        [&](auto&& frame) {
            m_staProfile = std::make_unique<std::decay_t<decltype(frame)>>(frame);
        },
        assoc);
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocRequest(
    std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>&& assoc)
{
    std::visit(
        [&](auto&& frame) {
            using T = std::decay_t<decltype(frame)>;
            m_staProfile = std::make_unique<T>(std::forward<T>(frame));
        },
        assoc);
}

bool
MultiLinkElement::PerStaProfileSubelement::HasAssocRequest() const
{
    return std::holds_alternative<std::unique_ptr<MgtAssocRequestHeader>>(m_staProfile);
}

bool
MultiLinkElement::PerStaProfileSubelement::HasReassocRequest() const
{
    return std::holds_alternative<std::unique_ptr<MgtReassocRequestHeader>>(m_staProfile);
}

AssocReqRefVariant
MultiLinkElement::PerStaProfileSubelement::GetAssocRequest() const
{
    if (HasAssocRequest())
    {
        return *std::get<std::unique_ptr<MgtAssocRequestHeader>>(m_staProfile);
    }
    NS_ABORT_UNLESS(HasReassocRequest());
    return *std::get<std::unique_ptr<MgtReassocRequestHeader>>(m_staProfile);
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocResponse(const MgtAssocResponseHeader& assoc)
{
    m_staProfile = std::make_unique<MgtAssocResponseHeader>(assoc);
}

void
MultiLinkElement::PerStaProfileSubelement::SetAssocResponse(MgtAssocResponseHeader&& assoc)
{
    m_staProfile = std::make_unique<MgtAssocResponseHeader>(std::move(assoc));
}

bool
MultiLinkElement::PerStaProfileSubelement::HasAssocResponse() const
{
    return std::holds_alternative<std::unique_ptr<MgtAssocResponseHeader>>(m_staProfile);
}

MgtAssocResponseHeader&
MultiLinkElement::PerStaProfileSubelement::GetAssocResponse() const
{
    NS_ABORT_IF(!HasAssocResponse());
    return *std::get<std::unique_ptr<MgtAssocResponseHeader>>(m_staProfile);
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

    auto staProfileSize = [&](auto&& frame) {
        using T = std::decay_t<decltype(frame)>;
        if constexpr (std::is_same_v<T, std::monostate>)
        {
            NS_ASSERT_MSG(std::holds_alternative<std::monostate>(m_containingFrame),
                          "Missing management frame for Per-STA Profile subelement");
            return static_cast<uint32_t>(0);
        }
        else
        {
            using U = std::decay_t<decltype(*frame)>;
            NS_ASSERT_MSG(
                std::holds_alternative<std::reference_wrapper<const U>>(m_containingFrame),
                "Containing frame type and frame type in Per-STA Profile do not match");
            const auto& containing = std::get<std::reference_wrapper<const U>>(m_containingFrame);
            return frame->GetSerializedSizeInPerStaProfile(containing);
        }
    };
    ret += std::visit(staProfileSize, m_staProfile);

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
    auto staProfileSerialize = [&](auto&& frame) {
        using T = std::decay_t<decltype(frame)>;
        if constexpr (std::is_same_v<T, std::monostate>)
        {
            NS_ASSERT_MSG(std::holds_alternative<std::monostate>(m_containingFrame),
                          "Missing management frame for Per-STA Profile subelement");
            return;
        }
        else
        {
            using U = std::decay_t<decltype(*frame)>;
            NS_ASSERT_MSG(
                std::holds_alternative<std::reference_wrapper<const U>>(m_containingFrame),
                "Containing frame type and frame type in Per-STA Profile do not match");
            const auto& containing = std::get<std::reference_wrapper<const U>>(m_containingFrame);
            frame->SerializeInPerStaProfile(start, containing);
        }
    };
    std::visit(staProfileSerialize, m_staProfile);
}

uint16_t
MultiLinkElement::PerStaProfileSubelement::DeserializeInformationField(Buffer::Iterator start,
                                                                       uint16_t length)
{
    Buffer::Iterator i = start;

    m_staControl = i.ReadLsbtohU16();
    i.ReadU8(); // STA Info Length

    if (HasStaMacAddress())
    {
        ReadFrom(i, m_staMacAddress);
    }

    // TODO add other subfields of the STA Info field
    uint16_t count = i.GetDistanceFrom(start);

    NS_ASSERT_MSG(count <= length,
                  "Bytes read (" << count << ") exceed expected number (" << length << ")");

    if (count == length)
    {
        return count;
    }

    auto staProfileDeserialize = [&](auto&& frame) {
        using T = std::decay_t<decltype(frame)>;
        if constexpr (!std::is_same_v<T, std::monostate>)
        {
            using U = std::decay_t<decltype(frame.get())>;
            U assoc;
            count += assoc.DeserializeFromPerStaProfile(i, length - count, frame.get());
            m_staProfile = std::make_unique<U>(std::move(assoc));
        }
    };
    std::visit(staProfileDeserialize, m_containingFrame);

    return count;
}

void
MultiLinkElement::AddPerStaProfileSubelement()
{
    auto variant = GetVariant();
    NS_ABORT_IF(variant == UNSET);
    m_perStaProfileSubelements.emplace_back(variant);
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
        subelement.m_containingFrame = m_containingFrame;
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
        case PER_STA_PROFILE_SUBELEMENT_ID: {
            AddPerStaProfileSubelement();
            auto& perStaProfile = GetPerStaProfile(GetNPerStaProfileSubelements() - 1);
            perStaProfile.m_containingFrame = m_containingFrame;
            i = perStaProfile.Deserialize(i);
            count = i.GetDistanceFrom(start);
        }
        break;
        default:
            NS_ABORT_MSG("Unsupported Subelement ID: " << +i.PeekU8());
        }
    }

    return count;
}

} // namespace ns3
