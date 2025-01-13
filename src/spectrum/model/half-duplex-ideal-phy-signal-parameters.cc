/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "half-duplex-ideal-phy-signal-parameters.h"

#include "ns3/log.h"
#include "ns3/packet.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HalfDuplexIdealPhySignalParameters");

HalfDuplexIdealPhySignalParameters::HalfDuplexIdealPhySignalParameters()
{
    NS_LOG_FUNCTION(this);
}

HalfDuplexIdealPhySignalParameters::HalfDuplexIdealPhySignalParameters(
    const HalfDuplexIdealPhySignalParameters& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    data = p.data->Copy();
}

Ptr<SpectrumSignalParameters>
HalfDuplexIdealPhySignalParameters::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<HalfDuplexIdealPhySignalParameters>(*this);
}

} // namespace ns3
