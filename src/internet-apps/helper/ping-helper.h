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

#ifndef PING_HELPER_H
#define PING_HELPER_H

#include "ns3/application-helper.h"
#include "ns3/ping.h"

namespace ns3
{

/**
 * @ingroup ping
 * @brief Create a ping application and associate it to a node
 *
 * This class creates one or multiple instances of ns3::Ping and associates
 * it/them to one/multiple node(s).
 */
class PingHelper : public ApplicationHelper
{
  public:
    /**
     * Create a PingHelper which is used to make life easier for people wanting
     * to use ping Applications.
     */
    PingHelper();

    /**
     * Create a PingHelper which is used to make life easier for people wanting
     * to use ping Applications.
     *
     * @param remote The address which should be pinged
     * @param local The source address
     */
    PingHelper(const Address& remote, const Address& local = Address());
};

} // namespace ns3

#endif /* PING_HELPER_H */
