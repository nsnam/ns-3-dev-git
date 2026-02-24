/*
 * Copyright (c) 2011 Yufei Cheng
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
 * Modified by: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *              Lorenzo Bartolini <l.bartolini02@gmail.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "dsr-rsendbuff.h"

#include "ns3/ipv4-route.h"
#include "ns3/log.h"
#include "ns3/socket.h"

#include <algorithm>
#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsrSendBuffer");

namespace dsr
{

uint32_t
DsrSendBuffer::GetSize()
{
    Purge();
    return m_sendBuffer.size();
}

bool
DsrSendBuffer::Enqueue(DsrSendBuffEntry& entry)
{
    Purge();
    for (auto i = m_sendBuffer.begin(); i != m_sendBuffer.end(); ++i)
    {
        //      NS_LOG_DEBUG ("packet id " << i->GetPacket ()->GetUid () << " " << entry.GetPacket
        //      ()->GetUid ()
        //                                << " dst " << i->GetDestination () << " " <<
        //                                entry.GetDestination ());

        if ((i->GetPacket()->GetUid() == entry.GetPacket()->GetUid()) &&
            (i->GetDestination() == entry.GetDestination()))
        {
            return false;
        }
    }

    entry.SetExpireTime(m_sendBufferTimeout); // Initialize the send buffer timeout
    /*
     * Drop the most aged packet when buffer reaches to max
     */
    if (m_sendBuffer.size() >= m_maxLen)
    {
        Drop(m_sendBuffer.front(), "Drop the most aged packet"); // Drop the most aged packet
        m_sendBuffer.erase(m_sendBuffer.begin());
    }
    // enqueue the entry
    m_sendBuffer.push_back(entry);
    return true;
}

void
DsrSendBuffer::DropPacketWithDst(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    Purge();
    /*
     * Drop the packet with destination address dst
     */
    for (auto i = m_sendBuffer.begin(); i != m_sendBuffer.end(); ++i)
    {
        if (i->GetDestination() == dst)
        {
            Drop(*i, "DropPacketWithDst");
        }
    }
    auto new_end =
        std::remove_if(m_sendBuffer.begin(), m_sendBuffer.end(), [&](const DsrSendBuffEntry& en) {
            return en.GetDestination() == dst;
        });
    m_sendBuffer.erase(new_end, m_sendBuffer.end());
}

bool
DsrSendBuffer::Dequeue(Ipv4Address dst, DsrSendBuffEntry& entry)
{
    Purge();
    /*
     * Dequeue the entry with destination address dst
     */
    for (auto i = m_sendBuffer.begin(); i != m_sendBuffer.end(); ++i)
    {
        if (i->GetDestination() == dst)
        {
            entry = *i;
            i = m_sendBuffer.erase(i);
            NS_LOG_DEBUG("Packet size while dequeuing " << entry.GetPacket()->GetSize());
            return true;
        }
    }
    return false;
}

bool
DsrSendBuffer::Find(Ipv4Address dst)
{
    /*
     * Make sure if the send buffer contains entry with certain dst
     */
    for (auto i = m_sendBuffer.begin(); i != m_sendBuffer.end(); ++i)
    {
        if (i->GetDestination() == dst)
        {
            NS_LOG_DEBUG("Found the packet");
            return true;
        }
    }
    return false;
}

void
DsrSendBuffer::Purge()
{
    /*
     * Purge the buffer to eliminate expired entries
     */
    NS_LOG_INFO("The send buffer size " << m_sendBuffer.size());
    for (const auto& item : m_sendBuffer)
    {
        if (item.GetExpireTime().IsStrictlyNegative())
        {
            NS_LOG_DEBUG("Dropping Queue Packets");
            Drop(item, "Drop out-dated packet ");
        }
    }

    [[maybe_unused]] auto erasedElementsNum =
        std::erase_if(m_sendBuffer, [](const DsrSendBuffEntry& e) {
            return e.GetExpireTime().IsStrictlyNegative();
        });
    NS_LOG_DEBUG("Purged " << erasedElementsNum << " from Send Buffer");
}

void
DsrSendBuffer::Drop(DsrSendBuffEntry en, std::string reason)
{
    NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " " << en.GetDestination());
    //  en.GetErrorCallback () (en.GetPacket (), en.GetDestination (),
    //     Socket::ERROR_NOROUTETOHOST);
}
} // namespace dsr
} // namespace ns3
