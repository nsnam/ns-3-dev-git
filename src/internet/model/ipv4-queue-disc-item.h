/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef IPV4_QUEUE_DISC_ITEM_H
#define IPV4_QUEUE_DISC_ITEM_H

#include "ipv4-header.h"

#include "ns3/packet.h"
#include "ns3/queue-item.h"

namespace ns3
{

/**
 * @ingroup ipv4
 * @ingroup traffic-control
 *
 * Ipv4QueueDiscItem is a subclass of QueueDiscItem which stores IPv4 packets.
 * Header and payload are kept separate to allow the queue disc to manipulate
 * the header, which is added to the packet when the packet is dequeued.
 */
class Ipv4QueueDiscItem : public QueueDiscItem
{
  public:
    /**
     * @brief Create an IPv4 queue disc item containing an IPv4 packet.
     * @param p the packet included in the created item.
     * @param addr the destination MAC address
     * @param protocol the protocol number
     * @param header the IPv4 header
     */
    Ipv4QueueDiscItem(Ptr<Packet> p,
                      const Address& addr,
                      uint16_t protocol,
                      const Ipv4Header& header);

    ~Ipv4QueueDiscItem() override;

    // Delete default constructor, copy constructor and assignment operator to avoid misuse
    Ipv4QueueDiscItem() = delete;
    Ipv4QueueDiscItem(const Ipv4QueueDiscItem&) = delete;
    Ipv4QueueDiscItem& operator=(const Ipv4QueueDiscItem&) = delete;

    /**
     * @return the correct packet size (header plus payload).
     */
    uint32_t GetSize() const override;

    /**
     * @return the header stored in this item..
     */
    const Ipv4Header& GetHeader() const;

    /**
     * @brief Add the header to the packet
     */
    void AddHeader() override;

    /**
     * @brief Print the item contents.
     * @param os output stream in which the data should be printed.
     */
    void Print(std::ostream& os) const override;

    /*
     * The values for the fields of the Ipv4 header are taken from m_header and
     * thus might differ from those present in the packet in case the header is
     * modified after being added to the packet. However, this function is likely
     * to be called before the header is added to the packet (i.e., before the
     * packet is dequeued from the queue disc)
     */
    bool GetUint8Value(Uint8Values field, uint8_t& value) const override;

    /**
     * @brief Marks the packet by setting ECN_CE bits if the packet has
     * ECN_ECT0 or ECN_ECT1 set.  If ECN_CE is already set, returns true.
     * @return true if the method results in a marked packet, false otherwise
     */
    bool Mark() override;

    /**
     * @brief Computes the hash of the packet's 5-tuple
     *
     * Computes the hash of the source and destination IP addresses, protocol
     * number and, if the transport protocol is either UDP or TCP, the source
     * and destination port
     *
     * @param perturbation hash perturbation value
     * @return the hash of the packet's 5-tuple
     */
    uint32_t Hash(uint32_t perturbation) const override;

  private:
    Ipv4Header m_header; //!< The IPv4 header.
    bool m_headerAdded;  //!< True if the header has already been added to the packet.
};

} // namespace ns3

#endif /* IPV4_QUEUE_DISC_ITEM_H */
