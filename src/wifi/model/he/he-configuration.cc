/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Washington
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

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "he-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HeConfiguration");
NS_OBJECT_ENSURE_REGISTERED (HeConfiguration);

HeConfiguration::HeConfiguration ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
HeConfiguration::GetTypeId (void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::HeConfiguration")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HeConfiguration> ()
    .AddAttribute ("GuardInterval",
                   "Specify the shortest guard interval duration that can be used for HE transmissions."
                   "Possible values are 800ns, 1600ns or 3200ns.",
                   TimeValue (NanoSeconds (3200)),
                   MakeTimeAccessor (&HeConfiguration::GetGuardInterval,
                                     &HeConfiguration::SetGuardInterval),
                   MakeTimeChecker (NanoSeconds (800), NanoSeconds (3200)))
    .AddAttribute ("BssColor",
                   "The BSS color",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HeConfiguration::m_bssColor),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("MpduBufferSize",
                   "The MPDU buffer size for receiving A-MPDUs",
                   UintegerValue (64),
                   MakeUintegerAccessor (&HeConfiguration::GetMpduBufferSize,
                                         &HeConfiguration::SetMpduBufferSize),
                   MakeUintegerChecker<uint16_t> (64, 256))
    ;
    return tid;
}

void
HeConfiguration::SetGuardInterval (Time guardInterval)
{
  NS_LOG_FUNCTION (this << guardInterval);
  NS_ASSERT (guardInterval == NanoSeconds (800) || guardInterval == NanoSeconds (1600) || guardInterval == NanoSeconds (3200));
  m_guardInterval = guardInterval;
}

Time
HeConfiguration::GetGuardInterval (void) const
{
  return m_guardInterval;
}

void
HeConfiguration::SetMpduBufferSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_mpduBufferSize = size;
}

uint16_t
HeConfiguration::GetMpduBufferSize (void) const
{
  return m_mpduBufferSize;
}

} //namespace ns3
