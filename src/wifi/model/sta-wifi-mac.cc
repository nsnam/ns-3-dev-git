/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "sta-wifi-mac.h"

#include "channel-access-manager.h"
#include "frame-exchange-manager.h"
#include "mgt-action-headers.h"
#include "qos-txop.h"
#include "snr-tag.h"
#include "wifi-assoc-manager.h"
#include "wifi-mac-queue.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/attribute-container.h"
#include "ns3/eht-configuration.h"
#include "ns3/emlsr-manager.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pair.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/shuffle.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <iterator>
#include <numeric>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("StaWifiMac");

NS_OBJECT_ENSURE_REGISTERED(StaWifiMac);

TypeId
StaWifiMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::StaWifiMac")
            .SetParent<WifiMac>()
            .SetGroupName("Wifi")
            .AddConstructor<StaWifiMac>()
            .AddAttribute("ProbeRequestTimeout",
                          "The duration to actively probe the channel.",
                          TimeValue(Seconds(0.05)),
                          MakeTimeAccessor(&StaWifiMac::m_probeRequestTimeout),
                          MakeTimeChecker())
            .AddAttribute("WaitBeaconTimeout",
                          "The duration to dwell on a channel while passively scanning for beacon",
                          TimeValue(MilliSeconds(120)),
                          MakeTimeAccessor(&StaWifiMac::m_waitBeaconTimeout),
                          MakeTimeChecker())
            .AddAttribute("AssocRequestTimeout",
                          "The interval between two consecutive association request attempts.",
                          TimeValue(Seconds(0.5)),
                          MakeTimeAccessor(&StaWifiMac::m_assocRequestTimeout),
                          MakeTimeChecker())
            .AddAttribute("MaxMissedBeacons",
                          "Number of beacons which much be consecutively missed before "
                          "we attempt to restart association.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&StaWifiMac::m_maxMissedBeacons),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "ActiveProbing",
                "If true, we send probe requests. If false, we don't."
                "NOTE: if more than one STA in your simulation is using active probing, "
                "you should enable it at a different simulation time for each STA, "
                "otherwise all the STAs will start sending probes at the same time resulting in "
                "collisions. "
                "See bug 1060 for more info.",
                BooleanValue(false),
                MakeBooleanAccessor(&StaWifiMac::SetActiveProbing, &StaWifiMac::GetActiveProbing),
                MakeBooleanChecker())
            .AddAttribute("ProbeDelay",
                          "Delay (in microseconds) to be used prior to transmitting a "
                          "Probe frame during active scanning.",
                          StringValue("ns3::UniformRandomVariable[Min=50.0|Max=250.0]"),
                          MakePointerAccessor(&StaWifiMac::m_probeDelay),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("AssocType",
                          "Type of association performed by this device (provided that it is "
                          "supported by the standard configured for this device, otherwise legacy "
                          "association is performed). By using this attribute, it is possible for "
                          "an EHT single-link device to perform ML setup with an AP MLD and for an "
                          "EHT multi-link device to perform legacy association with an AP MLD.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          EnumValue(WifiAssocType::ML_SETUP),
                          MakeEnumAccessor<WifiAssocType>(&StaWifiMac::m_assocType),
                          MakeEnumChecker(WifiAssocType::LEGACY,
                                          "LEGACY",
                                          WifiAssocType::ML_SETUP,
                                          "ML_SETUP"))
            .AddAttribute(
                "PowerSaveMode",
                "Enable/disable power save mode on the given link. The power management mode is "
                "actually changed when the AP acknowledges a frame sent with the Power Management "
                "field set to the value corresponding to the requested mode",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                PairValue<BooleanValue, UintegerValue>(),
                MakePairAccessor<BooleanValue, UintegerValue>(&StaWifiMac::SetPowerSaveMode),
                MakePairChecker<BooleanValue, UintegerValue>(MakeBooleanChecker(),
                                                             MakeUintegerChecker<uint8_t>()))
            .AddAttribute("PmModeSwitchTimeout",
                          "If switching to a new Power Management mode is not completed within "
                          "this amount of time, make another attempt at switching Power "
                          "Management mode.",
                          TimeValue(Seconds(0.1)),
                          MakeTimeAccessor(&StaWifiMac::m_pmModeSwitchTimeout),
                          MakeTimeChecker())
            .AddTraceSource("Assoc",
                            "Associated with an access point. If this is an MLD that associated "
                            "with an AP MLD, the AP MLD address is provided.",
                            MakeTraceSourceAccessor(&StaWifiMac::m_assocLogger),
                            "ns3::Mac48Address::TracedCallback")
            .AddTraceSource("LinkSetupCompleted",
                            "A link was setup in the context of ML setup with an AP MLD. "
                            "Provides ID of the setup link and AP MAC address",
                            MakeTraceSourceAccessor(&StaWifiMac::m_setupCompleted),
                            "ns3::StaWifiMac::LinkSetupCallback")
            .AddTraceSource("DeAssoc",
                            "Association with an access point lost. If this is an MLD "
                            "that disassociated with an AP MLD, the AP MLD address is provided.",
                            MakeTraceSourceAccessor(&StaWifiMac::m_deAssocLogger),
                            "ns3::Mac48Address::TracedCallback")
            .AddTraceSource("BeaconArrival",
                            "Time of beacons arrival from associated AP",
                            MakeTraceSourceAccessor(&StaWifiMac::m_beaconArrival),
                            "ns3::Time::TracedCallback")
            .AddTraceSource("ReceivedBeaconInfo",
                            "Information about every received Beacon frame",
                            MakeTraceSourceAccessor(&StaWifiMac::m_beaconInfo),
                            "ns3::ApInfo::TracedCallback")
            .AddTraceSource("EmlsrLinkSwitch",
                            "Trace start/end of EMLSR link switch events. Specifically, this trace "
                            "is fired: (i) when a PHY _operating on a link_ starts switching to "
                            "another link, thus the PHY is disconnected from the previous link; "
                            "(ii) when a PHY is connected to a new link after performing a channel "
                            "switch. This trace provides: the ID of the previous link, in "
                            "case the PHY is disconnected, or the ID of the new link, in case the "
                            "PHY is connected; a pointer to the PHY that switches link; a boolean "
                            "value indicating if the PHY is connected to (true) or disconnected "
                            "from (false) the given link.",
                            MakeTraceSourceAccessor(&StaWifiMac::m_emlsrLinkSwitchLogger),
                            "ns3::StaWifiMac::EmlsrLinkSwitchCallback");
    return tid;
}

StaWifiMac::StaWifiMac()
    : m_state(UNASSOCIATED),
      m_aid(0),
      m_assocRequestEvent()
{
    NS_LOG_FUNCTION(this);

    // Let the lower layers know that we are acting as a non-AP STA in
    // an infrastructure BSS.
    SetTypeOfStation(STA);
}

void
StaWifiMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    // an EMLSR client must perform ML setup by using its main PHY
    if (m_assocManager && m_emlsrManager)
    {
        auto mainPhyId = m_emlsrManager->GetMainPhyId();
        auto linkId = GetLinkForPhy(mainPhyId);
        NS_ASSERT(linkId);
        m_assocManager->SetAttribute(
            "AllowedLinks",
            AttributeContainerValue<UintegerValue>(std::list<uint8_t>{*linkId}));
    }
    if (m_emlsrManager)
    {
        m_emlsrManager->Initialize();
    }
    StartScanning();
    NS_ABORT_IF(!TraceConnectWithoutContext("AckedMpdu", MakeCallback(&StaWifiMac::TxOk, this)));
    WifiMac::DoInitialize();
}

void
StaWifiMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_assocManager)
    {
        m_assocManager->Dispose();
    }
    m_assocManager = nullptr;
    if (m_emlsrManager)
    {
        m_emlsrManager->Dispose();
    }
    m_emlsrManager = nullptr;
    for (auto& [phyId, event] : m_emlsrLinkSwitch)
    {
        event.Cancel();
    }
    m_emlsrLinkSwitch.clear();
    WifiMac::DoDispose();
}

StaWifiMac::~StaWifiMac()
{
    NS_LOG_FUNCTION(this);
}

void
StaWifiMac::DoCompleteConfig()
{
    NS_LOG_FUNCTION(this);
}

StaWifiMac::StaLinkEntity::~StaLinkEntity()
{
    NS_LOG_FUNCTION_NOARGS();
}

std::unique_ptr<WifiMac::LinkEntity>
StaWifiMac::CreateLinkEntity() const
{
    return std::make_unique<StaLinkEntity>();
}

StaWifiMac::StaLinkEntity&
StaWifiMac::GetLink(uint8_t linkId) const
{
    return static_cast<StaLinkEntity&>(WifiMac::GetLink(linkId));
}

StaWifiMac::StaLinkEntity&
StaWifiMac::GetStaLink(const std::unique_ptr<WifiMac::LinkEntity>& link) const
{
    return static_cast<StaLinkEntity&>(*link);
}

int64_t
StaWifiMac::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_probeDelay->SetStream(stream);
    auto currentStream = stream + 1;
    currentStream += WifiMac::AssignStreams(currentStream);
    return (currentStream - stream);
}

void
StaWifiMac::SetAssocManager(Ptr<WifiAssocManager> assocManager)
{
    NS_LOG_FUNCTION(this << assocManager);
    m_assocManager = assocManager;
    m_assocManager->SetStaWifiMac(this);
}

WifiAssocType
StaWifiMac::GetAssocType() const
{
    // non-EHT devices can only perform legacy association
    return GetEhtConfiguration() ? m_assocType : WifiAssocType::LEGACY;
}

void
StaWifiMac::SetEmlsrManager(Ptr<EmlsrManager> emlsrManager)
{
    NS_LOG_FUNCTION(this << emlsrManager);
    m_emlsrManager = emlsrManager;
    m_emlsrManager->SetWifiMac(this);
}

Ptr<EmlsrManager>
StaWifiMac::GetEmlsrManager() const
{
    return m_emlsrManager;
}

uint16_t
StaWifiMac::GetAssociationId() const
{
    NS_ASSERT_MSG(IsAssociated(), "This station is not associated to any AP");
    return m_aid;
}

void
StaWifiMac::SetActiveProbing(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_activeProbing = enable;
    if (m_state == SCANNING)
    {
        NS_LOG_DEBUG("STA is still scanning, reset scanning process");
        StartScanning();
    }
}

bool
StaWifiMac::GetActiveProbing() const
{
    return m_activeProbing;
}

void
StaWifiMac::SetWifiPhys(const std::vector<Ptr<WifiPhy>>& phys)
{
    NS_LOG_FUNCTION(this);
    WifiMac::SetWifiPhys(phys);
    for (auto& phy : phys)
    {
        phy->SetCapabilitiesChangedCallback(
            MakeCallback(&StaWifiMac::PhyCapabilitiesChanged, this));
    }
}

WifiScanParams::Channel
StaWifiMac::GetCurrentChannel(uint8_t linkId) const
{
    auto phy = GetWifiPhy(linkId);
    const auto width = phy->GetOperatingChannel().IsOfdm() ? MHz_u{20} : phy->GetChannelWidth();
    uint8_t ch = phy->GetPrimaryChannelNumber(width);
    return {ch, phy->GetPhyBand()};
}

void
StaWifiMac::NotifyEmlsrModeChanged(const std::set<uint8_t>& linkIds)
{
    std::stringstream ss;
    if (g_log.IsEnabled(ns3::LOG_FUNCTION))
    {
        std::copy(linkIds.cbegin(), linkIds.cend(), std::ostream_iterator<uint16_t>(ss, " "));
    }
    NS_LOG_FUNCTION(this << ss.str());

    for (const auto& [linkId, lnk] : GetLinks())
    {
        auto& link = GetStaLink(lnk);

        if (linkIds.contains(linkId))
        {
            // EMLSR mode enabled
            link.emlsrEnabled = true;
            link.pmMode = WIFI_PM_ACTIVE;
        }
        else
        {
            // EMLSR mode disabled
            if (link.emlsrEnabled)
            {
                link.pmMode = WIFI_PM_POWERSAVE;
            }
            link.emlsrEnabled = false;
        }
    }
}

bool
StaWifiMac::IsEmlsrLink(uint8_t linkId) const
{
    return GetLink(linkId).emlsrEnabled;
}

MgtProbeRequestHeader
StaWifiMac::GetProbeRequest(uint8_t linkId) const
{
    MgtProbeRequestHeader probe;
    probe.Get<Ssid>() = GetSsid();
    auto supportedRates = GetSupportedRates(linkId);
    probe.Get<SupportedRates>() = supportedRates.rates;
    probe.Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
    if (GetWifiPhy(linkId)->GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
    {
        DsssParameterSet params;
        params.SetCurrentChannel(GetWifiPhy(linkId)->GetChannelNumber());
        probe.Get<DsssParameterSet>() = params;
    }
    if (GetHtSupported(linkId))
    {
        probe.Get<ExtendedCapabilities>() = GetExtendedCapabilities();
        probe.Get<HtCapabilities>() = GetHtCapabilities(linkId);
    }
    if (GetVhtSupported(linkId))
    {
        probe.Get<VhtCapabilities>() = GetVhtCapabilities(linkId);
    }
    if (GetHeSupported())
    {
        probe.Get<HeCapabilities>() = GetHeCapabilities(linkId);
        if (Is6GhzBand(linkId))
        {
            probe.Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(linkId);
        }
    }
    if (GetEhtSupported())
    {
        probe.Get<EhtCapabilities>() = GetEhtCapabilities(linkId);
    }
    return probe;
}

MgtProbeRequestHeader
StaWifiMac::GetMultiLinkProbeRequest(uint8_t linkId,
                                     const std::vector<uint8_t>& apLinkIds,
                                     std::optional<uint8_t> apMldId) const
{
    NS_LOG_FUNCTION(this << linkId << apMldId.has_value());
    auto req = GetProbeRequest(linkId);

    if (GetAssocType() == WifiAssocType::LEGACY)
    {
        NS_LOG_DEBUG("Legacy association, not including Multi-link Element");
        return req;
    }

    req.Get<MultiLinkElement>() = GetProbeReqMultiLinkElement(apLinkIds, apMldId);
    return req;
}

void
StaWifiMac::EnqueueProbeRequest(const MgtProbeRequestHeader& probeReq,
                                uint8_t linkId,
                                const Mac48Address& addr1,
                                const Mac48Address& addr3)
{
    NS_LOG_FUNCTION(this << linkId << addr1 << addr3);
    WifiMacHeader hdr(WIFI_MAC_MGT_PROBE_REQUEST);
    hdr.SetAddr1(addr1);
    hdr.SetAddr2(GetFrameExchangeManager(linkId)->GetAddress());
    hdr.SetAddr3(addr3);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    auto packet = Create<Packet>();
    packet->AddHeader(probeReq);

    if (!GetQosSupported())
    {
        GetTxop()->Queue(Create<WifiMpdu>(packet, hdr));
    }
    // "A QoS STA that transmits a Management frame determines access category used
    // for medium access in transmission of the Management frame as follows
    // (If dot11QMFActivated is false or not present)
    // — If the Management frame is individually addressed to a non-QoS STA, category
    //   AC_BE should be selected.
    // — If category AC_BE was not selected by the previous step, category AC_VO
    //   shall be selected." (Sec. 10.2.3.2 of 802.11-2020)
    else
    {
        GetVOQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
}

std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>
StaWifiMac::GetAssociationRequest(bool isReassoc, uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << isReassoc << +linkId);

    std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader> mgtFrame;

    if (isReassoc)
    {
        MgtReassocRequestHeader reassoc;
        reassoc.SetCurrentApAddress(GetBssid(linkId));
        mgtFrame = std::move(reassoc);
    }
    else
    {
        mgtFrame = MgtAssocRequestHeader();
    }

    // lambda to set the fields of the (Re)Association Request
    auto fill = [&](auto&& frame) {
        frame.template Get<Ssid>() = GetSsid();
        auto supportedRates = GetSupportedRates(linkId);
        frame.template Get<SupportedRates>() = supportedRates.rates;
        frame.template Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
        frame.Capabilities() = GetCapabilities(linkId);
        frame.SetListenInterval(0);
        if (GetHtSupported(linkId))
        {
            frame.template Get<ExtendedCapabilities>() = GetExtendedCapabilities();
            frame.template Get<HtCapabilities>() = GetHtCapabilities(linkId);
        }
        if (GetVhtSupported(linkId))
        {
            frame.template Get<VhtCapabilities>() = GetVhtCapabilities(linkId);
        }
        if (GetHeSupported())
        {
            frame.template Get<HeCapabilities>() = GetHeCapabilities(linkId);
            if (Is6GhzBand(linkId))
            {
                frame.template Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(linkId);
            }
        }
        if (GetEhtSupported())
        {
            frame.template Get<EhtCapabilities>() = GetEhtCapabilities(linkId);
        }
    };

    std::visit(fill, mgtFrame);
    return mgtFrame;
}

MultiLinkElement
StaWifiMac::GetBasicMultiLinkElement(bool isReassoc, uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << isReassoc << +linkId);

    MultiLinkElement multiLinkElement(MultiLinkElement::BASIC_VARIANT);
    // The Common info field of the Basic Multi-Link element carried in the (Re)Association
    // Request frame shall include the MLD MAC address, the MLD Capabilities and Operations,
    // and the EML Capabilities subfields, and shall not include the Link ID Info, the BSS
    // Parameters Change Count, and the Medium Synchronization Delay Information subfields
    // (Sec. 35.3.5.4 of 802.11be D2.0)
    // TODO Add the MLD Capabilities and Operations subfield
    multiLinkElement.SetMldMacAddress(GetAddress());

    if (m_emlsrManager) // EMLSR Manager is only installed if EMLSR is activated
    {
        multiLinkElement.SetEmlsrSupported(true);
        TimeValue time;
        m_emlsrManager->GetAttribute("EmlsrPaddingDelay", time);
        multiLinkElement.SetEmlsrPaddingDelay(time.Get());
        m_emlsrManager->GetAttribute("EmlsrTransitionDelay", time);
        multiLinkElement.SetEmlsrTransitionDelay(time.Get());
        // When the Transition Timeout subfield is included in a frame sent by a non-AP STA
        // affiliated with a non-AP MLD, the Transition Timeout subfield is reserved
        // (Section 9.4.2.312.2.3 of 802.11be D2.3)
        // The Medium Synchronization Delay Information subfield in the Common Info subfield is
        // not present if the Basic Multi-Link element is sent by a non-AP STA. (Section
        // 9.4.2.312.2.3 of 802.11be D3.1)
    }

    // The MLD Capabilities And Operations subfield is present in the Common Info field of the
    // Basic Multi-Link element carried in Beacon, Probe Response, (Re)Association Request, and
    // (Re)Association Response frames. (Sec. 9.4.2.312.2.3 of 802.11be D3.1)
    auto& mldCapabilities = multiLinkElement.GetCommonInfoBasic().m_mldCapabilities;
    mldCapabilities.emplace();
    mldCapabilities->maxNSimultaneousLinks = GetNLinks() - 1; // assuming STR for now
    mldCapabilities->srsSupport = 0;

    auto ehtConfiguration = GetEhtConfiguration();
    NS_ASSERT(ehtConfiguration);

    mldCapabilities->tidToLinkMappingSupport =
        static_cast<uint8_t>(ehtConfiguration->m_tidLinkMappingSupport);
    mldCapabilities->freqSepForStrApMld = 0; // not supported yet
    mldCapabilities->aarSupport = 0;         // not supported yet

    // For each requested link in addition to the link on which the (Re)Association Request
    // frame is transmitted, the Link Info field of the Basic Multi-Link element carried
    // in the (Re)Association Request frame shall contain the corresponding Per-STA Profile
    // subelement(s).
    for (const auto& [index, link] : GetLinks())
    {
        const auto& staLink = GetStaLink(link);

        if (index != linkId && staLink.bssid.has_value())
        {
            multiLinkElement.AddPerStaProfileSubelement();
            auto& perStaProfile = multiLinkElement.GetPerStaProfile(
                multiLinkElement.GetNPerStaProfileSubelements() - 1);
            // The Link ID subfield of the STA Control field of the Per-STA Profile subelement
            // for the corresponding non-AP STA that requests a link for multi-link (re)setup
            // with the AP MLD is set to the link ID of the AP affiliated with the AP MLD that
            // is operating on that link. The link ID is obtained during multi-link discovery
            perStaProfile.SetLinkId(index);
            // For each Per-STA Profile subelement included in the Link Info field, the
            // Complete Profile subfield of the STA Control field shall be set to 1
            perStaProfile.SetCompleteProfile();
            // The MAC Address Present subfield indicates the presence of the STA MAC Address
            // subfield in the STA Info field and is set to 1 if the STA MAC Address subfield
            // is present in the STA Info field; otherwise set to 0. An STA sets this subfield
            // to 1 when the element carries complete profile.
            perStaProfile.SetStaMacAddress(staLink.feManager->GetAddress());
            perStaProfile.SetAssocRequest(GetAssociationRequest(isReassoc, index));
        }
    }

    return multiLinkElement;
}

MultiLinkElement
StaWifiMac::GetProbeReqMultiLinkElement(const std::vector<uint8_t>& apLinkIds,
                                        std::optional<uint8_t> apMldId) const
{
    // IEEE 802.11be D6.0 9.4.2.321.3
    MultiLinkElement mle(MultiLinkElement::PROBE_REQUEST_VARIANT);
    if (apMldId.has_value())
    {
        mle.SetApMldId(*apMldId);
    }

    for (const auto apLinkId : apLinkIds)
    {
        mle.AddPerStaProfileSubelement();
        auto& perStaProfile = mle.GetPerStaProfile(mle.GetNPerStaProfileSubelements() - 1);
        perStaProfile.SetLinkId(apLinkId);
        // Current support limited to Complete Profile request per link ID
        // TODO: Add support for Partial Per-STA Profile request
        perStaProfile.SetCompleteProfile();
    };

    return mle;
}

std::vector<TidToLinkMapping>
StaWifiMac::GetTidToLinkMappingElements(WifiTidToLinkMappingNegSupport apNegSupport)
{
    NS_LOG_FUNCTION(this << apNegSupport);

    auto ehtConfig = GetEhtConfiguration();
    NS_ASSERT(ehtConfig);

    auto negSupport = ehtConfig->m_tidLinkMappingSupport;

    NS_ABORT_MSG_IF(negSupport == WifiTidToLinkMappingNegSupport::NOT_SUPPORTED,
                    "Cannot request TID-to-Link Mapping if negotiation is not supported");

    // store the mappings, so that we can enforce them when the AP MLD accepts them
    m_dlTidLinkMappingInAssocReq = ehtConfig->GetTidLinkMapping(WifiDirection::DOWNLINK);
    m_ulTidLinkMappingInAssocReq = ehtConfig->GetTidLinkMapping(WifiDirection::UPLINK);

    bool mappingValidForNegType1 = TidToLinkMappingValidForNegType1(m_dlTidLinkMappingInAssocReq,
                                                                    m_ulTidLinkMappingInAssocReq);
    NS_ABORT_MSG_IF(
        negSupport == WifiTidToLinkMappingNegSupport::SAME_LINK_SET && !mappingValidForNegType1,
        "Mapping TIDs to distinct link sets is incompatible with negotiation support of 1");

    if (apNegSupport == WifiTidToLinkMappingNegSupport::SAME_LINK_SET && !mappingValidForNegType1)
    {
        // If the TID-to-link Mapping Negotiation Support subfield value received from a peer
        // MLD is equal to 1, the MLD that initiates a TID-to-link mapping negotiation with the
        // peer MLD shall send only the TID-to-link Mapping element where all TIDs are mapped to
        // the same link set (Sec. 35.3.7.1.3 of 802.11be D3.1). We use default mapping to meet
        // this requirement.
        NS_LOG_DEBUG("Using default mapping because AP MLD advertised negotiation support of 1");
        m_dlTidLinkMappingInAssocReq.clear();
        m_ulTidLinkMappingInAssocReq.clear();
    }

    std::vector<TidToLinkMapping> ret(1);

    ret.back().m_control.direction = WifiDirection::DOWNLINK;

    // lambda to fill the last TID-to-Link Mapping IE in the vector to return
    auto fillIe = [&ret](const auto& mapping) {
        ret.back().m_control.defaultMapping = mapping.empty();

        for (const auto& [tid, linkSet] : mapping)
        {
            // At any point in time, a TID shall always be mapped to at least one setup link both
            // in DL and UL, which means that a TID-to-link mapping change is only valid and
            // successful if it will not result in having any TID for which the link set for DL
            // or UL is made of zero setup links (Sec. 35.3.7.1.1 of 802.11be D3.1)
            NS_ABORT_MSG_IF(linkSet.empty(), "Cannot map a TID to an empty link set");
            ret.back().SetLinkMappingOfTid(tid, linkSet);
        }
    };

    fillIe(m_dlTidLinkMappingInAssocReq);

    if (m_ulTidLinkMappingInAssocReq == m_dlTidLinkMappingInAssocReq)
    {
        ret.back().m_control.direction = WifiDirection::BOTH_DIRECTIONS;
        return ret;
    }

    ret.emplace_back();
    ret.back().m_control.direction = WifiDirection::UPLINK;
    fillIe(m_ulTidLinkMappingInAssocReq);

    return ret;
}

void
StaWifiMac::SendAssociationRequest(bool isReassoc)
{
    // find the link where the (Re)Association Request has to be sent
    auto it = GetLinks().cbegin();
    while (it != GetLinks().cend())
    {
        if (GetStaLink(it->second).sendAssocReq)
        {
            break;
        }
        it++;
    }
    NS_ABORT_MSG_IF(it == GetLinks().cend(),
                    "No link selected to send the (Re)Association Request");
    uint8_t linkId = it->first;
    auto& link = GetLink(linkId);
    NS_ABORT_MSG_IF(!link.bssid.has_value(),
                    "No BSSID set for the link on which the (Re)Association Request is to be sent");

    NS_LOG_FUNCTION(this << *link.bssid << isReassoc);
    WifiMacHeader hdr;
    hdr.SetType(isReassoc ? WIFI_MAC_MGT_REASSOCIATION_REQUEST : WIFI_MAC_MGT_ASSOCIATION_REQUEST);
    hdr.SetAddr1(*link.bssid);
    hdr.SetAddr2(link.feManager->GetAddress());
    hdr.SetAddr3(*link.bssid);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    Ptr<Packet> packet = Create<Packet>();

    auto frame = GetAssociationRequest(isReassoc, linkId);

    // include a Multi-Link Element if this device performs ML Setup and the AP is a multi-link
    // device; if the AP MLD  has indicated a support of TID-to-link mapping negotiation, also
    // include the TID-to-link Mapping element(s)
    if (GetAssocType() == WifiAssocType::ML_SETUP &&
        GetWifiRemoteStationManager(linkId)->GetMldAddress(*link.bssid).has_value())
    {
        auto addMle = [&](auto&& frame) {
            frame.template Get<MultiLinkElement>() = GetBasicMultiLinkElement(isReassoc, linkId);
        };
        std::visit(addMle, frame);

        WifiTidToLinkMappingNegSupport negSupport;
        if (const auto& mldCapabilities =
                GetWifiRemoteStationManager(linkId)->GetStationMldCapabilities(*link.bssid);
            mldCapabilities && (negSupport = static_cast<WifiTidToLinkMappingNegSupport>(
                                    mldCapabilities->get().tidToLinkMappingSupport)) >
                                   WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
        {
            auto addTlm = [&](auto&& frame) {
                frame.template Get<TidToLinkMapping>() = GetTidToLinkMappingElements(negSupport);
            };
            std::visit(addTlm, frame);
        }
    }

    if (!isReassoc)
    {
        packet->AddHeader(std::get<MgtAssocRequestHeader>(frame));
    }
    else
    {
        packet->AddHeader(std::get<MgtReassocRequestHeader>(frame));
    }

    if (!GetQosSupported())
    {
        GetTxop()->Queue(Create<WifiMpdu>(packet, hdr));
    }
    // "A QoS STA that transmits a Management frame determines access category used
    // for medium access in transmission of the Management frame as follows
    // (If dot11QMFActivated is false or not present)
    // — If the Management frame is individually addressed to a non-QoS STA, category
    //   AC_BE should be selected.
    // — If category AC_BE was not selected by the previous step, category AC_VO
    //   shall be selected." (Sec. 10.2.3.2 of 802.11-2020)
    else if (!GetWifiRemoteStationManager(linkId)->GetQosSupported(*link.bssid))
    {
        GetBEQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
    else
    {
        GetVOQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }

    if (m_assocRequestEvent.IsPending())
    {
        m_assocRequestEvent.Cancel();
    }
    m_assocRequestEvent =
        Simulator::Schedule(m_assocRequestTimeout, &StaWifiMac::AssocRequestTimeout, this);
}

void
StaWifiMac::TryToEnsureAssociated()
{
    NS_LOG_FUNCTION(this);
    switch (m_state)
    {
    case ASSOCIATED:
        return;
    case SCANNING:
        /* we have initiated active or passive scanning, continue to wait
           and gather beacons or probe responses until the scanning timeout
         */
        break;
    case UNASSOCIATED:
        /* we were associated but we missed a bunch of beacons
         * so we should assume we are not associated anymore.
         * We try to initiate a scan now.
         */
        m_linkDown();
        StartScanning();
        break;
    case WAIT_ASSOC_RESP:
        /* we have sent an association request so we do not need to
           re-send an association request right now. We just need to
           wait until either assoc-request-timeout or until
           we get an association response.
         */
    case REFUSED:
        /* we have sent an association request and received a negative
           association response. We wait until someone restarts an
           association with a given SSID.
         */
        break;
    }
}

void
StaWifiMac::StartScanning()
{
    NS_LOG_FUNCTION(this);
    SetState(SCANNING);
    NS_ASSERT(m_assocManager);

    WifiScanParams scanParams;
    scanParams.ssid = GetSsid();
    for (const auto& [id, link] : GetLinks())
    {
        WifiScanParams::ChannelList channel{
            (link->phy->HasFixedPhyBand()) ? WifiScanParams::Channel{0, link->phy->GetPhyBand()}
                                           : WifiScanParams::Channel{0, WIFI_PHY_BAND_UNSPECIFIED}};

        scanParams.channelList.push_back(channel);
    }
    if (m_activeProbing)
    {
        scanParams.type = WifiScanType::ACTIVE;
        scanParams.probeDelay = MicroSeconds(m_probeDelay->GetValue());
        scanParams.minChannelTime = scanParams.maxChannelTime = m_probeRequestTimeout;
    }
    else
    {
        scanParams.type = WifiScanType::PASSIVE;
        scanParams.maxChannelTime = m_waitBeaconTimeout;
    }

    m_assocManager->StartScanning(std::move(scanParams));
}

void
StaWifiMac::ScanningTimeout(const std::optional<ApInfo>& bestAp)
{
    NS_LOG_FUNCTION(this);

    if (!bestAp.has_value())
    {
        NS_LOG_DEBUG("Exhausted list of candidate AP; restart scanning");
        StartScanning();
        return;
    }

    NS_LOG_DEBUG("Attempting to associate with AP: " << *bestAp);
    ApplyOperationalSettings(bestAp->m_frame, bestAp->m_apAddr, bestAp->m_bssid, bestAp->m_linkId);
    // reset info on links to setup
    for (auto& [id, link] : GetLinks())
    {
        auto& staLink = GetStaLink(link);
        staLink.sendAssocReq = false;
        staLink.bssid = std::nullopt;
    }
    // send Association Request on the link where the Beacon/Probe Response was received
    GetLink(bestAp->m_linkId).sendAssocReq = true;
    GetLink(bestAp->m_linkId).bssid = bestAp->m_bssid;
    std::shared_ptr<CommonInfoBasicMle> mleCommonInfo;
    // update info on links to setup (11be MLDs only)
    const auto& mle =
        std::visit([](auto&& frame) { return frame.template Get<MultiLinkElement>(); },
                   bestAp->m_frame);
    std::map<uint8_t, uint8_t> swapInfo;
    for (const auto& [localLinkId, apLinkId, bssid] : bestAp->m_setupLinks)
    {
        NS_ASSERT_MSG(mle, "We get here only for ML setup");
        NS_LOG_DEBUG("Setting up link (local ID=" << +localLinkId << ", AP ID=" << +apLinkId
                                                  << ")");
        GetLink(localLinkId).bssid = bssid;
        if (!mleCommonInfo)
        {
            mleCommonInfo = std::make_shared<CommonInfoBasicMle>(mle->GetCommonInfoBasic());
        }
        GetWifiRemoteStationManager(localLinkId)->AddStationMleCommonInfo(bssid, mleCommonInfo);
        swapInfo.emplace(localLinkId, apLinkId);
    }

    SwapLinks(swapInfo);

    // lambda to get beacon interval from Beacon or Probe Response
    auto getBeaconInterval = [](auto&& frame) {
        using T = std::decay_t<decltype(frame)>;
        if constexpr (std::is_same_v<T, MgtBeaconHeader> ||
                      std::is_same_v<T, MgtProbeResponseHeader>)
        {
            return MicroSeconds(frame.GetBeaconIntervalUs());
        }
        else
        {
            NS_ABORT_MSG("Unexpected frame type");
            return Seconds(0);
        }
    };
    Time beaconInterval = std::visit(getBeaconInterval, bestAp->m_frame);
    Time delay = beaconInterval * m_maxMissedBeacons;
    // restart beacon watchdog
    RestartBeaconWatchdog(delay);

    SetState(WAIT_ASSOC_RESP);
    SendAssociationRequest(false);
}

void
StaWifiMac::AssocRequestTimeout()
{
    NS_LOG_FUNCTION(this);
    SetState(WAIT_ASSOC_RESP);
    SendAssociationRequest(false);
}

void
StaWifiMac::MissedBeacons()
{
    NS_LOG_FUNCTION(this);

    if (m_beaconWatchdogEnd > Simulator::Now())
    {
        if (m_beaconWatchdog.IsPending())
        {
            m_beaconWatchdog.Cancel();
        }
        m_beaconWatchdog = Simulator::Schedule(m_beaconWatchdogEnd - Simulator::Now(),
                                               &StaWifiMac::MissedBeacons,
                                               this);
        return;
    }
    NS_LOG_DEBUG("beacon missed");
    // We need to switch to the UNASSOCIATED state. However, if we are receiving a frame, wait
    // until the RX is completed (otherwise, crashes may occur if we are receiving a MU frame
    // because its reception requires the STA-ID). We need to check that a PHY is operating on
    // the given link, because this may (temporarily) not be the case for EMLSR clients.
    Time delay;
    for (const auto& [id, link] : GetLinks())
    {
        if (link->phy && link->phy->IsStateRx())
        {
            delay = std::max(delay, link->phy->GetDelayUntilIdle());
        }
    }
    Simulator::Schedule(delay, &StaWifiMac::Disassociated, this);
}

void
StaWifiMac::Disassociated()
{
    NS_LOG_FUNCTION(this);

    Mac48Address apAddr; // the AP address to trace (MLD address in case of ML setup)

    for (const auto& [id, link] : GetLinks())
    {
        auto& bssid = GetStaLink(link).bssid;
        if (bssid)
        {
            apAddr = GetWifiRemoteStationManager(id)->GetMldAddress(*bssid).value_or(*bssid);
        }
        bssid = std::nullopt; // link is no longer setup
    }

    NS_LOG_DEBUG("Set state to UNASSOCIATED and start scanning");
    SetState(UNASSOCIATED);
    // cancel the association request timer (see issue #862)
    m_assocRequestEvent.Cancel();
    m_deAssocLogger(apAddr);
    m_aid = 0; // reset AID
    TryToEnsureAssociated();
}

void
StaWifiMac::RestartBeaconWatchdog(Time delay)
{
    NS_LOG_FUNCTION(this << delay);

    m_beaconWatchdogEnd = std::max(Simulator::Now() + delay, m_beaconWatchdogEnd);
    if (Simulator::GetDelayLeft(m_beaconWatchdog) < delay && m_beaconWatchdog.IsExpired())
    {
        NS_LOG_DEBUG("really restart watchdog.");
        m_beaconWatchdog = Simulator::Schedule(delay, &StaWifiMac::MissedBeacons, this);
    }
}

bool
StaWifiMac::IsAssociated() const
{
    return m_state == ASSOCIATED;
}

bool
StaWifiMac::IsWaitAssocResp() const
{
    return m_state == WAIT_ASSOC_RESP;
}

std::set<uint8_t>
StaWifiMac::GetSetupLinkIds() const
{
    if (!IsAssociated())
    {
        return {};
    }

    std::set<uint8_t> linkIds;
    for (const auto& [id, link] : GetLinks())
    {
        if (GetStaLink(link).bssid)
        {
            linkIds.insert(id);
        }
    }
    return linkIds;
}

Mac48Address
StaWifiMac::DoGetLocalAddress(const Mac48Address& remoteAddr) const
{
    for (const auto& [id, link] : GetLinks())
    {
        if (GetStaLink(link).bssid == remoteAddr)
        {
            // the remote address is the address of the AP we are associated with;
            return link->feManager->GetAddress();
        }
    }

    // the remote address is unknown

    if (!IsAssociated())
    {
        return GetAddress();
    }

    // if this device has performed ML setup with an AP MLD, return the MLD address of this device
    const auto linkIds = GetSetupLinkIds();
    NS_ASSERT(!linkIds.empty());
    const auto linkId = *linkIds.cbegin(); // a setup link

    if (GetLink(linkId).stationManager->GetMldAddress(GetBssid(linkId)))
    {
        return GetAddress();
    }

    // return the address of the link used to perform association with the AP
    return GetLink(linkId).feManager->GetAddress();
}

bool
StaWifiMac::CanForwardPacketsTo(Mac48Address to) const
{
    return IsAssociated();
}

void
StaWifiMac::NotifyDropPacketToEnqueue(Ptr<Packet> packet, Mac48Address to)
{
    NS_LOG_FUNCTION(this << packet << to);
    TryToEnsureAssociated();
}

void
StaWifiMac::Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from)
{
    NS_LOG_FUNCTION(this << *mpdu << to << from);

    auto& hdr = mpdu->GetHeader();

    // the Receiver Address (RA) and the Transmitter Address (TA) are the MLD addresses only for
    // non-broadcast data frames exchanged between two MLDs
    auto linkIds = GetSetupLinkIds();
    NS_ASSERT(!linkIds.empty());
    uint8_t linkId = *linkIds.begin();
    const auto apMldAddr = GetWifiRemoteStationManager(linkId)->GetMldAddress(GetBssid(linkId));

    hdr.SetAddr1(apMldAddr.value_or(GetBssid(linkId)));
    hdr.SetAddr2(apMldAddr ? GetAddress() : GetFrameExchangeManager(linkId)->GetAddress());
    hdr.SetAddr3(to);
    hdr.SetDsNotFrom();
    hdr.SetDsTo();

    auto txop = hdr.IsQosData() ? StaticCast<Txop>(GetQosTxop(hdr.GetQosTid())) : GetTxop();
    NS_ASSERT(txop);
    txop->Queue(mpdu);
}

void
StaWifiMac::BlockTxOnLink(uint8_t linkId, WifiQueueBlockedReason reason)
{
    NS_LOG_FUNCTION(this << linkId << reason);

    GetMacQueueScheduler()->BlockAllQueues(reason, {linkId});
}

void
StaWifiMac::UnblockTxOnLink(std::set<uint8_t> linkIds, WifiQueueBlockedReason reason)
{
    // shuffle link IDs not to unblock links always in the same order
    std::vector<uint8_t> shuffledLinkIds(linkIds.cbegin(), linkIds.cend());
    Shuffle(shuffledLinkIds.begin(), shuffledLinkIds.end(), m_shuffleLinkIdsGen.GetRv());

    std::stringstream ss;
    if (g_log.IsEnabled(ns3::LOG_FUNCTION))
    {
        std::copy(shuffledLinkIds.cbegin(),
                  shuffledLinkIds.cend(),
                  std::ostream_iterator<uint16_t>(ss, " "));
    }
    NS_LOG_FUNCTION(this << reason << ss.str());

    for (const auto linkId : shuffledLinkIds)
    {
        std::map<AcIndex, bool> hasFramesToTransmit;
        for (const auto& [acIndex, ac] : wifiAcList)
        {
            // save the status of the AC queues before unblocking the queues
            hasFramesToTransmit[acIndex] = GetQosTxop(acIndex)->HasFramesToTransmit(linkId);
        }

        GetMacQueueScheduler()->UnblockAllQueues(reason, {linkId});

        for (const auto& [acIndex, ac] : wifiAcList)
        {
            // request channel access if needed (schedule now because multiple invocations
            // of this method may be done in a loop at the caller)
            Simulator::ScheduleNow(&Txop::StartAccessAfterEvent,
                                   GetQosTxop(acIndex),
                                   linkId,
                                   hasFramesToTransmit[acIndex],
                                   Txop::CHECK_MEDIUM_BUSY); // generate backoff if medium busy
        }
    }
}

void
StaWifiMac::Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    // consider the MAC header of the original MPDU (makes a difference for data frames only)
    const WifiMacHeader* hdr = &mpdu->GetOriginal()->GetHeader();
    Ptr<const Packet> packet = mpdu->GetPacket();
    NS_ASSERT(!hdr->IsCtl());
    Mac48Address myAddr = hdr->IsData() ? Mac48Address::ConvertFrom(GetDevice()->GetAddress())
                                        : GetFrameExchangeManager(linkId)->GetAddress();
    if (hdr->GetAddr3() == myAddr)
    {
        NS_LOG_LOGIC("packet sent by us.");
        return;
    }
    if (hdr->GetAddr1() != myAddr && !hdr->GetAddr1().IsGroup())
    {
        NS_LOG_LOGIC("packet is not for us");
        NotifyRxDrop(packet);
        return;
    }
    if (hdr->IsData())
    {
        if (!IsAssociated())
        {
            NS_LOG_LOGIC("Received data frame while not associated: ignore");
            NotifyRxDrop(packet);
            return;
        }
        if (!(hdr->IsFromDs() && !hdr->IsToDs()))
        {
            NS_LOG_LOGIC("Received data frame not from the DS: ignore");
            NotifyRxDrop(packet);
            return;
        }
        std::set<Mac48Address> apAddresses; // link addresses of AP
        for (auto id : GetSetupLinkIds())
        {
            apAddresses.insert(GetBssid(id));
        }
        if (!apAddresses.contains(mpdu->GetHeader().GetAddr2()))
        {
            NS_LOG_LOGIC("Received data frame not from the BSS we are associated with: ignore");
            NotifyRxDrop(packet);
            return;
        }
        if (!hdr->HasData())
        {
            NS_LOG_LOGIC("Received (QoS) Null Data frame: ignore");
            NotifyRxDrop(packet);
            return;
        }
        if (hdr->IsQosData())
        {
            if (hdr->IsQosAmsdu())
            {
                NS_ASSERT(apAddresses.contains(mpdu->GetHeader().GetAddr3()));
                DeaggregateAmsduAndForward(mpdu);
                packet = nullptr;
            }
            else
            {
                ForwardUp(packet, hdr->GetAddr3(), hdr->GetAddr1());
            }
        }
        else
        {
            ForwardUp(packet, hdr->GetAddr3(), hdr->GetAddr1());
        }
        return;
    }

    switch (hdr->GetType())
    {
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
        // This is a frame aimed at an AP, so we can safely ignore it.
        NotifyRxDrop(packet);
        break;

    case WIFI_MAC_MGT_BEACON:
        ReceiveBeacon(mpdu, linkId);
        break;

    case WIFI_MAC_MGT_PROBE_RESPONSE:
        ReceiveProbeResp(mpdu, linkId);
        break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
        ReceiveAssocResp(mpdu, linkId);
        break;

    case WIFI_MAC_MGT_ACTION:
        if (auto [category, action] = WifiActionHeader::Peek(packet);
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // this is handled by the EMLSR Manager
            break;
        }

    default:
        // Invoke the receive handler of our parent class to deal with any other frames
        WifiMac::Receive(mpdu, linkId);
    }

    if (m_emlsrManager)
    {
        m_emlsrManager->NotifyMgtFrameReceived(mpdu, linkId);
    }
}

void
StaWifiMac::ReceiveBeacon(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();
    const auto from = hdr.GetAddr2();
    NS_ASSERT(hdr.IsBeacon());

    NS_LOG_DEBUG("Beacon received");
    MgtBeaconHeader beacon;
    mpdu->GetPacket()->PeekHeader(beacon);
    const auto& capabilities = beacon.Capabilities();
    NS_ASSERT(capabilities.IsEss());
    bool goodBeacon;
    if (IsWaitAssocResp() || IsAssociated())
    {
        // we have to process this Beacon only if sent by the AP we are associated
        // with or from which we are waiting an Association Response frame
        auto bssid = GetLink(linkId).bssid;
        goodBeacon = bssid.has_value() && (hdr.GetAddr3() == *bssid);
    }
    else
    {
        // we retain this Beacon as candidate AP if the supported rates fit the
        // configured BSS membership selector
        goodBeacon = CheckSupportedRates(beacon, linkId);
    }

    SnrTag snrTag;
    bool found = mpdu->GetPacket()->PeekPacketTag(snrTag);
    NS_ASSERT(found);
    ApInfo apInfo = {.m_bssid = hdr.GetAddr3(),
                     .m_apAddr = hdr.GetAddr2(),
                     .m_snr = snrTag.Get(),
                     .m_frame = std::move(beacon),
                     .m_channel = {GetCurrentChannel(linkId)},
                     .m_linkId = linkId};

    if (!m_beaconInfo.IsEmpty())
    {
        m_beaconInfo(apInfo);
    }

    RecordCapabilities(beacon, from, linkId);
    RecordOperations(beacon, from, linkId);

    if (!goodBeacon)
    {
        NS_LOG_LOGIC("Beacon is not for us");
        return;
    }
    if (m_state == ASSOCIATED)
    {
        m_beaconArrival(Simulator::Now());
        Time delay = MicroSeconds(std::get<MgtBeaconHeader>(apInfo.m_frame).GetBeaconIntervalUs() *
                                  m_maxMissedBeacons);
        RestartBeaconWatchdog(delay);
        ApplyOperationalSettings(apInfo.m_frame, hdr.GetAddr2(), hdr.GetAddr3(), linkId);
    }
    else
    {
        NS_LOG_DEBUG("Beacon received from " << hdr.GetAddr2());
        m_assocManager->NotifyApInfo(std::move(apInfo));
    }
}

void
StaWifiMac::ReceiveProbeResp(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();
    NS_ASSERT(hdr.IsProbeResp());

    const auto from = hdr.GetAddr2();
    NS_LOG_DEBUG("Probe response received from " << from);
    MgtProbeResponseHeader probeResp;
    mpdu->GetPacket()->PeekHeader(probeResp);

    RecordCapabilities(probeResp, from, linkId);
    RecordOperations(probeResp, from, linkId);

    if (!CheckSupportedRates(probeResp, linkId))
    {
        return;
    }
    SnrTag snrTag;
    bool found = mpdu->GetPacket()->PeekPacketTag(snrTag);
    NS_ASSERT(found);
    m_assocManager->NotifyApInfo(ApInfo{.m_bssid = hdr.GetAddr3(),
                                        .m_apAddr = hdr.GetAddr2(),
                                        .m_snr = snrTag.Get(),
                                        .m_frame = std::move(probeResp),
                                        .m_channel = {GetCurrentChannel(linkId)},
                                        .m_linkId = linkId});
}

void
StaWifiMac::ReceiveAssocResp(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();
    NS_ASSERT(hdr.IsAssocResp() || hdr.IsReassocResp());

    MgtAssocResponseHeader assocResp;
    mpdu->GetPacket()->PeekHeader(assocResp);

    RecordCapabilities(assocResp, hdr.GetAddr2(), linkId);
    RecordOperations(assocResp, hdr.GetAddr2(), linkId);

    if (m_state != WAIT_ASSOC_RESP)
    {
        return;
    }

    if (m_assocRequestEvent.IsPending())
    {
        m_assocRequestEvent.Cancel();
    }

    std::optional<Mac48Address> apMldAddress;
    if (assocResp.GetStatusCode().IsSuccess())
    {
        m_aid = assocResp.GetAssociationId();
        NS_LOG_DEBUG((hdr.IsReassocResp() ? "reassociation done" : "association completed"));
        ApplyOperationalSettings(assocResp, hdr.GetAddr2(), hdr.GetAddr3(), linkId);
        NS_ASSERT(GetLink(linkId).bssid.has_value() && *GetLink(linkId).bssid == hdr.GetAddr3());
        SetBssid(hdr.GetAddr3(), linkId);
        SetState(ASSOCIATED);
        if ((GetAssocType() == WifiAssocType::ML_SETUP) &&
            assocResp.Get<MultiLinkElement>().has_value())
        {
            // this is an ML setup, trace the setup link
            m_setupCompleted(linkId, hdr.GetAddr3());
            apMldAddress = GetWifiRemoteStationManager(linkId)->GetMldAddress(hdr.GetAddr3());
            NS_ASSERT(apMldAddress);

            if (const auto& mldCapabilities =
                    GetWifiRemoteStationManager(linkId)->GetStationMldCapabilities(hdr.GetAddr3());
                mldCapabilities && static_cast<WifiTidToLinkMappingNegSupport>(
                                       mldCapabilities->get().tidToLinkMappingSupport) >
                                       WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
            {
                // the AP MLD supports TID-to-Link Mapping negotiation, hence we included
                // TID-to-Link Mapping element(s) in the Association Request.
                if (assocResp.Get<TidToLinkMapping>().empty())
                {
                    // The AP MLD did not include a TID-to-Link Mapping element in the Association
                    // Response, hence it accepted the mapping, which we can now store.
                    UpdateTidToLinkMapping(*apMldAddress,
                                           WifiDirection::DOWNLINK,
                                           m_dlTidLinkMappingInAssocReq);
                    UpdateTidToLinkMapping(*apMldAddress,
                                           WifiDirection::UPLINK,
                                           m_ulTidLinkMappingInAssocReq);

                    // Apply the negotiated TID-to-Link Mapping (if any) for UL direction
                    ApplyTidLinkMapping(*apMldAddress, WifiDirection::UPLINK);
                }
            }
        }
        else
        {
            m_assocLogger(hdr.GetAddr3());
        }
        if (!m_linkUp.IsNull())
        {
            m_linkUp();
        }
    }
    else
    {
        // If the link on which the (Re)Association Request frame was received cannot be
        // accepted by the AP MLD, the AP MLD shall treat the multi-link (re)setup as a
        // failure and shall not accept any requested links. If the link on which the
        // (Re)Association Request frame was received is accepted by the AP MLD, the
        // multi-link (re)setup is successful. (Sec. 35.3.5.1 of 802.11be D3.1)
        NS_LOG_DEBUG("association refused");
        SetState(REFUSED);
        StartScanning();
        return;
    }

    // create a list of all local Link IDs. IDs are removed as we find a corresponding
    // Per-STA Profile Subelements indicating successful association. Links with
    // remaining IDs are not setup
    std::list<uint8_t> setupLinks;
    for (const auto& [id, link] : GetLinks())
    {
        setupLinks.push_back(id);
    }
    if (assocResp.GetStatusCode().IsSuccess())
    {
        setupLinks.remove(linkId);
    }

    // if a Multi-Link Element is present, this is an ML setup, hence check if we can setup (other)
    // links
    if (const auto& mle = assocResp.Get<MultiLinkElement>())
    {
        NS_ABORT_MSG_IF(!GetLink(linkId).bssid.has_value(),
                        "The link on which the Association Response was received "
                        "is not a link we requested to setup");
        NS_ABORT_MSG_IF(linkId != mle->GetLinkIdInfo(),
                        "The link ID of the AP that transmitted the Association "
                        "Response does not match the stored link ID");
        NS_ABORT_MSG_IF(GetWifiRemoteStationManager(linkId)->GetMldAddress(hdr.GetAddr2()) !=
                            mle->GetMldMacAddress(),
                        "The AP MLD MAC address in the received Multi-Link Element does not "
                        "match the address stored in the station manager for link "
                            << +linkId);
        // process the Per-STA Profile Subelements in the Multi-Link Element
        for (std::size_t elem = 0; elem < mle->GetNPerStaProfileSubelements(); elem++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(elem);
            uint8_t apLinkId = perStaProfile.GetLinkId();
            auto it = GetLinks().find(apLinkId);
            uint8_t staLinkid = 0;
            std::optional<Mac48Address> bssid;
            NS_ABORT_MSG_IF(it == GetLinks().cend() ||
                                !(bssid = GetLink((staLinkid = it->first)).bssid).has_value(),
                            "Setup for AP link ID " << apLinkId << " was not requested");
            NS_ABORT_MSG_IF(*bssid != perStaProfile.GetStaMacAddress(),
                            "The BSSID in the Per-STA Profile for link ID "
                                << +staLinkid << " does not match the stored BSSID");
            NS_ABORT_MSG_IF(GetWifiRemoteStationManager(staLinkid)->GetMldAddress(
                                perStaProfile.GetStaMacAddress()) != mle->GetMldMacAddress(),
                            "The AP MLD MAC address in the received Multi-Link Element does not "
                            "match the address stored in the station manager for link "
                                << +staLinkid);
            // process the Association Response contained in this Per-STA Profile
            MgtAssocResponseHeader assoc = perStaProfile.GetAssocResponse();
            RecordCapabilities(assoc, *bssid, staLinkid);
            RecordOperations(assoc, *bssid, staLinkid);
            if (assoc.GetStatusCode().IsSuccess())
            {
                NS_ABORT_MSG_IF(m_aid != 0 && m_aid != assoc.GetAssociationId(),
                                "AID should be the same for all the links");
                m_aid = assoc.GetAssociationId();
                NS_LOG_DEBUG("Setup on link " << staLinkid << " completed");
                ApplyOperationalSettings(assocResp, *bssid, *bssid, staLinkid);
                SetBssid(*bssid, staLinkid);
                m_setupCompleted(staLinkid, *bssid);
                SetState(ASSOCIATED);
                apMldAddress = GetWifiRemoteStationManager(staLinkid)->GetMldAddress(*bssid);
                if (!m_linkUp.IsNull())
                {
                    m_linkUp();
                }
            }
            // remove the ID of the link we setup
            setupLinks.remove(staLinkid);
        }
        if (apMldAddress)
        {
            // this is an ML setup, trace the MLD address of the AP (only once)
            m_assocLogger(*apMldAddress);
        }
    }
    // remaining links in setupLinks are not setup and hence must be disabled
    for (const auto& id : setupLinks)
    {
        GetLink(id).bssid = std::nullopt;
        GetLink(id).phy->SetOffMode();
    }

    // the station that associated with the AP may have dissociated and then associated again.
    // In this case, the station may store packets from the previous period in which it was
    // associated. Have the station restart access if it has packets queued.
    for (const auto& [id, link] : GetLinks())
    {
        if (GetStaLink(link).bssid)
        {
            if (const auto txop = GetTxop())
            {
                txop->StartAccessAfterEvent(id,
                                            Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                            Txop::CHECK_MEDIUM_BUSY);
            }
            for (const auto& [acIndex, ac] : wifiAcList)
            {
                if (const auto edca = GetQosTxop(acIndex))
                {
                    edca->StartAccessAfterEvent(id,
                                                Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                                Txop::CHECK_MEDIUM_BUSY);
                }
            }
        }
    }

    SetPmModeAfterAssociation(linkId);
}

void
StaWifiMac::SetPmModeAfterAssociation(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // STAs operating on setup links may need to transition to a new PM mode after the
    // acknowledgement of the Association Response. For this purpose, we connect a callback to
    // the PHY TX begin trace to catch the Ack transmitted after the Association Response.
    CallbackBase cb = Callback<void, WifiConstPsduMap, WifiTxVector, Watt_u>(
        [=, this](WifiConstPsduMap psduMap, WifiTxVector txVector, Watt_u /* txPower */) {
            NS_ASSERT_MSG(psduMap.size() == 1 && psduMap.begin()->second->GetNMpdus() == 1 &&
                              psduMap.begin()->second->GetHeader(0).IsAck(),
                          "Expected a Normal Ack after Association Response frame");

            auto ackDuration =
                WifiPhy::CalculateTxDuration(psduMap, txVector, GetLink(linkId).phy->GetPhyBand());

            for (const auto& [id, lnk] : GetLinks())
            {
                auto& link = GetStaLink(lnk);

                if (!link.bssid)
                {
                    // link has not been setup
                    continue;
                }

                if (id == linkId)
                {
                    /**
                     * When a link becomes enabled for a non-AP STA that is affiliated with a
                     * non-AP MLD after successful association with an AP MLD with (Re)Association
                     * Request/Response  frames transmitted on that link [..], the power management
                     * mode of the non-AP STA, immediately after the acknowledgement of the
                     * (Re)Association Response frame [..], is active mode.
                     * (Sec. 35.3.7.1.4 of 802.11be D3.0)
                     */
                    // if the user requested this link to be in powersave mode, we have to
                    // switch PM mode
                    if (link.pmMode == WIFI_PM_POWERSAVE)
                    {
                        Simulator::Schedule(ackDuration,
                                            &StaWifiMac::SetPowerSaveMode,
                                            this,
                                            std::pair<bool, uint8_t>{true, id});
                    }
                    link.pmMode = WIFI_PM_ACTIVE;
                }
                else
                {
                    /**
                     * When a link becomes enabled for a non-AP STA that is affiliated with a
                     * non-AP MLD after successful association with an AP MLD with (Re)Association
                     * Request/Response frames transmitted on another link [..], the power
                     * management mode of the non-AP STA, immediately after the acknowledgement of
                     * the (Re)Association Response frame [..], is power save mode, and its power
                     * state is doze. (Sec. 35.3.7.1.4 of 802.11be D3.0)
                     */
                    // if the user requested this link to be in active mode, we have to
                    // switch PM mode
                    if (link.pmMode == WIFI_PM_ACTIVE)
                    {
                        Simulator::Schedule(ackDuration,
                                            &StaWifiMac::SetPowerSaveMode,
                                            this,
                                            std::pair<bool, uint8_t>{false, id});
                    }
                    link.pmMode = WIFI_PM_POWERSAVE;
                }
            }
        });

    // connect the callback to the PHY TX begin trace to catch the Ack and disconnect
    // after its transmission begins
    auto phy = GetLink(linkId).phy;
    phy->TraceConnectWithoutContext("PhyTxPsduBegin", cb);
    Simulator::Schedule(phy->GetSifs() + NanoSeconds(1),
                        [=]() { phy->TraceDisconnectWithoutContext("PhyTxPsduBegin", cb); });
}

bool
StaWifiMac::CheckSupportedRates(std::variant<MgtBeaconHeader, MgtProbeResponseHeader> frame,
                                uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);

    // lambda to invoke on the current frame variant
    auto check = [&](auto&& mgtFrame) -> bool {
        // check supported rates
        NS_ASSERT(mgtFrame.template Get<SupportedRates>());
        const auto rates = AllSupportedRates{*mgtFrame.template Get<SupportedRates>(),
                                             mgtFrame.template Get<ExtendedSupportedRatesIE>()};
        for (const auto& selector : GetWifiPhy(linkId)->GetBssMembershipSelectorList())
        {
            if (!rates.IsBssMembershipSelectorRate(selector))
            {
                NS_LOG_DEBUG("Supported rates do not fit with the BSS membership selector");
                return false;
            }
        }

        return true;
    };

    return std::visit(check, frame);
}

void
StaWifiMac::RecordOperations(const MgtFrameType& frame, const Mac48Address& from, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << frame.index() << from << linkId);
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);
    auto phy = GetWifiPhy(linkId);

    // lambda processing Information Elements included in all frame types
    auto recordFromOpIes = [&](auto&& frame) {
        const auto& edcaParameters = frame.template Get<EdcaParameterSet>();
        const auto qosSupported = edcaParameters.has_value();
        GetWifiRemoteStationManager(linkId)->SetQosSupport(from, qosSupported);

        if (GetHtSupported(linkId))
        {
            /* HT station */
            if (const auto& htOperation = frame.template Get<HtOperation>())
            {
                remoteStationManager->AddStationHtOperation(from, *htOperation);
            }
        }

        if (GetVhtSupported(linkId))
        {
            /* VHT station */
            if (const auto& vhtOperation = frame.template Get<VhtOperation>())
            {
                remoteStationManager->AddStationVhtOperation(from, *vhtOperation);
            }
        }

        if (!GetHeSupported())
        {
            return;
        }
        /* HE station */
        if (const auto& heOperation = frame.template Get<HeOperation>())
        {
            remoteStationManager->AddStationHeOperation(from, *heOperation);
        }

        if (!GetEhtSupported())
        {
            return;
        }
        /* EHT station */
        if (const auto& ehtOperation = frame.template Get<EhtOperation>())
        {
            remoteStationManager->AddStationEhtOperation(from, *ehtOperation);
        }
    };

    // process Information Elements included in the current frame variant
    std::visit(recordFromOpIes, frame);
}

void
StaWifiMac::ApplyOperationalSettings(const MgtFrameType& frame,
                                     const Mac48Address& apAddr,
                                     const Mac48Address& bssid,
                                     uint8_t linkId)
{
    NS_LOG_FUNCTION(this << frame.index() << apAddr << bssid << +linkId);

    // ERP Information is not present in Association Response frames
    const std::optional<ErpInformation>* erpInformation = nullptr;

    if (const auto* beacon = std::get_if<MgtBeaconHeader>(&frame))
    {
        erpInformation = &beacon->Get<ErpInformation>();
    }
    else if (const auto* probe = std::get_if<MgtProbeResponseHeader>(&frame))
    {
        erpInformation = &probe->Get<ErpInformation>();
    }

    // lambda processing Information Elements included in all frame types sent by APs
    auto processOtherIes = [&](auto&& frame) {
        const auto& capabilities = frame.Capabilities();
        bool isShortPreambleEnabled = capabilities.IsShortPreamble();
        auto remoteStationManager = GetWifiRemoteStationManager(linkId);
        if (erpInformation && erpInformation->has_value() && GetErpSupported(linkId))
        {
            isShortPreambleEnabled &= !(*erpInformation)->GetBarkerPreambleMode();
            if ((*erpInformation)->GetUseProtection() != 0)
            {
                remoteStationManager->SetUseNonErpProtection(true);
            }
            else
            {
                remoteStationManager->SetUseNonErpProtection(false);
            }
            if (capabilities.IsShortSlotTime() == true)
            {
                // enable short slot time
                GetWifiPhy(linkId)->SetSlot(MicroSeconds(9));
            }
            else
            {
                // disable short slot time
                GetWifiPhy(linkId)->SetSlot(MicroSeconds(20));
            }
        }
        remoteStationManager->SetShortPreambleEnabled(isShortPreambleEnabled);
        remoteStationManager->SetShortSlotTimeEnabled(capabilities.IsShortSlotTime());

        if (!GetQosSupported())
        {
            return;
        }
        /* QoS station */
        const auto& edcaParameters = frame.template Get<EdcaParameterSet>();
        if (edcaParameters.has_value())
        {
            // The value of the TXOP Limit field is specified as an unsigned integer, with the least
            // significant octet transmitted first, in units of 32 μs.
            SetEdcaParameters({AC_BE,
                               edcaParameters->GetBeCWmin(),
                               edcaParameters->GetBeCWmax(),
                               edcaParameters->GetBeAifsn(),
                               32 * MicroSeconds(edcaParameters->GetBeTxopLimit())},
                              linkId);
            SetEdcaParameters({AC_BK,
                               edcaParameters->GetBkCWmin(),
                               edcaParameters->GetBkCWmax(),
                               edcaParameters->GetBkAifsn(),
                               32 * MicroSeconds(edcaParameters->GetBkTxopLimit())},
                              linkId);
            SetEdcaParameters({AC_VI,
                               edcaParameters->GetViCWmin(),
                               edcaParameters->GetViCWmax(),
                               edcaParameters->GetViAifsn(),
                               32 * MicroSeconds(edcaParameters->GetViTxopLimit())},
                              linkId);
            SetEdcaParameters({AC_VO,
                               edcaParameters->GetVoCWmin(),
                               edcaParameters->GetVoCWmax(),
                               edcaParameters->GetVoAifsn(),
                               32 * MicroSeconds(edcaParameters->GetVoTxopLimit())},
                              linkId);
        }

        if (GetHtSupported(linkId))
        {
            /* HT station */
            if (const auto& htCapabilities = frame.template Get<HtCapabilities>();
                !htCapabilities.has_value())
            {
                remoteStationManager->RemoveAllSupportedMcs(apAddr);
            }
        }

        if (!GetHeSupported())
        {
            return;
        }
        /* HE station */
        if (const auto& heOperation = frame.template Get<HeOperation>())
        {
            GetHeConfiguration()->m_bssColor = heOperation->m_bssColorInfo.m_bssColor;
        }

        const auto& muEdcaParameters = frame.template Get<MuEdcaParameterSet>();
        if (muEdcaParameters.has_value())
        {
            SetMuEdcaParameters({AC_BE,
                                 muEdcaParameters->GetMuCwMin(AC_BE),
                                 muEdcaParameters->GetMuCwMax(AC_BE),
                                 muEdcaParameters->GetMuAifsn(AC_BE),
                                 muEdcaParameters->GetMuEdcaTimer(AC_BE)},
                                linkId);
            SetMuEdcaParameters({AC_BK,
                                 muEdcaParameters->GetMuCwMin(AC_BK),
                                 muEdcaParameters->GetMuCwMax(AC_BK),
                                 muEdcaParameters->GetMuAifsn(AC_BK),
                                 muEdcaParameters->GetMuEdcaTimer(AC_BK)},
                                linkId);
            SetMuEdcaParameters({AC_VI,
                                 muEdcaParameters->GetMuCwMin(AC_VI),
                                 muEdcaParameters->GetMuCwMax(AC_VI),
                                 muEdcaParameters->GetMuAifsn(AC_VI),
                                 muEdcaParameters->GetMuEdcaTimer(AC_VI)},
                                linkId);
            SetMuEdcaParameters({AC_VO,
                                 muEdcaParameters->GetMuCwMin(AC_VO),
                                 muEdcaParameters->GetMuCwMax(AC_VO),
                                 muEdcaParameters->GetMuAifsn(AC_VO),
                                 muEdcaParameters->GetMuEdcaTimer(AC_VO)},
                                linkId);
        }

        if (!GetEhtSupported())
        {
            return;
        }
        /* EHT station */
        if (const auto& mle = frame.template Get<MultiLinkElement>(); mle && m_emlsrManager)
        {
            if (mle->HasEmlCapabilities())
            {
                m_emlsrManager->SetTransitionTimeout(mle->GetTransitionTimeout());
            }
            if (const auto& common = mle->GetCommonInfoBasic(); common.m_mediumSyncDelayInfo)
            {
                m_emlsrManager->SetMediumSyncDuration(common.GetMediumSyncDelayTimer());
                m_emlsrManager->SetMediumSyncOfdmEdThreshold(common.GetMediumSyncOfdmEdThreshold());
                m_emlsrManager->SetMediumSyncMaxNTxops(common.GetMediumSyncMaxNTxops());
            }
        }
    };

    // process Information Elements included in the current frame variant
    std::visit(processOtherIes, frame);
}

void
StaWifiMac::SetPowerSaveMode(const std::pair<bool, uint8_t>& enableLinkIdPair)
{
    const auto [enable, linkId] = enableLinkIdPair;
    NS_LOG_FUNCTION(this << enable << linkId);

    auto& link = GetLink(linkId);

    if (!IsAssociated())
    {
        NS_LOG_DEBUG("Not associated yet, record the PM mode to switch to upon association");
        link.pmMode = enable ? WIFI_PM_POWERSAVE : WIFI_PM_ACTIVE;
        return;
    }

    if (!link.bssid)
    {
        NS_LOG_DEBUG("Link " << +linkId << " has not been setup, ignore request");
        return;
    }

    if ((enable && link.pmMode == WIFI_PM_POWERSAVE) || (!enable && link.pmMode == WIFI_PM_ACTIVE))
    {
        NS_LOG_DEBUG("No PM mode change needed");
        return;
    }

    link.pmMode = enable ? WIFI_PM_SWITCHING_TO_PS : WIFI_PM_SWITCHING_TO_ACTIVE;

    // reschedule a call to this function to make sure that the PM mode switch
    // is eventually completed
    Simulator::Schedule(m_pmModeSwitchTimeout,
                        &StaWifiMac::SetPowerSaveMode,
                        this,
                        enableLinkIdPair);

    if (HasFramesToTransmit(linkId))
    {
        NS_LOG_DEBUG("Next transmitted frame will be sent with PM=" << enable);
        return;
    }

    // No queued frames. Enqueue a Data Null frame to inform the AP of the PM mode change
    WifiMacHeader hdr(WIFI_MAC_DATA_NULL);

    hdr.SetAddr1(GetBssid(linkId));
    hdr.SetAddr2(GetFrameExchangeManager(linkId)->GetAddress());
    hdr.SetAddr3(GetBssid(linkId));
    hdr.SetDsNotFrom();
    hdr.SetDsTo();
    enable ? hdr.SetPowerManagement() : hdr.SetNoPowerManagement();
    if (GetQosSupported())
    {
        GetQosTxop(AC_BE)->Queue(Create<WifiMpdu>(Create<Packet>(), hdr));
    }
    else
    {
        m_txop->Queue(Create<WifiMpdu>(Create<Packet>(), hdr));
    }
}

WifiPowerManagementMode
StaWifiMac::GetPmMode(uint8_t linkId) const
{
    return GetLink(linkId).pmMode;
}

void
StaWifiMac::TxOk(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    auto linkId = GetLinkIdByAddress(mpdu->GetHeader().GetAddr2());

    if (!linkId)
    {
        // the given MPDU may be the original copy containing MLD addresses and not carrying
        // a valid PM bit (which is set on the aliases).
        auto linkIds = mpdu->GetInFlightLinkIds();
        NS_ASSERT_MSG(!linkIds.empty(),
                      "The TA of the acked MPDU (" << *mpdu
                                                   << ") is not a link "
                                                      "address and the MPDU is not inflight");
        // in case the ack'ed MPDU is inflight on multiple links, we cannot really know if
        // it was received by the AP on all links or only on some links. Hence, we only
        // consider the first link ID in the set, given that in the most common case of MPDUs
        // that cannot be sent concurrently on multiple links, there will be only one link ID
        linkId = *linkIds.begin();
        mpdu = GetTxopQueue(mpdu->GetQueueAc())->GetAlias(mpdu, *linkId);
    }

    auto& link = GetLink(*linkId);
    const WifiMacHeader& hdr = mpdu->GetHeader();

    // we received an acknowledgment while switching PM mode; the PM mode change is effective now
    if (hdr.IsPowerManagement() && link.pmMode == WIFI_PM_SWITCHING_TO_PS)
    {
        link.pmMode = WIFI_PM_POWERSAVE;
    }
    else if (!hdr.IsPowerManagement() && link.pmMode == WIFI_PM_SWITCHING_TO_ACTIVE)
    {
        link.pmMode = WIFI_PM_ACTIVE;
    }
}

AllSupportedRates
StaWifiMac::GetSupportedRates(uint8_t linkId) const
{
    AllSupportedRates rates;
    for (const auto& mode : GetWifiPhy(linkId)->GetModeList())
    {
        uint64_t modeDataRate = mode.GetDataRate(GetWifiPhy(linkId)->GetChannelWidth());
        NS_LOG_DEBUG("Adding supported rate of " << modeDataRate);
        rates.AddSupportedRate(modeDataRate);
    }
    if (GetHtSupported(linkId))
    {
        for (const auto& selector : GetWifiPhy(linkId)->GetBssMembershipSelectorList())
        {
            rates.AddBssMembershipSelectorRate(selector);
        }
    }
    return rates;
}

CapabilityInformation
StaWifiMac::GetCapabilities(uint8_t linkId) const
{
    CapabilityInformation capabilities;
    capabilities.SetShortPreamble(GetWifiPhy(linkId)->GetShortPhyPreambleSupported() ||
                                  GetErpSupported(linkId));
    capabilities.SetShortSlotTime(GetShortSlotTimeSupported() && GetErpSupported(linkId));
    return capabilities;
}

void
StaWifiMac::SetState(MacState value)
{
    m_state = value;
}

void
StaWifiMac::SetEdcaParameters(const EdcaParams& params, uint8_t linkId)
{
    Ptr<QosTxop> edca = GetQosTxop(params.ac);
    edca->SetMinCw(params.cwMin, linkId);
    edca->SetMaxCw(params.cwMax, linkId);
    edca->SetAifsn(params.aifsn, linkId);
    edca->SetTxopLimit(params.txopLimit, linkId);
}

void
StaWifiMac::SetMuEdcaParameters(const MuEdcaParams& params, uint8_t linkId)
{
    Ptr<QosTxop> edca = GetQosTxop(params.ac);
    edca->SetMuCwMin(params.cwMin, linkId);
    edca->SetMuCwMax(params.cwMax, linkId);
    edca->SetMuAifsn(params.aifsn, linkId);
    edca->SetMuEdcaTimer(params.muEdcaTimer, linkId);
}

void
StaWifiMac::PhyCapabilitiesChanged()
{
    NS_LOG_FUNCTION(this);
    if (IsAssociated())
    {
        NS_LOG_DEBUG("PHY capabilities changed: send reassociation request");
        SetState(WAIT_ASSOC_RESP);
        SendAssociationRequest(true);
    }
}

/**
 * Initial configuration:
 *
 *        ┌───┬───┬───┐        ┌────┐       ┌───────┐
 * Link A │FEM│RSM│CAM│◄──────►│Main├──────►│Channel│
 *        │   │   │   │        │PHY │       │   A   │
 *        └───┴───┴───┘        └────┘       └───────┘
 *
 *        ┌───┬───┬───┐        ┌────┐       ┌───────┐
 * Link B │FEM│RSM│CAM│        │Aux │       │Channel│
 *        │   │   │   │◄──────►│PHY ├──────►│   B   │
 *        └───┴───┴───┘        └────┘       └───────┘
 *
 * A link switching/swapping is notified by the EMLSR Manager and the Channel Access Manager
 * (CAM) notifies us that a first PHY (i.e., the Main PHY) switches to Channel B. We connect
 * the Main PHY to the MAC stack B:
 *
 *
 *        ┌───┬───┬───┐        ┌────┐       ┌───────┐
 * Link A │FEM│RSM│CAM│   ┌───►│Main├───┐   │Channel│
 *        │   │   │   │   │    │PHY │   │   │   A   │
 *        └───┴───┴───┘   │    └────┘   │   └───────┘
 *                        │             │
 *        ┌───┬───┬───┐   │    ┌────┐   │   ┌───────┐
 * Link B │FEM│RSM│CAM│◄──┘    │Aux │   └──►│Channel│
 *        │   │   │   │◄─ ─ ─ ─│PHY ├──────►│   B   │
 *        └───┴───┴───┘INACTIVE└────┘       └───────┘
 *
 * MAC stack B keeps a PHY listener associated with the Aux PHY, even though it is inactive,
 * meaning that the PHY listener will only notify channel switches (no CCA, no RX).
 * If the EMLSR Manager requested a link switching, this configuration will be kept until
 * further requests. If the EMLSR Manager requested a link swapping, link B's CAM will be
 * notified by its (inactive) PHY listener upon the channel switch performed by the Aux PHY.
 * In this case, we remove the inactive PHY listener and connect the Aux PHY to MAC stack A:
 *
 *        ┌───┬───┬───┐        ┌────┐       ┌───────┐
 * Link A │FEM│RSM│CAM│◄─┐ ┌──►│Main├───┐   │Channel│
 *        │   │   │   │  │ │   │PHY │ ┌─┼──►│   A   │
 *        └───┴───┴───┘  │ │   └────┘ │ │   └───────┘
 *                       │ │          │ │
 *        ┌───┬───┬───┐  │ │   ┌────┐ │ │   ┌───────┐
 * Link B │FEM│RSM│CAM│◄─┼─┘   │Aux │ │ └──►│Channel│
 *        │   │   │   │  └────►│PHY ├─┘     │   B   │
 *        └───┴───┴───┘        └────┘       └───────┘
 */

void
StaWifiMac::NotifySwitchingEmlsrLink(Ptr<WifiPhy> phy, uint8_t linkId, Time delay)
{
    NS_LOG_FUNCTION(this << phy << linkId << delay.As(Time::US));

    // if the PHY that is starting a channel switch was operating on a link (i.e., there is a link,
    // other than the new link, that points to the PHY), then it is no longer operating on that
    // link and we have to reset the phy pointer of the link.
    for (auto& [id, link] : GetLinks())
    {
        if (link->phy == phy && id != linkId)
        {
            // we do not get here if the PHY is not operating on any link, which happens if:
            // - PHY is an aux PHY to reconnect to its link
            // - PHY is an aux PHY that is starting switching to the link previously occupied by the
            //   main PHY (because the main PHY is now operating on the aux PHY link)
            // - PHY is the main PHY that completed the channel switch but connecting it to the link
            //   was postponed until now (e.g. because the aux PHY on the link was receiving an ICF)
            // - PHY is the main PHY that was switching, the switch was interrupted and it is
            //   now starting switching to another link
            link->phy = nullptr;
            m_emlsrLinkSwitchLogger(id, phy, false);
        }
    }

    // lambda to connect the PHY to the new link
    auto connectPhy = [=, this]() mutable {
        auto& newLink = GetLink(linkId);
        // The MAC stack associated with the new link uses the given PHY
        newLink.phy = phy;
        // Setup a PHY listener for the given PHY on the CAM associated with the new link
        newLink.channelAccessManager->SetupPhyListener(phy);
        NS_ASSERT(m_emlsrManager);
        if (m_emlsrManager->GetCamStateReset())
        {
            newLink.channelAccessManager->ResetState();
        }
        // Disconnect the FEM on the new link from the current PHY
        newLink.feManager->ResetPhy();
        // Connect the FEM on the new link to the given PHY
        newLink.feManager->SetWifiPhy(phy);
        // Connect the station manager on the new link to the given PHY
        newLink.stationManager->SetupPhy(phy);
        // log link switch
        m_emlsrLinkSwitchLogger(linkId, phy, true);
    };

    // cancel any pending event for the given PHY to switch link
    CancelEmlsrPhyConnectEvent(phy->GetPhyId());

    // connect the PHY to the new link when the channel switch is completed, unless there is a PHY
    // operating on the new link that is possibly receiving an ICF, in which case the PHY is
    // connected when the frame reception is completed
    if (delay.IsStrictlyPositive())
    {
        auto lambda = [=, this]() mutable {
            const auto [maybeIcf, extension] = m_emlsrManager->CheckPossiblyReceivingIcf(linkId);
            if (maybeIcf && extension.IsStrictlyPositive())
            {
                NS_ASSERT_MSG(phy->GetPhyId() == m_emlsrManager->GetMainPhyId(),
                              "Only the main PHY is expected to move to a link on which another "
                              "PHY is operating. PHY ID="
                                  << +phy->GetPhyId());
                NS_LOG_DEBUG("Connecting main PHY to link " << +linkId << " is postponed by "
                                                            << extension.As(Time::US));
                NotifySwitchingEmlsrLink(phy, linkId, extension);
            }
            else
            {
                connectPhy();
            }
        };

        m_emlsrLinkSwitch.emplace(phy->GetPhyId(), Simulator::Schedule(delay, lambda));
    }
    else
    {
        connectPhy();
    }
}

void
StaWifiMac::CancelEmlsrPhyConnectEvent(uint8_t phyId)
{
    NS_LOG_FUNCTION(this << phyId);
    if (auto eventIt = m_emlsrLinkSwitch.find(phyId); eventIt != m_emlsrLinkSwitch.end())
    {
        eventIt->second.Cancel();
        m_emlsrLinkSwitch.erase(eventIt);
    }
}

void
StaWifiMac::NotifyChannelSwitching(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);

    WifiMac::NotifyChannelSwitching(linkId);

    if (IsInitialized() && IsAssociated())
    {
        Disassociated();
    }

    // notify association manager
    m_assocManager->NotifyChannelSwitched(linkId);
}

std::ostream&
operator<<(std::ostream& os, const StaWifiMac::ApInfo& apInfo)
{
    os << "BSSID=" << apInfo.m_bssid << ", AP addr=" << apInfo.m_apAddr << ", SNR=" << apInfo.m_snr
       << ", Channel={" << apInfo.m_channel.number << "," << apInfo.m_channel.band
       << "}, Link ID=" << +apInfo.m_linkId << ", Frame=[";
    std::visit([&os](auto&& frame) { frame.Print(os); }, apInfo.m_frame);
    os << "]";
    return os;
}

} // namespace ns3
