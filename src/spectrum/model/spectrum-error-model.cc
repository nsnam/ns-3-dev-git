/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-error-model.h"

#include "ns3/log.h"
#include "ns3/nstime.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ShannonSpectrumErrorModel");

NS_OBJECT_ENSURE_REGISTERED(SpectrumErrorModel);

TypeId
SpectrumErrorModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SpectrumErrorModel").SetParent<Object>().SetGroupName("Spectrum")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

SpectrumErrorModel::~SpectrumErrorModel()
{
}

NS_OBJECT_ENSURE_REGISTERED(ShannonSpectrumErrorModel);

/* static */
TypeId
ShannonSpectrumErrorModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ShannonSpectrumErrorModel")
                            .SetParent<SpectrumErrorModel>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<ShannonSpectrumErrorModel>();
    return tid;
}

void
ShannonSpectrumErrorModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    SpectrumErrorModel::DoDispose();
}

void
ShannonSpectrumErrorModel::StartRx(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    m_bytes = p->GetSize();
    NS_LOG_LOGIC("bytes to deliver: " << m_bytes);
    m_deliverableBytes = 0;
}

void
ShannonSpectrumErrorModel::EvaluateChunk(const SpectrumValue& sinr, Time duration)
{
    NS_LOG_FUNCTION(this << sinr << duration);
    SpectrumValue CapacityPerHertz = Log2(1 + sinr);
    double capacity = 0;

    auto bi = CapacityPerHertz.ConstBandsBegin();
    auto vi = CapacityPerHertz.ConstValuesBegin();

    while (bi != CapacityPerHertz.ConstBandsEnd())
    {
        NS_ASSERT(vi != CapacityPerHertz.ConstValuesEnd());
        capacity += (bi->fh - bi->fl) * (*vi);
        ++bi;
        ++vi;
    }
    NS_ASSERT(vi == CapacityPerHertz.ConstValuesEnd());
    NS_LOG_LOGIC("ChunkCapacity = " << capacity);
    m_deliverableBytes += static_cast<uint32_t>(capacity * duration.GetSeconds() / 8);
    NS_LOG_LOGIC("DeliverableBytes = " << m_deliverableBytes);
}

bool
ShannonSpectrumErrorModel::IsRxCorrect()
{
    NS_LOG_FUNCTION(this);
    return (m_deliverableBytes > m_bytes);
}

} // namespace ns3
