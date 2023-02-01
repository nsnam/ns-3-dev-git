/*
 * Copyright (c) 2011 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-signal-parameters.h"

#include "spectrum-phy.h"
#include "spectrum-value.h"

#include <ns3/antenna-model.h>
#include <ns3/log.h>

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
