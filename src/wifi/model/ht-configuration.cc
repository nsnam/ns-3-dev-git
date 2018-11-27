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
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ht-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HtConfiguration");

NS_OBJECT_ENSURE_REGISTERED (HtConfiguration);

HtConfiguration::HtConfiguration ()
{
  NS_LOG_FUNCTION (this);
}

HtConfiguration::~HtConfiguration ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
HtConfiguration::GetTypeId (void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::HtConfiguration")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HtConfiguration> ()
    .AddAttribute ("ShortGuardIntervalSupported",
                   "Whether or not short guard interval is supported.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&HtConfiguration::GetShortGuardIntervalSupported,
                                        &HtConfiguration::SetShortGuardIntervalSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("GreenfieldSupported",
                   "Whether or not Greenfield is supported.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&HtConfiguration::GetGreenfieldSupported,
                                        &HtConfiguration::SetGreenfieldSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("RifsSupported",
                   "Whether or not RIFS is supported.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&HtConfiguration::SetRifsSupported,
                                        &HtConfiguration::GetRifsSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("VoMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VO access class. "
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_voMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 7935))
    .AddAttribute ("ViMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VI access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_viMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 7935))
    .AddAttribute ("BeMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BE access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_beMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 7935))
    .AddAttribute ("BkMaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BK access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_bkMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 7935))
    .AddAttribute ("VoMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VO access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_voMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("ViMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VI access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&HtConfiguration::m_viMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("BeMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BE access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&HtConfiguration::m_beMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("BkMaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BK access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HtConfiguration::m_bkMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    ;
    return tid;
}

void
HtConfiguration::SetShortGuardIntervalSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_sgiSupported = enable;
}

bool
HtConfiguration::GetShortGuardIntervalSupported (void) const
{
  return m_sgiSupported;
}

void
HtConfiguration::SetGreenfieldSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_greenfieldSupported = enable;
}

bool
HtConfiguration::GetGreenfieldSupported (void) const
{
  return m_greenfieldSupported;
}

void
HtConfiguration::SetRifsSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_rifsSupported = enable;
}

bool
HtConfiguration::GetRifsSupported (void) const
{
  return m_rifsSupported;
}

} //namespace ns3
