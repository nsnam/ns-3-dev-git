/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "dso-manager.h"

#include "uhr-configuration.h"
#include "uhr-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-operation.h"
#include "ns3/he-operation.h"
#include "ns3/ht-operation.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/vht-configuration.h"
#include "ns3/vht-operation.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ns3-constants.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsoManager");

NS_OBJECT_ENSURE_REGISTERED(DsoManager);

TypeId
DsoManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DsoManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute("DsoSwitchBackDelay",
                          "The DSO Switch Back Delay.",
                          TimeValue(MicroSeconds(16 /* aSIFSTime */ + 9 /* aSlotTime */ +
                                                 20 /* aRxPHYStartDelay */ +
                                                 DEFAULT_CHANNEL_SWITCH_DELAY_USEC)),
                          MakeTimeAccessor(&DsoManager::m_dsoSwitchBackDelay),
                          MakeTimeChecker())
            .AddTraceSource("DsoTxopEvent",
                            "Trace source indicating a DSO TXOP event.",
                            MakeTraceSourceAccessor(&DsoManager::m_dsoTxopEventTrace),
                            "ns3::DsoManager::DsoTxopEventTracedCallback");
    return tid;
}

DsoManager::DsoManager()
{
    NS_LOG_FUNCTION(this);
}

DsoManager::~DsoManager()
{
    NS_LOG_FUNCTION(this);
}

void
DsoManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_staMac = nullptr;
    Object::DoDispose();
}

void
DsoManager::SetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ASSERT(mac);
    m_staMac = mac;

    NS_ABORT_MSG_IF(!m_staMac->GetUhrConfiguration(), "DsoManager requires UHR support");
    NS_ABORT_MSG_IF(!m_staMac->GetUhrConfiguration()->GetDsoActivated(),
                    "DsoManager requires DSO to be activated");
    NS_ABORT_MSG_IF(m_staMac->GetTypeOfStation() != STA,
                    "DsoManager can only be installed on non-AP MLDs");
}

Ptr<StaWifiMac>
DsoManager::GetStaMac() const
{
    return m_staMac;
}

Ptr<UhrFrameExchangeManager>
DsoManager::GetUhrFem(uint8_t linkId) const
{
    return StaticCast<UhrFrameExchangeManager>(m_staMac->GetFrameExchangeManager(linkId));
}

void
DsoManager::NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    if (const auto& hdr = mpdu->GetHeader();
        (hdr.IsAssocResp() || hdr.IsReassocResp()) && GetStaMac()->IsAssociated())
    {
        MgtAssocResponseHeader assocResp;
        mpdu->GetPacket()->PeekHeader(assocResp);
        // TODO: DSO currently only supports a single link, for MLD we need to read the Multi-Link
        // Element
        ComputeSubbands(linkId, assocResp);
    }
}

void
DsoManager::NotifyIcfReceived(uint8_t linkId, std::optional<WifiRu::RuSpec> ru)
{
    if (!ru)
    {
        NS_LOG_FUNCTION(this << linkId);
        m_staMac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::DSO_WAIT_FOR_TF,
                                        GetUhrFem(linkId)->GetBssid(),
                                        {linkId});
        return;
    }

    NS_LOG_FUNCTION(this << linkId << *ru);
    m_dsoTxopEventTrace(DsoTxopEvent::RX_ICF, linkId);
    m_staMac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::DSO_WAIT_FOR_TF,
                                      GetUhrFem(linkId)->GetBssid(),
                                      {linkId});

    if (m_dsoSubbands.empty())
    {
        // nothing to do, we do not have any DSO subband to switch to
        return;
    }

    NS_ASSERT_MSG(!IsOnDsoSubband(linkId),
                  "No ICF should be received when already operating on a DSO subband");

    const auto& phy = m_staMac->GetWifiPhy(linkId);
    const auto& currentChannel = phy->GetOperatingChannel();
    NS_ASSERT(WifiRu::IsEht(*ru) && (currentChannel.GetWidth() < MHz_t{320}));
    auto ehtRu = std::get<EhtRu::RuSpec>(*ru);
    if (ehtRu.GetPrimary160MHz() &&
        (ehtRu.GetPrimary80MHzOrLower80MHz() || currentChannel.GetWidth() == MHz_t{160}))

    {
        // nothing to do, we received an ICF with RU assigned in main channel
        return;
    }

    const auto staChannelWidth = phy->GetChannelWidth();
    const auto& dsoSubbands = GetDsoSubbands(linkId);
    const auto itDsoSubband =
        dsoSubbands.find({ehtRu.GetPrimary160MHz(),
                          (staChannelWidth > MHz_t{80}) || ehtRu.GetPrimary80MHzOrLower80MHz()});
    NS_ASSERT_MSG(itDsoSubband != dsoSubbands.cend(), "No DSO subband found for the given RU spec");
    SwitchPhyChannel(linkId, itDsoSubband->second, GetSwitchingDelayToDsoSubband());
}

void
DsoManager::NotifyIcrTransmitted(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    m_dsoTxopEventTrace(DsoTxopEvent::TX_ICR, linkId);
}

void
DsoManager::NotifyTxopEnd(uint8_t linkId,
                          bool failedResponse,
                          Ptr<const WifiPsdu> psdu,
                          std::optional<std::reference_wrapper<const WifiTxVector>> txVector)
{
    NS_LOG_FUNCTION(this << linkId << failedResponse << psdu);

    if (!m_staMac->IsAssociated())
    {
        return;
    }

    m_staMac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::DSO_WAIT_FOR_TF,
                                    GetUhrFem(linkId)->GetBssid(),
                                    {linkId});

    auto phy = m_staMac->GetWifiPhy(linkId);
    if (psdu)
    {
        NS_ASSERT(txVector);
        if (const auto& hdr = psdu->GetHeader(0); hdr.IsCfEnd())
        {
            m_dsoTxopEventTrace(DsoTxopEvent::RX_CF_END, linkId);
        }
        else if (!GetUhrFem(linkId)->IsIntraBssPpdu(psdu->GetHeader(0), *txVector))
        {
            m_dsoTxopEventTrace(DsoTxopEvent::RX_OBSS, linkId);
        }
        else
        {
            m_dsoTxopEventTrace(DsoTxopEvent::RX_OTHER, linkId);
        }
    }
    else if (BooleanValue earlyTxopEndDetect;
             GetUhrFem(linkId)->GetAttributeFailSafe("EarlyTxopEndDetect", earlyTxopEndDetect) &&
             earlyTxopEndDetect.Get())
    {
        m_dsoTxopEventTrace(DsoTxopEvent::DURATION_DETECT_END, linkId);
    }
    else
    {
        m_dsoTxopEventTrace(failedResponse ? DsoTxopEvent::FAILED_RESPONSE : DsoTxopEvent::TIMEOUT,
                            linkId);
    }

    if (!IsOnDsoSubband(linkId))
    {
        return;
    }

    SwitchPhyChannel(linkId, GetPrimarySubband(linkId), phy->GetChannelSwitchDelay());
}

void
DsoManager::SwitchPhyChannel(uint8_t linkId,
                             const WifiPhyOperatingChannel& channel,
                             const Time& delay)
{
    NS_LOG_FUNCTION(this << linkId << channel << delay);

    auto phy = m_staMac->GetWifiPhy(linkId);
    const auto initialDelay = phy->GetChannelSwitchDelay();

    // switch channel within the provided delay
    phy->SetAttribute("ChannelSwitchDelay", TimeValue(delay));
    GetUhrFem(linkId)->NotifyDsoSwitching();
    phy->SetOperatingChannel(channel);

    // restore previous channel switch delay
    phy->SetAttribute("ChannelSwitchDelay", TimeValue(initialDelay));
}

void
DsoManager::ComputeSubbands(uint8_t linkId, const MgtAssocResponseHeader& assocResp)
{
    NS_LOG_FUNCTION(this << linkId << assocResp);

    const auto phy = m_staMac->GetWifiPhy(linkId);
    const auto rsm = m_staMac->GetWifiRemoteStationManager(linkId);
    const auto bssid = GetUhrFem(linkId)->GetBssid();
    const auto apStandard = rsm->GetStandard(bssid);
    const auto apBw = rsm->GetChannelWidthSupported(bssid);
    const auto apChannel = GetApOperatingChannel(apBw,
                                                 phy->GetPrimaryChannelNumber(MHz_t{20}),
                                                 phy->GetPhyBand(),
                                                 apStandard,
                                                 assocResp);
    NS_LOG_DEBUG("AP operating on channel " << apChannel << " for link " << +linkId);

    ComputeSubbands(linkId, apChannel);
}

void
DsoManager::ComputeSubbands(uint8_t linkId, const WifiPhyOperatingChannel& apChannel)
{
    NS_LOG_FUNCTION(this << linkId << apChannel);

    m_primarySubbands.erase(linkId);
    m_dsoSubbands.erase(linkId);

    auto phy = m_staMac->GetWifiPhy(linkId);
    const auto staChannelWidth = phy->GetChannelWidth();
    const auto& mainChannel = phy->GetOperatingChannel();
    NS_LOG_DEBUG("Main operating channel is " << mainChannel << " for link " << +linkId);
    m_primarySubbands.emplace(linkId, mainChannel);

    const auto phyBand = mainChannel.GetPhyBand();
    NS_ASSERT_MSG((phyBand == WIFI_PHY_BAND_5GHZ) || (phyBand == WIFI_PHY_BAND_6GHZ),
                  "DSO is only supported in the 5 GHz and 6 GHz bands");

    const auto bssid = GetUhrFem(linkId)->GetBssid();
    const auto apBw =
        m_staMac->GetWifiRemoteStationManager(linkId)->GetChannelWidthSupported(bssid);

    if ((staChannelWidth < MHz_t{80}) || (apBw < MHz_t{160}) || (apBw <= staChannelWidth))
    {
        NS_LOG_DEBUG("No DSO subband for link " << +linkId);
        return;
    }

    const std::size_t numSubbands = apBw / staChannelWidth;
    for (std::size_t i = 0; i < numSubbands; ++i)
    {
        // retrieve DSO subband
        WifiPhyOperatingChannel subChannel;
        WifiPhyOperatingChannel dsoSubband;
        if (i < 2)
        {
            subChannel = apChannel.GetPrimaryChannel(staChannelWidth * 2);
        }
        else
        {
            subChannel = apChannel.GetSecondaryChannel(staChannelWidth * 2);
        }
        if ((i % 2) == 0)
        {
            dsoSubband = subChannel.GetPrimaryChannel(staChannelWidth);
        }
        else
        {
            dsoSubband = subChannel.GetSecondaryChannel(staChannelWidth);
        }
        const auto p160 = (apBw == MHz_t{160}) || (i < (numSubbands / 2));
        // When is i >= 2, we are in the S160, which means the primary 20 index is 0 because
        // GetSecondaryChannel does not affect the default value. Hence, if i == 2, we know this is
        // the lowest 80 MHz subband since the primary 80MHz of a 160 MHz channel with primary index
        // of 0 will always return the lowest 80 MHz subchannel.
        const auto p80orLower80 = staChannelWidth > MHz_t{80} || (i % 2 == 0);
        if (dsoSubband == mainChannel)
        {
            // skip main channel
            continue;
        }
        NS_LOG_DEBUG("Add DSO subband " << dsoSubband << " for link " << +linkId);
        m_dsoSubbands[linkId].emplace(std::make_pair(p160, p80orLower80), dsoSubband);

        // make sure PHY interface of DSO subband is tracking signals
        auto spectrumPhy = DynamicCast<SpectrumWifiPhy>(phy);
        NS_ASSERT_MSG(spectrumPhy, "Spectrum PHY should be used for DSO");
        spectrumPhy->ConfigureInterface(dsoSubband.GetFrequencies(), dsoSubband.GetWidth());
    }
}

const WifiPhyOperatingChannel&
DsoManager::GetPrimarySubband(uint8_t linkId) const
{
    const auto it = m_primarySubbands.find(linkId);
    NS_ASSERT_MSG(it != m_primarySubbands.cend(),
                  "Primary subband for PHY on link ID " << +linkId << " not found");
    return it->second;
}

const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>&
DsoManager::GetDsoSubbands(uint8_t linkId) const
{
    const auto it = m_dsoSubbands.find(linkId);
    NS_ASSERT_MSG(it != m_dsoSubbands.end(),
                  "DSO subband for PHY on link ID " << +linkId << " not found");
    return it->second;
}

bool
DsoManager::IsOnDsoSubband(uint8_t linkId) const
{
    return m_staMac->GetWifiPhy(linkId)->GetOperatingChannel() != GetPrimarySubband(linkId);
}

std::ostream&
operator<<(std::ostream& os, DsoTxopEvent reason)
{
    switch (reason)
    {
    case DsoTxopEvent::RX_ICF:
        return (os << "RX_ICF");
    case DsoTxopEvent::TX_ICR:
        return (os << "TX_ICR");
    case DsoTxopEvent::TIMEOUT:
        return (os << "TIMEOUT");
    case DsoTxopEvent::RX_CF_END:
        return (os << "RX_CF_END");
    case DsoTxopEvent::RX_OBSS:
        return (os << "RX_OBSS");
    case DsoTxopEvent::RX_OTHER:
        return (os << "RX_OTHER");
    case DsoTxopEvent::FAILED_RESPONSE:
        return (os << "FAILED_RESPONSE");
    default:
        NS_FATAL_ERROR("Unknown reason");
        return (os << "UNKNOWN");
    }
}

} // namespace ns3
