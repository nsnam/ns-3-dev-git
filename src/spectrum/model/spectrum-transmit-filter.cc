/*
 * Copyright (c) 2022 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "spectrum-transmit-filter.h"

#include "spectrum-phy.h"
#include "spectrum-signal-parameters.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumTransmitFilter");

NS_OBJECT_ENSURE_REGISTERED(SpectrumTransmitFilter);

TypeId
SpectrumTransmitFilter::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SpectrumTransmitFilter").SetParent<Object>().SetGroupName("Spectrum");
    return tid;
}

SpectrumTransmitFilter::SpectrumTransmitFilter()
{
    NS_LOG_FUNCTION(this);
}

void
SpectrumTransmitFilter::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_next)
    {
        m_next->Dispose();
    }
    m_next = nullptr;
    Object::DoDispose();
}

void
SpectrumTransmitFilter::SetNext(Ptr<SpectrumTransmitFilter> next)
{
    m_next = next;
}

Ptr<const SpectrumTransmitFilter>
SpectrumTransmitFilter::GetNext() const
{
    return m_next;
}

bool
SpectrumTransmitFilter::Filter(Ptr<const SpectrumSignalParameters> params,
                               Ptr<const SpectrumPhy> receiverPhy)
{
    NS_LOG_FUNCTION(this << params << receiverPhy);
    bool result = DoFilter(params, receiverPhy);
    if (result)
    {
        return true;
    }
    else if (m_next)
    {
        return m_next->Filter(params, receiverPhy);
    }
    else
    {
        return false;
    }
}

int64_t
SpectrumTransmitFilter::AssignStreams(int64_t stream)
{
    auto currentStream = stream;
    currentStream += DoAssignStreams(stream);
    if (m_next)
    {
        currentStream += m_next->AssignStreams(currentStream);
    }
    return (currentStream - stream);
}

} // namespace ns3
