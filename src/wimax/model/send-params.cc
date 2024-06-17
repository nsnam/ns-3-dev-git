/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
