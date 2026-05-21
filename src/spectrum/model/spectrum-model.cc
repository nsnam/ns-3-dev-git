/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-model.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumModel");

bool
operator==(const SpectrumModel& lhs, const SpectrumModel& rhs)
{
    return (lhs.m_uid == rhs.m_uid);
}

SpectrumModelUid_t SpectrumModel::m_uidCount = 0;

SpectrumModel::SpectrumModel(const std::vector<double>& centerFreqs)
{
    NS_LOG_FUNCTION(this << centerFreqs);
    NS_ASSERT(centerFreqs.size() > 1);
    m_bands.reserve(centerFreqs.size());
    for (auto it = centerFreqs.begin(); it != centerFreqs.end(); ++it)
    {
        BandInfo e;
        e.fc = *it;
        if (it == centerFreqs.begin())
        {
            double delta = ((*(it + 1)) - (*it)) / 2;
            e.fl = *it - delta;
            e.fh = *it + delta;
        }
        else if (it == centerFreqs.end() - 1)
        {
            double delta = ((*it) - (*(it - 1))) / 2;
            e.fl = *it - delta;
            e.fh = *it + delta;
        }
        else
        {
            e.fl = ((*it) + (*(it - 1))) / 2;
            e.fh = ((*(it + 1)) + (*it)) / 2;
        }
        m_bands.push_back(e);
    }
    InitModel();
}

SpectrumModel::SpectrumModel(const Bands& bands)
{
    NS_LOG_FUNCTION(this);
    m_bands = bands;
    InitModel();
}

SpectrumModel::SpectrumModel(Bands&& bands)
    : m_bands(std::move(bands))
{
    NS_LOG_FUNCTION(this);
    InitModel();
}

void
SpectrumModel::InitModel()
{
    m_uid = ++m_uidCount;
    NS_LOG_INFO("creating new SpectrumModel, m_uid=" << m_uid);
    // sort bands by increasing frequency
    std::sort(m_bands.begin(), m_bands.end());
    // check if bands are contiguous, i.e. if the upper limit of a band is equal to the lower limit
    // of the next band
    m_contiguousBands = true;
    if (!m_bands.empty())
    {
        auto it = m_bands.cbegin();
        auto currentFh = it->fh;
        for (++it; it != m_bands.cend(); ++it)
        {
            if (it->fl > currentFh + std::numeric_limits<double>::epsilon())
            {
                m_contiguousBands = false;
                break;
            }
            if (it->fh > currentFh)
            {
                currentFh = it->fh;
            }
        }
    }
    // check if all bands have the same size
    const auto bandSize = m_bands.cbegin()->fh - m_bands.cbegin()->fl;
    m_uniqueBandSize =
        (m_bands.size() == 1) ||
        std::all_of(m_bands.cbegin(), m_bands.cend(), [bandSize](const auto& b) {
            return std::abs((b.fh - b.fl) - bandSize) <= std::numeric_limits<double>::epsilon();
        });
}

Bands::const_iterator
SpectrumModel::Begin() const
{
    return m_bands.begin();
}

Bands::const_iterator
SpectrumModel::End() const
{
    return m_bands.end();
}

size_t
SpectrumModel::GetNumBands() const
{
    return m_bands.size();
}

SpectrumModelUid_t
SpectrumModel::GetUid() const
{
    return m_uid;
}

bool
SpectrumModel::IsOrthogonal(const SpectrumModel& other) const
{
    if (m_contiguousBands && other.m_contiguousBands)
    {
        return (m_bands.rbegin()->fh <= other.m_bands.begin()->fl) ||
               (other.m_bands.rbegin()->fh <= m_bands.begin()->fl);
    }
    for (auto myIt = Begin(); myIt != End(); ++myIt)
    {
        for (auto otherIt = other.Begin(); otherIt != other.End(); ++otherIt)
        {
            if (!(myIt->fh <= otherIt->fl || otherIt->fh <= myIt->fl))
            {
                return false;
            }
        }
    }
    return true;
}

bool
SpectrumModel::IsAligned(const SpectrumModel& other) const
{
    if (m_uniqueBandSize && other.m_uniqueBandSize)
    {
        // same band size, only one band has to be checked
        const auto bandBandWidth = m_bands.begin()->fh - m_bands.begin()->fl;
        const auto otherBandWidth = other.m_bands.begin()->fh - other.m_bands.begin()->fl;
        return std::abs(bandBandWidth - otherBandWidth) <= std::numeric_limits<double>::epsilon();
    }
    auto myIt = Begin();
    auto otherIt = other.Begin();
    while (myIt != End() && otherIt != other.End())
    {
        while (otherIt != other.End() && otherIt->fh <= myIt->fl)
        {
            ++otherIt;
        }
        if (otherIt == other.End())
        {
            break;
        }
        if (std::abs(myIt->fl - otherIt->fl) > std::numeric_limits<double>::epsilon() ||
            std::abs(myIt->fh - otherIt->fh) > std::numeric_limits<double>::epsilon())
        {
            return false;
        }
        ++myIt;
        ++otherIt;
    }
    return true;
}

} // namespace ns3
