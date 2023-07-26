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
 * Modified by Marco Miozzo <mmiozzo@cttc.es> (add data and ctrl diversity)
 */

#include "wifi-spectrum-signal-parameters.h"

#include "wifi-ppdu.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiSpectrumSignalParameters");

WifiSpectrumSignalParameters::WifiSpectrumSignalParameters()
    : SpectrumSignalParameters(),
      ppdu(nullptr)
{
    NS_LOG_FUNCTION(this);
}

WifiSpectrumSignalParameters::WifiSpectrumSignalParameters(const WifiSpectrumSignalParameters& p)
    : SpectrumSignalParameters(p),
      ppdu(p.ppdu)
{
    NS_LOG_FUNCTION(this << &p);
}

Ptr<SpectrumSignalParameters>
WifiSpectrumSignalParameters::Copy() const
{
    NS_LOG_FUNCTION(this);
    // Ideally we would use:
    //   return Copy<WifiSpectrumSignalParameters> (*this);
    // but for some reason it doesn't work. Another alternative is
    //   return Copy<WifiSpectrumSignalParameters> (this);
    // but it causes a double creation of the object, hence it is less efficient.
    // The solution below is copied from the implementation of Copy<> (Ptr<>) in ptr.h
    Ptr<WifiSpectrumSignalParameters> wssp(new WifiSpectrumSignalParameters(*this), false);
    return wssp;
}

} // namespace ns3
