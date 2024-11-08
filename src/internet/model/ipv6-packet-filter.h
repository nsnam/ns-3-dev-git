/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *               2016 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 *           Pasquale Imputato <p.imputato@gmail.com>
 */

#ifndef IPV6_PACKET_FILTER_H
#define IPV6_PACKET_FILTER_H

#include "ns3/object.h"
#include "ns3/packet-filter.h"

namespace ns3
{

/**
 * @ingroup ipv6
 * @ingroup traffic-control
 *
 * Ipv6PacketFilter is the abstract base class for filters defined for IPv6 packets.
 */
class Ipv6PacketFilter : public PacketFilter
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ipv6PacketFilter();
    ~Ipv6PacketFilter() override;

  private:
    bool CheckProtocol(Ptr<QueueDiscItem> item) const override;
    int32_t DoClassify(Ptr<QueueDiscItem> item) const override = 0;
};

} // namespace ns3

#endif /* IPV6_PACKET_FILTER */
