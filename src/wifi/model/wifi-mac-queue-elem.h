/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_ELEM_H
#define WIFI_MAC_QUEUE_ELEM_H

#include "qos-utils.h"

#include "ns3/callback.h"
#include "ns3/nstime.h"

#include <map>

namespace ns3
{

class WifiMpdu;

/**
 * @ingroup wifi
 * Type of elements stored in a WifiMacQueue container.
 *
 * Such elements can be accessed by the WifiMacQueue (via iterators) and
 * by the WifiMpdu itself (via the iterator it stores).
 *
 * Data frames transmitted by an 11be MLD must include link addresses as (RA, TA)
 * which are different than the MLD addresses seen by the upper layer. In order
 * to keep the original version of the data frame, we create an alias when a data
 * frame is sent over a link. Aliases are stored in the map of in-flight MPDUs, which
 * is indexed by the ID of the link over which the alias is in-flight.
 * For consistency, also data frame transmitted by non-MLDs have an alias, which is
 * simply a pointer to the original version of the data frame.
 */
struct WifiMacQueueElem
{
    Ptr<WifiMpdu> mpdu;                         ///< MPDU stored by this element
    Time expiryTime{0};                         ///< expiry time of the MPDU (set by WifiMacQueue)
    AcIndex ac{AC_UNDEF};                       ///< the Access Category associated with the queue
                                                ///< storing this element (set by WifiMacQueue)
    bool expired{false};                        ///< whether this MPDU has been marked as expired
    std::map<uint8_t, Ptr<WifiMpdu>> inflights; ///< map of MPDUs in-flight on each link
    Callback<void, Ptr<WifiMpdu>> deleter;      ///< reset the iterator stored by the MPDU

    /**
     * Constructor.
     * @param item the MPDU stored by this queue element
     */
    WifiMacQueueElem(Ptr<WifiMpdu> item);

    ~WifiMacQueueElem();
};

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_ELEM_H */
