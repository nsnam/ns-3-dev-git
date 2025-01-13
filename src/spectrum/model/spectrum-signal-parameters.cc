/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-signal-parameters.h"

#include "spectrum-phy.h"
#include "spectrum-value.h"

#include "ns3/antenna-model.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumSignalParameters");

SpectrumSignalParameters::SpectrumSignalParameters()
{
    NS_LOG_FUNCTION(this);
}

SpectrumSignalParameters::~SpectrumSignalParameters()
{
    NS_LOG_FUNCTION(this);
}

SpectrumSignalParameters::SpectrumSignalParameters(const SpectrumSignalParameters& p)
{
    NS_LOG_FUNCTION(this << &p);
    psd = p.psd->Copy();
    duration = p.duration;
    txPhy = p.txPhy;
    txAntenna = p.txAntenna;
    spectrumChannelMatrix = p.spectrumChannelMatrix; // we do not need a deep copy, it will not be
                                                     // changed once is created
    precodingMatrix =
        p.precodingMatrix; // we do not need a deep copy, it will not be changed once is created
}

Ptr<SpectrumSignalParameters>
SpectrumSignalParameters::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<SpectrumSignalParameters>(*this);
}

} // namespace ns3
