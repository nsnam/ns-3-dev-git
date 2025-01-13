/*
 * Copyright (c) 2012 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */
#include "lr-wpan-spectrum-signal-parameters.h"

#include "ns3/log.h"
#include "ns3/packet-burst.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("LrWpanSpectrumSignalParameters");

LrWpanSpectrumSignalParameters::LrWpanSpectrumSignalParameters()
{
    NS_LOG_FUNCTION(this);
}

LrWpanSpectrumSignalParameters::LrWpanSpectrumSignalParameters(
    const LrWpanSpectrumSignalParameters& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    packetBurst = p.packetBurst->Copy();
}

Ptr<SpectrumSignalParameters>
LrWpanSpectrumSignalParameters::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<LrWpanSpectrumSignalParameters>(*this);
}

} // namespace lrwpan
} // namespace ns3
