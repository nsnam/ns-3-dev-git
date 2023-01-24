/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-configuration.h"

#include "ns3/boolean.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtConfiguration");

NS_OBJECT_ENSURE_REGISTERED(EhtConfiguration);

EhtConfiguration::EhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

EhtConfiguration::~EhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
EhtConfiguration::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::EhtConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<EhtConfiguration>()
            .AddAttribute("EmlsrActivated",
                          "Whether EMLSR option is activated. If activated, EMLSR mode can be "
                          "enabled on the EMLSR links by an installed EMLSR Manager.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          BooleanValue(false),
                          MakeBooleanAccessor(&EhtConfiguration::m_emlsrActivated),
                          MakeBooleanChecker())
            .AddAttribute("TransitionTimeout",
                          "The Transition Timeout (not used by non-AP MLDs). "
                          "Possible values are 0us or 2^n us, with n=7..16.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&EhtConfiguration::m_transitionTimeout),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(65536)));
    return tid;
}

} // namespace ns3
