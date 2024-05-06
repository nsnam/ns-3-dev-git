/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "common-info-basic-mle.h"

#include "ns3/address-utils.h"

namespace ns3
{

uint16_t
CommonInfoBasicMle::GetPresenceBitmap() const
{
    // see Sec. 9.4.2.312.2.1 of 802.11be D1.5
    return (m_linkIdInfo.has_value() ? 0x0001 : 0x0) |
           (m_bssParamsChangeCount.has_value() ? 0x0002 : 0x0) |
           (m_mediumSyncDelayInfo.has_value() ? 0x0004 : 0x0) |
           (m_emlCapabilities.has_value() ? 0x0008 : 0x0) |
           (m_mldCapabilities.has_value() ? 0x0010 : 0x0) | (m_apMldId.has_value() ? 0x0020 : 0x0) |
           (m_extMldCapabilities.has_value() ? 0x0040 : 0x0);
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
    ret += (m_apMldId.has_value() ? 1 : 0);
    ret += (m_extMldCapabilities.has_value() ? 2 : 0);
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
    if (m_apMldId.has_value())
    {
        start.WriteU8(*m_apMldId);
    }
    if (m_extMldCapabilities.has_value())
    {
        uint16_t val = m_extMldCapabilities->opParamUpdateSupp |
                       (m_extMldCapabilities->recommMaxSimulLinks << 1) |
                       (m_extMldCapabilities->nstrStatusUpdateSupp << 5);
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
    if ((presence & 0x0020) != 0)
    {
        m_apMldId = i.ReadU8();
        count++;
    }
    if ((presence & 0x0040) != 0)
    {
        m_extMldCapabilities = ExtMldCapabilities();
        auto val = i.ReadLsbtohU16();
        m_extMldCapabilities->opParamUpdateSupp = val & 0x0001;
        m_extMldCapabilities->recommMaxSimulLinks = (val >> 1) & 0x000f;
        m_extMldCapabilities->nstrStatusUpdateSupp = (val >> 5) & 0x0001;
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

} // namespace ns3
