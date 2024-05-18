/*
 * Copyright (c) 2024
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
 * Authors: Jiwoong Lee <porce@berkeley.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_UNITS_H
#define WIFI_UNITS_H

#include <cstdint>

/**
 * \file
 * \ingroup wifi
 * Declaration of the SI units (as weak types aliases) used across wifi module
 */

namespace ns3
{

using mWatt_u = double;       //!< mWatt weak type
using Watt_u = double;        //!< Watt weak type
using dBW_u = double;         //!< dBW weak type
using dBm_u = double;         //!< dBm weak type
using dB_u = double;          //!< dB weak type
using dBr_u = dB_u;           //!< dBr weak type
using Hz_u = double;          //!< Hz weak type
using MHz_u = double;         //!< MHz weak type
using meter_u = double;       //!< meter weak type
using ampere_u = double;      //!< ampere weak type
using volt_u = double;        //!< volt weak type
using degree_u = double;      //!< degree weak type (angle)
using joule_u = double;       //!< joule weak type
using dBm_per_Hz_u = double;  //!< dBm/Hz weak type
using dBm_per_MHz_u = double; //!< dBm/MHz weak type

} // namespace ns3

#endif // WIFI_UNITS_H
