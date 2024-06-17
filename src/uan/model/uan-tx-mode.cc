/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */
#include "uan-tx-mode.h"

#include "ns3/log.h"

#include <map>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UanTxMode");

UanTxMode::UanTxMode()
{
}

UanTxMode::~UanTxMode()
{
}

UanTxMode::ModulationType
UanTxMode::GetModType() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_type;
}

uint32_t
UanTxMode::GetDataRateBps() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_dataRateBps;
}

uint32_t
UanTxMode::GetPhyRateSps() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_phyRateSps;
}

uint32_t
UanTxMode::GetCenterFreqHz() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_cfHz;
}

uint32_t
UanTxMode::GetBandwidthHz() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_bwHz;
}

uint32_t
UanTxMode::GetConstellationSize() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_constSize;
}

std::string
UanTxMode::GetName() const
{
    return UanTxModeFactory::GetFactory().GetModeItem(m_uid).m_name;
}

uint32_t
UanTxMode::GetUid() const
{
    return m_uid;
}

std::ostream&
operator<<(std::ostream& os, const UanTxMode& mode)
{
    os << mode.m_uid;
    return os;
}

std::istream&
operator>>(std::istream& is, UanTxMode& mode)
{
    std::string name;
    uint32_t duh;

    is >> duh;

    mode.m_uid = duh;
    return is;
}

UanTxModeFactory::UanTxModeFactory()
    : m_nextUid(0)
{
}

UanTxModeFactory::~UanTxModeFactory()
{
    m_modes.clear();
}

bool
UanTxModeFactory::NameUsed(std::string name)
{
    auto it = m_modes.begin();

    for (; it != m_modes.end(); it++)
    {
        if ((*it).second.m_name == name)
        {
            return true;
        }
    }
    return false;
}

UanTxMode
UanTxModeFactory::CreateMode(UanTxMode::ModulationType type,
                             uint32_t dataRateBps,
                             uint32_t phyRateSps,
                             uint32_t cfHz,
                             uint32_t bwHz,
                             uint32_t constSize,
                             std::string name)
{
    UanTxModeFactory& factory = UanTxModeFactory::GetFactory();

    UanTxModeItem* item;

    if (factory.NameUsed(name))
    {
        NS_LOG_WARN("Redefining UanTxMode with name \"" << name << "\"");
        item = &factory.GetModeItem(name);
    }
    else
    {
        item = &factory.m_modes[factory.m_nextUid];
        item->m_uid = factory.m_nextUid++;
    }

    item->m_type = type;
    item->m_dataRateBps = dataRateBps;
    item->m_phyRateSps = phyRateSps;
    item->m_cfHz = cfHz;
    item->m_bwHz = bwHz;
    item->m_constSize = constSize;
    item->m_name = name;
    return factory.MakeModeFromItem(*item);
}

UanTxModeFactory::UanTxModeItem&
UanTxModeFactory::GetModeItem(uint32_t uid)
{
    if (uid >= m_nextUid)
    {
        NS_FATAL_ERROR("Attempting to retrieve UanTxMode with uid, " << uid << ", >= m_nextUid");
    }

    return m_modes[uid];
}

UanTxModeFactory::UanTxModeItem&
UanTxModeFactory::GetModeItem(std::string name)
{
    auto it = m_modes.begin();
    for (; it != m_modes.end(); it++)
    {
        if ((*it).second.m_name == name)
        {
            return (*it).second;
        }
    }
    NS_FATAL_ERROR("Unknown mode, \"" << name << "\", requested from mode factory");
    return (*it).second; // dummy to get rid of warning
}

UanTxMode
UanTxModeFactory::GetMode(std::string name)
{
    UanTxModeFactory& factory = UanTxModeFactory::GetFactory();
    return factory.MakeModeFromItem(factory.GetModeItem(name));
}

UanTxMode
UanTxModeFactory::GetMode(uint32_t uid)
{
    UanTxModeFactory& factory = UanTxModeFactory::GetFactory();
    return factory.MakeModeFromItem(factory.GetModeItem(uid));
}

UanTxMode
UanTxModeFactory::MakeModeFromItem(const UanTxModeItem& item)
{
    UanTxMode mode;
    mode.m_uid = item.m_uid;
    return mode;
}

UanTxModeFactory&
UanTxModeFactory::GetFactory()
{
    static UanTxModeFactory factory;
    return factory;
}

UanModesList::UanModesList()
{
}

UanModesList::~UanModesList()
{
    m_modes.clear();
}

void
UanModesList::AppendMode(UanTxMode newMode)
{
    m_modes.push_back(newMode);
}

void
UanModesList::DeleteMode(uint32_t modeNum)
{
    NS_ASSERT(modeNum < m_modes.size());

    auto it = m_modes.begin();
    for (uint32_t i = 0; i < modeNum; i++)
    {
        it++;
    }
    it = m_modes.erase(it);
}

UanTxMode
UanModesList::operator[](uint32_t i) const
{
    NS_ASSERT(i < m_modes.size());
    return m_modes[i];
}

uint32_t
UanModesList::GetNModes() const
{
    return static_cast<uint32_t>(m_modes.size());
}

std::ostream&
operator<<(std::ostream& os, const UanModesList& ml)
{
    os << ml.GetNModes() << "|";

    for (uint32_t i = 0; i < ml.m_modes.size(); i++)
    {
        os << ml[i] << "|";
    }
    return os;
}

std::istream&
operator>>(std::istream& is, UanModesList& ml)
{
    char c = '\0';

    int numModes;

    is >> numModes >> c;
    if (c != '|')
    {
        is.setstate(std::ios_base::failbit);
        return is;
    }
    ml.m_modes.clear();
    ml.m_modes.resize(numModes);

    if (numModes == 0 && is.peek() == std::istream::traits_type::eof())
    {
        return is;
    }

    int i;
    for (i = 0; is.peek() != std::istream::traits_type::eof() && i < numModes; i++)
    {
        is >> ml.m_modes[i] >> c;
        if (c != '|')
        {
            is.setstate(std::ios_base::failbit);
            return is;
        }
    }
    if (i < numModes)
    {
        is.setstate(std::ios_base::failbit);
    }

    return is;
}

ATTRIBUTE_HELPER_CPP(UanModesList);

} // namespace ns3
