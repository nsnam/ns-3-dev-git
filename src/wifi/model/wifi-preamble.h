/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 */

#ifndef WIFI_PREAMBLE_H
#define WIFI_PREAMBLE_H

#include "ns3/fatal-error.h"

namespace ns3 {

/**
 * \ingroup wifi
 * The type of preamble to be used by an IEEE 802.11 transmission
 */
enum WifiPreamble
{
  WIFI_PREAMBLE_LONG,
  WIFI_PREAMBLE_SHORT,
  WIFI_PREAMBLE_HT_MF,
  WIFI_PREAMBLE_HT_GF,
  WIFI_PREAMBLE_VHT_SU,
  WIFI_PREAMBLE_VHT_MU,
  WIFI_PREAMBLE_HE_SU,
  WIFI_PREAMBLE_HE_ER_SU,
  WIFI_PREAMBLE_HE_MU,
  WIFI_PREAMBLE_HE_TB
};

/**
* \brief Stream insertion operator.
*
* \param os the stream
* \param preamble the preamble
* \returns a reference to the stream
*/
inline std::ostream& operator<< (std::ostream& os, WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_LONG:
      return (os << "LONG");
    case WIFI_PREAMBLE_SHORT:
      return (os << "SHORT");
    case WIFI_PREAMBLE_HT_MF:
      return (os << "HT_MF");
    case WIFI_PREAMBLE_HT_GF:
      return (os << "HT_GF");
    case WIFI_PREAMBLE_VHT_SU:
      return (os << "VHT_SU");
    case WIFI_PREAMBLE_VHT_MU:
      return (os << "VHT_MU");
    case WIFI_PREAMBLE_HE_SU:
      return (os << "HE_SU");
    case WIFI_PREAMBLE_HE_ER_SU:
      return (os << "HE_ER_SU");
    case WIFI_PREAMBLE_HE_MU:
      return (os << "HE_MU");
    case WIFI_PREAMBLE_HE_TB:
      return (os << "HE_TB");
    default:
      NS_FATAL_ERROR ("Invalid preamble");
      return (os << "INVALID");
    }
}

} //namespace ns3

#endif /* WIFI_PREAMBLE_H */
