/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef MAC_RX_MIDDLE_H
#define MAC_RX_MIDDLE_H

#include "ns3/callback.h"
#include "ns3/simple-ref-count.h"

#include <map>

namespace ns3
{

class WifiMacHeader;
class OriginatorRxStatus;
class Packet;
class Mac48Address;
class WifiMpdu;

/**
 * @ingroup wifi
 *
 * This class handles duplicate detection and recomposition of fragments.
 */
class MacRxMiddle : public SimpleRefCount<MacRxMiddle>
{
  public:
    /**
     * typedef for callback
     */
    using ForwardUpCallback = Callback<void, Ptr<const WifiMpdu>, uint8_t /* link ID */>;

    MacRxMiddle();
    ~MacRxMiddle();

    /**
     * Set a callback to forward the packet up.
     *
     * @param callback the callback to set
     */
    void SetForwardCallback(ForwardUpCallback callback);

    /**
     * Receive an MPDU on the given link.
     *
     * @param mpdu the MPDU
     * @param linkId the ID of the given link
     */
    void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

  private:
    /// allow MacRxMiddleTest associated class access
    friend class MacRxMiddleTest;
    /**
     * Look up for OriginatorRxStatus associated with the sender address
     * (by looking at ADDR2 field in the header) for unicast. For groupcast,
     * the (nonconcealed) group address is used instead.
     * The method creates a new OriginatorRxStatus if one is not already presented.
     *
     * @param mpdu the received MPDU
     *
     * @return OriginatorRxStatus
     */
    OriginatorRxStatus& Lookup(Ptr<const WifiMpdu> mpdu);
    /**
     * Check if we have already received the packet from the sender before
     * (by looking at the sequence control field).
     *
     * @param hdr the MAC header
     * @param originator the packet originator status
     *
     * @return true if we already received the packet previously,
     *         false otherwise
     */
    bool IsDuplicate(const WifiMacHeader& hdr, const OriginatorRxStatus& originator) const;
    /**
     * Check if the received packet is a fragment and handle it appropriately.
     * If the packet is not a fragment, the method returns the packet.
     * If the packet is a fragment (not the last fragment), the method initiates
     * de-fragmentation process and return 0.
     * If the packet is the last fragment, the method tries to re-construct the full packet
     * and return the packet if success.
     *
     * @param packet the packet
     * @param hdr the MAC header
     * @param originator the packet originator status
     *
     * @return a packet if the packet is successfully reassembled (or not a fragment),
     *         0 if we failed to reassemble the packet (e.g. missing fragments/out-of-order).
     */
    Ptr<const Packet> HandleFragments(Ptr<const Packet> packet,
                                      const WifiMacHeader& hdr,
                                      OriginatorRxStatus& originator);

    /**
     * typedef for a map between address and OriginatorRxStatus
     */
    using Originators = std::map<Mac48Address, OriginatorRxStatus, std::less<>>;

    /**
     * typedef for a map between address, OriginatorRxStatus, and Traffic ID
     */
    using QosOriginators =
        std::map<std::pair<Mac48Address, uint8_t>, OriginatorRxStatus, std::less<>>;

    /**
     * typedef for an iterator for Originators
     */
    using OriginatorsI = std::map<Mac48Address, OriginatorRxStatus, std::less<>>::iterator;

    /**
     * typedef for an iterator for QosOriginators
     */
    using QosOriginatorsI =
        std::map<std::pair<Mac48Address, uint8_t>, OriginatorRxStatus, std::less<>>::iterator;

    Originators m_originatorStatus;       ///< originator status
    QosOriginators m_qosOriginatorStatus; ///< QOS originator status
    ForwardUpCallback m_callback;         ///< forward up callback
};

} // namespace ns3

#endif /* MAC_RX_MIDDLE_H */
