/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef PACKET_LOSS_COUNTER_H
#define PACKET_LOSS_COUNTER_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"

namespace ns3
{

class Socket;
class Packet;

/**
 * @ingroup udpclientserver
 *
 * @brief A class to count the number of lost packets.
 *
 * This class records the packet lost in a client/server transmission
 * leveraging a sequence number. All the packets outside a given window
 * (i.e., too old with respect to the last sequence number seen) are considered lost,
 */
class PacketLossCounter
{
  public:
    /**
     * @brief Constructor
     * @param bitmapSize The window size. Must be a multiple of 8.
     */
    PacketLossCounter(uint8_t bitmapSize);
    ~PacketLossCounter();
    /**
     * @brief Record a successfully received packet
     * @param seq the packet sequence number
     */
    void NotifyReceived(uint32_t seq);
    /**
     * @brief Get the number of lost packets.
     * @returns the number of lost packets.
     */
    uint32_t GetLost() const;
    /**
     * @brief Return the size of the window used to compute the packet loss.
     * @return the window size.
     */
    uint16_t GetBitMapSize() const;
    /**
     * @brief Set the size of the window used to compute the packet loss.
     *
     * @param size The window size. Must be a multiple of 8.
     */
    void SetBitMapSize(uint16_t size);

  private:
    /**
     * @brief Check if a sequence number in the window has been received.
     * @param seqNum the sequence number.
     * @returns false if the packet has not been received.
     */
    bool GetBit(uint32_t seqNum);
    /**
     * @brief Set a sequence number to a given state.
     * @param seqNum the sequence number.
     * @param val false if the packet has not been received.
     */
    void SetBit(uint32_t seqNum, bool val);

    uint32_t m_lost;          //!< Lost packets counter.
    uint16_t m_bitMapSize;    //!< Window size.
    uint32_t m_lastMaxSeqNum; //!< Last sequence number seen.
    uint8_t* m_receiveBitMap; //!< Received packets in the window size.
};
} // namespace ns3

#endif /* PACKET_LOSS_COUNTER_H */
