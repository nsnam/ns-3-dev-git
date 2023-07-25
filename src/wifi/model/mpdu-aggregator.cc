/*
 * Copyright (c) 2013
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Ghada Badawy <gbadawy@gmail.com>
 *         Stefano Avallone <stavallo@unina.it>
 */

#include "mpdu-aggregator.h"

#include "ampdu-subframe-header.h"
#include "ap-wifi-mac.h"
#include "ctrl-headers.h"
#include "gcr-manager.h"
#include "msdu-aggregator.h"
#include "qos-txop.h"
#include "wifi-mac-trailer.h"
#include "wifi-mpdu.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"
#include "wifi-remote-station-manager.h"
#include "wifi-tx-parameters.h"
#include "wifi-tx-vector.h"

#include "ns3/eht-capabilities.h"
#include "ns3/he-capabilities.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/vht-capabilities.h"

NS_LOG_COMPONENT_DEFINE("MpduAggregator");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(MpduAggregator);

TypeId
MpduAggregator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MpduAggregator")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MpduAggregator>();
    return tid;
}

void
MpduAggregator::DoDispose()
{
    m_mac = nullptr;
    m_htFem = nullptr;
    Object::DoDispose();
}

void
MpduAggregator::SetWifiMac(const Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
    m_htFem = DynamicCast<HtFrameExchangeManager>(m_mac->GetFrameExchangeManager(m_linkId));
}

void
MpduAggregator::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
    if (m_mac)
    {
        m_htFem = DynamicCast<HtFrameExchangeManager>(m_mac->GetFrameExchangeManager(m_linkId));
    }
}

void
MpduAggregator::Aggregate(Ptr<const WifiMpdu> mpdu, Ptr<Packet> ampdu, bool isSingle)
{
    NS_LOG_FUNCTION(mpdu << ampdu << isSingle);
    NS_ASSERT(ampdu);
    // if isSingle is true, then ampdu must be empty
    NS_ASSERT(!isSingle || ampdu->GetSize() == 0);

    // pad the previous A-MPDU subframe if the A-MPDU is not empty
    if (ampdu->GetSize() > 0)
    {
        uint8_t padding = CalculatePadding(ampdu->GetSize());

        if (padding)
        {
            Ptr<Packet> pad = Create<Packet>(padding);
            ampdu->AddAtEnd(pad);
        }
    }

    // add MPDU header and trailer
    Ptr<Packet> tmp = mpdu->GetPacket()->Copy();
    tmp->AddHeader(mpdu->GetHeader());
    AddWifiMacTrailer(tmp);

    // add A-MPDU subframe header and MPDU to the A-MPDU
    AmpduSubframeHeader hdr =
        GetAmpduSubframeHeader(static_cast<uint16_t>(tmp->GetSize()), isSingle);

    tmp->AddHeader(hdr);
    ampdu->AddAtEnd(tmp);
}

uint32_t
MpduAggregator::GetSizeIfAggregated(uint32_t mpduSize, uint32_t ampduSize)
{
    NS_LOG_FUNCTION(mpduSize << ampduSize);

    return ampduSize + CalculatePadding(ampduSize) + 4 + mpduSize;
}

uint32_t
MpduAggregator::GetMaxAmpduSize(Mac48Address recipient,
                                uint8_t tid,
                                WifiModulationClass modulation) const
{
    NS_LOG_FUNCTION(this << recipient << +tid << modulation);

    if (auto apMac = DynamicCast<ApWifiMac>(m_mac);
        IsGroupcast(recipient) && (m_mac->GetTypeOfStation() == AP) && apMac->GetGcrManager())
    {
        recipient = apMac->GetGcrManager()->GetIndividuallyAddressedRecipient(recipient);
    }

    AcIndex ac = QosUtilsMapTidToAc(tid);
    // Find the A-MPDU max size configured on this device
    uint32_t maxAmpduSize = m_mac->GetMaxAmpduSize(ac);

    if (maxAmpduSize == 0)
    {
        NS_LOG_DEBUG("A-MPDU Aggregation is disabled on this station for " << ac);
        return 0;
    }

    Ptr<WifiRemoteStationManager> stationManager = m_mac->GetWifiRemoteStationManager(m_linkId);
    NS_ASSERT(stationManager);

    // Retrieve the Capabilities elements advertised by the recipient
    auto ehtCapabilities = stationManager->GetStationEhtCapabilities(recipient);
    auto heCapabilities = stationManager->GetStationHeCapabilities(recipient);
    auto he6GhzCapabilities = stationManager->GetStationHe6GhzCapabilities(recipient);
    auto vhtCapabilities = stationManager->GetStationVhtCapabilities(recipient);
    auto htCapabilities = stationManager->GetStationHtCapabilities(recipient);

    // Determine the constraint imposed by the recipient based on the PPDU
    // format used to transmit the A-MPDU
    if (modulation >= WIFI_MOD_CLASS_EHT)
    {
        NS_ABORT_MSG_IF(!ehtCapabilities,
                        "EHT Capabilities element not received for " << recipient);

        maxAmpduSize = std::min(maxAmpduSize, ehtCapabilities->GetMaxAmpduLength());
    }
    else if (modulation >= WIFI_MOD_CLASS_HE)
    {
        NS_ABORT_MSG_IF(!heCapabilities, "HE Capabilities element not received for " << recipient);

        maxAmpduSize = std::min(maxAmpduSize, heCapabilities->GetMaxAmpduLength());
        if (he6GhzCapabilities)
        {
            maxAmpduSize = std::min(maxAmpduSize, he6GhzCapabilities->GetMaxAmpduLength());
        }
    }
    else if (modulation == WIFI_MOD_CLASS_VHT)
    {
        NS_ABORT_MSG_IF(!vhtCapabilities,
                        "VHT Capabilities element not received for " << recipient);

        maxAmpduSize = std::min(maxAmpduSize, vhtCapabilities->GetMaxAmpduLength());
    }
    else if (modulation == WIFI_MOD_CLASS_HT)
    {
        NS_ABORT_MSG_IF(!htCapabilities, "HT Capabilities element not received for " << recipient);

        maxAmpduSize = std::min(maxAmpduSize, htCapabilities->GetMaxAmpduLength());
    }
    else // non-HT PPDU
    {
        NS_LOG_DEBUG("A-MPDU aggregation is not available for non-HT PHYs");

        maxAmpduSize = 0;
    }

    return maxAmpduSize;
}

uint8_t
MpduAggregator::CalculatePadding(uint32_t ampduSize)
{
    return (4 - (ampduSize % 4)) % 4;
}

AmpduSubframeHeader
MpduAggregator::GetAmpduSubframeHeader(uint16_t mpduSize, bool isSingle)
{
    AmpduSubframeHeader hdr;
    hdr.SetLength(mpduSize);
    if (isSingle)
    {
        hdr.SetEof(true);
    }
    return hdr;
}

std::vector<Ptr<WifiMpdu>>
MpduAggregator::GetNextAmpdu(Ptr<WifiMpdu> mpdu,
                             WifiTxParameters& txParams,
                             Time availableTime) const
{
    NS_LOG_FUNCTION(this << *mpdu << &txParams << availableTime);

    std::vector<Ptr<WifiMpdu>> mpduList;

    const auto& header = mpdu->GetHeader();
    const auto recipient = GetIndividuallyAddressedRecipient(m_mac, header);
    NS_ASSERT(header.IsQosData() && !recipient.IsBroadcast());

    const auto& origAddr1 = mpdu->GetOriginal()->GetHeader().GetAddr1();
    auto origRecipient = GetIndividuallyAddressedRecipient(m_mac, mpdu->GetOriginal()->GetHeader());

    const auto tid = header.GetQosTid();
    auto qosTxop = m_mac->GetQosTxop(tid);
    NS_ASSERT(qosTxop);

    const auto isGcr = IsGcr(m_mac, header);
    const auto bufferSize = qosTxop->GetBaBufferSize(origRecipient, tid, isGcr);
    const auto startSeq = qosTxop->GetBaStartingSequence(origRecipient, tid, isGcr);

    // Have to make sure that the block ack agreement is established and A-MPDU is enabled
    auto apMac = DynamicCast<ApWifiMac>(m_mac);
    const auto agreementEstablished =
        isGcr ? apMac->IsGcrBaAgreementEstablishedWithAllMembers(header.GetAddr1(), tid)
              : m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid).has_value();
    if (agreementEstablished &&
        GetMaxAmpduSize(recipient, tid, txParams.m_txVector.GetModulationClass()) > 0)
    {
        /* here is performed MPDU aggregation */
        Ptr<WifiMpdu> nextMpdu = mpdu;

        while (nextMpdu)
        {
            const auto isGcrUr = isGcr && (apMac->GetGcrManager()->GetRetransmissionPolicy() ==
                                           GroupAddressRetransmissionPolicy::GCR_UNSOLICITED_RETRY);
            if (isGcrUr && header.IsRetry() && !nextMpdu->GetHeader().IsRetry())
            {
                // if this is a retransmitted A-MPDU transmitted via GCR-UR, do not add new MPDU
                break;
            }
            if (isGcr &&
                apMac->GetGcrManager()->GetRetransmissionPolicyFor(header) !=
                    apMac->GetGcrManager()->GetRetransmissionPolicyFor(nextMpdu->GetHeader()))
            {
                // if an MPDU has been previously transmitted using No-Ack/No-Retry,
                // do not add a new MPDU that still needs to be transmitted using No-Ack/No-Retry,
                // unless No-Ack/No-Retry is the only selected retransmission policy
                break;
            }

            // if we are here, nextMpdu can be aggregated to the A-MPDU.
            NS_LOG_DEBUG("Adding packet with sequence number "
                         << nextMpdu->GetHeader().GetSequenceNumber()
                         << " to A-MPDU, packet size = " << nextMpdu->GetSize()
                         << ", A-MPDU size = " << txParams.GetSize(recipient));

            mpduList.push_back(nextMpdu);

            // If allowed by the BA agreement, get the next MPDU
            auto peekedMpdu =
                qosTxop->PeekNextMpdu(m_linkId, tid, origAddr1, nextMpdu->GetOriginal());
            nextMpdu = nullptr;

            if (peekedMpdu)
            {
                // PeekNextMpdu() does not return an MPDU that is beyond the transmit window
                NS_ASSERT(
                    IsInWindow(peekedMpdu->GetHeader().GetSequenceNumber(), startSeq, bufferSize));

                peekedMpdu = m_htFem->CreateAliasIfNeeded(peekedMpdu);
                // get the next MPDU to aggregate, provided that the constraints on size
                // and duration limit are met. Note that the returned MPDU differs from
                // the peeked MPDU if A-MSDU aggregation is enabled.
                NS_LOG_DEBUG("Trying to aggregate another MPDU");
                nextMpdu =
                    qosTxop->GetNextMpdu(m_linkId, peekedMpdu, txParams, availableTime, false);
            }
        }

        if (mpduList.size() == 1)
        {
            // return an empty vector if it was not possible to aggregate at least two MPDUs
            mpduList.clear();
        }
    }

    return mpduList;
}

} // namespace ns3
