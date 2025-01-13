/*
 * Copyright (c) 2010 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by : Marco Miozzo <mmiozzo@cttc.es>
 *        (move from CQI to Ctrl and Data SINR Chunk processors
 */

#include "lte-chunk-processor.h"

#include "ns3/log.h"
#include "ns3/spectrum-value.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteChunkProcessor");

LteChunkProcessor::LteChunkProcessor()
{
    NS_LOG_FUNCTION(this);
}

LteChunkProcessor::~LteChunkProcessor()
{
    NS_LOG_FUNCTION(this);
}

void
LteChunkProcessor::AddCallback(LteChunkProcessorCallback c)
{
    NS_LOG_FUNCTION(this);
    m_lteChunkProcessorCallbacks.push_back(c);
}

void
LteChunkProcessor::Start()
{
    NS_LOG_FUNCTION(this);
    m_sumValues = nullptr;
    m_totDuration = MicroSeconds(0);
}

void
LteChunkProcessor::EvaluateChunk(const SpectrumValue& sinr, Time duration)
{
    NS_LOG_FUNCTION(this << sinr << duration);
    if (!m_sumValues)
    {
        m_sumValues = Create<SpectrumValue>(sinr.GetSpectrumModel());
    }
    (*m_sumValues) += sinr * duration.GetSeconds();
    m_totDuration += duration;
}

void
LteChunkProcessor::End()
{
    NS_LOG_FUNCTION(this);
    if (m_totDuration.GetSeconds() > 0)
    {
        for (auto it = m_lteChunkProcessorCallbacks.begin();
             it != m_lteChunkProcessorCallbacks.end();
             it++)
        {
            (*it)((*m_sumValues) / m_totDuration.GetSeconds());
        }
    }
    else
    {
        NS_LOG_WARN("m_numSinr == 0");
    }
}

void
LteSpectrumValueCatcher::ReportValue(const SpectrumValue& value)
{
    m_value = value.Copy();
}

Ptr<SpectrumValue>
LteSpectrumValueCatcher::GetValue()
{
    return m_value;
}

} // namespace ns3
