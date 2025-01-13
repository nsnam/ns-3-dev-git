/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef COMMON_INFO_PROBE_REQ_MLE_H
#define COMMON_INFO_PROBE_REQ_MLE_H

#include "ns3/buffer.h"

#include <cstdint>
#include <optional>

namespace ns3
{

/**
 * Common Info field of Multi-link Element Probe Request variant.
 * IEEE 802.11be D6.0 9.4.2.321.3
 */
struct CommonInfoProbeReqMle
{
    std::optional<uint8_t> m_apMldId; ///< AP MLD ID

    /**
     * Get the Presence Bitmap subfield of the Common Info field
     *
     * @return the Presence Bitmap subfield of the Common Info field
     */
    uint16_t GetPresenceBitmap() const;

    /**
     * Get the size of the serialized Common Info field
     *
     * @return the size of the serialized Common Info field
     */
    uint8_t GetSize() const;

    /**
     * Serialize the Common Info field
     *
     * @param start iterator pointing to where the Common Info field should be written to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize the Common Info field
     *
     * @param start iterator pointing to where the Common Info field should be read from
     * @param presence the value of the Presence Bitmap field indicating which subfields
     *                 are present in the Common Info field
     * @return the number of bytes read
     */
    uint8_t Deserialize(Buffer::Iterator start, uint16_t presence);
};

} // namespace ns3

#endif // COMMON_INFO_PROBE_REQ_MLE_H
