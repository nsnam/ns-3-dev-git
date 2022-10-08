/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 Dalian University of Technology
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Author: Junling Bu <linlinjavaer@gmail.com>
 */
#include "wave-mac-helper.h"

#include "ns3/boolean.h"

namespace ns3
{

NqosWaveMacHelper::NqosWaveMacHelper()
{
}

NqosWaveMacHelper::~NqosWaveMacHelper()
{
}

NqosWaveMacHelper
NqosWaveMacHelper::Default()
{
    NqosWaveMacHelper helper;
    // We're making non QoS-enabled Wi-Fi MACs here, so we set the
    // necessary attribute. I've carefully positioned this here so that
    // someone who knows what they're doing can override with explicit
    // attributes.
    helper.SetType("ns3::OcbWifiMac", "QosSupported", BooleanValue(false));
    return helper;
}

/**********  QosWifi80211pMacHelper *********/
QosWaveMacHelper::QosWaveMacHelper()
{
}

QosWaveMacHelper::~QosWaveMacHelper()
{
}

QosWaveMacHelper
QosWaveMacHelper::Default()
{
    QosWaveMacHelper helper;

    // We're making QoS-enabled Wi-Fi MACs here, so we set the necessary
    // attribute. I've carefully positioned this here so that someone
    // who knows what they're doing can override with explicit
    // attributes.
    helper.SetType("ns3::OcbWifiMac", "QosSupported", BooleanValue(true));

    return helper;
}
} // namespace ns3
