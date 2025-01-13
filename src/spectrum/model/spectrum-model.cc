
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

#include <cmath>
#include <cstddef>

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
    NS_ASSERT(centerFreqs.size() > 1);
    m_uid = ++m_uidCount;

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
}

SpectrumModel::SpectrumModel(const Bands& bands)
{
    m_uid = ++m_uidCount;
    NS_LOG_INFO("creating new SpectrumModel, m_uid=" << m_uid);
    m_bands = bands;
}

SpectrumModel::SpectrumModel(Bands&& bands)
    : m_bands(std::move(bands))
{
    m_uid = ++m_uidCount;
    NS_LOG_INFO("creating new SpectrumModel, m_uid=" << m_uid);
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
    for (auto myIt = Begin(); myIt != End(); ++myIt)
    {
        for (auto otherIt = other.Begin(); otherIt != other.End(); ++otherIt)
        {
            if (std::max(myIt->fl, otherIt->fl) < std::min(myIt->fh, otherIt->fh))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace ns3
