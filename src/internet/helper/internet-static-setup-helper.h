/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef INTERNET_STATIC_SETUP_HELPER_H
#define INTERNET_STATIC_SETUP_HELPER_H

#include "ns3/nstime.h"

namespace ns3
{

/// one year's time
extern const Time ONE_YEAR;

/**
 * @ingroup InternetStaticSetupHelper
 *
 * @brief A helper class to populate ARP cache in network simulations
 *
 * This class is useful to bypass ARP request-response frame exchanges
 *
 * Note: this helper is only designed to work with IPv4.
 */
class InternetStaticSetupHelper
{
  public:
    /**
     * @brief Populate ARP cache for the nodes registered in the network
     */
    static void PopulateArpCache();
};

} // namespace ns3

#endif /* INTERNET_STATIC_SETUP_HELPER_H */
