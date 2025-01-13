/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "bulk-send-helper.h"

#include "ns3/string.h"

namespace ns3
{

BulkSendHelper::BulkSendHelper(const std::string& protocol, const Address& address)
    : ApplicationHelper("ns3::BulkSendApplication")
{
    m_factory.Set("Protocol", StringValue(protocol));
    m_factory.Set("Remote", AddressValue(address));
}

} // namespace ns3
