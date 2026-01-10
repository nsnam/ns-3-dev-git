/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
    // the given MPDU may be an alias and we need its original version to correctly identify the
    // container queue in which the MPDU is enqueued
    const auto& hdr = mpdu->GetOriginal()->GetHeader();

    WifiRcvAddr addrType;
    std::optional<Mac48Address> addr1;
    std::optional<Mac48Address> addr2;

    if (hdr.GetAddr1().IsBroadcast())
    {
        addrType = WifiRcvAddr::BROADCAST;
        addr2 = hdr.GetAddr2();
    }
    else if (hdr.GetAddr1().IsGroup())
    {
        addrType = WifiRcvAddr::GROUPCAST;
        addr1 = hdr.IsQosAmsdu() ? mpdu->begin()->second.GetDestinationAddr() : hdr.GetAddr1();
        addr2 = hdr.GetAddr2();
    }
    else
    {
        addrType = WifiRcvAddr::UNICAST;
        addr1 = hdr.GetAddr1();
    }

    if (hdr.IsCtl())
    {
        return {WIFI_CTL_QUEUE, addrType, addr1, addr2, std::nullopt};
    }
    if (hdr.IsMgt())
    {
        return {WIFI_MGT_QUEUE, addrType, addr1, addr2, std::nullopt};
    }
    if (hdr.IsQosData())
    {
        return {WIFI_QOSDATA_QUEUE, addrType, addr1, addr2, hdr.GetQosTid()};
    }
    return {WIFI_DATA_QUEUE, addrType, addr1, addr2, std::nullopt};
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

        if (lastExpiredIt == firstExpiredIt)
        {
            break;
        }

        // transfer non-inflight MPDUs with expired lifetime to the tail of m_expiredQueue
        m_expiredQueue.splice(m_expiredQueue.end(), queue, firstExpiredIt, lastExpiredIt);
        ret->second = m_expiredQueue.end();

    } while (true);

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

std::ostream&
operator<<(std::ostream& os, WifiContainerQueueType queueType)
{
    switch (queueType)
    {
    case WIFI_CTL_QUEUE:
        return os << "CTL_QUEUE";
    case WIFI_MGT_QUEUE:
        return os << "MGT_QUEUE";
    case WIFI_QOSDATA_QUEUE:
        return os << "QOSDATA_QUEUE";
    case WIFI_DATA_QUEUE:
        return os << "DATA_QUEUE";
    }
    return os << "UNKNOWN(" << static_cast<uint16_t>(queueType) << ")";
}

std::ostream&
operator<<(std::ostream& os, WifiRcvAddr rcvAddrType)
{
    switch (rcvAddrType)
    {
    case WifiRcvAddr::UNICAST:
        return os << "UNICAST";
    case WifiRcvAddr::BROADCAST:
        return os << "BROADCAST";
    case WifiRcvAddr::GROUPCAST:
        return os << "GROUPCAST";
    case WifiRcvAddr::COUNT:
        return os << "COUNT";
    }
    return os << "UNKNOWN(" << static_cast<uint16_t>(rcvAddrType) << ")";
}

std::ostream&
operator<<(std::ostream& os, const WifiContainerQueueId& queueId)
{
    os << "{" << queueId.type << ", " << queueId.addrType;
    if (const auto& addr1 = queueId.addr1)
    {
        os << ", addr1=" << addr1.value();
    }
    if (const auto& addr2 = queueId.addr2)
    {
        os << ", addr2=" << addr2.value();
    }
    if (const auto& tid = queueId.tid)
    {
        os << ", tid=" << +tid.value();
    }
    return os << "}";
}

} // namespace ns3

/****************************************************
 *      Global Functions (outside namespace ns3)
 ***************************************************/

std::size_t
std::hash<ns3::WifiContainerQueueId>::operator()(ns3::WifiContainerQueueId queueId) const
{
    std::vector<uint8_t> buffer;
    buffer.reserve(1 + 1 + 6 + 6 + 1); // reserve the maximum possible size of the buffer
    buffer.emplace_back(queueId.type);
    buffer.emplace_back(static_cast<uint8_t>(queueId.addrType));
    if (queueId.addr1.has_value())
    {
        const auto size = buffer.size();
        buffer.resize(size + 6);
        queueId.addr1.value().CopyTo(&buffer[size]);
    }
    if (queueId.addr2.has_value())
    {
        const auto size = buffer.size();
        buffer.resize(size + 6);
        queueId.addr2.value().CopyTo(&buffer[size]);
    }
    if (queueId.tid.has_value())
    {
        buffer.emplace_back(*queueId.tid);
    }

    std::string s(buffer.begin(), buffer.end());
    return std::hash<std::string>{}(s);
}
