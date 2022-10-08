/*
 * Copyright (c) 2007,2008 INRIA
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
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "send-params.h"

#include "ns3/packet.h"

namespace ns3
{

SendParams::SendParams()
{
}

SendParams::~SendParams()
{
}

// -----------------------------------------

OfdmSendParams::OfdmSendParams(Ptr<PacketBurst> burst, uint8_t modulationType, uint8_t direction)
    : SendParams(),
      m_burst(burst),
      m_modulationType(modulationType),
      m_direction(direction)
{
}

OfdmSendParams::~OfdmSendParams()
{
}

} // namespace ns3
