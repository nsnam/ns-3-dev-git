/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include <list>
#include <optional>

namespace ns3
{

/**
 * \ingroup wifi
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
 * \ingroup wifi
 *
 * WifiMpdu stores (const) packets along with their Wifi MAC headers
 * and the time when they were enqueued.
 */
class WifiMpdu : public SimpleRefCount<WifiMpdu>
{
  public:
    /**
     * \brief Create a Wifi MAC queue item containing a packet and a Wifi MAC header.
     * \param p the const packet included in the created item.
     * \param header the Wifi MAC header included in the created item.
     */
    WifiMpdu(Ptr<const Packet> p, const WifiMacHeader& header);

    virtual ~WifiMpdu();

    /**
     * \brief Get the packet stored in this item
     * \return the packet stored in this item.
     */
    Ptr<const Packet> GetPacket() const;

    /**
     * \brief Get the header stored in this item
     * \return the header stored in this item.
     */
    const WifiMacHeader& GetHeader() const;

    /**
     * \brief Get the header stored in this item
     * \return the header stored in this item.
     */
    WifiMacHeader& GetHeader();

    /**
     * \brief Return the destination address present in the header
     * \return the destination address
     */
    Mac48Address GetDestinationAddress() const;

    /**
     * \brief Return the size of the packet stored by this item, including header
     *        size and trailer size
     *
     * \return the size of the packet stored by this item in bytes.
     */
    uint32_t GetSize() const;

    /**
     * \brief Return the size in bytes of the packet or control header or management
     *        header stored by this item.
     *
     * \return the size in bytes of the packet or control header or management header
     *         stored by this item
     */
    uint32_t GetPacketSize() const;

    /**
     * Return true if this item contains an MSDU fragment, false otherwise
     * \return true if this item contains an MSDU fragment, false otherwise
     */
    bool IsFragment() const;

    /**
     * \brief Aggregate the MSDU contained in the given MPDU to this MPDU (thus
     *        constituting an A-MSDU). Note that the given MPDU cannot contain
     *        an A-MSDU.
     * \param msdu the MPDU containing the MSDU to aggregate
     */
    void Aggregate(Ptr<const WifiMpdu> msdu);

    /// DeaggregatedMsdus typedef
    typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader>> DeaggregatedMsdus;
    /// DeaggregatedMsdusCI typedef
    typedef std::list<std::pair<Ptr<const Packet>, AmsduSubframeHeader>>::const_iterator
        DeaggregatedMsdusCI;

    /**
     * \brief Get a constant iterator pointing to the first MSDU in the list of aggregated MSDUs.
     *
     * \return a constant iterator pointing to the first MSDU in the list of aggregated MSDUs
     */
    DeaggregatedMsdusCI begin() const;
    /**
     * \brief Get a constant iterator indicating past-the-last MSDU in the list of aggregated MSDUs.
     *
     * \return a constant iterator indicating past-the-last MSDU in the list of aggregated MSDUs
     */
    DeaggregatedMsdusCI end() const;

    /// Const iterator typedef
    typedef std::list<WifiMacQueueElem>::iterator Iterator;

    /**
     * Set the queue iterator stored by this object.
     *
     * \param queueIt the queue iterator for this object
     * \param tag a wifi MAC queue iterator tag (allows only WifiMacQueue to call this method)
     */
    void SetQueueIt(std::optional<Iterator> queueIt, WmqIteratorTag tag);
    /**
     * \param tag a wifi MAC queue iterator tag (allows only WifiMacQueue to call this method)
     * \return the queue iterator stored by this object
     */
    Iterator GetQueueIt(WmqIteratorTag tag) const;

    /**
     * Return true if this item is stored in some queue, false otherwise.
     *
     * \return true if this item is stored in some queue, false otherwise
     */
    bool IsQueued() const;
    /**
     * Get the AC of the queue this item is stored into. Abort if this item
     * is not stored in a queue.
     *
     * \return the AC of the queue this item is stored into
     */
    AcIndex GetQueueAc() const;
    /**
     * \return the expiry time of this MPDU
     */
    Time GetExpiryTime() const;

    /**
     * \brief Get the MAC protocol data unit (MPDU) corresponding to this item
     *        (i.e. a copy of the packet stored in this item wrapped with MAC
     *        header and trailer)
     * \return the MAC protocol data unit corresponding to this item.
     */
    Ptr<Packet> GetProtocolDataUnit() const;

    /**
     * Mark this MPDU as being in flight (only used if Block Ack agreement established).
     */
    void SetInFlight();
    /**
     * Mark this MPDU as not being in flight (only used if Block Ack agreement established).
     */
    void ResetInFlight();
    /**
     * Return true if this MPDU is in flight, false otherwise.
     *
     * \return true if this MPDU is in flight, false otherwise
     */
    bool IsInFlight() const;

    /**
     * \brief Print the item contents.
     * \param os output stream in which the data should be printed.
     */
    virtual void Print(std::ostream& os) const;

  private:
    /**
     * \brief Aggregate the MSDU contained in the given MPDU to this MPDU (thus
     *        constituting an A-MSDU). Note that the given MPDU cannot contain
     *        an A-MSDU.
     * \param msdu the MPDU containing the MSDU to aggregate
     */
    void DoAggregate(Ptr<const WifiMpdu> msdu);

    Ptr<const Packet> m_packet;        //!< The packet (MSDU or A-MSDU) contained in this queue item
    WifiMacHeader m_header;            //!< Wifi MAC header associated with the packet
    DeaggregatedMsdus m_msduList;      //!< The list of aggregated MSDUs included in this MPDU
    std::optional<Iterator> m_queueIt; //!< Queue iterator pointing to this MPDU, if queued
    bool m_inFlight;                   //!< whether the MPDU is in flight
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the output stream
 * \param item the WifiMpdu
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const WifiMpdu& item);

} // namespace ns3

#endif /* WIFI_MPDU_H */
