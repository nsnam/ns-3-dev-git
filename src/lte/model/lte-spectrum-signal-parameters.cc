/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by Marco Miozzo <mmiozzo@cttc.es> (add data and ctrl diversity)
 */

#include "lte-spectrum-signal-parameters.h"

#include "lte-control-messages.h"

#include "ns3/log.h"
#include "ns3/packet-burst.h"
#include "ns3/ptr.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteSpectrumSignalParameters");

LteSpectrumSignalParameters::LteSpectrumSignalParameters()
{
    NS_LOG_FUNCTION(this);
}

LteSpectrumSignalParameters::LteSpectrumSignalParameters(const LteSpectrumSignalParameters& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    packetBurst = p.packetBurst->Copy();
}

Ptr<SpectrumSignalParameters>
LteSpectrumSignalParameters::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<LteSpectrumSignalParameters>(*this);
}

LteSpectrumSignalParametersDataFrame::LteSpectrumSignalParametersDataFrame()
{
    NS_LOG_FUNCTION(this);
}

LteSpectrumSignalParametersDataFrame::LteSpectrumSignalParametersDataFrame(
    const LteSpectrumSignalParametersDataFrame& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    cellId = p.cellId;
    if (p.packetBurst)
    {
        packetBurst = p.packetBurst->Copy();
    }
    ctrlMsgList = p.ctrlMsgList;
}

Ptr<SpectrumSignalParameters>
LteSpectrumSignalParametersDataFrame::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<LteSpectrumSignalParametersDataFrame>(*this);
}

LteSpectrumSignalParametersDlCtrlFrame::LteSpectrumSignalParametersDlCtrlFrame()
{
    NS_LOG_FUNCTION(this);
}

LteSpectrumSignalParametersDlCtrlFrame::LteSpectrumSignalParametersDlCtrlFrame(
    const LteSpectrumSignalParametersDlCtrlFrame& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    cellId = p.cellId;
    pss = p.pss;
    ctrlMsgList = p.ctrlMsgList;
}

Ptr<SpectrumSignalParameters>
LteSpectrumSignalParametersDlCtrlFrame::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<LteSpectrumSignalParametersDlCtrlFrame>(*this);
}

LteSpectrumSignalParametersUlSrsFrame::LteSpectrumSignalParametersUlSrsFrame()
{
    NS_LOG_FUNCTION(this);
}

LteSpectrumSignalParametersUlSrsFrame::LteSpectrumSignalParametersUlSrsFrame(
    const LteSpectrumSignalParametersUlSrsFrame& p)
    : SpectrumSignalParameters(p)
{
    NS_LOG_FUNCTION(this << &p);
    cellId = p.cellId;
}

Ptr<SpectrumSignalParameters>
LteSpectrumSignalParametersUlSrsFrame::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<LteSpectrumSignalParametersUlSrsFrame>(*this);
}

} // namespace ns3
