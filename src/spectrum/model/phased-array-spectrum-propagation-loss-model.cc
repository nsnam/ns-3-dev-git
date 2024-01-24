/*
 * Copyright (c) 2021 CTTC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "phased-array-spectrum-propagation-loss-model.h"

#include "spectrum-signal-parameters.h"

#include <ns3/log.h>
#include <ns3/phased-array-model.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PhasedArraySpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(PhasedArraySpectrumPropagationLossModel);

PhasedArraySpectrumPropagationLossModel::PhasedArraySpectrumPropagationLossModel()
    : m_next(nullptr)
{
}

PhasedArraySpectrumPropagationLossModel::~PhasedArraySpectrumPropagationLossModel()
{
}

void
PhasedArraySpectrumPropagationLossModel::DoDispose()
{
    m_next = nullptr;
}

TypeId
PhasedArraySpectrumPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PhasedArraySpectrumPropagationLossModel")
                            .SetParent<Object>()
                            .SetGroupName("Spectrum");
    return tid;
}

void
PhasedArraySpectrumPropagationLossModel::SetNext(Ptr<PhasedArraySpectrumPropagationLossModel> next)
{
    m_next = next;
}

Ptr<PhasedArraySpectrumPropagationLossModel>
PhasedArraySpectrumPropagationLossModel::GetNext() const
{
    return m_next;
}

Ptr<SpectrumSignalParameters>
PhasedArraySpectrumPropagationLossModel::CalcRxPowerSpectralDensity(
    Ptr<const SpectrumSignalParameters> params,
    Ptr<const MobilityModel> a,
    Ptr<const MobilityModel> b,
    Ptr<const PhasedArrayModel> aPhasedArrayModel,
    Ptr<const PhasedArrayModel> bPhasedArrayModel) const
{
    // Here we assume that all the models in the chain of models are of type
    // PhasedArraySpectrumPropagationLossModel that provides the implementation of
    // this function, i.e. has phased array model of TX and RX as parameters
    auto rxParams =
        DoCalcRxPowerSpectralDensity(params, a, b, aPhasedArrayModel, bPhasedArrayModel);

    if (m_next)
    {
        rxParams =
            m_next->CalcRxPowerSpectralDensity(params, a, b, aPhasedArrayModel, bPhasedArrayModel);
    }
    return rxParams;
}

int64_t
PhasedArraySpectrumPropagationLossModel::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    currentStream += DoAssignStreams(stream);
    if (m_next)
    {
        currentStream += m_next->AssignStreams(currentStream);
    }
    return (currentStream - stream);
}

} // namespace ns3
