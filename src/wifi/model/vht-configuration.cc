/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018  Sébastien Deronne
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

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "vht-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VhtConfiguration");

NS_OBJECT_ENSURE_REGISTERED (VhtConfiguration);

VhtConfiguration::VhtConfiguration ()
{
  NS_LOG_FUNCTION (this);
}

VhtConfiguration::~VhtConfiguration ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
VhtConfiguration::GetTypeId (void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::VhtConfiguration")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<VhtConfiguration> ()
    .AddAttribute ("VoMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VO access class. "
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_voMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11454))
    .AddAttribute ("ViMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VI access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_viMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11454))
    .AddAttribute ("BeMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BE access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_beMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11454))
    .AddAttribute ("BkMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BK access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_bkMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11454))
    .AddAttribute ("VoMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VO access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_voMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 1048575))
    .AddAttribute ("ViMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VI access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (1048575),
                   MakeUintegerAccessor (&VhtConfiguration::m_viMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 1048575))
    .AddAttribute ("BeMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BE access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (1048575),
                   MakeUintegerAccessor (&VhtConfiguration::m_beMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 1048575))
    .AddAttribute ("BkMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BK access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&VhtConfiguration::m_bkMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 1048575))
    ;
    return tid;
}

} //namespace ns3
