/*
 * Copyright (c) 2012 Telum (www.telum.ru)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@telum.ru>
 */

#include "jakes-propagation-loss-model.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Jakes");

NS_OBJECT_ENSURE_REGISTERED(JakesPropagationLossModel);

JakesPropagationLossModel::JakesPropagationLossModel()
{
    m_uniformVariable = CreateObject<UniformRandomVariable>();
    m_uniformVariable->SetAttribute("Min", DoubleValue(-1.0 * M_PI));
    m_uniformVariable->SetAttribute("Max", DoubleValue(M_PI));
}

JakesPropagationLossModel::~JakesPropagationLossModel()
{
}

TypeId
JakesPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::JakesPropagationLossModel")
                            .SetParent<PropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<JakesPropagationLossModel>();
    return tid;
}

void
JakesPropagationLossModel::DoDispose()
{
    m_uniformVariable = nullptr;
    m_propagationCache.Cleanup();
}

double
JakesPropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                         Ptr<MobilityModel> a,
                                         Ptr<MobilityModel> b) const
{
    Ptr<JakesProcess> pathData = m_propagationCache.GetPathData(
        a,
        b,
        0 /**Spectrum model uid is not used in PropagationLossModel*/);
    if (!pathData)
    {
        pathData = CreateObject<JakesProcess>();
        pathData->SetPropagationLossModel(this);
        m_propagationCache.AddPathData(
            pathData,
            a,
            b,
            0 /**Spectrum model uid is not used in PropagationLossModel*/);
    }
    return txPowerDbm + pathData->GetChannelGainDb();
}

Ptr<UniformRandomVariable>
JakesPropagationLossModel::GetUniformRandomVariable() const
{
    return m_uniformVariable;
}

int64_t
JakesPropagationLossModel::DoAssignStreams(int64_t stream)
{
    m_uniformVariable->SetStream(stream);
    return 1;
}

} // namespace ns3
