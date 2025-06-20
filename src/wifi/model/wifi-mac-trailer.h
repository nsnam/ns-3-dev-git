/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef WIFI_MAC_TRAILER_H
#define WIFI_MAC_TRAILER_H

#include "wifi-standard-constants.h"

#include "ns3/trailer.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * Implements the IEEE 802.11 MAC trailer
 */
class WifiMacTrailer : public Trailer
{
  public:
    WifiMacTrailer();
    ~WifiMacTrailer() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
};

} // namespace ns3

#endif /* WIFI_MAC_TRAILER_H */
