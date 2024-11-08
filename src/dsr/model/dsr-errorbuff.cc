/*
 * Copyright (c) 2011 Yufei Cheng
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
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

#include "dsr-errorbuff.h"

#include "ns3/ipv4-route.h"
#include "ns3/log.h"
#include "ns3/socket.h"

#include <algorithm>
#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsrErrorBuffer");

namespace dsr
{

uint32_t
DsrErrorBuffer::GetSize()
{
    Purge();
    return m_errorBuffer.size();
}

bool
DsrErrorBuffer::Enqueue(DsrErrorBuffEntry& entry)
{
    Purge();
    for (auto i = m_errorBuffer.begin(); i != m_errorBuffer.end(); ++i)
    {
        NS_LOG_INFO("packet id " << i->GetPacket()->GetUid() << " " << entry.GetPacket()->GetUid()
                                 << " source " << i->GetSource() << " " << entry.GetSource()
                                 << " next hop " << i->GetNextHop() << " " << entry.GetNextHop()
                                 << " dst " << i->GetDestination() << " "
                                 << entry.GetDestination());

        /// @todo check the source and destination over here
        if ((i->GetPacket()->GetUid() == entry.GetPacket()->GetUid()) &&
            (i->GetSource() == entry.GetSource()) && (i->GetNextHop() == entry.GetSource()) &&
            (i->GetDestination() == entry.GetDestination()))
        {
            return false;
        }
    }

    entry.SetExpireTime(m_errorBufferTimeout); // Initialize the send buffer timeout
    /*
     * Drop the most aged packet when buffer reaches to max
     */
    if (m_errorBuffer.size() >= m_maxLen)
    {
        Drop(m_errorBuffer.front(), "Drop the most aged packet"); // Drop the most aged packet
        m_errorBuffer.erase(m_errorBuffer.begin());
    }
    // enqueue the entry
    m_errorBuffer.push_back(entry);
    return true;
}

void
DsrErrorBuffer::DropPacketForErrLink(Ipv4Address source, Ipv4Address nextHop)
{
    NS_LOG_FUNCTION(this << source << nextHop);
    Purge();
    std::vector<Ipv4Address> list;
    list.push_back(source);
    list.push_back(nextHop);
    const std::vector<Ipv4Address> link = list;
    /*
     * Drop the packet with the error link source----------nextHop
     */
    for (auto i = m_errorBuffer.begin(); i != m_errorBuffer.end(); ++i)
    {
        if ((i->GetSource() == link[0]) && (i->GetNextHop() == link[1]))
        {
            DropLink(*i, "DropPacketForErrLink");
        }
    }

    auto new_end =
        std::remove_if(m_errorBuffer.begin(),
                       m_errorBuffer.end(),
                       [&](const DsrErrorBuffEntry& en) {
                           return (en.GetSource() == link[0]) && (en.GetNextHop() == link[1]);
                       });
    m_errorBuffer.erase(new_end, m_errorBuffer.end());
}

bool
DsrErrorBuffer::Dequeue(Ipv4Address dst, DsrErrorBuffEntry& entry)
{
    Purge();
    /*
     * Dequeue the entry with destination address dst
     */
    for (auto i = m_errorBuffer.begin(); i != m_errorBuffer.end(); ++i)
    {
        if (i->GetDestination() == dst)
        {
            entry = *i;
            i = m_errorBuffer.erase(i);
            NS_LOG_DEBUG("Packet size while dequeuing " << entry.GetPacket()->GetSize());
            return true;
        }
    }
    return false;
}

bool
DsrErrorBuffer::Find(Ipv4Address dst)
{
    /*
     * Make sure if the send buffer contains entry with certain dst
     */
    for (auto i = m_errorBuffer.begin(); i != m_errorBuffer.end(); ++i)
    {
        if (i->GetDestination() == dst)
        {
            NS_LOG_DEBUG("Found the packet");
            return true;
        }
    }
    return false;
}

/// IsExpired structure
struct IsExpired
{
    /**
     * @brief comparison operator
     * @param e entry to compare
     * @return true if entry expired
     */
    bool operator()(const DsrErrorBuffEntry& e) const
    {
        // NS_LOG_DEBUG("Expire time for packet in req queue: "<<e.GetExpireTime ());
        return (e.GetExpireTime().IsStrictlyNegative());
    }
};

void
DsrErrorBuffer::Purge()
{
    /*
     * Purge the buffer to eliminate expired entries
     */
    NS_LOG_DEBUG("The error buffer size " << m_errorBuffer.size());
    IsExpired pred;
    for (auto i = m_errorBuffer.begin(); i != m_errorBuffer.end(); ++i)
    {
        if (pred(*i))
        {
            NS_LOG_DEBUG("Dropping Queue Packets");
            Drop(*i, "Drop out-dated packet ");
        }
    }
    m_errorBuffer.erase(std::remove_if(m_errorBuffer.begin(), m_errorBuffer.end(), pred),
                        m_errorBuffer.end());
}

void
DsrErrorBuffer::Drop(DsrErrorBuffEntry en, std::string reason)
{
    NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " " << en.GetDestination());
    //  en.GetErrorCallback () (en.GetPacket (), en.GetDestination (),
    //     Socket::ERROR_NOROUTETOHOST);
}

void
DsrErrorBuffer::DropLink(DsrErrorBuffEntry en, std::string reason)
{
    NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " " << en.GetSource() << " "
                        << en.GetNextHop());
    //  en.GetErrorCallback () (en.GetPacket (), en.GetDestination (),
    //     Socket::ERROR_NOROUTETOHOST);
}
} // namespace dsr
} // namespace ns3
