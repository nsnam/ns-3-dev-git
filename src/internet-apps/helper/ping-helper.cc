/*
 * Copyright (c) 2022 Universita' di Firenze, Italy
 * Copyright (c) 2008-2009 Strasbourg University (original Ping6 helper)
 * Copyright (c) 2008 INRIA (original v4Ping helper)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *
 * Derived from original v4Ping helper (author: Mathieu Lacage)
 * Derived from original ping6 helper (author: Sebastien Vincent)
 */

#include "ping-helper.h"

namespace ns3
{

PingHelper::PingHelper()
    : ApplicationHelper("ns3::Ping")
{
}

PingHelper::PingHelper(const Address& remote, const Address& local)
    : ApplicationHelper("ns3::Ping")
{
    m_factory.Set("Destination", AddressValue(remote));
    m_factory.Set("InterfaceAddress", AddressValue(local));
}

} // namespace ns3
