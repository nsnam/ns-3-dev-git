/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 *         Stefano Avallone <stavallo@unina.it>
 */

#include "msdu-aggregator.h"

#include "ap-wifi-mac.h"
#include "gcr-manager.h"
#include "qos-txop.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-remote-station-manager.h"
#include "wifi-tx-parameters.h"

#include "ns3/ht-capabilities.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/packet.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MsduAggregator");

NS_OBJECT_ENSURE_REGISTERED(MsduAggregator);

TypeId
MsduAggregator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MsduAggregator")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MsduAggregator>();
    return tid;
}

void
MsduAggregator::DoDispose()
{
    m_mac = nullptr;
    m_htFem = nullptr;
    Object::DoDispose();
}

void
MsduAggregator::SetWifiMac(const Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
    m_htFem = DynamicCast<HtFrameExchangeManager>(m_mac->GetFrameExchangeManager(m_linkId));
}

void
MsduAggregator::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
    if (m_mac)
    {
        m_htFem = DynamicCast<HtFrameExchangeManager>(m_mac->GetFrameExchangeManager(m_linkId));
    }
}

uint16_t
MsduAggregator::GetSizeIfAggregated(uint16_t msduSize, uint16_t amsduSize)
{
    NS_LOG_FUNCTION(msduSize << amsduSize);

    // the size of the A-MSDU subframe header is 14 bytes: DA (6), SA (6) and Length (2)
    return amsduSize + CalculatePadding(amsduSize) + 14 + msduSize;
}

Ptr<WifiMpdu>
MsduAggregator::GetNextAmsdu(Ptr<WifiMpdu> peekedItem,
                             WifiTxParameters& txParams,
                             Time availableTime) const
{
    NS_LOG_FUNCTION(this << *peekedItem << &txParams << availableTime);

    /* "The Address 1 field of an MPDU carrying an A-MSDU shall be set to an
     * individual address or to the GCR concealment address" (Section 10.12
     * of 802.11-2016)
     */
    const auto& header = peekedItem->GetOriginal()->GetHeader();
    const auto recipient = GetIndividuallyAddressedRecipient(m_mac, header);
    NS_ABORT_MSG_IF(recipient.IsBroadcast(), "Recipient address is broadcast");

    /* "A STA shall not transmit an A-MSDU within a QoS Data frame under a block
     * ack agreement unless the recipient indicates support for A-MSDU by setting
     * the A-MSDU Supported field to 1 in its BlockAck Parameter Set field of the
     * ADDBA Response frame" (Section 10.12 of 802.11-2016)
     */
    // No check required for now, as we always set the A-MSDU Supported field to 1

    // TODO Add support for the Max Number Of MSDUs In A-MSDU field in the Extended
    // Capabilities element sent by the recipient

    NS_ASSERT(m_htFem);

    auto queue = m_mac->GetTxopQueue(peekedItem->GetQueueAc());

    // if GCR, A-MSDU is always used with a single A-MSDU subframe
    if (IsGcr(m_mac, header))
    {
        auto apMac = DynamicCast<ApWifiMac>(m_mac);
        NS_ASSERT(apMac);
        auto gcrManager = apMac->GetGcrManager();
        if (!gcrManager->UseConcealment(peekedItem->GetHeader()))
        {
            return nullptr;
        }
        auto msdu = peekedItem->GetOriginal();
        auto gcrAmsdu =
            Create<WifiMpdu>(msdu->GetPacket(), msdu->GetHeader(), msdu->GetTimestamp());
        gcrAmsdu->Aggregate(nullptr);
        queue->Replace(msdu, gcrAmsdu);
        if (msdu->GetHeader().IsRetry())
        {
            gcrAmsdu->AssignSeqNo(msdu->GetHeader().GetSequenceNumber());
        }
        return m_htFem->CreateAliasIfNeeded(gcrAmsdu);
    }
    else if (IsGroupcast(recipient))
    {
        NS_LOG_DEBUG("No A-MSDU aggregation for groupcast frames without GCR service");
        return nullptr;
    }

    const auto tid = header.GetQosTid();
    if (GetMaxAmsduSize(recipient, tid, txParams.m_txVector.GetModulationClass()) == 0)
    {
        NS_LOG_DEBUG("A-MSDU aggregation disabled");
        return nullptr;
    }

    // perform A-MSDU aggregation
    auto amsdu = queue->GetOriginal(peekedItem);
    std::size_t nMsdu = 1;
    peekedItem = queue->PeekByTidAndAddress(tid, recipient, peekedItem->GetOriginal());

    // stop aggregation if we find an A-MSDU in the queue. This likely happens when an A-MSDU is
    // prepared but not transmitted due to RTS/CTS failure
    while (peekedItem && !peekedItem->GetHeader().IsQosAmsdu() &&
           m_htFem->TryAggregateMsdu(peekedItem = m_htFem->CreateAliasIfNeeded(peekedItem),
                                     txParams,
                                     availableTime))
    {
        NS_ASSERT_MSG(!peekedItem->HasSeqNoAssigned(),
                      "Found item with sequence number assignment after one without: perhaps "
                      "sequence numbers were not released correctly?");
        // find the next MPDU before dequeuing the current one
        auto msdu = peekedItem->GetOriginal();
        peekedItem = queue->PeekByTidAndAddress(tid, recipient, msdu);
        queue->DequeueIfQueued({amsdu});
        // perform A-MSDU aggregation
        amsdu->Aggregate(msdu);
        queue->Replace(msdu, amsdu);

        nMsdu++;
    }

    if (nMsdu == 1)
    {
        NS_LOG_DEBUG("Aggregation failed (could not aggregate at least two MSDUs)");
        return nullptr;
    }

    // Aggregation succeeded
    return m_htFem->CreateAliasIfNeeded(amsdu);
}

uint8_t
MsduAggregator::CalculatePadding(uint16_t amsduSize)
{
    return (4 - (amsduSize % 4)) % 4;
}

uint16_t
MsduAggregator::GetMaxAmsduSize(Mac48Address recipient,
                                uint8_t tid,
                                WifiModulationClass modulation) const
{
    NS_LOG_FUNCTION(this << recipient << +tid << modulation);

    AcIndex ac = QosUtilsMapTidToAc(tid);

    // Find the A-MSDU max size configured on this device
    uint16_t maxAmsduSize = m_mac->GetMaxAmsduSize(ac);

    if (maxAmsduSize == 0)
    {
        NS_LOG_DEBUG("A-MSDU Aggregation is disabled on this station for " << ac);
        return 0;
    }

    Ptr<WifiRemoteStationManager> stationManager = m_mac->GetWifiRemoteStationManager(m_linkId);
    NS_ASSERT(stationManager);

    // Retrieve the Capabilities elements advertised by the recipient
    auto ehtCapabilities = stationManager->GetStationEhtCapabilities(recipient);
    auto he6GhzCapabilities = stationManager->GetStationHe6GhzCapabilities(recipient);
    auto vhtCapabilities = stationManager->GetStationVhtCapabilities(recipient);
    auto htCapabilities = stationManager->GetStationHtCapabilities(recipient);

    // Determine the maximum MPDU size, which is used to indirectly constrain the maximum
    // A-MSDU size in some cases (see below). The maximum MPDU size is advertised
    // in the EHT Capabilities element, for the 2.4 GHz band, or in the VHT Capabilities
    // element, otherwise.
    uint16_t maxMpduSize = 0;
    if (ehtCapabilities && m_mac->GetWifiPhy(m_linkId)->GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
    {
        maxMpduSize = ehtCapabilities->GetMaxMpduLength();
    }
    else if (he6GhzCapabilities && m_mac->Is6GhzBand(m_linkId))
    {
        maxMpduSize = he6GhzCapabilities->GetMaxMpduLength();
    }
    else if (vhtCapabilities && m_mac->GetWifiPhy(m_linkId)->GetPhyBand() != WIFI_PHY_BAND_2_4GHZ)
    {
        maxMpduSize = vhtCapabilities->GetMaxMpduLength();
    }

    if (!htCapabilities && !he6GhzCapabilities)
    {
        /* "A non-DMG STA shall not transmit an A-MSDU to a STA from which it has
         * not received a frame containing an HT Capabilities element" (Section
         * 10.12 of 802.11-2016)
         */
        NS_LOG_DEBUG("A-MSDU Aggregation disabled because the recipient did not"
                     " send an HT Capabilities element");
        return 0;
    }

    // Determine the constraint imposed by the recipient based on the PPDU
    // format used to transmit the A-MSDU
    if (modulation >= WIFI_MOD_CLASS_EHT)
    {
        // the maximum A-MSDU size is indirectly constrained by the maximum MPDU size
        // supported by the recipient (see Table 9-34 of 802.11be D2.0)
        NS_ABORT_MSG_IF(maxMpduSize == 0, "Max MPDU size not advertised");
        maxAmsduSize = std::min(maxAmsduSize, static_cast<uint16_t>(maxMpduSize - 56));
    }
    else if (modulation == WIFI_MOD_CLASS_HE)
    {
        // for a non-EHT STA operating in the 2.4 GHz band, the maximum A-MSDU size is
        // advertised in the HT Capabilities element. Otherwise, the maximum A-MSDU size is
        // indirectly constrained by the maximum MPDU size supported by the recipient
        // (see Table 9-34 of 802.11be D2.0)
        if (m_mac->GetWifiPhy(m_linkId)->GetStandard() < WIFI_STANDARD_80211be &&
            m_mac->GetWifiPhy(m_linkId)->GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
        {
            maxAmsduSize = std::min(maxAmsduSize, htCapabilities->GetMaxAmsduLength());
        }
        else
        {
            NS_ABORT_MSG_IF(maxMpduSize == 0, "Max MPDU size not advertised");
            maxAmsduSize = std::min(maxAmsduSize, static_cast<uint16_t>(maxMpduSize - 56));
        }
    }
    else if (modulation == WIFI_MOD_CLASS_VHT)
    {
        // the maximum A-MSDU size is indirectly constrained by the maximum MPDU
        // size supported by the recipient and advertised in the VHT Capabilities
        // element (see Table 9-25 of 802.11-2020)
        NS_ABORT_MSG_IF(maxMpduSize == 0, "Max MPDU size not advertised");
        maxAmsduSize = std::min(maxAmsduSize, static_cast<uint16_t>(maxMpduSize - 56));
    }
    else if (modulation >= WIFI_MOD_CLASS_HT)
    {
        // the maximum A-MSDU size is constrained by the maximum A-MSDU size
        // supported by the recipient and advertised in the HT Capabilities
        // element (see Table 9-19 of 802.11-2016)
        maxAmsduSize = std::min(maxAmsduSize, htCapabilities->GetMaxAmsduLength());
    }
    else // non-HT PPDU
    {
        // the maximum A-MSDU size is indirectly constrained by the maximum PSDU size
        // supported by the recipient (see Table 9-19 of 802.11-2016)
        maxAmsduSize = std::min(maxAmsduSize, static_cast<uint16_t>(3839));
    }

    return maxAmsduSize;
}

WifiMpdu::DeaggregatedMsdus
MsduAggregator::Deaggregate(Ptr<Packet> aggregatedPacket)
{
    NS_LOG_FUNCTION_NOARGS();
    WifiMpdu::DeaggregatedMsdus set;

    AmsduSubframeHeader hdr;
    Ptr<Packet> extractedMsdu = Create<Packet>();
    uint32_t maxSize = aggregatedPacket->GetSize();
    uint16_t extractedLength;
    uint8_t padding;
    uint32_t deserialized = 0;

    while (deserialized < maxSize)
    {
        deserialized += aggregatedPacket->RemoveHeader(hdr);
        extractedLength = hdr.GetLength();
        extractedMsdu = aggregatedPacket->CreateFragment(0, static_cast<uint32_t>(extractedLength));
        aggregatedPacket->RemoveAtStart(extractedLength);
        deserialized += extractedLength;

        padding = (4 - ((extractedLength + 14) % 4)) % 4;

        if (padding > 0 && deserialized < maxSize)
        {
            aggregatedPacket->RemoveAtStart(padding);
            deserialized += padding;
        }

        std::pair<Ptr<const Packet>, AmsduSubframeHeader> packetHdr(extractedMsdu, hdr);
        set.push_back(packetHdr);
    }
    NS_LOG_INFO("Deaggreated A-MSDU: extracted " << set.size() << " MSDUs");
    return set;
}

} // namespace ns3
