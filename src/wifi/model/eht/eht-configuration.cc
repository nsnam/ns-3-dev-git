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
    static ns3::TypeId tid = ns3::TypeId("ns3::EhtConfiguration")
                                 .SetParent<Object>()
                                 .SetGroupName("Wifi")
                                 .AddConstructor<EhtConfiguration>();
    return tid;
}

} // namespace ns3
