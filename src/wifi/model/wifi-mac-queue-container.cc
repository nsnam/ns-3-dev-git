/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue-container.h"

#include "ctrl-headers.h"
#include "wifi-mpdu.h"

#include "ns3/mac48-address.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3
{

void
WifiMacQueueContainer::clear()
{
    m_queues.clear();
    m_expiredQueue.clear();
    m_nBytesPerQueue.clear();
}

WifiMacQueueContainer::iterator
WifiMacQueueContainer::insert(const_iterator pos, Ptr<WifiMpdu> item)
{
    WifiContainerQueueId queueId = GetQueueId(item);

    NS_ABORT_MSG_UNLESS(pos == m_queues[queueId].cend() || GetQueueId(pos->mpdu) == queueId,
                        "pos iterator does not point to the correct container queue");
    NS_ABORT_MSG_IF(!item->IsOriginal(), "Only the original copy of an MPDU can be inserted");

    auto [it, ret] = m_nBytesPerQueue.insert({queueId, 0});
    it->second += item->GetSize();

    return m_queues[queueId].emplace(pos, item);
}

WifiMacQueueContainer::iterator
WifiMacQueueContainer::erase(const_iterator pos)
{
    if (pos->expired)
    {
        return m_expiredQueue.erase(pos);
    }

    WifiContainerQueueId queueId = GetQueueId(pos->mpdu);
    auto it = m_nBytesPerQueue.find(queueId);
    NS_ASSERT(it != m_nBytesPerQueue.end());
    NS_ASSERT(it->second >= pos->mpdu->GetSize());
    it->second -= pos->mpdu->GetSize();

    return m_queues[queueId].erase(pos);
}

Ptr<WifiMpdu>
WifiMacQueueContainer::GetItem(const const_iterator it) const
{
    return it->mpdu;
}

WifiContainerQueueId
WifiMacQueueContainer::GetQueueId(Ptr<const WifiMpdu> mpdu)
{
    const WifiMacHeader& hdr = mpdu->GetHeader();

    auto addrType = hdr.GetAddr1().IsGroup() ? WIFI_BROADCAST : WIFI_UNICAST;
    auto address = hdr.GetAddr1().IsGroup() ? hdr.GetAddr2() : hdr.GetAddr1();

    if (hdr.IsCtl())
    {
        return {WIFI_CTL_QUEUE, addrType, address, std::nullopt};
    }
    if (hdr.IsMgt())
    {
        return {WIFI_MGT_QUEUE, addrType, address, std::nullopt};
    }
    if (hdr.IsQosData())
    {
        return {WIFI_QOSDATA_QUEUE, addrType, address, hdr.GetQosTid()};
    }
    return {WIFI_DATA_QUEUE, addrType, address, std::nullopt};
}

const WifiMacQueueContainer::ContainerQueue&
WifiMacQueueContainer::GetQueue(const WifiContainerQueueId& queueId) const
{
    return m_queues[queueId];
}

uint32_t
WifiMacQueueContainer::GetNBytes(const WifiContainerQueueId& queueId) const
{
    if (auto it = m_queues.find(queueId); it == m_queues.end() || it->second.empty())
    {
        return 0;
    }
    return m_nBytesPerQueue.at(queueId);
}

std::pair<WifiMacQueueContainer::iterator, WifiMacQueueContainer::iterator>
WifiMacQueueContainer::ExtractExpiredMpdus(const WifiContainerQueueId& queueId) const
{
    return DoExtractExpiredMpdus(m_queues[queueId]);
}

std::pair<WifiMacQueueContainer::iterator, WifiMacQueueContainer::iterator>
WifiMacQueueContainer::DoExtractExpiredMpdus(ContainerQueue& queue) const
{
    std::optional<std::pair<WifiMacQueueContainer::iterator, WifiMacQueueContainer::iterator>> ret;
    auto firstExpiredIt = queue.begin();
    auto lastExpiredIt = firstExpiredIt;
    Time now = Simulator::Now();

    do
    {
        // advance firstExpiredIt and lastExpiredIt to skip all inflight MPDUs
        for (firstExpiredIt = lastExpiredIt;
             firstExpiredIt != queue.end() && !firstExpiredIt->inflights.empty();
             ++firstExpiredIt, ++lastExpiredIt)
        {
        }

        if (!ret)
        {
            // we get here in the first iteration only
            ret = std::make_pair(firstExpiredIt, lastExpiredIt);
        }

        // advance lastExpiredIt as we encounter MPDUs with expired lifetime that are not inflight
        while (lastExpiredIt != queue.end() && lastExpiredIt->expiryTime <= now &&
               lastExpiredIt->inflights.empty())
        {
            lastExpiredIt->expired = true;
            // this MPDU is no longer queued
            lastExpiredIt->ac = AC_UNDEF;
            lastExpiredIt->deleter(lastExpiredIt->mpdu);

            WifiContainerQueueId queueId = GetQueueId(lastExpiredIt->mpdu);
            auto it = m_nBytesPerQueue.find(queueId);
            NS_ASSERT(it != m_nBytesPerQueue.end());
            NS_ASSERT(it->second >= lastExpiredIt->mpdu->GetSize());
            it->second -= lastExpiredIt->mpdu->GetSize();

            ++lastExpiredIt;
        }

        if (lastExpiredIt != firstExpiredIt)
        {
            // transfer non-inflight MPDUs with expired lifetime to the tail of m_expiredQueue
            m_expiredQueue.splice(m_expiredQueue.end(), queue, firstExpiredIt, lastExpiredIt);
            ret->second = m_expiredQueue.end();
        }

    } while (lastExpiredIt != firstExpiredIt);

    return *ret;
}

std::pair<WifiMacQueueContainer::iterator, WifiMacQueueContainer::iterator>
WifiMacQueueContainer::ExtractAllExpiredMpdus() const
{
    std::optional<WifiMacQueueContainer::iterator> firstExpiredIt;

    for (auto& queue : m_queues)
    {
        auto [firstIt, lastIt] = DoExtractExpiredMpdus(queue.second);

        if (firstIt != lastIt && !firstExpiredIt)
        {
            // this is the first queue with MPDUs with expired lifetime
            firstExpiredIt = firstIt;
        }
    }
    return std::make_pair(firstExpiredIt ? *firstExpiredIt : m_expiredQueue.end(),
                          m_expiredQueue.end());
}

std::pair<WifiMacQueueContainer::iterator, WifiMacQueueContainer::iterator>
WifiMacQueueContainer::GetAllExpiredMpdus() const
{
    return {m_expiredQueue.begin(), m_expiredQueue.end()};
}

} // namespace ns3

/****************************************************
 *      Global Functions (outside namespace ns3)
 ***************************************************/

std::size_t
std::hash<ns3::WifiContainerQueueId>::operator()(ns3::WifiContainerQueueId queueId) const
{
    auto [type, addrType, address, tid] = queueId;
    const std::size_t size = tid.has_value() ? 8 : 7;

    std::vector<uint8_t> buffer(size);
    buffer[0] = type;
    address.CopyTo(&buffer[1]);
    if (tid.has_value())
    {
        buffer[7] = *tid;
    }

    std::string s(buffer.begin(), buffer.end());
    return std::hash<std::string>{}(s);
}
