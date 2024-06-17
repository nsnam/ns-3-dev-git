/*
 *  Copyright (c) 2009 INRIA, UDCAST
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "packet-loss-counter.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketLossCounter");

PacketLossCounter::PacketLossCounter(uint8_t bitmapSize)
    : m_lost(0),
      m_bitMapSize(0),
      m_lastMaxSeqNum(0),
      m_receiveBitMap(nullptr)
{
    NS_LOG_FUNCTION(this << bitmapSize);
    SetBitMapSize(bitmapSize);
}

PacketLossCounter::~PacketLossCounter()
{
    NS_LOG_FUNCTION(this);
    delete[] m_receiveBitMap;
}

uint16_t
PacketLossCounter::GetBitMapSize() const
{
    NS_LOG_FUNCTION(this);
    return m_bitMapSize * 8;
}

void
PacketLossCounter::SetBitMapSize(uint16_t winSize)
{
    NS_LOG_FUNCTION(this << winSize);

    NS_ASSERT_MSG(winSize % 8 == 0, "The packet window size should be a multiple of 8");
    m_bitMapSize = winSize / 8;
    if (m_receiveBitMap != nullptr)
    {
        delete[] m_receiveBitMap;
    }
    m_receiveBitMap = new uint8_t[m_bitMapSize]();
    memset(m_receiveBitMap, 0xFF, m_bitMapSize);
}

uint32_t
PacketLossCounter::GetLost() const
{
    NS_LOG_FUNCTION(this);
    return m_lost;
}

bool
PacketLossCounter::GetBit(uint32_t seqNum)
{
    NS_LOG_FUNCTION(this << seqNum);
    return ((m_receiveBitMap[(seqNum % (m_bitMapSize * 8)) / 8] >> (7 - (seqNum % 8))) & 0x01);
}

void
PacketLossCounter::SetBit(uint32_t seqNum, bool val)
{
    NS_LOG_FUNCTION(this << seqNum << val);
    if (val)
    {
        m_receiveBitMap[(seqNum % (m_bitMapSize * 8)) / 8] |= 0x80 >> (seqNum % 8);
    }
    else
    {
        m_receiveBitMap[(seqNum % (m_bitMapSize * 8)) / 8] &= ~(0x80 >> (seqNum % 8));
    }
}

/*
 * This algo works as follows:
 * When a packet is received:
 * 1) From the last received packet to the current one:
 *    1.1) check the corresponding bit in the bitMAP.
 *    This bit indicates if the packet with (SeqNum-bitMapSizeInBit) is
 *    received (1) or not (0)
 *    1.2) Mark the packet as lost (0) in the bitMap
 * 2) Mark the current packet as received (1) in the bitMap
 * 3) Update the value of the last received packet
 */

void
PacketLossCounter::NotifyReceived(uint32_t seqNum)
{
    NS_LOG_FUNCTION(this << seqNum);
    for (uint32_t i = m_lastMaxSeqNum + 1; i <= seqNum; i++)
    {
        if (!GetBit(i))
        {
            NS_LOG_INFO("Packet lost: " << i - (m_bitMapSize * 8));
            m_lost++;
        }
        SetBit(i, false);
    }
    SetBit(seqNum, true);
    if (seqNum > m_lastMaxSeqNum)
    {
        m_lastMaxSeqNum = seqNum;
    }
}

} // namespace ns3
