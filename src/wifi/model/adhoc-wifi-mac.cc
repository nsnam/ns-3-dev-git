/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "adhoc-wifi-mac.h"

#include "capability-information.h"
#include "edca-parameter-set.h"
#include "mgt-headers.h"
#include "qos-txop.h"
#include "supported-rates.h"
#include "wifi-phy.h"

#include "ns3/dsss-parameter-set.h"
#include "ns3/eht-capabilities.h"
#include "ns3/eht-operation.h"
#include "ns3/erp-information.h"
#include "ns3/he-capabilities.h"
#include "ns3/he-configuration.h"
#include "ns3/he-operation.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-configuration.h"
#include "ns3/ht-operation.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/vht-capabilities.h"
#include "ns3/vht-operation.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdhocWifiMac");

NS_OBJECT_ENSURE_REGISTERED(AdhocWifiMac);

TypeId
AdhocWifiMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AdhocWifiMac")
            .SetParent<WifiMac>()
            .SetGroupName("Wifi")
            .AddConstructor<AdhocWifiMac>()
            .AddAttribute("BeaconInterval",
                          "Delay between two beacons",
                          TimeValue(MicroSeconds(102400)),
                          MakeTimeAccessor(&AdhocWifiMac::GetBeaconInterval,
                                           &AdhocWifiMac::SetBeaconInterval),
                          MakeTimeChecker())
            .AddAttribute("BeaconGeneration",
                          "Whether or not beacons are generated.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdhocWifiMac::SetBeaconGeneration),
                          MakeBooleanChecker())
            .AddAttribute("BeaconAc",
                          "The Access Category to use for beacons.",
                          EnumValue(AcIndex::AC_VI),
                          MakeEnumAccessor<AcIndex>(&AdhocWifiMac::m_beaconAc),
                          MakeEnumChecker(AcIndex::AC_BE,
                                          "AC_BE",
                                          AcIndex::AC_VI,
                                          "AC_VI",
                                          AcIndex::AC_VO,
                                          "AC_VO",
                                          AcIndex::AC_BK,
                                          "AC_BK"))
            .AddAttribute("EmlsrPeer",
                          "Whether the peer STA operates in EMLSR mode.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdhocWifiMac::m_emlsrPeer),
                          MakeBooleanChecker())
            .AddAttribute("EmlsrPeerPaddingDelay",
                          "The EMLSR Paddind Delay used by peer STA operating in EMLSR mode. "
                          "Possible values are 0 us, 32 us, 64 us, 128 us or 256 us.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&AdhocWifiMac::m_emlsrPeerPaddingDelay),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(256)))
            .AddAttribute("EmlsrPeerTransitionDelay",
                          "The EMLSR Transition Delay used by peer STA operating in EMLSR mode. "
                          "Possible values are 0 us, 16 us, 32 us, 64 us, 128 us or 256 us.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&AdhocWifiMac::m_emlsrPeerTransitionDelay),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(256)))
            .AddAttribute("EmlsrUpdateCwAfterFailedIcf",
                          "Whether CW shall be doubled upon CTS timeout after an MU-RTS sent "
                          "to an EMLSR client.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdhocWifiMac::m_emlsrUpdateCwAfterFailedIcf),
                          MakeBooleanChecker())
            .AddAttribute("EmlsrReportFailedIcf",
                          "Whether the failure of an ICF sent to an EMLSR client should be "
                          "reported to the remote station manager.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdhocWifiMac::m_emlsrReportFailedIcf),
                          MakeBooleanChecker());
    return tid;
}

AdhocWifiMac::AdhocWifiMac()
    : m_enableBeaconGeneration(false)
{
    NS_LOG_FUNCTION(this);
    // Let the lower layers know that we are acting in an IBSS
    SetTypeOfStation(ADHOC_STA);
}

AdhocWifiMac::~AdhocWifiMac()
{
    NS_LOG_FUNCTION(this);
}

void
AdhocWifiMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    if (m_enableBeaconGeneration)
    {
        m_beaconEvent.Cancel();
        NS_LOG_DEBUG("Start beaconing");
        m_beaconEvent = Simulator::ScheduleNow(&AdhocWifiMac::SendOneBeacon, this);
    }
    WifiMac::DoInitialize();
}

void
AdhocWifiMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_enableBeaconGeneration = false;
    m_beaconEvent.Cancel();
    WifiMac::DoDispose();
}

void
AdhocWifiMac::SetBeaconGeneration(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    if (!enable)
    {
        m_beaconEvent.Cancel();
    }
    else if (!m_enableBeaconGeneration)
    {
        m_beaconEvent = Simulator::ScheduleNow(&AdhocWifiMac::SendOneBeacon, this);
    }
    m_enableBeaconGeneration = enable;
}

void
AdhocWifiMac::SetBeaconInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    if ((interval.GetMicroSeconds() % 1024) != 0)
    {
        NS_FATAL_ERROR("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE "
                       "Std. 802.11-2012");
    }
    if (interval.GetMicroSeconds() > (1024 * 65535))
    {
        NS_FATAL_ERROR(
            "beacon interval should be smaller then or equal to 65535 * 1024us (802.11 time unit)");
    }
    m_beaconInterval = interval;
}

Time
AdhocWifiMac::GetBeaconInterval() const
{
    return m_beaconInterval;
}

Ptr<Txop>
AdhocWifiMac::GetBeaconTxop() const
{
    return GetQosSupported() ? StaticCast<Txop>(GetQosTxop(m_beaconAc)) : GetTxop();
}

void
AdhocWifiMac::DoCompleteConfig()
{
    NS_LOG_FUNCTION(this);
}

bool
AdhocWifiMac::CanForwardPacketsTo(Mac48Address to) const
{
    return (!m_enableBeaconGeneration || !GetWifiRemoteStationManager()->IsBrandNew(to));
}

void
AdhocWifiMac::SetAllCapabilities(const Mac48Address& address)
{
    // assume the destination supports all the capabilities we support
    if (GetHtSupported(SINGLE_LINK_OP_ID))
    {
        GetWifiRemoteStationManager()->AddAllSupportedMcs(address);
        GetWifiRemoteStationManager()->AddStationHtCapabilities(
            address,
            GetHtCapabilities(SINGLE_LINK_OP_ID));
    }
    if (GetVhtSupported(SINGLE_LINK_OP_ID))
    {
        GetWifiRemoteStationManager()->AddStationVhtCapabilities(
            address,
            GetVhtCapabilities(SINGLE_LINK_OP_ID));
    }
    if (GetHeSupported())
    {
        GetWifiRemoteStationManager()->AddStationHeCapabilities(
            address,
            GetHeCapabilities(SINGLE_LINK_OP_ID));
        if (Is6GhzBand(SINGLE_LINK_OP_ID))
        {
            GetWifiRemoteStationManager()->AddStationHe6GhzCapabilities(
                address,
                GetHe6GhzBandCapabilities(SINGLE_LINK_OP_ID));
        }
    }
    if (GetEhtSupported())
    {
        GetWifiRemoteStationManager()->AddStationEhtCapabilities(
            address,
            GetEhtCapabilities(SINGLE_LINK_OP_ID));
    }
    GetWifiRemoteStationManager()->AddAllSupportedModes(address);
    GetWifiRemoteStationManager()->RecordAdhocPeer(address);
    GetWifiRemoteStationManager()->SetEmlsrEnabled(address, m_emlsrPeer);
}

void
AdhocWifiMac::Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from)
{
    NS_LOG_FUNCTION(this << *mpdu << to << from);

    if (GetWifiRemoteStationManager()->IsBrandNew(to))
    {
        if (m_enableBeaconGeneration)
        {
            NS_LOG_LOGIC("Capabilities for " << to << " are unknown: drop packet");
            NotifyTxDrop(mpdu->GetPacket());
            return;
        }
        else
        {
            SetAllCapabilities(to);
        }
    }

    auto& hdr = mpdu->GetHeader();

    hdr.SetAddr1(to);
    hdr.SetAddr2(GetAddress());
    hdr.SetAddr3(GetBssid(SINGLE_LINK_OP_ID));
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    auto txop = hdr.IsQosData() ? StaticCast<Txop>(GetQosTxop(hdr.GetQosTid())) : GetTxop();
    NS_ASSERT(txop);
    txop->Queue(mpdu);
}

void
AdhocWifiMac::SetLinkUpCallback(Callback<void> linkUp)
{
    NS_LOG_FUNCTION(this << &linkUp);
    WifiMac::SetLinkUpCallback(linkUp);

    // The approach taken here is that, from the point of view of a STA
    // in IBSS mode, the link is always up, so we immediately invoke the
    // callback if one is set
    linkUp();
}

void
AdhocWifiMac::SendOneBeacon()
{
    NS_LOG_FUNCTION(this);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_BEACON);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(GetAddress());
    hdr.SetAddr3(GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    MgtBeaconHeader beacon;
    beacon.Get<Ssid>() = GetSsid();
    auto supportedRates = GetSupportedRates();
    beacon.Get<SupportedRates>() = supportedRates.rates;
    beacon.Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
    beacon.m_beaconInterval = GetBeaconInterval().GetMicroSeconds();
    beacon.m_capability = GetCapabilities();
    if (GetDsssSupported(SINGLE_LINK_OP_ID))
    {
        beacon.Get<DsssParameterSet>() = GetDsssParameterSet();
    }
    if (GetErpSupported(SINGLE_LINK_OP_ID))
    {
        beacon.Get<ErpInformation>() = GetErpInformation();
    }
    if (GetQosSupported())
    {
        beacon.Get<EdcaParameterSet>() = GetEdcaParameterSet();
    }
    if (GetHtSupported(SINGLE_LINK_OP_ID))
    {
        beacon.Get<ExtendedCapabilities>() = GetExtendedCapabilities();
        beacon.Get<HtCapabilities>() = GetHtCapabilities(SINGLE_LINK_OP_ID);
        beacon.Get<HtOperation>() = GetHtOperation();
    }
    if (GetVhtSupported(SINGLE_LINK_OP_ID))
    {
        beacon.Get<VhtCapabilities>() = GetVhtCapabilities(SINGLE_LINK_OP_ID);
        beacon.Get<VhtOperation>() = GetVhtOperation();
    }
    if (GetHeSupported())
    {
        beacon.Get<HeCapabilities>() = GetHeCapabilities(SINGLE_LINK_OP_ID);
        beacon.Get<HeOperation>() = GetHeOperation();
        if (Is6GhzBand(SINGLE_LINK_OP_ID))
        {
            beacon.Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(SINGLE_LINK_OP_ID);
        }
    }
    if (GetEhtSupported())
    {
        beacon.Get<EhtCapabilities>() = GetEhtCapabilities(SINGLE_LINK_OP_ID);
        beacon.Get<EhtOperation>() = GetEhtOperation();
    }

    auto packet = Create<Packet>();
    packet->AddHeader(beacon);
    GetBeaconTxop()->Queue(Create<WifiMpdu>(packet, hdr));

    m_beaconEvent = Simulator::Schedule(GetBeaconInterval(), &AdhocWifiMac::SendOneBeacon, this);
}

void
AdhocWifiMac::Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const auto& hdr = mpdu->GetHeader();
    NS_ASSERT(!hdr.IsCtl());
    const auto from = hdr.GetAddr2();
    const auto to = hdr.GetAddr1();
    auto packet = mpdu->GetPacket();
    if (GetWifiRemoteStationManager()->IsBrandNew(from) && !m_enableBeaconGeneration)
    {
        SetAllCapabilities(from);
    }
    if (hdr.IsData())
    {
        if (hdr.IsFromDs() || hdr.IsToDs())
        {
            NS_LOG_LOGIC("Received data frame not part of an ad-hoc network: ignore");
            NotifyRxDrop(packet);
            return;
        }
        if (hdr.IsQosData() && hdr.IsQosAmsdu())
        {
            NS_LOG_DEBUG("Received A-MSDU from" << from);
            DeaggregateAmsduAndForward(mpdu);
        }
        else
        {
            ForwardUp(packet, from, to);
        }
        return;
    }

    switch (hdr.GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
        // This is a frame not aimed for IBSS, so we can safely ignore it.
        NotifyRxDrop(packet);
        break;

    case WIFI_MAC_MGT_BEACON:
        ReceiveBeacon(mpdu, linkId);
        break;

    case WIFI_MAC_MGT_PROBE_REQUEST:
        ReceiveProbeRequest(mpdu, linkId);
        break;

    default:
        // Invoke the receive handler of our parent class to deal with any
        // other frames. Specifically, this will handle Block Ack-related
        // Management Action frames.
        WifiMac::Receive(mpdu, linkId);
    }
}

void
AdhocWifiMac::ReceiveBeacon(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();
    NS_ASSERT(hdr.IsBeacon());

    const auto from = hdr.GetAddr2();
    NS_LOG_DEBUG("Beacon received from " << from);

    if (!GetWifiRemoteStationManager()->IsBrandNew(from))
    {
        // capabilities already learnt: nothing to do
        return;
    }

    // store capabilities from received beacon
    MgtBeaconHeader beacon;
    mpdu->GetPacket()->PeekHeader(beacon);
    RecordCapabilities(beacon, from, linkId);

    GetWifiRemoteStationManager()->RecordAdhocPeer(from);
}

void
AdhocWifiMac::ReceiveProbeRequest(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();
    NS_ASSERT(hdr.IsProbeReq());

    const auto from = hdr.GetAddr2();
    NS_LOG_DEBUG("Probe request received from " << hdr.GetAddr2());

    if (!GetWifiRemoteStationManager()->IsBrandNew(from))
    {
        // capabilities already learnt: nothing to do
        return;
    }

    // store capabilities from received beacon
    MgtProbeRequestHeader probeReq;
    mpdu->GetPacket()->PeekHeader(probeReq);
    RecordCapabilities(probeReq, from, linkId);
    GetWifiRemoteStationManager(linkId)->SetEmlsrEnabled(from, m_emlsrPeer);

    // change state of the STA to ensure it is no longer considered as unknown
    GetWifiRemoteStationManager()->RecordAdhocPeer(from);
}

AllSupportedRates
AdhocWifiMac::GetSupportedRates() const
{
    AllSupportedRates rates;
    for (const auto& mode : GetWifiPhy()->GetModeList())
    {
        uint64_t modeDataRate = mode.GetDataRate(GetWifiPhy()->GetChannelWidth());
        NS_LOG_DEBUG("Adding supported rate of " << modeDataRate);
        rates.AddSupportedRate(modeDataRate);
    }
    if (GetHtSupported(SINGLE_LINK_OP_ID))
    {
        for (const auto& selector : GetWifiPhy()->GetBssMembershipSelectorList())
        {
            rates.AddBssMembershipSelectorRate(selector);
        }
    }
    return rates;
}

DsssParameterSet
AdhocWifiMac::GetDsssParameterSet() const
{
    NS_ASSERT(GetDsssSupported(SINGLE_LINK_OP_ID));
    DsssParameterSet dsssParameters;
    dsssParameters.SetCurrentChannel(GetWifiPhy()->GetChannelNumber());
    return dsssParameters;
}

CapabilityInformation
AdhocWifiMac::GetCapabilities() const
{
    CapabilityInformation capabilities;
    capabilities.SetShortPreamble(GetWifiPhy()->GetShortPhyPreambleSupported() ||
                                  GetErpSupported(SINGLE_LINK_OP_ID));
    capabilities.SetShortSlotTime(GetShortSlotTimeSupported() &&
                                  GetErpSupported(SINGLE_LINK_OP_ID));
    capabilities.SetIbss();
    return capabilities;
}

ErpInformation
AdhocWifiMac::GetErpInformation() const
{
    NS_ASSERT(GetErpSupported(SINGLE_LINK_OP_ID));
    ErpInformation information;
    return information;
}

EdcaParameterSet
AdhocWifiMac::GetEdcaParameterSet() const
{
    NS_ASSERT(GetQosSupported());
    EdcaParameterSet edcaParameters;

    Ptr<QosTxop> edca;
    Time txopLimit;

    edca = GetQosTxop(AC_BE);
    txopLimit = edca->GetTxopLimit(SINGLE_LINK_OP_ID);
    edcaParameters.SetBeAci(0);
    edcaParameters.SetBeCWmin(edca->GetMinCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetBeCWmax(edca->GetMaxCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetBeAifsn(edca->GetAifsn(SINGLE_LINK_OP_ID));
    edcaParameters.SetBeTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_BK);
    txopLimit = edca->GetTxopLimit(SINGLE_LINK_OP_ID);
    edcaParameters.SetBkAci(1);
    edcaParameters.SetBkCWmin(edca->GetMinCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetBkCWmax(edca->GetMaxCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetBkAifsn(edca->GetAifsn(SINGLE_LINK_OP_ID));
    edcaParameters.SetBkTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_VI);
    txopLimit = edca->GetTxopLimit(SINGLE_LINK_OP_ID);
    edcaParameters.SetViAci(2);
    edcaParameters.SetViCWmin(edca->GetMinCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetViCWmax(edca->GetMaxCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetViAifsn(edca->GetAifsn(SINGLE_LINK_OP_ID));
    edcaParameters.SetViTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_VO);
    txopLimit = edca->GetTxopLimit(SINGLE_LINK_OP_ID);
    edcaParameters.SetVoAci(3);
    edcaParameters.SetVoCWmin(edca->GetMinCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetVoCWmax(edca->GetMaxCw(SINGLE_LINK_OP_ID));
    edcaParameters.SetVoAifsn(edca->GetAifsn(SINGLE_LINK_OP_ID));
    edcaParameters.SetVoTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edcaParameters.SetQosInfo(0);

    return edcaParameters;
}

HtOperation
AdhocWifiMac::GetHtOperation() const
{
    NS_ASSERT(GetHtSupported(SINGLE_LINK_OP_ID));

    HtOperation operation;

    operation.SetPrimaryChannel(GetWifiPhy()->GetPrimaryChannelNumber(MHz_t{20}));
    if (GetWifiPhy()->GetChannelWidth() > MHz_t{20})
    {
        operation.SetSecondaryChannelOffset(1);
        operation.SetStaChannelWidth(1);
    }
    uint64_t maxSupportedRate = 0; // in bit/s
    for (const auto& mcs : GetWifiPhy()->GetMcsList(WIFI_MOD_CLASS_HT))
    {
        uint8_t nss = (mcs.GetMcsValue() / 8) + 1;
        NS_ASSERT(nss > 0 && nss < 5);
        uint64_t dataRate =
            mcs.GetDataRate(GetWifiPhy()->GetChannelWidth(),
                            NanoSeconds(GetHtConfiguration()->m_sgiSupported ? 400 : 800),
                            nss);
        if (dataRate > maxSupportedRate)
        {
            maxSupportedRate = dataRate;
            NS_LOG_DEBUG("Updating maxSupportedRate to " << maxSupportedRate);
        }
    }
    operation.SetRxHighestSupportedDataRate(
        static_cast<uint16_t>(maxSupportedRate / 1e6)); // in Mbit/s
    operation.SetTxMcsSetDefined(!GetWifiPhy()->GetMcsList(WIFI_MOD_CLASS_HT).empty());
    operation.SetTxMaxNSpatialStreams(GetWifiPhy()->GetMaxSupportedTxSpatialStreams());

    return operation;
}

VhtOperation
AdhocWifiMac::GetVhtOperation() const
{
    NS_ASSERT(GetVhtSupported(SINGLE_LINK_OP_ID));

    VhtOperation operation;

    const auto bssBandwidth = GetWifiPhy()->GetChannelWidth();
    // Set to 0 for 20 MHz or 40 MHz BSS bandwidth.
    // Set to 1 for 80 MHz, 160 MHz or 80+80 MHz BSS bandwidth.
    operation.SetChannelWidth((bssBandwidth > MHz_t{40}) ? 1 : 0);
    // For 20, 40, or 80 MHz BSS bandwidth, indicates the channel center frequency
    // index for the 20, 40, or 80 MHz channel on which the VHT BSS operates.
    // For 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
    // indicates the channel center frequency index of the 80 MHz channel
    // segment that contains the primary channel.
    operation.SetChannelCenterFrequencySegment0(
        (bssBandwidth == MHz_t{160}) ? GetWifiPhy()->GetPrimaryChannelNumber(MHz_t{80})
                                     : GetWifiPhy()->GetChannelNumber());
    // For a 20, 40, or 80 MHz BSS bandwidth, this subfield is set to 0.
    // For a 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
    // indicates the channel center frequency index of the 160 MHz channel on
    // which the VHT BSS operates.
    operation.SetChannelCenterFrequencySegment1(
        (bssBandwidth == MHz_t{160}) ? GetWifiPhy()->GetChannelNumber() : 0);
    for (uint8_t nss = 1; nss <= GetWifiPhy()->GetMaxSupportedRxSpatialStreams(); nss++)
    {
        uint8_t maxMcs =
            9; // TBD: hardcode to 9 for now since we assume all MCS values are supported
        operation.SetMaxVhtMcsPerNss(nss, maxMcs);
    }

    return operation;
}

HeOperation
AdhocWifiMac::GetHeOperation() const
{
    NS_ASSERT(GetHeSupported());
    HeOperation operation;

    const auto maxSpatialStream = GetWifiPhy()->GetMaxSupportedRxSpatialStreams();
    for (uint8_t nss = 1; nss <= maxSpatialStream; nss++)
    {
        operation.SetMaxHeMcsPerNss(
            nss,
            11); // TBD: hardcode to 11 for now since we assume all MCS values are supported
    }
    operation.m_bssColorInfo.m_bssColor = GetHeConfiguration()->m_bssColor;

    if (GetWifiPhy()->GetPhyBand() == WIFI_PHY_BAND_6GHZ)
    {
        HeOperation::OpInfo6GHz op6Ghz;
        const auto bw = GetWifiPhy()->GetChannelWidth();
        const auto ch = GetWifiPhy()->GetOperatingChannel();
        op6Ghz.m_chWid = (bw == MHz_t{20}) ? 0 : (bw == MHz_t{40}) ? 1 : (bw == MHz_t{80}) ? 2 : 3;
        op6Ghz.m_primCh = ch.GetPrimaryChannelNumber(MHz_t{20}, WIFI_STANDARD_80211ax);
        op6Ghz.m_chCntrFreqSeg0 = (bw == MHz_t{160})
                                      ? ch.GetPrimaryChannelNumber(MHz_t{80}, WIFI_STANDARD_80211ax)
                                      : ch.GetNumber();
        op6Ghz.m_chCntrFreqSeg1 =
            (bw == MHz_t{160})
                ? ((ch.GetWidthType() == WifiChannelWidthType::CW_80_PLUS_80MHZ)
                       ? ch.GetSecondaryChannelNumber(MHz_t{80}, WIFI_STANDARD_80211ax)
                       : ch.GetNumber())
                : 0;

        operation.m_6GHzOpInfo = op6Ghz;
    }

    return operation;
}

EhtOperation
AdhocWifiMac::GetEhtOperation() const
{
    NS_ASSERT(GetEhtSupported());

    EhtOperation operation;

    const auto maxSpatialStream = GetWifiPhy()->GetMaxSupportedRxSpatialStreams();
    operation.SetMaxRxNss(maxSpatialStream, 0, WIFI_EHT_MAX_MCS_INDEX);
    operation.SetMaxTxNss(maxSpatialStream, 0, WIFI_EHT_MAX_MCS_INDEX);

    return operation;
}

} // namespace ns3
