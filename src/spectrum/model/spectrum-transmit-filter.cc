/*
 * Copyright (c) 2022 University of Washington
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

#include "spectrum-transmit-filter.h"

#include "spectrum-phy.h"
#include "spectrum-signal-parameters.h"

#include <ns3/log.h>

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

} // namespace ns3
