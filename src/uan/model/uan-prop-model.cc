/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-prop-model.h"

#include "ns3/nstime.h"

#include <complex>
#include <vector>

namespace ns3
{

std::ostream&
operator<<(std::ostream& os, const UanPdp& pdp)
{
    os << pdp.GetNTaps() << '|';
    os << pdp.GetResolution().GetSeconds() << '|';

    auto it = pdp.m_taps.begin();
    for (; it != pdp.m_taps.end(); it++)
    {
        os << (*it).GetAmp() << '|';
    }
    return os;
}

std::istream&
operator>>(std::istream& is, UanPdp& pdp)
{
    uint32_t ntaps;
    double resolution;
    char c1;

    is >> ntaps >> c1;
    if (c1 != '|')
    {
        NS_FATAL_ERROR("UanPdp data corrupted at # of taps");
        return is;
    }
    is >> resolution >> c1;
    if (c1 != '|')
    {
        NS_FATAL_ERROR("UanPdp data corrupted at resolution");
        return is;
    }
    pdp.m_resolution = Seconds(resolution);

    std::complex<double> amp;
    pdp.m_taps = std::vector<Tap>(ntaps);
    for (uint32_t i = 0; i < ntaps && !is.eof(); i++)
    {
        is >> amp >> c1;
        if (c1 != '|')
        {
            NS_FATAL_ERROR("UanPdp data corrupted at tap " << i);
            return is;
        }
        pdp.m_taps[i] = Tap(Seconds(resolution * i), amp);
    }
    return is;
}

Tap::Tap()
    : m_amplitude(0.0),
      m_delay()
{
}

Tap::Tap(Time delay, std::complex<double> amp)
    : m_amplitude(amp),
      m_delay(delay)
{
}

std::complex<double>
Tap::GetAmp() const
{
    return m_amplitude;
}

Time
Tap::GetDelay() const
{
    return m_delay;
}

UanPdp::UanPdp()
{
}

UanPdp::UanPdp(std::vector<Tap> taps, Time resolution)
    : m_taps(taps),
      m_resolution(resolution)
{
}

UanPdp::UanPdp(std::vector<std::complex<double>> amps, Time resolution)
    : m_resolution(resolution)
{
    m_taps.resize(amps.size());
    Time arrTime;
    for (uint32_t index = 0; index < amps.size(); index++)
    {
        m_taps[index] = Tap(arrTime, amps[index]);
        arrTime = arrTime + m_resolution;
    }
}

UanPdp::UanPdp(std::vector<double> amps, Time resolution)
    : m_resolution(resolution)
{
    m_taps.resize(amps.size());
    Time arrTime;
    for (uint32_t index = 0; index < amps.size(); index++)
    {
        m_taps[index] = Tap(arrTime, amps[index]);
        arrTime = arrTime + m_resolution;
    }
}

UanPdp::~UanPdp()
{
    m_taps.clear();
}

void
UanPdp::SetTap(std::complex<double> amp, uint32_t index)
{
    if (m_taps.size() <= index)
    {
        m_taps.resize(index + 1);
    }

    Time delay = index * m_resolution;
    m_taps[index] = Tap(delay, amp);
}

const Tap&
UanPdp::GetTap(uint32_t i) const
{
    NS_ASSERT_MSG(i < GetNTaps(), "Call to UanPdp::GetTap with requested tap out of range");
    return m_taps[i];
}

void
UanPdp::SetNTaps(uint32_t nTaps)
{
    m_taps.resize(nTaps);
}

void
UanPdp::SetResolution(Time resolution)
{
    m_resolution = resolution;
}

UanPdp::Iterator
UanPdp::GetBegin() const
{
    return m_taps.begin();
}

UanPdp::Iterator
UanPdp::GetEnd() const
{
    return m_taps.end();
}

uint32_t
UanPdp::GetNTaps() const
{
    return static_cast<uint32_t>(m_taps.size());
}

Time
UanPdp::GetResolution() const
{
    return m_resolution;
}

std::complex<double>
UanPdp::SumTapsFromMaxC(Time delay, Time duration) const
{
    if (m_resolution.IsNegative())
    {
        NS_ASSERT_MSG(GetNTaps() == 1,
                      "Attempted to sum taps over time interval in "
                      "UanPdp with resolution 0 and multiple taps");

        if (delay.IsZero())
        {
            return m_taps[0].GetAmp();
        }
        return std::complex<double>(0.0, 0.0);
    }

    uint32_t numTaps = (duration / m_resolution + 0.5).GetHigh();
    double maxAmp = -1;
    uint32_t maxTapIndex = 0;

    for (uint32_t i = 0; i < GetNTaps(); i++)
    {
        if (std::abs(m_taps[i].GetAmp()) > maxAmp)
        {
            maxAmp = std::abs(m_taps[i].GetAmp());
            maxTapIndex = i;
        }
    }
    uint32_t start = maxTapIndex + (delay / m_resolution).GetHigh();
    uint32_t end = std::min(start + numTaps, GetNTaps());
    std::complex<double> sum = 0;
    for (uint32_t i = start; i < end; i++)
    {
        sum += m_taps[i].GetAmp();
    }
    return sum;
}

double
UanPdp::SumTapsFromMaxNc(Time delay, Time duration) const
{
    if (m_resolution.IsNegative())
    {
        NS_ASSERT_MSG(GetNTaps() == 1,
                      "Attempted to sum taps over time interval in "
                      "UanPdp with resolution 0 and multiple taps");

        if (delay.IsZero())
        {
            return std::abs(m_taps[0].GetAmp());
        }
        return 0;
    }

    uint32_t numTaps = (duration / m_resolution + 0.5).GetHigh();
    double maxAmp = -1;
    uint32_t maxTapIndex = 0;

    for (uint32_t i = 0; i < GetNTaps(); i++)
    {
        if (std::abs(m_taps[i].GetAmp()) > maxAmp)
        {
            maxAmp = std::abs(m_taps[i].GetAmp());
            maxTapIndex = i;
        }
    }

    uint32_t start = maxTapIndex + (delay / m_resolution).GetHigh();
    uint32_t end = std::min(start + numTaps, GetNTaps());
    double sum = 0;
    for (uint32_t i = start; i < end; i++)

    {
        sum += std::abs(m_taps[i].GetAmp());
    }
    return sum;
}

double
UanPdp::SumTapsNc(Time begin, Time end) const
{
    if (m_resolution.IsNegative())
    {
        NS_ASSERT_MSG(GetNTaps() == 1,
                      "Attempted to sum taps over time interval in "
                      "UanPdp with resolution 0 and multiple taps");

        if (begin.IsNegative() && end.IsPositive())
        {
            return std::abs(m_taps[0].GetAmp());
        }
        else
        {
            return 0.0;
        }
    }

    uint32_t stIndex = (begin / m_resolution + 0.5).GetHigh();
    uint32_t endIndex = (end / m_resolution + 0.5).GetHigh();

    endIndex = std::min(endIndex, GetNTaps());
    double sum = 0;
    for (uint32_t i = stIndex; i < endIndex; i++)
    {
        sum += std::abs(m_taps[i].GetAmp());
    }
    return sum;
}

std::complex<double>
UanPdp::SumTapsC(Time begin, Time end) const
{
    if (m_resolution.IsNegative())
    {
        NS_ASSERT_MSG(GetNTaps() == 1,
                      "Attempted to sum taps over time interval in "
                      "UanPdp with resolution 0 and multiple taps");

        if (begin.IsNegative() && end.IsPositive())
        {
            return m_taps[0].GetAmp();
        }
        else
        {
            return std::complex<double>(0.0);
        }
    }

    uint32_t stIndex = (begin / m_resolution + 0.5).GetHigh();
    uint32_t endIndex = (end / m_resolution + 0.5).GetHigh();

    endIndex = std::min(endIndex, GetNTaps());

    std::complex<double> sum = 0;
    for (uint32_t i = stIndex; i < endIndex; i++)
    {
        sum += m_taps[i].GetAmp();
    }
    return sum;
}

UanPdp
UanPdp::NormalizeToSumNc() const
{
    double sumNc = 0.0;
    std::vector<Tap> newTaps;
    newTaps.reserve(GetNTaps());

    for (uint32_t i = 0; i < GetNTaps(); i++)
    {
        sumNc += std::abs(m_taps[i].GetAmp());
    }

    for (uint32_t i = 0; i < GetNTaps(); i++)
    {
        newTaps.emplace_back(m_taps[i].GetDelay(), (m_taps[i].GetAmp() / sumNc));
    }

    return UanPdp(newTaps, m_resolution);
}

UanPdp
UanPdp::CreateImpulsePdp()
{
    UanPdp pdp;
    pdp.SetResolution(Seconds(0));
    pdp.SetTap(1.0, 0);
    return pdp;
}

NS_OBJECT_ENSURE_REGISTERED(UanPropModel);

TypeId
UanPropModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanPropModel").SetParent<Object>().SetGroupName("Uan");
    return tid;
}

void
UanPropModel::Clear()
{
}

void
UanPropModel::DoDispose()
{
    Clear();
    Object::DoDispose();
}

} // namespace ns3
