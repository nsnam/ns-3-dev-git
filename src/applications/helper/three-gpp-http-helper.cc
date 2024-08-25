/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2013 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original work author (from packet-sink-helper.cc):
 * - Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *
 * Converted to 3GPP HTTP web browsing traffic models by:
 * - Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "three-gpp-http-helper.h"

namespace ns3
{

// 3GPP HTTP CLIENT HELPER /////////////////////////////////////////////////////////

ThreeGppHttpClientHelper::ThreeGppHttpClientHelper(const Address& address)
    : ApplicationHelper("ns3::ThreeGppHttpClient")
{
    m_factory.Set("Remote", AddressValue(address));
}

// HTTP SERVER HELPER /////////////////////////////////////////////////////////

ThreeGppHttpServerHelper::ThreeGppHttpServerHelper(const Address& address)
    : ApplicationHelper("ns3::ThreeGppHttpServer")
{
    m_factory.Set("Local", AddressValue(address));
}

} // namespace ns3
