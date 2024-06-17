/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "constant-spectrum-propagation-loss.h"

#include "spectrum-signal-parameters.h"

#include "ns3/double.h"
#include "ns3/log.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ConstantSpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(ConstantSpectrumPropagationLossModel);

ConstantSpectrumPropagationLossModel::ConstantSpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

ConstantSpectrumPropagationLossModel::~ConstantSpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

TypeId
ConstantSpectrumPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ConstantSpectrumPropagationLossModel")
            .SetParent<SpectrumPropagationLossModel>()
            .SetGroupName("Spectrum")
            .AddConstructor<ConstantSpectrumPropagationLossModel>()
            .AddAttribute("Loss",
                          "Path loss (dB) between transmitter and receiver",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&ConstantSpectrumPropagationLossModel::SetLossDb,
                                             &ConstantSpectrumPropagationLossModel::GetLossDb),
                          MakeDoubleChecker<double>());
    return tid;
}

void
ConstantSpectrumPropagationLossModel::SetLossDb(double lossDb)
{
    NS_LOG_FUNCTION(this);
    m_lossDb = lossDb;
    m_lossLinear = std::pow(10, m_lossDb / 10);
}

double
ConstantSpectrumPropagationLossModel::GetLossDb() const
{
    NS_LOG_FUNCTION(this);
    return m_lossDb;
}

Ptr<SpectrumValue>
ConstantSpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity(
    Ptr<const SpectrumSignalParameters> params,
    Ptr<const MobilityModel> a,
    Ptr<const MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    Ptr<SpectrumValue> rxPsd = Copy<SpectrumValue>(params->psd);
    auto vit = rxPsd->ValuesBegin();
    auto fit = rxPsd->ConstBandsBegin();

    while (vit != rxPsd->ValuesEnd())
    {
        NS_ASSERT(fit != rxPsd->ConstBandsEnd());
        NS_LOG_LOGIC("Ptx = " << *vit);
        *vit /= m_lossLinear; // Prx = Ptx / loss
        NS_LOG_LOGIC("Prx = " << *vit);
        ++vit;
        ++fit;
    }
    return rxPsd;
}

int64_t
ConstantSpectrumPropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
