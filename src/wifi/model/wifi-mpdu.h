/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MPDU_H
#define WIFI_MPDU_H

#include "amsdu-subframe-header.h"
#include "wifi-mac-header.h"
#include "wifi-mac-queue-elem.h"

#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <list>
#include <optional>
#include <set>
#include <variant>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * Tag used to allow (only) WifiMacQueue to access the queue iterator stored
 * by a WifiMpdu.
 */
class WmqIteratorTag
{
    friend class WifiMacQueue;
    WmqIteratorTag() = default;
};

/**
 * @ingroup wifi
 *
 * WifiMpdu stores a (const) packet along with a MAC header. To support 802.11be
 * Multi-Link Operation (MLO), a WifiMpdu variant, referred to as WifiMpdu alias,
 * is added. A WifiMpdu alias stores its own MAC header and a pointer to the original
 * copy of the WifiMpdu.
 */
class WifiMpdu : public SimpleRefCount<WifiMpdu>
{
  public:
    /**
     * @brief Create a Wifi MAC queue item containing a packet and a Wifi MAC header.
     * @param p the const packet included in the created item.
     * @param header the Wifi MAC header included in the created item.
     * @param stamp the timestamp to associate with the MPDU
     */
    WifiMpdu(Ptr<const Packet> p, const WifiMacHeader& header, Time stamp = Simulator::Now());

    virtual ~WifiMpdu();

    /**
     * @return whether this is the original version of the MPDU
     */
    bool IsOriginal() const;

    /**
     * @return the original version of this MPDU
     */
    Ptr<const WifiMpdu> GetOriginal() const;

    /**
     * @brief Get the packet stored in this item
     * @return the packet stored in this item.
     */
    Ptr<const Packet> GetPacket() const;

    /**
     * @brief Get the header stored in this item
     * @return the header stored in this item.
     */
    const WifiMacHeader& GetHeader() const;

    /**
     * @brief Get the header stored in this item
     * @return the header stored in this item.
     */
    WifiMacHeader& GetHeader();

    /**
     * @brief Return the destination address present in the header
     * @return the destination address
     */
    Mac48Address GetDestinationAddress() const;

    /**
     * @brief Return the size of the packet stored by this item, including header
     *        size and trailer size
     *
     * @return the size of the packet stored by this item in bytes.
     */
    uint32_t GetSize() const;

    /**
     * @brief Return the size in bytes of the packet or control header or management
     *        header stored by this item.
     *
     * @return the size in bytes of the packet or control header or management header
     *         stored by this item
     */
    uint32_t GetPacketSize() const;

    /**
     * Return true if this item contains an MSDU fragment, false otherwise
     * @return true if this item contains an MSDU fragment, false otherwise
     */
    bool IsFragment() const;

    /**
     * @brief Aggregate the MSDU contained in the given MPDU to this MPDU (thus
     *        constituting an A-MSDU). Note that the given MPDU cannot contain
     *        an A-MSDU. If the given MPDU is a null pointer, the effect of this
     *        call is to add only an A-MSDU subframe header, thus producing an A-MSDU
     *        containing a single MSDU.
     * @param msdu the MPDU containing the MSDU to aggregate
     */
    void Aggregate(Ptr<const WifiMpdu> msdu);

    /// DeaggregatedMsdus typedef
    typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader>> DeaggregatedMsdus;
    /// DeaggregatedMsdusCI typedef
    typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader>>::const_iterator
        DeaggregatedMsdusCI;

    /**
     * @brief Get a constant iterator pointing to the first MSDU in the list of aggregated MSDUs.
     *
     * @return a constant iterator pointing to the first MSDU in the list of aggregated MSDUs
     */
    DeaggregatedMsdusCI begin() const;
    /**
     * @brief Get a constant iterator indicating past-the-last MSDU in the list of aggregated MSDUs.
     *
     * @return a constant iterator indicating past-the-last MSDU in the list of aggregated MSDUs
     */
    DeaggregatedMsdusCI end() const;

    /// Const iterator typedef
    typedef std::list<WifiMacQueueElem>::iterator Iterator;

    /**
     * Set the queue iterator stored by this object.
     *
     * @param queueIt the queue iterator for this object
     * @param tag a wifi MAC queue iterator tag (allows only WifiMacQueue to call this method)
     */
    void SetQueueIt(std::optional<Iterator> queueIt, WmqIteratorTag tag);
    /**
     * @param tag a wifi MAC queue iterator tag (allows only WifiMacQueue to call this method)
     * @return the queue iterator stored by this object
     */
    Iterator GetQueueIt(WmqIteratorTag tag) const;

    /**
     * Return true if this item is stored in some queue, false otherwise.
     *
     * @return true if this item is stored in some queue, false otherwise
     */
    bool IsQueued() const;
    /**
     * Get the AC of the queue this item is stored into. Abort if this item
     * is not stored in a queue.
     *
     * @return the AC of the queue this item is stored into
     */
    AcIndex GetQueueAc() const;
    /**
     * @return the time this MPDU was constructed
     */
    Time GetTimestamp() const;
    /**
     * @return the expiry time of this MPDU
     */
    Time GetExpiryTime() const;

    /**
     * @return the frame retry count
     */
    uint32_t GetRetryCount() const;

    /**
     * Increment the frame retry count.
     */
    void IncrementRetryCount();

    /**
     * @brief Get the MAC protocol data unit (MPDU) corresponding to this item
     *        (i.e. a copy of the packet stored in this item wrapped with MAC
     *        header and trailer)
     * @return the MAC protocol data unit corresponding to this item.
     */
    Ptr<Packet> GetProtocolDataUnit() const;

    /**
     * Mark this MPDU as being in flight on the given link.
     *
     * @param linkId the ID of the given link
     */
    void SetInFlight(uint8_t linkId) const;
    /**
     * Mark this MPDU as not being in flight on the given link.
     *
     * @param linkId the ID of the given link
     */
    void ResetInFlight(uint8_t linkId) const;
    /**
     * @return the set of IDs of the links on which this MPDU is currently in flight
     */
    std::set<uint8_t> GetInFlightLinkIds() const;
    /**
     * @return true if this MPDU is in flight on any link, false otherwise
     */
    bool IsInFlight() const;

    /**
     * Set the sequence number of this MPDU (and of the original copy, if this is an alias)
     * to the given value. Also, record that a sequence number has been assigned to this MPDU.
     *
     * @param seqNo the given sequence number
     */
    void AssignSeqNo(uint16_t seqNo);
    /**
     * @return whether a sequence number has been assigned to this MPDU
     */
    bool HasSeqNoAssigned() const;
    /**
     * Record that a sequence number is no (longer) assigned to this MPDU.
     */
    void UnassignSeqNo();

    /**
     * Create an alias for this MPDU (which must be an original copy) for transmission
     * on the link with the given ID. Aliases have their own copy of the MAC header and
     * cannot be used to perform non-const operations on the frame body.
     *
     * @param linkId the ID of the given link
     * @return an alias for this MPDU
     */
    Ptr<WifiMpdu> CreateAlias(uint8_t linkId) const;

    /**
     * @brief Print the item contents.
     * @param os output stream in which the data should be printed.
     */
    virtual void Print(std::ostream& os) const;

  private:
    /**
     * @brief Aggregate the MSDU contained in the given MPDU to this MPDU (thus
     *        constituting an A-MSDU). Note that the given MPDU cannot contain
     *        an A-MSDU.
     * @param msdu the MPDU containing the MSDU to aggregate
     */
    void DoAggregate(Ptr<const WifiMpdu> msdu);

    /**
     * @return the queue iterator stored by this object
     */
    Iterator GetQueueIt() const;

    /**
     * Private default constructor (used to construct aliases).
     */
    WifiMpdu() = default;

    /**
     * Information stored by both the original copy and the aliases
     */
    WifiMacHeader m_header; //!< Wifi MAC header associated with the packet

    /**
     * Information stored by the original copy only.
     */
    struct OriginalInfo
    {
        Ptr<const Packet> m_packet;        //!< MSDU or A-MSDU contained in this queue item
        Time m_timestamp;                  //!< construction time
        DeaggregatedMsdus m_msduList;      //!< list of aggregated MSDUs included in this MPDU
        std::optional<Iterator> m_queueIt; //!< Queue iterator pointing to this MPDU, if queued
        bool m_seqNoAssigned;              //!< whether a sequence number has been assigned
        uint32_t m_retryCount; //!< the frame retry count maintained for each MSDU, A-MSDU or MMPDU
    };

    /**
     * @return a reference to the information held by the original copy of the MPDU.
     */
    OriginalInfo& GetOriginalInfo();
    /**
     * @return a const reference to the information held by the original copy of the MPDU.
     */
    const OriginalInfo& GetOriginalInfo() const;

    /// Information stored by the original copy and an alias, respectively
    using InstanceInfo = std::variant<OriginalInfo, Ptr<WifiMpdu>>;

    InstanceInfo m_instanceInfo; //!< information associated with the instance type
    static constexpr std::size_t ORIGINAL =
        0;                                  //!< index of original copy in the InstanceInfo variant
    static constexpr std::size_t ALIAS = 1; //!< index of an alias in the InstanceInfo variant
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the output stream
 * @param item the WifiMpdu
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const WifiMpdu& item);

} // namespace ns3

#endif /* WIFI_MPDU_H */
