/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "supported-rates.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SupportedRates");

#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127
#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HE_PHY 122
#define BSS_MEMBERSHIP_SELECTOR_EHT_PHY 121 // TODO not defined yet as of 802.11be D1.4

SupportedRates::SupportedRates()
{
    NS_LOG_FUNCTION(this);
}

void
SupportedRates::Print(std::ostream& os) const
{
    os << "rates=[";
    for (std::size_t i = 0; i < m_rates.size(); i++)
    {
        if ((m_rates[i] & 0x80) > 0)
        {
            os << "*";
        }
        os << GetRate(i) / 1000000 << "mbs";
        if (i < m_rates.size() - 1)
        {
            os << " ";
        }
    }
    os << "]";
}

bool
AllSupportedRates::IsBasicRate(uint64_t bs) const
{
    NS_LOG_FUNCTION(this << bs);
    uint8_t rate = static_cast<uint8_t>(bs / 500000) | 0x80;
    return std::find(rates.m_rates.cbegin(), rates.m_rates.cend(), rate) != rates.m_rates.cend() ||
           (extendedRates &&
            std::find(extendedRates->m_rates.cbegin(), extendedRates->m_rates.cend(), rate) !=
                extendedRates->m_rates.cend());
}

void
AllSupportedRates::AddSupportedRate(uint64_t bs)
{
    NS_LOG_FUNCTION(this << bs);
    NS_ASSERT_MSG(IsBssMembershipSelectorRate(bs) == false, "Invalid rate");
    if (IsSupportedRate(bs))
    {
        return;
    }
    if (rates.m_rates.size() < 8)
    {
        rates.m_rates.emplace_back(static_cast<uint8_t>(bs / 500000));
    }
    else
    {
        if (!extendedRates)
        {
            extendedRates.emplace();
        }
        extendedRates->m_rates.emplace_back(static_cast<uint8_t>(bs / 500000));
    }
    NS_LOG_DEBUG("add rate=" << bs << ", n rates=" << +GetNRates());
}

void
AllSupportedRates::SetBasicRate(uint64_t bs)
{
    NS_LOG_FUNCTION(this << bs);
    NS_ASSERT_MSG(IsBssMembershipSelectorRate(bs) == false, "Invalid rate");
    auto rate = static_cast<uint8_t>(bs / 500000);
    for (uint8_t i = 0; i < GetNRates(); i++)
    {
        auto& currRate = i < 8 ? rates.m_rates[i] : extendedRates->m_rates[i - 8];
        if ((rate | 0x80) == currRate)
        {
            return;
        }
        if (rate == currRate)
        {
            NS_LOG_DEBUG("set basic rate=" << bs << ", n rates=" << +GetNRates());
            currRate |= 0x80;
            return;
        }
    }
    AddSupportedRate(bs);
    SetBasicRate(bs);
}

void
AllSupportedRates::AddBssMembershipSelectorRate(uint64_t bs)
{
    NS_LOG_FUNCTION(this << bs);
    NS_ASSERT_MSG(bs == BSS_MEMBERSHIP_SELECTOR_HT_PHY || bs == BSS_MEMBERSHIP_SELECTOR_VHT_PHY ||
                      bs == BSS_MEMBERSHIP_SELECTOR_HE_PHY || bs == BSS_MEMBERSHIP_SELECTOR_EHT_PHY,
                  "Value " << bs << " not a BSS Membership Selector");
    auto rate = static_cast<uint8_t>(bs / 500000);
    for (std::size_t i = 0; i < rates.m_rates.size(); i++)
    {
        if (rate == rates.m_rates[i])
        {
            return;
        }
    }
    if (extendedRates)
    {
        for (std::size_t i = 0; i < extendedRates->m_rates.size(); i++)
        {
            if (rate == extendedRates->m_rates[i])
            {
                return;
            }
        }
    }
    if (rates.m_rates.size() < 8)
    {
        rates.m_rates.emplace_back(rate);
    }
    else
    {
        if (!extendedRates)
        {
            extendedRates.emplace();
        }
        extendedRates->m_rates.emplace_back(rate);
    }
    NS_LOG_DEBUG("add BSS membership selector rate " << bs << " as rate " << +rate);
}

bool
AllSupportedRates::IsSupportedRate(uint64_t bs) const
{
    NS_LOG_FUNCTION(this << bs);
    auto rate = static_cast<uint8_t>(bs / 500000);
    for (std::size_t i = 0; i < rates.m_rates.size(); i++)
    {
        if (rate == rates.m_rates[i] || (rate | 0x80) == rates.m_rates[i])
        {
            return true;
        }
    }
    if (extendedRates)
    {
        for (std::size_t i = 0; i < extendedRates->m_rates.size(); i++)
        {
            if (rate == extendedRates->m_rates[i] || (rate | 0x80) == extendedRates->m_rates[i])
            {
                return true;
            }
        }
    }
    return false;
}

bool
AllSupportedRates::IsBssMembershipSelectorRate(uint64_t bs) const
{
    NS_LOG_FUNCTION(this << bs);
    return (bs & 0x7f) == BSS_MEMBERSHIP_SELECTOR_HT_PHY ||
           (bs & 0x7f) == BSS_MEMBERSHIP_SELECTOR_VHT_PHY ||
           (bs & 0x7f) == BSS_MEMBERSHIP_SELECTOR_HE_PHY ||
           (bs & 0x7f) == BSS_MEMBERSHIP_SELECTOR_EHT_PHY;
}

uint8_t
AllSupportedRates::GetNRates() const
{
    return rates.m_rates.size() + (extendedRates ? extendedRates->m_rates.size() : 0);
}

uint32_t
SupportedRates::GetRate(uint8_t i) const
{
    return (m_rates[i] & 0x7f) * 500000;
}

WifiInformationElementId
SupportedRates::ElementId() const
{
    return IE_SUPPORTED_RATES;
}

uint16_t
SupportedRates::GetInformationFieldSize() const
{
    return m_rates.size();
}

void
SupportedRates::SerializeInformationField(Buffer::Iterator start) const
{
    for (const uint8_t rate : m_rates)
    {
        start.WriteU8(rate);
    }
}

uint16_t
SupportedRates::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    NS_ASSERT(length <= 8);
    for (uint16_t i = 0; i < length; i++)
    {
        m_rates.push_back(start.ReadU8());
    }
    return length;
}

WifiInformationElementId
ExtendedSupportedRatesIE::ElementId() const
{
    return IE_EXTENDED_SUPPORTED_RATES;
}

} // namespace ns3
