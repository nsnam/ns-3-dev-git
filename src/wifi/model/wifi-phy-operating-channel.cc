/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021
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
 * Authors: Stefano Avallone <stavallo@unina.it>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/log.h"
#include "wifi-phy.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhyOperatingChannel");

WifiPhyOperatingChannel::WifiPhyOperatingChannel ()
  : m_channelIt (WifiPhy::m_frequencyChannels.end ()),
    m_primary20Index (0)
{
  NS_LOG_FUNCTION (this);
}

WifiPhyOperatingChannel::~WifiPhyOperatingChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
WifiPhyOperatingChannel::IsSet (void) const
{
  return m_channelIt != WifiPhy::m_frequencyChannels.end ();
}

void
WifiPhyOperatingChannel::Set (uint8_t number, uint16_t frequency, uint16_t width,
                              WifiPhyStandard standard, WifiPhyBand band)
{
  NS_LOG_FUNCTION (this << +number << frequency << width << standard << band);

  auto channelIt = FindFirst (number, frequency, width, standard, band, WifiPhy::m_frequencyChannels.begin ());

  if (channelIt != WifiPhy::m_frequencyChannels.end ()
      && FindFirst (number, frequency, width, standard, band, std::next (channelIt))
         == WifiPhy::m_frequencyChannels.end ())
    {
      // a unique channel matches the specified criteria
      m_channelIt = channelIt;
      m_primary20Index = 0;
      return;
    }

  // if a unique channel was not found, throw an exception (mainly for unit testing this code)
  throw std::runtime_error ("WifiPhyOperatingChannel: No unique channel found given the specified criteria");
}

void
WifiPhyOperatingChannel::SetDefault (uint16_t width, WifiPhyStandard standard, WifiPhyBand band)
{
  NS_LOG_FUNCTION (this << width << standard << band);

  auto channelIt = FindFirst (0, 0, width, standard, band, WifiPhy::m_frequencyChannels.begin ());

  if (channelIt != WifiPhy::m_frequencyChannels.end ())
    {
      // a channel matches the specified criteria
      m_channelIt = channelIt;
      m_primary20Index = 0;
      return;
    }

  // if a default channel was not found, throw an exception (mainly for unit testing this code)
  throw std::runtime_error ("WifiPhyOperatingChannel: No default channel found of the given width and for the given PHY standard and band");
}

WifiPhyOperatingChannel::ConstIterator
WifiPhyOperatingChannel::FindFirst (uint8_t number, uint16_t frequency, uint16_t width,
                                    WifiPhyStandard standard, WifiPhyBand band,
                                    ConstIterator start) const
{
  NS_LOG_FUNCTION (this << +number << frequency << width << standard << band);

  // lambda used to match channels against the specified criteria
  auto predicate = [&](const FrequencyChannelInfo& channel)
    {
      if (number != 0 && std::get<0> (channel) != number)
        {
          return false;
        }
      if (frequency != 0 && std::get<1> (channel) != frequency)
        {
          return false;
        }
      if (width > GetMaximumChannelWidth (standard))
        {
          return false;
        }
      if (width != 0 && std::get<2> (channel) != width)
        {
          return false;
        }
      if (std::get<3> (channel) != GetFrequencyChannelType (standard))
        {
          return false;
        }
      if (std::get<4> (channel) != band)
        {
          return false;
        }
      return true;
    };

  return std::find_if (start, WifiPhy::m_frequencyChannels.end (), predicate);
}

uint8_t
WifiPhyOperatingChannel::GetNumber (void) const
{
  NS_ASSERT (IsSet ());
  return std::get<0> (*m_channelIt);
}

uint16_t
WifiPhyOperatingChannel::GetFrequency (void) const
{
  NS_ASSERT (IsSet ());
  return std::get<1> (*m_channelIt);
}

uint16_t
WifiPhyOperatingChannel::GetWidth (void) const
{
  NS_ASSERT (IsSet ());
  return std::get<2> (*m_channelIt);
}

uint8_t
WifiPhyOperatingChannel::GetPrimaryChannelIndex (uint16_t primaryChannelWidth) const
{
  NS_LOG_FUNCTION (this << primaryChannelWidth);

  if (primaryChannelWidth % 20 != 0)
    {
      NS_LOG_DEBUG ("The operating channel width is not a multiple of 20 MHz; return 0");
      return 0;
    }

  NS_ASSERT (primaryChannelWidth <= GetWidth ());

  // the index of primary40 is half the index of primary20; the index of
  // primary80 is half the index of primary40, ...
  uint16_t width = 20;
  uint8_t index = m_primary20Index;

  while (width < primaryChannelWidth)
    {
      index /= 2;
      width *= 2;
    }
  NS_LOG_LOGIC ("Return " << +index);
  return index;
}

void
WifiPhyOperatingChannel::SetPrimary20Index (uint8_t index)
{
  NS_LOG_FUNCTION (this << +index);

  NS_ABORT_MSG_IF (index > 0 && index >= GetWidth () / 20, "Primary20 index out of range");
  m_primary20Index = index;
}

uint16_t
WifiPhyOperatingChannel::GetPrimaryChannelCenterFrequency (uint16_t primaryChannelWidth) const
{
  uint16_t freq = GetFrequency () - GetWidth () / 2.
                  + (GetPrimaryChannelIndex (primaryChannelWidth) + 0.5) * primaryChannelWidth;

  NS_LOG_FUNCTION (this << primaryChannelWidth << freq);
  return freq;
}

} //namespace ns3
