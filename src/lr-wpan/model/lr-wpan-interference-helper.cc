/*
 * Copyright (c) 2013 Fraunhofer FKIE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#include "lr-wpan-interference-helper.h"

#include "ns3/log.h"
#include "ns3/spectrum-model.h"
#include "ns3/spectrum-value.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("LrWpanInterferenceHelper");

LrWpanInterferenceHelper::LrWpanInterferenceHelper(Ptr<const SpectrumModel> spectrumModel)
    : m_spectrumModel(spectrumModel),
      m_dirty(false)
{
    m_signal = Create<SpectrumValue>(m_spectrumModel);
}

LrWpanInterferenceHelper::~LrWpanInterferenceHelper()
{
    m_spectrumModel = nullptr;
    m_signal = nullptr;
    m_signals.clear();
}

bool
LrWpanInterferenceHelper::AddSignal(Ptr<const SpectrumValue> signal)
{
    NS_LOG_FUNCTION(this << signal);

    bool result = false;

    if (signal->GetSpectrumModel() == m_spectrumModel)
    {
        result = m_signals.insert(signal).second;
        if (result && !m_dirty)
        {
            *m_signal += *signal;
        }
    }
    return result;
}

bool
LrWpanInterferenceHelper::RemoveSignal(Ptr<const SpectrumValue> signal)
{
    NS_LOG_FUNCTION(this << signal);

    bool result = false;

    if (signal->GetSpectrumModel() == m_spectrumModel)
    {
        result = (m_signals.erase(signal) == 1);
        if (result)
        {
            m_dirty = true;
        }
    }
    return result;
}

void
LrWpanInterferenceHelper::ClearSignals()
{
    NS_LOG_FUNCTION(this);

    m_signals.clear();
    m_dirty = true;
}

Ptr<SpectrumValue>
LrWpanInterferenceHelper::GetSignalPsd() const
{
    NS_LOG_FUNCTION(this);

    if (m_dirty)
    {
        // Sum up the current interference PSD.
        m_signal = Create<SpectrumValue>(m_spectrumModel);
        for (auto it = m_signals.begin(); it != m_signals.end(); ++it)
        {
            *m_signal += *(*it);
        }
        m_dirty = false;
    }

    return m_signal->Copy();
}

} // namespace lrwpan
} // namespace ns3
