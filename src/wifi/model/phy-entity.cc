/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#include "phy-entity.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PhyEntity");

/*******************************************************
 *       Abstract base class for PHY entities
 *******************************************************/

PhyEntity::~PhyEntity ()
{
  m_modeList.clear ();
}

bool
PhyEntity::IsModeSupported (WifiMode mode) const
{
  for (const auto & m : m_modeList)
    {
      if (m == mode)
        {
          return true;
        }
    }
  return false;
}

uint8_t
PhyEntity::GetNumModes (void) const
{
  return m_modeList.size ();
}

WifiMode
PhyEntity::GetMcs (uint8_t /* index */) const
{
  NS_ABORT_MSG ("This method should be used only for HtPhy and child classes. Use GetMode instead.");
  return WifiMode ();
}

bool
PhyEntity::IsMcsSupported (uint8_t /* index */) const
{
  NS_ABORT_MSG ("This method should be used only for HtPhy and child classes. Use IsModeSupported instead.");
  return false;
}

bool
PhyEntity::HandlesMcsModes (void) const
{
  return false;
}

std::list<WifiMode>::const_iterator
PhyEntity::begin (void) const
{
  return m_modeList.begin ();
}

std::list<WifiMode>::const_iterator
PhyEntity::end (void) const
{
  return m_modeList.end ();
}

WifiMode
PhyEntity::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  NS_FATAL_ERROR ("PPDU field is not a SIG field (no sense in retrieving the signaled mode) or is unsupported: " << field);
  return WifiMode (); //should be overloaded
}

WifiPpduField
PhyEntity::GetNextField (WifiPpduField currentField, WifiPreamble preamble) const
{
  auto ppduFormats = GetPpduFormats ();
  const auto itPpdu = ppduFormats.find (preamble);
  if (itPpdu != ppduFormats.end ())
    {
      const auto itField = std::find (itPpdu->second.begin (), itPpdu->second.end (), currentField);
      if (itField != itPpdu->second.end ())
        {
          const auto itNextField = std::next (itField, 1);
          if (itNextField != itPpdu->second.end ())
            {
              return *(itNextField);
            }
          NS_FATAL_ERROR ("No field after " << currentField << " for " << preamble << " for the provided PPDU formats");
        }
      else
        {
          NS_FATAL_ERROR ("Unsupported PPDU field " << currentField << " for " << preamble << " for the provided PPDU formats");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported preamble " << preamble << " for the provided PPDU formats");
    }
}

Time
PhyEntity::GetDuration (WifiPpduField field, WifiTxVector txVector) const
{
  if (field > WIFI_PPDU_FIELD_SIG_B)
    {
      NS_FATAL_ERROR ("Unsupported PPDU field");
    }
  return MicroSeconds (0); //should be overloaded
}

Time
PhyEntity::CalculatePhyPreambleAndHeaderDuration (WifiTxVector txVector) const
{
  Time duration = MicroSeconds (0);
  for (uint8_t field = WIFI_PPDU_FIELD_PREAMBLE; field < WIFI_PPDU_FIELD_DATA; ++field)
    {
      duration += GetDuration (static_cast<WifiPpduField> (field), txVector);
    }
  return duration;
}

PhyEntity::PhyHeaderSections
PhyEntity::GetPhyHeaderSections (WifiTxVector txVector, Time ppduStart) const
{
  PhyHeaderSections map;
  WifiPpduField field = WIFI_PPDU_FIELD_PREAMBLE; //preamble always present
  Time start = ppduStart;

  while (field != WIFI_PPDU_FIELD_DATA)
    {
      Time duration = GetDuration (field, txVector);
      map[field] = std::make_pair (std::make_pair (start, start + duration),
                                   GetSigMode (field, txVector));
      //Move to next field
      start += duration;
      field = GetNextField (field, txVector.GetPreambleType ());
    }
  return map;
}

} //namespace ns3
