/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ap-wifi-mac.h"

#include "amsdu-subframe-header.h"
#include "channel-access-manager.h"
#include "gcr-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mgt-action-headers.h"
#include "msdu-aggregator.h"
#include "qos-txop.h"
#include "reduced-neighbor-report.h"
#include "wifi-mac-queue-scheduler.h"
#include "wifi-mac-queue.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/ap-emlsr-manager.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED(ApWifiMac);

TypeId
ApWifiMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ApWifiMac")
            .SetParent<WifiMac>()
            .SetGroupName("Wifi")
            .AddConstructor<ApWifiMac>()
            .AddAttribute(
                "BeaconInterval",
                "Delay between two beacons",
                TimeValue(MicroSeconds(102400)),
                MakeTimeAccessor(&ApWifiMac::GetBeaconInterval, &ApWifiMac::SetBeaconInterval),
                MakeTimeChecker())
            .AddAttribute("BeaconJitter",
                          "A uniform random variable to cause the initial beacon starting time "
                          "(after simulation time 0) "
                          "to be distributed between 0 and the BeaconInterval.",
                          StringValue("ns3::UniformRandomVariable"),
                          MakePointerAccessor(&ApWifiMac::m_beaconJitter),
                          MakePointerChecker<UniformRandomVariable>())
            .AddAttribute("EnableBeaconJitter",
                          "If beacons are enabled, whether to jitter the initial send event.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ApWifiMac::m_enableBeaconJitter),
                          MakeBooleanChecker())
            .AddAttribute("BeaconGeneration",
                          "Whether or not beacons are generated.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ApWifiMac::SetBeaconGeneration),
                          MakeBooleanChecker())
            .AddAttribute("FdBeaconInterval6GHz",
                          "Time between a Beacon frame and a FILS Discovery (FD) frame or between "
                          "two FD frames to be sent on a 6GHz link. A value of zero disables the "
                          "transmission of FD frames.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&ApWifiMac::m_fdBeaconInterval6GHz),
                          MakeTimeChecker())
            .AddAttribute("FdBeaconIntervalNon6GHz",
                          "Time between a Beacon frame and a FILS Discovery (FD) frame or between "
                          "two FD frames to be sent on a non-6GHz link. A value of zero disables "
                          "the transmission of FD frames.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&ApWifiMac::m_fdBeaconIntervalNon6GHz),
                          MakeTimeChecker())
            .AddAttribute("SendUnsolProbeResp",
                          "Send unsolicited broadcast Probe Response instead of FILS Discovery",
                          BooleanValue(false),
                          MakeBooleanAccessor(&ApWifiMac::m_sendUnsolProbeResp),
                          MakeBooleanChecker())
            .AddAttribute("EnableNonErpProtection",
                          "Whether or not protection mechanism should be used when non-ERP STAs "
                          "are present within the BSS."
                          "This parameter is only used when ERP is supported by the AP.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ApWifiMac::m_enableNonErpProtection),
                          MakeBooleanChecker())
            .AddAttribute("BsrLifetime",
                          "Lifetime of Buffer Status Reports received from stations.",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&ApWifiMac::m_bsrLifetime),
                          MakeTimeChecker())
            .AddAttribute(
                "CwMinsForSta",
                "The CW min values that the AP advertises in EDCA Parameter Set elements and the "
                "associated stations will use. The value of this attribute is an AC-indexed map "
                "containing the CW min values for given ACs for all the links (sorted in "
                "increasing order of link ID). If no values are provided for an AC, the same "
                "values used by the AP are advertised. In case a string is used to set this "
                "attribute, the string shall contain the pairs separated by a semicolon (;); "
                "in every pair, the AC index and the list of values are separated by a blank "
                "space, and the values of a list are separated by a comma (,) without spaces. "
                "E.g. \"BE 31,31,31; VI 15,15,15\" defines the CW min values for AC BE and AC VI "
                "for an AP MLD having three links.",
                StringValue(""),
                MakeAttributeContainerAccessor<UintAccessParamsPairValue, ';'>(
                    &ApWifiMac::m_cwMinsForSta),
                GetUintAccessParamsChecker<uint32_t>())
            .AddAttribute(
                "CwMaxsForSta",
                "The CW max values that the AP advertises in EDCA Parameter Set elements and the "
                "associated stations will use. The value of this attribute is an AC-indexed map "
                "containing the CW max values for given ACs for all the links (sorted in "
                "increasing order of link ID). If no values are provided for an AC, the same "
                "values used by the AP are advertised. In case a string is used to set this "
                "attribute, the string shall contain the pairs separated by a semicolon (;); "
                "in every pair, the AC index and the list of values are separated by a blank "
                "space, and the values of a list are separated by a comma (,) without spaces. "
                "E.g. \"BE 31,31,31; VI 15,15,15\" defines the CW max values for AC BE and AC VI "
                "for an AP MLD having three links.",
                StringValue(""),
                MakeAttributeContainerAccessor<UintAccessParamsPairValue, ';'>(
                    &ApWifiMac::m_cwMaxsForSta),
                GetUintAccessParamsChecker<uint32_t>())
            .AddAttribute(
                "AifsnsForSta",
                "The AIFSN values that the AP advertises in EDCA Parameter Set elements and the "
                "associated stations will use. The value of this attribute is an AC-indexed map "
                "containing the AIFSN values for given ACs for all the links (sorted in "
                "increasing order of link ID). If no values are provided for an AC, the same "
                "values used by the AP are advertised. In case a string is used to set this "
                "attribute, the string shall contain the pairs separated by a semicolon (;); "
                "in every pair, the AC index and the list of values are separated by a blank "
                "space, and the values of a list are separated by a comma (,) without spaces. "
                "E.g. \"BE 3,3,3; VI 2,2,2\" defines the AIFSN values for AC BE and AC VI "
                "for an AP MLD having three links.",
                StringValue(""),
                MakeAttributeContainerAccessor<UintAccessParamsPairValue, ';'>(
                    &ApWifiMac::m_aifsnsForSta),
                GetUintAccessParamsChecker<uint8_t>())
            .AddAttribute(
                "TxopLimitsForSta",
                "The TXOP limit values that the AP advertises in EDCA Parameter Set elements and "
                "the associated stations will use. The value of this attribute is an AC-indexed "
                "map containing the TXOP limit values for given ACs for all the links (sorted in "
                "increasing order of link ID). If no values are provided for an AC, the same "
                "values used by the AP are advertised. In case a string is used to set this "
                "attribute, the string shall contain the pairs separated by a semicolon (;); "
                "in every pair, the AC index and the list of values are separated by a blank "
                "space, and the values of a list are separated by a comma (,) without spaces. "
                "E.g. \"BE 3200us,3200us,3200us; VI 2400us,2400us,2400us\" defines the TXOP limit "
                "values for AC BE and AC VI for an AP MLD having three links.",
                StringValue(""),
                MakeAttributeContainerAccessor<TimeAccessParamsPairValue, ';'>(
                    &ApWifiMac::m_txopLimitsForSta),
                GetTimeAccessParamsChecker())
            .AddAttribute("GcrManager",
                          "The GCR manager object.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          PointerValue(),
                          MakePointerAccessor(&ApWifiMac::GetGcrManager, &ApWifiMac::SetGcrManager),
                          MakePointerChecker<GcrManager>())
            .AddTraceSource("AssociatedSta",
                            "A station associated with this access point.",
                            MakeTraceSourceAccessor(&ApWifiMac::m_assocLogger),
                            "ns3::ApWifiMac::AssociationCallback")
            .AddTraceSource("DeAssociatedSta",
                            "A station lost association with this access point.",
                            MakeTraceSourceAccessor(&ApWifiMac::m_deAssocLogger),
                            "ns3::ApWifiMac::AssociationCallback");
    return tid;
}

template <class T>
Ptr<const AttributeChecker>
ApWifiMac::GetUintAccessParamsChecker()
{
    return MakeAttributeContainerChecker<UintAccessParamsPairValue, ';'>(
        MakePairChecker<EnumValue<AcIndex>,
                        AttributeContainerValue<UintegerValue, ',', std::vector>>(
            MakeEnumChecker(AC_BE, "BE", AC_BK, "BK", AC_VI, "VI", AC_VO, "VO"),
            MakeAttributeContainerChecker<UintegerValue, ',', std::vector>(
                MakeUintegerChecker<T>())));
}

Ptr<const AttributeChecker>
ApWifiMac::GetTimeAccessParamsChecker()
{
    return MakeAttributeContainerChecker<TimeAccessParamsPairValue, ';'>(
        MakePairChecker<EnumValue<AcIndex>, AttributeContainerValue<TimeValue, ',', std::vector>>(
            MakeEnumChecker(AC_BE, "BE", AC_BK, "BK", AC_VI, "VI", AC_VO, "VO"),
            MakeAttributeContainerChecker<TimeValue, ',', std::vector>(MakeTimeChecker())));
}

ApWifiMac::ApWifiMac()
    : m_enableBeaconGeneration(false)
{
    NS_LOG_FUNCTION(this);
    m_beaconTxop = CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BEACON"));
    m_beaconTxop->SetTxMiddle(m_txMiddle);

    // Let the lower layers know that we are acting as an AP.
    SetTypeOfStation(AP);
}

ApWifiMac::~ApWifiMac()
{
    NS_LOG_FUNCTION(this);
}

void
ApWifiMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_beaconTxop->Dispose();
    m_beaconTxop = nullptr;
    m_enableBeaconGeneration = false;
    if (m_apEmlsrManager)
    {
        m_apEmlsrManager->Dispose();
    }
    m_apEmlsrManager = nullptr;
    if (m_gcrManager)
    {
        m_gcrManager->Dispose();
    }
    m_gcrManager = nullptr;
    WifiMac::DoDispose();
}

ApWifiMac::ApLinkEntity::~ApLinkEntity()
{
    NS_LOG_FUNCTION_NOARGS();
    beaconEvent.Cancel();
}

std::unique_ptr<WifiMac::LinkEntity>
ApWifiMac::CreateLinkEntity() const
{
    return std::make_unique<ApLinkEntity>();
}

ApWifiMac::ApLinkEntity&
ApWifiMac::GetLink(uint8_t linkId) const
{
    return static_cast<ApLinkEntity&>(WifiMac::GetLink(linkId));
}

void
ApWifiMac::SetApEmlsrManager(Ptr<ApEmlsrManager> apEmlsrManager)
{
    NS_LOG_FUNCTION(this << apEmlsrManager);
    m_apEmlsrManager = apEmlsrManager;
    m_apEmlsrManager->SetWifiMac(this);
}

Ptr<ApEmlsrManager>
ApWifiMac::GetApEmlsrManager() const
{
    return m_apEmlsrManager;
}

void
ApWifiMac::SetGcrManager(Ptr<GcrManager> gcrManager)
{
    NS_LOG_FUNCTION(this << gcrManager);
    m_gcrManager = gcrManager;
    m_gcrManager->SetWifiMac(this);
}

Ptr<GcrManager>
ApWifiMac::GetGcrManager() const
{
    return m_gcrManager;
}

bool
ApWifiMac::UseGcr(const WifiMacHeader& hdr) const
{
    if (!hdr.IsQosData())
    {
        return false;
    }

    if (!IsGroupcast(hdr.GetAddr1()))
    {
        return false;
    }

    if (!m_gcrManager)
    {
        return false;
    }

    if (m_gcrManager->GetRetransmissionPolicy() ==
        GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY)
    {
        return false;
    }

    /*
     * 802.11-2020 11.21.16.3.4 (GCR operation):
     * An AP or mesh STA shall transmit a frame belonging to a group address
     * via the GCR service if any associated STA or peer mesh STA has a GCR
     * agreement for the group address and, otherwise, does not transmit the
     * frame via the GCR service.
     */
    if (m_gcrManager->GetMemberStasForGroupAddress(hdr.GetAddr1()).empty())
    {
        return false;
    }

    return true;
}

void
ApWifiMac::DoCompleteConfig()
{
    NS_LOG_FUNCTION(this);
    m_beaconTxop->SetWifiMac(this);
    // DCF behavior may be edited here; the default is PIFS access with zero backoff
    m_beaconTxop->SetAifsns(std::vector<uint8_t>(GetNLinks(), 1));
    m_beaconTxop->SetMinCws(std::vector<uint32_t>(GetNLinks(), 0));
    m_beaconTxop->SetMaxCws(std::vector<uint32_t>(GetNLinks(), 0));
    for (uint8_t linkId = 0; linkId < GetNLinks(); linkId++)
    {
        GetLink(linkId).channelAccessManager->Add(m_beaconTxop);
    }
}

Ptr<WifiMacQueue>
ApWifiMac::GetTxopQueue(AcIndex ac) const
{
    if (ac == AC_BEACON)
    {
        return m_beaconTxop->GetWifiMacQueue();
    }
    return WifiMac::GetTxopQueue(ac);
}

void
ApWifiMac::SetBeaconGeneration(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    for (uint8_t linkId = 0; linkId < GetNLinks(); ++linkId)
    {
        if (!enable)
        {
            GetLink(linkId).beaconEvent.Cancel();
        }
        else if (!m_enableBeaconGeneration)
        {
            GetLink(linkId).beaconEvent =
                Simulator::ScheduleNow(&ApWifiMac::SendOneBeacon, this, linkId);
        }
    }
    m_enableBeaconGeneration = enable;
}

Time
ApWifiMac::GetBeaconInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_beaconInterval;
}

void
ApWifiMac::SetLinkUpCallback(Callback<void> linkUp)
{
    NS_LOG_FUNCTION(this << &linkUp);
    WifiMac::SetLinkUpCallback(linkUp);

    // The approach taken here is that, from the point of view of an AP,
    // the link is always up, so we immediately invoke the callback if
    // one is set
    linkUp();
}

void
ApWifiMac::SetBeaconInterval(Time interval)
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

int64_t
ApWifiMac::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_beaconJitter->SetStream(stream);
    auto currentStream = stream + 1;
    currentStream += m_beaconTxop->AssignStreams(currentStream);
    currentStream += WifiMac::AssignStreams(currentStream);
    return (currentStream - stream);
}

void
ApWifiMac::UpdateShortSlotTimeEnabled(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    auto& link = GetLink(linkId);
    if (GetErpSupported(linkId) && GetShortSlotTimeSupported() && (link.numNonErpStations == 0))
    {
        for (const auto& sta : link.staList)
        {
            if (!GetWifiRemoteStationManager(linkId)->GetShortSlotTimeSupported(sta.second))
            {
                link.shortSlotTimeEnabled = false;
                return;
            }
        }
        link.shortSlotTimeEnabled = true;
    }
    else
    {
        link.shortSlotTimeEnabled = false;
    }
}

void
ApWifiMac::UpdateShortPreambleEnabled(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    auto& link = GetLink(linkId);
    if (GetErpSupported(linkId) && GetWifiPhy(linkId)->GetShortPhyPreambleSupported())
    {
        for (const auto& sta : link.staList)
        {
            if (!GetWifiRemoteStationManager(linkId)->GetErpOfdmSupported(sta.second) ||
                !GetWifiRemoteStationManager(linkId)->GetShortPreambleSupported(sta.second))
            {
                link.shortPreambleEnabled = false;
                return;
            }
        }
        link.shortPreambleEnabled = true;
    }
    else
    {
        link.shortPreambleEnabled = false;
    }
}

bool
ApWifiMac::CanForwardPacketsTo(Mac48Address to) const
{
    return (to.IsGroup() || IsAssociated(to));
}

void
ApWifiMac::Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from)
{
    NS_LOG_FUNCTION(this << *mpdu << to << from);

    std::list<Mac48Address> addr2Set;
    if (to.IsGroup())
    {
        // broadcast frames are transmitted on all the links
        for (uint8_t linkId = 0; linkId < GetNLinks(); linkId++)
        {
            addr2Set.push_back(GetFrameExchangeManager(linkId)->GetAddress());
        }
    }
    else
    {
        // the Transmitter Address (TA) is the MLD address only for non-broadcast data frames
        // exchanged between two MLDs
        addr2Set = {GetAddress()};
        auto linkId = IsAssociated(to);
        NS_ASSERT_MSG(linkId, "Station " << to << "is not associated, cannot send it a frame");
        if (GetNLinks() == 1 || !GetWifiRemoteStationManager(*linkId)->GetMldAddress(to))
        {
            addr2Set = {GetFrameExchangeManager(*linkId)->GetAddress()};
        }
    }

    for (auto addr2 = addr2Set.cbegin(); addr2 != addr2Set.cend(); ++addr2)
    {
        auto& hdr = mpdu->GetHeader();

        hdr.SetAddr1(to);
        hdr.SetAddr2(*addr2);
        hdr.SetAddr3(from);
        hdr.SetDsFrom();
        hdr.SetDsNotTo();

        auto txop = hdr.IsQosData() ? StaticCast<Txop>(GetQosTxop(hdr.GetQosTid())) : GetTxop();
        NS_ASSERT(txop);
        txop->Queue(mpdu);

        // create another MPDU if needed
        if (std::next(addr2) != addr2Set.cend())
        {
            mpdu = Create<WifiMpdu>(mpdu->GetPacket()->Copy(), hdr);
        }
    }
}

bool
ApWifiMac::SupportsSendFrom() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

AllSupportedRates
ApWifiMac::GetSupportedRates(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    AllSupportedRates rates;
    // Send the set of supported rates and make sure that we indicate
    // the Basic Rate set in this set of supported rates.
    for (const auto& mode : GetWifiPhy(linkId)->GetModeList())
    {
        uint64_t modeDataRate = mode.GetDataRate(GetWifiPhy(linkId)->GetChannelWidth());
        NS_LOG_DEBUG("Adding supported rate of " << modeDataRate);
        rates.AddSupportedRate(modeDataRate);
        // Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
        // here we choose to add the mandatory rates to the BSSBasicRateSet,
        // except for 802.11b where we assume that only the non HR-DSSS rates are part of the
        // BSSBasicRateSet
        if (mode.IsMandatory() && (mode.GetModulationClass() != WIFI_MOD_CLASS_HR_DSSS))
        {
            NS_LOG_DEBUG("Adding basic mode " << mode.GetUniqueName());
            GetWifiRemoteStationManager(linkId)->AddBasicMode(mode);
        }
    }
    // set the basic rates
    for (uint8_t j = 0; j < GetWifiRemoteStationManager(linkId)->GetNBasicModes(); j++)
    {
        WifiMode mode = GetWifiRemoteStationManager(linkId)->GetBasicMode(j);
        uint64_t modeDataRate = mode.GetDataRate(GetWifiPhy(linkId)->GetChannelWidth());
        NS_LOG_DEBUG("Setting basic rate " << mode.GetUniqueName());
        rates.SetBasicRate(modeDataRate);
    }
    // If it is a HT AP, then add the BSSMembershipSelectorSet
    // The standard says that the BSSMembershipSelectorSet
    // must have its MSB set to 1 (must be treated as a Basic Rate)
    // Also the standard mentioned that at least 1 element should be included in the SupportedRates
    // the rest can be in the ExtendedSupportedRates
    if (GetHtSupported(linkId))
    {
        for (const auto& selector : GetWifiPhy(linkId)->GetBssMembershipSelectorList())
        {
            rates.AddBssMembershipSelectorRate(selector);
        }
    }
    return rates;
}

DsssParameterSet
ApWifiMac::GetDsssParameterSet(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetDsssSupported(linkId));
    DsssParameterSet dsssParameters;
    dsssParameters.SetCurrentChannel(GetWifiPhy(linkId)->GetChannelNumber());
    return dsssParameters;
}

CapabilityInformation
ApWifiMac::GetCapabilities(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    CapabilityInformation capabilities;
    capabilities.SetShortPreamble(GetLink(linkId).shortPreambleEnabled);
    capabilities.SetShortSlotTime(GetLink(linkId).shortSlotTimeEnabled);
    capabilities.SetEss();
    return capabilities;
}

ErpInformation
ApWifiMac::GetErpInformation(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetErpSupported(linkId));
    ErpInformation information;

    information.SetNonErpPresent(GetLink(linkId).numNonErpStations > 0);
    information.SetUseProtection(GetUseNonErpProtection(linkId));
    if (GetLink(linkId).shortPreambleEnabled)
    {
        information.SetBarkerPreambleMode(0);
    }
    else
    {
        information.SetBarkerPreambleMode(1);
    }

    return information;
}

EdcaParameterSet
ApWifiMac::GetEdcaParameterSet(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetQosSupported());
    EdcaParameterSet edcaParameters;

    Ptr<QosTxop> edca;
    Time txopLimit;

    edca = GetQosTxop(AC_BE);
    edcaParameters.SetBeAci(0);
    edcaParameters.SetBeCWmin(m_cwMinsForSta.contains(AC_BE) ? m_cwMinsForSta.at(AC_BE).at(linkId)
                                                             : edca->GetMinCw(linkId));
    edcaParameters.SetBeCWmax(m_cwMaxsForSta.contains(AC_BE) ? m_cwMaxsForSta.at(AC_BE).at(linkId)
                                                             : edca->GetMaxCw(linkId));
    edcaParameters.SetBeAifsn(m_aifsnsForSta.contains(AC_BE) ? m_aifsnsForSta.at(AC_BE).at(linkId)
                                                             : edca->GetAifsn(linkId));
    txopLimit = m_txopLimitsForSta.contains(AC_BE) ? m_txopLimitsForSta.at(AC_BE).at(linkId)
                                                   : edca->GetTxopLimit(linkId);
    edcaParameters.SetBeTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_BK);
    edcaParameters.SetBkAci(1);
    edcaParameters.SetBkCWmin(m_cwMinsForSta.contains(AC_BK) ? m_cwMinsForSta.at(AC_BK).at(linkId)
                                                             : edca->GetMinCw(linkId));
    edcaParameters.SetBkCWmax(m_cwMaxsForSta.contains(AC_BK) ? m_cwMaxsForSta.at(AC_BK).at(linkId)
                                                             : edca->GetMaxCw(linkId));
    edcaParameters.SetBkAifsn(m_aifsnsForSta.contains(AC_BK) ? m_aifsnsForSta.at(AC_BK).at(linkId)
                                                             : edca->GetAifsn(linkId));
    txopLimit = m_txopLimitsForSta.contains(AC_BK) ? m_txopLimitsForSta.at(AC_BK).at(linkId)
                                                   : edca->GetTxopLimit(linkId);
    edcaParameters.SetBkTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_VI);
    edcaParameters.SetViAci(2);
    edcaParameters.SetViCWmin(m_cwMinsForSta.contains(AC_VI) ? m_cwMinsForSta.at(AC_VI).at(linkId)
                                                             : edca->GetMinCw(linkId));
    edcaParameters.SetViCWmax(m_cwMaxsForSta.contains(AC_VI) ? m_cwMaxsForSta.at(AC_VI).at(linkId)
                                                             : edca->GetMaxCw(linkId));
    edcaParameters.SetViAifsn(m_aifsnsForSta.contains(AC_VI) ? m_aifsnsForSta.at(AC_VI).at(linkId)
                                                             : edca->GetAifsn(linkId));
    txopLimit = m_txopLimitsForSta.contains(AC_VI) ? m_txopLimitsForSta.at(AC_VI).at(linkId)
                                                   : edca->GetTxopLimit(linkId);
    edcaParameters.SetViTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edca = GetQosTxop(AC_VO);
    edcaParameters.SetVoAci(3);
    edcaParameters.SetVoCWmin(m_cwMinsForSta.contains(AC_VO) ? m_cwMinsForSta.at(AC_VO).at(linkId)
                                                             : edca->GetMinCw(linkId));
    edcaParameters.SetVoCWmax(m_cwMaxsForSta.contains(AC_VO) ? m_cwMaxsForSta.at(AC_VO).at(linkId)
                                                             : edca->GetMaxCw(linkId));
    edcaParameters.SetVoAifsn(m_aifsnsForSta.contains(AC_VO) ? m_aifsnsForSta.at(AC_VO).at(linkId)
                                                             : edca->GetAifsn(linkId));
    txopLimit = m_txopLimitsForSta.contains(AC_VO) ? m_txopLimitsForSta.at(AC_VO).at(linkId)
                                                   : edca->GetTxopLimit(linkId);
    edcaParameters.SetVoTxopLimit(static_cast<uint16_t>(txopLimit.GetMicroSeconds() / 32));

    edcaParameters.SetQosInfo(0);

    return edcaParameters;
}

std::optional<MuEdcaParameterSet>
ApWifiMac::GetMuEdcaParameterSet() const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(GetHeSupported());

    Ptr<HeConfiguration> heConfiguration = GetHeConfiguration();
    NS_ASSERT(heConfiguration);

    MuEdcaParameterSet muEdcaParameters;
    muEdcaParameters.SetQosInfo(0);

    muEdcaParameters.SetMuAifsn(AC_BE, heConfiguration->m_muBeAifsn);
    muEdcaParameters.SetMuCwMin(AC_BE, heConfiguration->m_muBeCwMin);
    muEdcaParameters.SetMuCwMax(AC_BE, heConfiguration->m_muBeCwMax);
    muEdcaParameters.SetMuEdcaTimer(AC_BE, heConfiguration->m_beMuEdcaTimer);

    muEdcaParameters.SetMuAifsn(AC_BK, heConfiguration->m_muBkAifsn);
    muEdcaParameters.SetMuCwMin(AC_BK, heConfiguration->m_muBkCwMin);
    muEdcaParameters.SetMuCwMax(AC_BK, heConfiguration->m_muBkCwMax);
    muEdcaParameters.SetMuEdcaTimer(AC_BK, heConfiguration->m_bkMuEdcaTimer);

    muEdcaParameters.SetMuAifsn(AC_VI, heConfiguration->m_muViAifsn);
    muEdcaParameters.SetMuCwMin(AC_VI, heConfiguration->m_muViCwMin);
    muEdcaParameters.SetMuCwMax(AC_VI, heConfiguration->m_muViCwMax);
    muEdcaParameters.SetMuEdcaTimer(AC_VI, heConfiguration->m_viMuEdcaTimer);

    muEdcaParameters.SetMuAifsn(AC_VO, heConfiguration->m_muVoAifsn);
    muEdcaParameters.SetMuCwMin(AC_VO, heConfiguration->m_muVoCwMin);
    muEdcaParameters.SetMuCwMax(AC_VO, heConfiguration->m_muVoCwMax);
    muEdcaParameters.SetMuEdcaTimer(AC_VO, heConfiguration->m_voMuEdcaTimer);

    // The timers of the MU EDCA Parameter Set must be either all zero or all
    // non-zero. The information element is advertised if all timers are non-zero
    auto timerNotNull = [&muEdcaParameters](uint8_t aci) {
        return !muEdcaParameters.GetMuEdcaTimer(aci).IsZero();
    };
    auto aci = {0, 1, 2, 3};
    if (std::all_of(aci.begin(), aci.end(), timerNotNull))
    {
        return muEdcaParameters;
    }

    NS_ABORT_MSG_UNLESS(std::none_of(aci.begin(), aci.end(), timerNotNull),
                        "MU EDCA Timers must be all zero if the IE is not advertised.");

    return std::nullopt;
}

std::optional<ReducedNeighborReport>
ApWifiMac::GetReducedNeighborReport(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);

    if (GetNLinks() <= 1)
    {
        return std::nullopt;
    }

    NS_ABORT_IF(!GetEhtSupported());
    ReducedNeighborReport rnr;

    for (uint8_t index = 0; index < GetNLinks(); ++index)
    {
        if (index != linkId) // all links but the one used to send this Beacon frame
        {
            rnr.AddNbrApInfoField();
            std::size_t nbrId = rnr.GetNNbrApInfoFields() - 1;
            rnr.SetOperatingChannel(nbrId, GetLink(index).phy->GetOperatingChannel());
            rnr.AddTbttInformationField(nbrId);
            rnr.SetBssid(nbrId, 0, GetLink(index).feManager->GetAddress());
            rnr.SetShortSsid(nbrId, 0, 0);
            rnr.SetBssParameters(nbrId, 0, 0);
            rnr.SetPsd20MHz(nbrId, 0, 0);
            rnr.SetMldParameters(nbrId, 0, {0, index, 0, 0, 0});
        }
    }
    return rnr;
}

MultiLinkElement
ApWifiMac::GetMultiLinkElement(uint8_t linkId,
                               WifiMacType frameType,
                               const Mac48Address& to,
                               const std::optional<MultiLinkElement>& mlProbeReqMle)
{
    NS_LOG_FUNCTION(this << +linkId << frameType << to);
    NS_ABORT_IF(GetNLinks() == 1);
    NS_ABORT_MSG_IF(mlProbeReqMle.has_value() && frameType != WIFI_MAC_MGT_PROBE_RESPONSE,
                    "ML Probe Request Multi-Link Element cannot be provided for frame type "
                        << frameType);

    MultiLinkElement mle(MultiLinkElement::BASIC_VARIANT);
    mle.SetMldMacAddress(GetAddress());
    mle.SetLinkIdInfo(linkId);
    mle.SetBssParamsChangeCount(0);

    auto ehtConfiguration = GetEhtConfiguration();
    NS_ASSERT(ehtConfiguration);

    if (ehtConfiguration->m_emlsrActivated)
    {
        mle.SetEmlsrSupported(true);
        // When the EMLSR Padding Delay subfield is included in a frame sent by an AP affiliated
        // with an AP MLD, the EMLSR Padding Delay subfield is reserved.
        // When the EMLSR Transition Delay subfield is included in a frame sent by an AP affiliated
        // with an AP MLD, the EMLSR Transition Delay subfield is reserved. (Sec. 9.4.2.312.2.3
        // of 802.11be D2.3)
        TimeValue time;
        mle.SetTransitionTimeout(ehtConfiguration->m_transitionTimeout);

        // An AP affiliated with an AP MLD may include the Medium Synchronization Delay Information
        // subfield in the Common Info field of the Basic Multi-Link element carried in transmitted
        // (Re)Association Response or Multi-Link Probe Response frames to provide medium
        // synchronization information used by the AP MLD. (Section 35.3.16.8.2 of 802.11be D3.1)
        if (frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE)
        {
            auto& commonInfo = mle.GetCommonInfoBasic();
            commonInfo.SetMediumSyncDelayTimer(ehtConfiguration->m_mediumSyncDuration);
            commonInfo.SetMediumSyncOfdmEdThreshold(ehtConfiguration->m_msdOfdmEdThreshold);
            commonInfo.SetMediumSyncMaxNTxops(ehtConfiguration->m_msdMaxNTxops);
        }
    }

    // The MLD Capabilities And Operations subfield is present in the Common Info field of the
    // Basic Multi-Link element carried in Beacon, Probe Response, (Re)Association Request, and
    // (Re)Association Response frames. (Sec. 9.4.2.312.2.3 of 802.11be D3.1)
    if (frameType == WIFI_MAC_MGT_BEACON || frameType == WIFI_MAC_MGT_PROBE_RESPONSE ||
        frameType == WIFI_MAC_MGT_ASSOCIATION_REQUEST ||
        frameType == WIFI_MAC_MGT_REASSOCIATION_REQUEST ||
        frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE)
    {
        auto& mldCapabilities = mle.GetCommonInfoBasic().m_mldCapabilities;
        mldCapabilities.emplace();
        mldCapabilities->maxNSimultaneousLinks = GetNLinks() - 1; // assuming STR for now
        mldCapabilities->srsSupport = 0;
        mldCapabilities->tidToLinkMappingSupport =
            static_cast<uint8_t>(ehtConfiguration->m_tidLinkMappingSupport);
        mldCapabilities->freqSepForStrApMld = 0; // not supported yet
        mldCapabilities->aarSupport = 0;         // not supported yet
    }

    // if the Multi-Link Element is being inserted in a (Re)Association Response frame
    // and the remote station is affiliated with an MLD, try multi-link setup
    if (auto staMldAddress = GetWifiRemoteStationManager(linkId)->GetMldAddress(to);
        (frameType == WIFI_MAC_MGT_ASSOCIATION_RESPONSE ||
         frameType == WIFI_MAC_MGT_REASSOCIATION_RESPONSE) &&
        staMldAddress.has_value())
    {
        for (uint8_t i = 0; i < GetNLinks(); i++)
        {
            auto remoteStationManager = GetWifiRemoteStationManager(i);
            if (auto staAddress = remoteStationManager->GetAffiliatedStaAddress(*staMldAddress);
                i != linkId && staAddress.has_value() &&
                (remoteStationManager->IsWaitAssocTxOk(*staAddress) ||
                 remoteStationManager->IsAssocRefused(*staAddress)))
            {
                // For each requested link in addition to the link on which the
                // (Re)Association Response frame is transmitted, the Link Info field
                // of the Basic Multi-Link element carried in the (Re)Association
                // Response frame shall contain the corresponding Per-STA Profile
                // subelement(s) (Sec. 35.3.5.4 of 802.11be D2.0)
                mle.AddPerStaProfileSubelement();
                auto& perStaProfile = mle.GetPerStaProfile(mle.GetNPerStaProfileSubelements() - 1);
                // The Link ID subfield of the STA Control field of the Per-STA Profile
                // subelement for the AP corresponding to a link is set to the link ID
                // of the AP affiliated with the AP MLD that is operating on that link.
                perStaProfile.SetLinkId(i);
                perStaProfile.SetCompleteProfile();
                // For each Per-STA Profile subelement included in the Link Info field,
                // the Complete Profile subfield of the STA Control field shall be set to 1
                perStaProfile.SetStaMacAddress(GetFrameExchangeManager(i)->GetAddress());
                perStaProfile.SetAssocResponse(GetAssocResp(*staAddress, i));
            }
        }
    }

    if (!mlProbeReqMle.has_value())
    {
        return mle; // not a multi-link probe request
    }

    auto reqVar = mlProbeReqMle->GetVariant();
    NS_ASSERT_MSG(reqVar == MultiLinkElement::PROBE_REQUEST_VARIANT,
                  "Invalid MLE variant " << reqVar);

    // IEEE 802.11be D6.0 35.3.4.2 Use of multi-link probe request and response
    // If either the Address 1 field or the Address 3 field of the multi-link probe request is set
    // to the MAC address of the responding AP that operates on the same link where the multi-link
    // probe request is sent, then the AP MLD ID subfield shall be present in the Probe Request
    // Multi-Link element of the multi-link probe request value and targeted AP MLD is identified by
    // AP MLD ID subfield, which is set to the same AP MLD ID as the one used by the AP that is
    // addressed by the multi-link probe request to identify the AP MLD
    //  in the Beacon and Probe Response frames that it transmits.
    auto apMldId = mlProbeReqMle->GetApMldId();
    NS_ASSERT_MSG(apMldId.has_value(), "AP MLD ID subfield missing");

    // IEEE 802.11be D6.0 9.4.2.169.2 Neighbor AP Information field
    // If the reported AP is affiliated with the same MLD as the reporting AP sending the frame
    // carrying this element, the AP MLD ID subfield is set to 0. AP MLD ID value advertised in
    // Beacons and Probe Responses is 0. Multi-BSSID feature not supported.
    NS_ASSERT_MSG(*apMldId == 0, "AP MLD ID expected value is 0. value = " << +apMldId.value());

    // Using set to handle case of multiple Per-STA Profiles including same link ID
    std::set<uint8_t> respLinkIds{}; // Set of Link IDs to include in Probe Response
    if (const auto nProfiles = mlProbeReqMle->GetNPerStaProfileSubelements(); nProfiles == 0)
    {
        // IEEE 802.11be D6.0 35.3.4.2 Use of multi-link probe request and response
        // If the Probe Request Multi-link element in the multi-link probe request does not include
        // any per-STA profile, then all APs affiliated with the same AP MLD as the AP identified in
        // the Address 1 or Address 3 field or AP MLD ID shall be requested APs.
        for (std::size_t i = 0; i < GetNLinks(); ++i)
        {
            if (i != linkId)
            {
                respLinkIds.insert(i);
            }
        }
    }

    for (std::size_t i = 0; i < mlProbeReqMle->GetNPerStaProfileSubelements(); ++i)
    {
        // IEEE 802.11be D6.0 35.3.4.2 Use of multi-link probe request and response
        // If the Probe Request Multi-Link element in the multi-link probe request includes one or
        // more per-STA profiles, then only APs affiliated with the same AP MLD whose link ID is
        // equal to the value in the Link ID Field in a per-STA profile in the Probe Request
        // Multi-link element shall be requested APs.
        const auto& perStaProfile = mlProbeReqMle->GetPerStaProfile(i);
        auto currLinkId = perStaProfile.GetLinkId();
        if ((currLinkId < GetNLinks()) && (currLinkId != linkId))
        {
            respLinkIds.insert(currLinkId); // Only consider valid link IDs
        }
    }

    auto setPerStaProfile = [&](uint8_t id) -> void {
        mle.AddPerStaProfileSubelement();
        auto& perStaProfile = mle.GetPerStaProfile(mle.GetNPerStaProfileSubelements() - 1);
        perStaProfile.SetLinkId(id);
        // Current support limited to Complete Profile request per link ID
        // TODO: Add support for Partial Per-STA Profile request
        perStaProfile.SetProbeResponse(GetProbeRespProfile(id));
        perStaProfile.SetCompleteProfile();
    };

    std::for_each(respLinkIds.begin(), respLinkIds.end(), setPerStaProfile);
    return mle;
}

HtOperation
ApWifiMac::GetHtOperation(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetHtSupported(linkId));
    HtOperation operation;
    auto phy = GetWifiPhy(linkId);
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);

    operation.SetPrimaryChannel(phy->GetPrimaryChannelNumber(MHz_u{20}));
    operation.SetRifsMode(false);
    operation.SetNonGfHtStasPresent(true);
    if (phy->GetChannelWidth() > MHz_u{20})
    {
        operation.SetSecondaryChannelOffset(1);
        operation.SetStaChannelWidth(1);
    }
    if (GetLink(linkId).numNonHtStations == 0)
    {
        operation.SetHtProtection(NO_PROTECTION);
    }
    else
    {
        operation.SetHtProtection(MIXED_MODE_PROTECTION);
    }
    uint64_t maxSupportedRate = 0; // in bit/s
    for (const auto& mcs : phy->GetMcsList(WIFI_MOD_CLASS_HT))
    {
        uint8_t nss = (mcs.GetMcsValue() / 8) + 1;
        NS_ASSERT(nss > 0 && nss < 5);
        uint64_t dataRate =
            mcs.GetDataRate(phy->GetChannelWidth(),
                            NanoSeconds(GetHtConfiguration()->m_sgiSupported ? 400 : 800),
                            nss);
        if (dataRate > maxSupportedRate)
        {
            maxSupportedRate = dataRate;
            NS_LOG_DEBUG("Updating maxSupportedRate to " << maxSupportedRate);
        }
    }
    uint8_t maxSpatialStream = phy->GetMaxSupportedTxSpatialStreams();
    auto mcsList = phy->GetMcsList(WIFI_MOD_CLASS_HT);
    uint8_t nMcs = mcsList.size();
    for (const auto& sta : GetLink(linkId).staList)
    {
        if (remoteStationManager->GetHtSupported(sta.second) ||
            remoteStationManager->GetStationHe6GhzCapabilities(sta.second))
        {
            uint64_t maxSupportedRateByHtSta = 0; // in bit/s
            auto itMcs = mcsList.begin();
            for (uint8_t j = 0;
                 j < (std::min(nMcs, remoteStationManager->GetNMcsSupported(sta.second)));
                 j++)
            {
                WifiMode mcs = *itMcs++;
                uint8_t nss = (mcs.GetMcsValue() / 8) + 1;
                NS_ASSERT(nss > 0 && nss < 5);
                uint64_t dataRate = mcs.GetDataRate(
                    remoteStationManager->GetChannelWidthSupported(sta.second),
                    NanoSeconds(remoteStationManager->GetShortGuardIntervalSupported(sta.second)
                                    ? 400
                                    : 800),
                    nss);
                if (dataRate > maxSupportedRateByHtSta)
                {
                    maxSupportedRateByHtSta = dataRate;
                }
            }
            if (maxSupportedRateByHtSta < maxSupportedRate)
            {
                maxSupportedRate = maxSupportedRateByHtSta;
            }
            if (remoteStationManager->GetNMcsSupported(sta.second) < nMcs)
            {
                nMcs = remoteStationManager->GetNMcsSupported(sta.second);
            }
            if (remoteStationManager->GetNumberOfSupportedStreams(sta.second) < maxSpatialStream)
            {
                maxSpatialStream = remoteStationManager->GetNumberOfSupportedStreams(sta.second);
            }
        }
    }
    operation.SetRxHighestSupportedDataRate(
        static_cast<uint16_t>(maxSupportedRate / 1e6)); // in Mbit/s
    operation.SetTxMcsSetDefined(nMcs > 0);
    operation.SetTxMaxNSpatialStreams(maxSpatialStream);
    // To be filled in once supported
    operation.SetObssNonHtStasPresent(0);
    operation.SetDualBeacon(0);
    operation.SetDualCtsProtection(0);
    operation.SetStbcBeacon(0);
    operation.SetLSigTxopProtectionFullSupport(0);
    operation.SetPcoActive(0);
    operation.SetPhase(0);
    operation.SetRxMcsBitmask(0);
    operation.SetTxRxMcsSetUnequal(0);
    operation.SetTxUnequalModulation(0);

    return operation;
}

VhtOperation
ApWifiMac::GetVhtOperation(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetVhtSupported(linkId));
    VhtOperation operation;
    auto phy = GetWifiPhy(linkId);
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);

    const auto bssBandwidth = phy->GetChannelWidth();
    // Set to 0 for 20 MHz or 40 MHz BSS bandwidth.
    // Set to 1 for 80 MHz, 160 MHz or 80+80 MHz BSS bandwidth.
    operation.SetChannelWidth((bssBandwidth > MHz_u{40}) ? 1 : 0);
    // For 20, 40, or 80 MHz BSS bandwidth, indicates the channel center frequency
    // index for the 20, 40, or 80 MHz channel on which the VHT BSS operates.
    // For 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
    // indicates the channel center frequency index of the 80 MHz channel
    // segment that contains the primary channel.
    // For 80+80 MHz BSS bandwidth and the Channel Width subfield equal to 1 or 3,
    // indicates the channel center frequency index for the primary 80 MHz channel of the VHT BSS.
    operation.SetChannelCenterFrequencySegment0((bssBandwidth == MHz_u{160})
                                                    ? phy->GetPrimaryChannelNumber(MHz_u{80})
                                                    : phy->GetChannelNumber());
    // For a 20, 40, or 80 MHz BSS bandwidth, this subfield is set to 0.
    // For a 160 MHz BSS bandwidth and the Channel Width subfield equal to 1,
    // indicates the channel center frequency index of the 160 MHz channel on
    // which the VHT BSS operates.
    // For an 80+80 MHz BSS bandwidth and the Channel Width subfield equal to 1 or 3,
    // indicates the channel center frequency index of the secondary 80 MHz channel of the VHT BSS.
    const auto& operatingChannel = phy->GetOperatingChannel();
    const auto is80Plus80 =
        operatingChannel.GetWidthType() == WifiChannelWidthType::CW_80_PLUS_80MHZ;
    operation.SetChannelCenterFrequencySegment1((bssBandwidth == MHz_u{160})
                                                    ? is80Plus80 ? operatingChannel.GetNumber(1)
                                                                 : phy->GetChannelNumber()
                                                    : 0);
    uint8_t maxSpatialStream = phy->GetMaxSupportedRxSpatialStreams();
    for (const auto& sta : GetLink(linkId).staList)
    {
        if (remoteStationManager->GetVhtSupported(sta.second))
        {
            if (remoteStationManager->GetNumberOfSupportedStreams(sta.second) < maxSpatialStream)
            {
                maxSpatialStream = remoteStationManager->GetNumberOfSupportedStreams(sta.second);
            }
        }
    }
    for (uint8_t nss = 1; nss <= maxSpatialStream; nss++)
    {
        uint8_t maxMcs =
            9; // TBD: hardcode to 9 for now since we assume all MCS values are supported
        operation.SetMaxVhtMcsPerNss(nss, maxMcs);
    }

    return operation;
}

HeOperation
ApWifiMac::GetHeOperation(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetHeSupported());
    HeOperation operation;
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);

    uint8_t maxSpatialStream = GetWifiPhy(linkId)->GetMaxSupportedRxSpatialStreams();
    for (const auto& sta : GetLink(linkId).staList)
    {
        if (remoteStationManager->GetHeSupported(sta.second))
        {
            if (remoteStationManager->GetNumberOfSupportedStreams(sta.second) < maxSpatialStream)
            {
                maxSpatialStream = remoteStationManager->GetNumberOfSupportedStreams(sta.second);
            }
        }
    }
    for (uint8_t nss = 1; nss <= maxSpatialStream; nss++)
    {
        operation.SetMaxHeMcsPerNss(
            nss,
            11); // TBD: hardcode to 11 for now since we assume all MCS values are supported
    }
    operation.m_bssColorInfo.m_bssColor = GetHeConfiguration()->m_bssColor;

    if (auto phy = GetWifiPhy(linkId); phy && phy->GetPhyBand() == WIFI_PHY_BAND_6GHZ)
    {
        HeOperation::OpInfo6GHz op6Ghz;
        const auto bw = phy->GetChannelWidth();
        const auto ch = phy->GetOperatingChannel();
        op6Ghz.m_chWid = (bw == MHz_u{20}) ? 0 : (bw == MHz_u{40}) ? 1 : (bw == MHz_u{80}) ? 2 : 3;
        op6Ghz.m_primCh = ch.GetPrimaryChannelNumber(MHz_u{20}, WIFI_STANDARD_80211ax);
        op6Ghz.m_chCntrFreqSeg0 = (bw == MHz_u{160})
                                      ? ch.GetPrimaryChannelNumber(MHz_u{80}, WIFI_STANDARD_80211ax)
                                      : ch.GetNumber();
        // TODO: for 80+80 MHz channels, set this field to the secondary 80 MHz segment number
        op6Ghz.m_chCntrFreqSeg1 = (bw == MHz_u{160}) ? ch.GetNumber() : 0;

        operation.m_6GHzOpInfo = op6Ghz;
    }

    return operation;
}

EhtOperation
ApWifiMac::GetEhtOperation(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << +linkId);
    NS_ASSERT(GetEhtSupported());
    EhtOperation operation;
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);

    auto maxSpatialStream = GetWifiPhy(linkId)->GetMaxSupportedRxSpatialStreams();
    for (const auto& sta : GetLink(linkId).staList)
    {
        if (remoteStationManager->GetEhtSupported(sta.second))
        {
            if (remoteStationManager->GetNumberOfSupportedStreams(sta.second) < maxSpatialStream)
            {
                maxSpatialStream = remoteStationManager->GetNumberOfSupportedStreams(sta.second);
            }
        }
    }
    operation.SetMaxRxNss(maxSpatialStream, 0, WIFI_EHT_MAX_MCS_INDEX);
    operation.SetMaxTxNss(maxSpatialStream, 0, WIFI_EHT_MAX_MCS_INDEX);
    return operation;
}

void
ApWifiMac::EnqueueProbeResp(const MgtProbeResponseHeader& probeResp,
                            Mac48Address to,
                            uint8_t linkId)
{
    NS_LOG_FUNCTION(this << to << +linkId);
    WifiMacHeader hdr(WIFI_MAC_MGT_PROBE_RESPONSE);
    hdr.SetAddr1(to);
    hdr.SetAddr2(GetLink(linkId).feManager->GetAddress());
    hdr.SetAddr3(GetLink(linkId).feManager->GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(probeResp);

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
    else if (!GetWifiRemoteStationManager(linkId)->GetQosSupported(to))
    {
        GetBEQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
    else
    {
        GetVOQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
}

MgtProbeResponseHeader
ApWifiMac::GetProbeRespProfile(uint8_t linkId) const
{
    MgtProbeResponseHeader probe;
    probe.Get<Ssid>() = GetSsid();
    auto supportedRates = GetSupportedRates(linkId);
    probe.Get<SupportedRates>() = supportedRates.rates;
    probe.Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
    probe.SetBeaconIntervalUs(GetBeaconInterval().GetMicroSeconds());
    probe.Capabilities() = GetCapabilities(linkId);
    GetWifiRemoteStationManager(linkId)->SetShortPreambleEnabled(
        GetLink(linkId).shortPreambleEnabled);
    GetWifiRemoteStationManager(linkId)->SetShortSlotTimeEnabled(
        GetLink(linkId).shortSlotTimeEnabled);
    if (GetDsssSupported(linkId))
    {
        probe.Get<DsssParameterSet>() = GetDsssParameterSet(linkId);
    }
    if (GetErpSupported(linkId))
    {
        probe.Get<ErpInformation>() = GetErpInformation(linkId);
    }
    if (GetQosSupported())
    {
        probe.Get<EdcaParameterSet>() = GetEdcaParameterSet(linkId);
    }
    if (GetHtSupported(linkId))
    {
        probe.Get<ExtendedCapabilities>() = GetExtendedCapabilities();
        probe.Get<HtCapabilities>() = GetHtCapabilities(linkId);
        probe.Get<HtOperation>() = GetHtOperation(linkId);
    }
    if (GetVhtSupported(linkId))
    {
        probe.Get<VhtCapabilities>() = GetVhtCapabilities(linkId);
        probe.Get<VhtOperation>() = GetVhtOperation(linkId);
    }
    if (GetHeSupported())
    {
        probe.Get<HeCapabilities>() = GetHeCapabilities(linkId);
        probe.Get<HeOperation>() = GetHeOperation(linkId);
        if (auto muEdcaParameterSet = GetMuEdcaParameterSet())
        {
            probe.Get<MuEdcaParameterSet>() = std::move(*muEdcaParameterSet);
        }
        if (Is6GhzBand(linkId))
        {
            probe.Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(linkId);
        }
    }
    if (GetEhtSupported())
    {
        probe.Get<EhtCapabilities>() = GetEhtCapabilities(linkId);
        probe.Get<EhtOperation>() = GetEhtOperation(linkId);
    }

    return probe;
}

MgtProbeResponseHeader
ApWifiMac::GetProbeResp(uint8_t linkId, const std::optional<MultiLinkElement>& reqMle)
{
    NS_LOG_FUNCTION(this << linkId << reqMle.has_value());
    NS_ASSERT_MSG(linkId < GetNLinks(), "Invalid link ID = " << +linkId);

    auto probeResp = GetProbeRespProfile(linkId);

    if (GetNLinks() > 1)
    {
        /*
         * If an AP is affiliated with an AP MLD and does not correspond to a nontransmitted
         * BSSID, then the Beacon and Probe Response frames transmitted by the AP shall
         * include a TBTT Information field in a Reduced Neighbor Report element with the
         * TBTT Information Length field set to 16 or higher, for each of the other APs
         * (if any) affiliated with the same AP MLD. (Sec. 35.3.4.1 of 802.11be D2.1.1)
         */
        if (auto rnr = GetReducedNeighborReport(linkId); rnr.has_value())
        {
            probeResp.Get<ReducedNeighborReport>() = std::move(*rnr);
        }
        /*
         * If an AP affiliated with an AP MLD is not in a multiple BSSID set [..], the AP
         * shall include, in a Beacon frame or a Probe Response frame, which is not a
         * Multi-Link probe response, only the Common Info field of the Basic Multi-Link
         * element for the AP MLD unless conditions in 35.3.11 (Multi-link procedures for
         * channel switching, extended channel switching, and channel quieting) are
         * satisfied. (Sec. 35.3.4.4 of 802.11be D2.1.1)
         */
        probeResp.Get<MultiLinkElement>() = GetMultiLinkElement(linkId,
                                                                WIFI_MAC_MGT_PROBE_RESPONSE,
                                                                Mac48Address::GetBroadcast(),
                                                                reqMle);
    }
    return probeResp;
}

MgtAssocResponseHeader
ApWifiMac::GetAssocResp(Mac48Address to, uint8_t linkId)
{
    MgtAssocResponseHeader assoc;
    StatusCode code;
    auto remoteStationManager = GetWifiRemoteStationManager(linkId);
    if (remoteStationManager->IsWaitAssocTxOk(to))
    {
        code.SetSuccess();
    }
    else
    {
        NS_ABORT_IF(!remoteStationManager->IsAssocRefused(to));
        // reset state
        remoteStationManager->RecordDisassociated(to);
        code.SetFailure();
    }
    auto supportedRates = GetSupportedRates(linkId);
    assoc.Get<SupportedRates>() = supportedRates.rates;
    assoc.Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
    assoc.SetStatusCode(code);
    assoc.Capabilities() = GetCapabilities(linkId);
    if (GetQosSupported())
    {
        assoc.Get<EdcaParameterSet>() = GetEdcaParameterSet(linkId);
    }
    if (GetHtSupported(linkId))
    {
        assoc.Get<ExtendedCapabilities>() = GetExtendedCapabilities();
        assoc.Get<HtCapabilities>() = GetHtCapabilities(linkId);
        assoc.Get<HtOperation>() = GetHtOperation(linkId);
    }
    if (GetVhtSupported(linkId))
    {
        assoc.Get<VhtCapabilities>() = GetVhtCapabilities(linkId);
        assoc.Get<VhtOperation>() = GetVhtOperation(linkId);
    }
    if (GetHeSupported())
    {
        assoc.Get<HeCapabilities>() = GetHeCapabilities(linkId);
        assoc.Get<HeOperation>() = GetHeOperation(linkId);
        if (auto muEdcaParameterSet = GetMuEdcaParameterSet(); muEdcaParameterSet.has_value())
        {
            assoc.Get<MuEdcaParameterSet>() = std::move(*muEdcaParameterSet);
        }
        if (Is6GhzBand(linkId))
        {
            assoc.Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(linkId);
        }
    }
    if (GetEhtSupported())
    {
        assoc.Get<EhtCapabilities>() = GetEhtCapabilities(linkId);
        assoc.Get<EhtOperation>() = GetEhtOperation(linkId);
        // The AP MLD that accepts the requested TID-to-link mapping shall not include in the
        // (Re)Association Response frame the TID-to-link Mapping element.
        // (Sec. 35.3.7.1.8 of 802.11be D3.1).
        // For now, we assume that AP MLDs always accept requested TID-to-link mappings.
    }
    return assoc;
}

ApWifiMac::LinkIdStaAddrMap
ApWifiMac::GetLinkIdStaAddrMap(MgtAssocResponseHeader& assoc,
                               const Mac48Address& to,
                               uint8_t linkId)
{
    // find all the links to setup (i.e., those for which status code is success)
    std::map<uint8_t /* link ID */, Mac48Address> linkIdStaAddrMap;

    if (assoc.GetStatusCode().IsSuccess())
    {
        linkIdStaAddrMap[linkId] = to;
    }

    if (const auto& mle = assoc.Get<MultiLinkElement>())
    {
        const auto staMldAddress = GetWifiRemoteStationManager(linkId)->GetMldAddress(to);
        NS_ABORT_MSG_IF(!staMldAddress.has_value(),
                        "Sending a Multi-Link Element to a single link device");
        for (std::size_t idx = 0; idx < mle->GetNPerStaProfileSubelements(); idx++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(idx);
            if (perStaProfile.HasAssocResponse() &&
                perStaProfile.GetAssocResponse().GetStatusCode().IsSuccess())
            {
                uint8_t otherLinkId = perStaProfile.GetLinkId();
                auto staAddress = GetWifiRemoteStationManager(otherLinkId)
                                      ->GetAffiliatedStaAddress(*staMldAddress);
                NS_ABORT_MSG_IF(!staAddress.has_value(),
                                "No STA to associate with on link " << +otherLinkId);
                const auto [it, inserted] = linkIdStaAddrMap.insert({otherLinkId, *staAddress});
                NS_ABORT_MSG_IF(!inserted,
                                "More than one Association Response to MLD "
                                    << *staMldAddress << " on link ID " << +otherLinkId);
            }
        }
    }

    return linkIdStaAddrMap;
}

void
ApWifiMac::SetAid(MgtAssocResponseHeader& assoc, const LinkIdStaAddrMap& linkIdStaAddrMap)
{
    if (linkIdStaAddrMap.empty())
    {
        // no link to setup, nothing to do
        return;
    }

    const auto& [linkId, staAddr] = *linkIdStaAddrMap.cbegin();
    const auto addr = GetWifiRemoteStationManager(linkId)->GetMldAddress(staAddr).value_or(staAddr);

    // check if an AID is already allocated to the device that is associating
    std::set<uint16_t> aids;

    for (const auto& [id, link] : GetLinks())
    {
        if (const auto aid = link->stationManager->GetAssociationId(addr); aid != SU_STA_ID)
        {
            aids.insert(aid);
        }
    }

    NS_ABORT_MSG_IF(aids.size() > 1, addr << " cannot have more than one AID assigned");

    const auto aid = aids.empty() ? GetNextAssociationId() : *aids.cbegin();

    // store the MLD or link address in the AID-to-address map
    const auto [it, inserted] = m_aidToMldOrLinkAddress.emplace(aid, addr);

    NS_ABORT_MSG_IF(!inserted, "AID " << aid << " already present, cannot be assigned to " << addr);

    for (const auto& [id, staAddr] : linkIdStaAddrMap)
    {
        auto& link = GetLink(id);

        if (const auto [it, inserted] = link.staList.emplace(aid, staAddr); inserted)
        {
            // the STA on this link had no AID assigned
            link.stationManager->SetAssociationId(staAddr, aid);

            if (link.stationManager->GetDsssSupported(staAddr) &&
                !link.stationManager->GetErpOfdmSupported(staAddr))
            {
                link.numNonErpStations++;
            }
            if (!link.stationManager->GetHtSupported(staAddr) &&
                !link.stationManager->GetStationHe6GhzCapabilities(staAddr))
            {
                link.numNonHtStations++;
            }
            UpdateShortSlotTimeEnabled(id);
            UpdateShortPreambleEnabled(id);
        }
        else
        {
            // the STA on this link had an AID assigned
            NS_ABORT_MSG_IF(it->first != aid,
                            "AID " << it->first << " already assigned to " << staAddr
                                   << ", could not assign " << aid);
        }
    }

    // set the AID in all the Association Responses. NOTE that the Association
    // Responses included in the Per-STA Profile Subelements of the Multi-Link
    // Element must not contain the AID field. We set the AID field in such
    // Association Responses anyway, in order to ease future implementation of
    // the inheritance mechanism.
    if (assoc.GetStatusCode().IsSuccess())
    {
        assoc.SetAssociationId(aid);
    }
    if (const auto& mle = assoc.Get<MultiLinkElement>())
    {
        for (std::size_t idx = 0; idx < mle->GetNPerStaProfileSubelements(); idx++)
        {
            if (const auto& perStaProfile = mle->GetPerStaProfile(idx);
                perStaProfile.HasAssocResponse() &&
                perStaProfile.GetAssocResponse().GetStatusCode().IsSuccess())
            {
                perStaProfile.GetAssocResponse().SetAssociationId(aid);
            }
        }
    }
}

void
ApWifiMac::SendAssocResp(Mac48Address to, bool isReassoc, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << to << isReassoc << +linkId);
    WifiMacHeader hdr;
    hdr.SetType(isReassoc ? WIFI_MAC_MGT_REASSOCIATION_RESPONSE
                          : WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
    hdr.SetAddr1(to);
    hdr.SetAddr2(GetFrameExchangeManager(linkId)->GetAddress());
    hdr.SetAddr3(GetFrameExchangeManager(linkId)->GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    MgtAssocResponseHeader assoc = GetAssocResp(to, linkId);

    // The AP that is affiliated with the AP MLD and that responds to an (Re)Association
    // Request frame that carries a Basic Multi-Link element shall include a Basic
    // Multi-Link element in the (Re)Association Response frame that it transmits
    // (Sec. 35.3.5.4 of 802.11be D2.0)
    // If the STA included a Multi-Link Element in the (Re)Association Request, we
    // stored its MLD address in the remote station manager
    if (GetNLinks() > 1 && GetWifiRemoteStationManager(linkId)->GetMldAddress(to).has_value())
    {
        assoc.Get<MultiLinkElement>() = GetMultiLinkElement(linkId, hdr.GetType(), to);
    }

    auto linkIdStaAddrMap = GetLinkIdStaAddrMap(assoc, to, linkId);
    SetAid(assoc, linkIdStaAddrMap);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(assoc);

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
    else if (!GetWifiRemoteStationManager(linkId)->GetQosSupported(to))
    {
        GetBEQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
    else
    {
        GetVOQueue()->Queue(Create<WifiMpdu>(packet, hdr));
    }
}

void
ApWifiMac::SendOneBeacon(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    auto& link = GetLink(linkId);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_BEACON);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(link.feManager->GetAddress());
    hdr.SetAddr3(link.feManager->GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    Ptr<Packet> packet = Create<Packet>();
    MgtBeaconHeader beacon;
    beacon.Get<Ssid>() = GetSsid();
    auto supportedRates = GetSupportedRates(linkId);
    beacon.Get<SupportedRates>() = supportedRates.rates;
    beacon.Get<ExtendedSupportedRatesIE>() = supportedRates.extendedRates;
    beacon.SetBeaconIntervalUs(GetBeaconInterval().GetMicroSeconds());
    beacon.Capabilities() = GetCapabilities(linkId);
    GetWifiRemoteStationManager(linkId)->SetShortPreambleEnabled(link.shortPreambleEnabled);
    GetWifiRemoteStationManager(linkId)->SetShortSlotTimeEnabled(link.shortSlotTimeEnabled);
    if (GetDsssSupported(linkId))
    {
        beacon.Get<DsssParameterSet>() = GetDsssParameterSet(linkId);
    }
    if (GetErpSupported(linkId))
    {
        beacon.Get<ErpInformation>() = GetErpInformation(linkId);
    }
    if (GetQosSupported())
    {
        beacon.Get<EdcaParameterSet>() = GetEdcaParameterSet(linkId);
    }
    if (GetHtSupported(linkId))
    {
        beacon.Get<ExtendedCapabilities>() = GetExtendedCapabilities();
        beacon.Get<HtCapabilities>() = GetHtCapabilities(linkId);
        beacon.Get<HtOperation>() = GetHtOperation(linkId);
    }
    if (GetVhtSupported(linkId))
    {
        beacon.Get<VhtCapabilities>() = GetVhtCapabilities(linkId);
        beacon.Get<VhtOperation>() = GetVhtOperation(linkId);
    }
    if (GetHeSupported())
    {
        beacon.Get<HeCapabilities>() = GetHeCapabilities(linkId);
        beacon.Get<HeOperation>() = GetHeOperation(linkId);
        if (auto muEdcaParameterSet = GetMuEdcaParameterSet(); muEdcaParameterSet.has_value())
        {
            beacon.Get<MuEdcaParameterSet>() = std::move(*muEdcaParameterSet);
        }
        if (Is6GhzBand(linkId))
        {
            beacon.Get<He6GhzBandCapabilities>() = GetHe6GhzBandCapabilities(linkId);
        }
    }
    if (GetEhtSupported())
    {
        beacon.Get<EhtCapabilities>() = GetEhtCapabilities(linkId);
        beacon.Get<EhtOperation>() = GetEhtOperation(linkId);

        if (GetNLinks() > 1)
        {
            /*
             * If an AP is affiliated with an AP MLD and does not correspond to a nontransmitted
             * BSSID, then the Beacon and Probe Response frames transmitted by the AP shall
             * include a TBTT Information field in a Reduced Neighbor Report element with the
             * TBTT Information Length field set to 16 or higher, for each of the other APs
             * (if any) affiliated with the same AP MLD. (Sec. 35.3.4.1 of 802.11be D2.1.1)
             */
            if (auto rnr = GetReducedNeighborReport(linkId); rnr.has_value())
            {
                beacon.Get<ReducedNeighborReport>() = std::move(*rnr);
            }
            /*
             * If an AP affiliated with an AP MLD is not in a multiple BSSID set [..], the AP
             * shall include, in a Beacon frame or a Probe Response frame, which is not a
             * Multi-Link probe response, only the Common Info field of the Basic Multi-Link
             * element for the AP MLD unless conditions in 35.3.11 (Multi-link procedures for
             * channel switching, extended channel switching, and channel quieting) are
             * satisfied. (Sec. 35.3.4.4 of 802.11be D2.1.1)
             */
            beacon.Get<MultiLinkElement>() = GetMultiLinkElement(linkId, WIFI_MAC_MGT_BEACON);
        }
    }
    packet->AddHeader(beacon);

    NS_LOG_INFO("Generating beacon from " << link.feManager->GetAddress() << " linkID " << +linkId);
    // The beacon has it's own special queue, so we load it in there
    m_beaconTxop->Queue(Create<WifiMpdu>(packet, hdr));
    link.beaconEvent =
        Simulator::Schedule(GetBeaconInterval(), &ApWifiMac::SendOneBeacon, this, linkId);

    ScheduleFilsDiscOrUnsolProbeRespFrames(linkId);

    // If a STA that does not support Short Slot Time associates,
    // the AP shall use long slot time beginning at the first Beacon
    // subsequent to the association of the long slot time STA.
    if (GetErpSupported(linkId))
    {
        if (link.shortSlotTimeEnabled)
        {
            // Enable short slot time
            GetWifiPhy(linkId)->SetSlot(MicroSeconds(9));
        }
        else
        {
            // Disable short slot time
            GetWifiPhy(linkId)->SetSlot(MicroSeconds(20));
        }
    }
}

Ptr<WifiMpdu>
ApWifiMac::GetFilsDiscovery(uint8_t linkId) const
{
    WifiMacHeader hdr(WIFI_MAC_MGT_ACTION);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    auto& link = GetLink(linkId);
    hdr.SetAddr2(link.feManager->GetAddress());
    hdr.SetAddr3(link.feManager->GetAddress());
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.publicAction = WifiActionHeader::FILS_DISCOVERY;
    actionHdr.SetAction(WifiActionHeader::PUBLIC, action);

    FilsDiscHeader fils;
    fils.SetSsid(GetSsid().PeekString());
    fils.m_beaconInt = (m_beaconInterval / WIFI_TU).GetHigh();

    fils.m_fdCap = FilsDiscHeader::FdCapability{};
    fils.m_fdCap->SetOpChannelWidth(link.phy->GetChannelWidth());
    fils.m_fdCap->SetMaxNss(std::min(link.phy->GetMaxSupportedTxSpatialStreams(),
                                     link.phy->GetMaxSupportedRxSpatialStreams()));
    fils.m_fdCap->SetStandard(link.phy->GetStandard());

    fils.SetLengthSubfield();
    fils.m_rnr = GetReducedNeighborReport(linkId);

    auto packet = Create<Packet>();
    packet->AddHeader(fils);
    packet->AddHeader(actionHdr);

    return Create<WifiMpdu>(packet, hdr);
}

void
ApWifiMac::ScheduleFilsDiscOrUnsolProbeRespFrames(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    auto phy = GetLink(linkId).phy;

    auto fdBeaconInterval = (phy->GetPhyBand() == WIFI_PHY_BAND_6GHZ) ? m_fdBeaconInterval6GHz
                                                                      : m_fdBeaconIntervalNon6GHz;

    if (!fdBeaconInterval.IsStrictlyPositive())
    {
        NS_LOG_DEBUG("Sending FILS Discovery/unsolicited Probe Response disabled");
        return;
    }

    // Schedule FD or unsolicited Probe Response frames (IEEE Std 802.11ax-2021 26.17.2.3.2)
    for (uint8_t count = 1; count < (m_beaconInterval / fdBeaconInterval).GetHigh(); ++count)
    {
        if (m_sendUnsolProbeResp)
        {
            Simulator::Schedule(fdBeaconInterval * count,
                                &ApWifiMac::EnqueueProbeResp,
                                this,
                                GetProbeResp(linkId, std::nullopt),
                                Mac48Address::GetBroadcast(),
                                linkId);
        }
        else
        {
            Simulator::Schedule(fdBeaconInterval * count,
                                [=, this]() { m_beaconTxop->Queue(GetFilsDiscovery(linkId)); });
        }
    }
}

void
ApWifiMac::TxOk(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsAssocResp() || hdr.IsReassocResp())
    {
        MgtAssocResponseHeader assocResp;
        mpdu->GetPacket()->PeekHeader(assocResp);
        auto aid = assocResp.GetAssociationId();

        auto linkId = GetLinkIdByAddress(hdr.GetAddr2());
        NS_ABORT_MSG_IF(!linkId.has_value(), "No link ID matching the TA");

        if (GetWifiRemoteStationManager(*linkId)->IsWaitAssocTxOk(hdr.GetAddr1()))
        {
            NS_LOG_DEBUG("AP=" << hdr.GetAddr2() << " associated with STA=" << hdr.GetAddr1());
            GetWifiRemoteStationManager(*linkId)->RecordGotAssocTxOk(hdr.GetAddr1());
            m_assocLogger(aid, hdr.GetAddr1());
        }

        if (auto staMldAddress =
                GetWifiRemoteStationManager(*linkId)->GetMldAddress(hdr.GetAddr1());
            staMldAddress.has_value())
        {
            /**
             * The STA is affiliated with an MLD. From Sec. 35.3.7.1.4 of 802.11be D3.0:
             * When a link becomes enabled for a non-AP STA that is affiliated with a non-AP MLD
             * after successful association with an AP MLD with (Re)Association Request/Response
             * frames transmitted on another link [...], the power management mode of the non-AP
             * STA, immediately after the acknowledgement of the (Re)Association Response frame
             * [...], is power save mode, and its power state is doze.
             *
             * Thus, STAs operating on all the links but the link used to establish association
             * transition to power save mode.
             */
            for (uint8_t i = 0; i < GetNLinks(); i++)
            {
                auto stationManager = GetWifiRemoteStationManager(i);
                if (auto staAddress = stationManager->GetAffiliatedStaAddress(*staMldAddress);
                    staAddress.has_value() && i != *linkId &&
                    stationManager->IsWaitAssocTxOk(*staAddress))
                {
                    NS_LOG_DEBUG("AP=" << GetFrameExchangeManager(i)->GetAddress()
                                       << " associated with STA=" << *staAddress);
                    stationManager->RecordGotAssocTxOk(*staAddress);
                    m_assocLogger(aid, *staAddress);
                    StaSwitchingToPsMode(*staAddress, i);
                }
            }

            // Apply the negotiated TID-to-Link Mapping (if any) for DL direction
            ApplyTidLinkMapping(*staMldAddress, WifiDirection::DOWNLINK);
        }

        if (auto extendedCapabilities =
                GetWifiRemoteStationManager(*linkId)->GetStationExtendedCapabilities(
                    hdr.GetAddr1());
            m_gcrManager)
        {
            const auto isGcrCapable =
                extendedCapabilities && extendedCapabilities->m_robustAvStreaming;
            m_gcrManager->NotifyStaAssociated(hdr.GetAddr1(), isGcrCapable);
        }
    }
    else if (hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EMLSR client acknowledged the EML Operating Mode Notification frame;
            // we can stop the timer and enforce the configuration deriving from the
            // EML Notification frame sent by the EMLSR client
            if (auto eventIt = m_transitionTimeoutEvents.find(hdr.GetAddr1());
                eventIt != m_transitionTimeoutEvents.cend() && eventIt->second.IsPending())
            {
                // no need to wait until the expiration of the transition timeout
                eventIt->second.PeekEventImpl()->Invoke();
                eventIt->second.Cancel();
            }
        }
    }
}

void
ApWifiMac::TxFailed(WifiMacDropReason timeoutReason, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +timeoutReason << *mpdu);
    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsAssocResp() || hdr.IsReassocResp())
    {
        auto linkId = GetLinkIdByAddress(hdr.GetAddr2());
        NS_ABORT_MSG_IF(!linkId.has_value(), "No link ID matching the TA");

        if (GetWifiRemoteStationManager(*linkId)->IsWaitAssocTxOk(hdr.GetAddr1()))
        {
            NS_LOG_DEBUG("AP=" << hdr.GetAddr2()
                               << " association failed with STA=" << hdr.GetAddr1());
            GetWifiRemoteStationManager(*linkId)->RecordGotAssocTxFailed(hdr.GetAddr1());
        }

        if (auto staMldAddress =
                GetWifiRemoteStationManager(*linkId)->GetMldAddress(hdr.GetAddr1());
            staMldAddress.has_value())
        {
            // the STA is affiliated with an MLD
            for (uint8_t i = 0; i < GetNLinks(); i++)
            {
                auto stationManager = GetWifiRemoteStationManager(i);
                if (auto staAddress = stationManager->GetAffiliatedStaAddress(*staMldAddress);
                    staAddress.has_value() && i != *linkId &&
                    stationManager->IsWaitAssocTxOk(*staAddress))
                {
                    NS_LOG_DEBUG("AP=" << GetFrameExchangeManager(i)->GetAddress()
                                       << " association failed with STA=" << *staAddress);
                    stationManager->RecordGotAssocTxFailed(*staAddress);
                }
            }
        }

        // free the assigned AID
        MgtAssocResponseHeader assocResp;
        mpdu->GetPacket()->PeekHeader(assocResp);
        auto aid = assocResp.GetAssociationId();
        m_aidToMldOrLinkAddress.erase(aid);
        for (const auto& [id, lnk] : GetLinks())
        {
            auto& link = GetLink(id);
            link.staList.erase(aid);
        }
    }
}

void
ApWifiMac::ProcessPowerManagementFlag(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    Mac48Address staAddr = mpdu->GetHeader().GetAddr2();
    bool staInPsMode = GetWifiRemoteStationManager(linkId)->IsInPsMode(staAddr);

    if (!staInPsMode && mpdu->GetHeader().IsPowerManagement())
    {
        // the sending STA is switching to Power Save mode
        StaSwitchingToPsMode(staAddr, linkId);
    }
    else if (staInPsMode && !mpdu->GetHeader().IsPowerManagement())
    {
        // the sending STA is switching back to Active mode
        StaSwitchingToActiveModeOrDeassociated(staAddr, linkId);
    }
}

void
ApWifiMac::StaSwitchingToPsMode(const Mac48Address& staAddr, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << staAddr << linkId);

    GetWifiRemoteStationManager(linkId)->SetPsMode(staAddr, true);

    // Block frames addressed to the STA in PS mode
    NS_LOG_DEBUG("Block destination " << staAddr << " on link " << +linkId);
    auto staMldAddr = GetWifiRemoteStationManager(linkId)->GetMldAddress(staAddr).value_or(staAddr);
    BlockUnicastTxOnLinks(WifiQueueBlockedReason::POWER_SAVE_MODE, staMldAddr, {linkId});
}

void
ApWifiMac::StaSwitchingToActiveModeOrDeassociated(const Mac48Address& staAddr, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << staAddr << linkId);

    GetWifiRemoteStationManager(linkId)->SetPsMode(staAddr, false);

    if (GetWifiRemoteStationManager(linkId)->IsAssociated(staAddr))
    {
        // the station is still associated, unblock its frames
        NS_LOG_DEBUG("Unblock destination " << staAddr << " on link " << +linkId);
        auto staMldAddr =
            GetWifiRemoteStationManager(linkId)->GetMldAddress(staAddr).value_or(staAddr);
        UnblockUnicastTxOnLinks(WifiQueueBlockedReason::POWER_SAVE_MODE, staMldAddr, {linkId});
    }
}

std::optional<uint8_t>
ApWifiMac::IsAssociated(const Mac48Address& address) const
{
    for (uint8_t linkId = 0; linkId < GetNLinks(); linkId++)
    {
        if (GetWifiRemoteStationManager(linkId)->IsAssociated(address))
        {
            return linkId;
        }
    }
    NS_LOG_DEBUG(address << " is not associated");
    return std::nullopt;
}

Mac48Address
ApWifiMac::DoGetLocalAddress(const Mac48Address& remoteAddr) const
{
    auto linkId = IsAssociated(remoteAddr);
    NS_ASSERT_MSG(linkId, remoteAddr << " is not associated");
    return GetFrameExchangeManager(*linkId)->GetAddress();
}

std::optional<Mac48Address>
ApWifiMac::GetMldOrLinkAddressByAid(uint16_t aid) const
{
    if (const auto staIt = m_aidToMldOrLinkAddress.find(aid);
        staIt != m_aidToMldOrLinkAddress.cend())
    {
        return staIt->second;
    }
    return std::nullopt;
}

void
ApWifiMac::Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << +linkId);
    // consider the MAC header of the original MPDU (makes a difference for data frames only)
    const WifiMacHeader* hdr = &mpdu->GetOriginal()->GetHeader();
    Ptr<const Packet> packet = mpdu->GetPacket();
    Mac48Address from = hdr->GetAddr2();
    if (hdr->IsData())
    {
        std::optional<uint8_t> apLinkId;
        if (!hdr->IsFromDs() && hdr->IsToDs() &&
            (apLinkId = IsAssociated(mpdu->GetHeader().GetAddr2())) &&
            mpdu->GetHeader().GetAddr1() == GetFrameExchangeManager(*apLinkId)->GetAddress())
        {
            // this MPDU is being acknowledged by the AP, so we can process
            // the Power Management flag
            ProcessPowerManagementFlag(mpdu, *apLinkId);

            Mac48Address to = hdr->GetAddr3();
            // Address3 can be our MLD address (e.g., this is an MPDU containing a single MSDU
            // addressed to us) or a BSSID (e.g., this is an MPDU containing an A-MSDU)
            if (to == GetAddress() ||
                (hdr->IsQosData() && hdr->IsQosAmsdu() && to == mpdu->GetHeader().GetAddr1()))
            {
                NS_LOG_DEBUG("frame for me from=" << from);
                if (hdr->IsQosData())
                {
                    if (hdr->IsQosAmsdu())
                    {
                        NS_LOG_DEBUG("Received A-MSDU from=" << from
                                                             << ", size=" << packet->GetSize());
                        DeaggregateAmsduAndForward(mpdu);
                        packet = nullptr;
                    }
                    else if (hdr->HasData())
                    {
                        ForwardUp(packet, from, GetAddress());
                    }
                }
                else if (hdr->HasData())
                {
                    ForwardUp(packet, from, GetAddress());
                }
            }
            else if (to.IsGroup() || IsAssociated(to))
            {
                NS_LOG_DEBUG("forwarding frame from=" << from << ", to=" << to);
                Ptr<Packet> copy = packet->Copy();

                // If the frame we are forwarding is of type QoS Data,
                // then we need to preserve the UP in the QoS control
                // header...
                if (hdr->IsQosData())
                {
                    WifiMac::Enqueue(copy, to, from, hdr->GetQosTid());
                }
                else
                {
                    WifiMac::Enqueue(copy, to, from);
                }
                ForwardUp(packet, from, to);
            }
            else if (hdr->HasData())
            {
                ForwardUp(packet, from, to);
            }
        }
        // NOLINTBEGIN(bugprone-branch-clone)
        else if (hdr->IsFromDs() && hdr->IsToDs())
        {
            // this is an AP-to-AP frame
            // we ignore for now.
            NotifyRxDrop(packet);
        }
        // NOLINTEND(bugprone-branch-clone)
        else
        {
            // we can ignore these frames since
            // they are not targeted at the AP
            NotifyRxDrop(packet);
        }
        return;
    }
    else if (hdr->IsMgt())
    {
        if (hdr->GetAddr1() == GetFrameExchangeManager(linkId)->GetAddress() &&
            GetWifiRemoteStationManager(linkId)->IsAssociated(from))
        {
            // this MPDU is being acknowledged by the AP, so we can process
            // the Power Management flag
            ProcessPowerManagementFlag(mpdu, linkId);
        }
        if (hdr->IsProbeReq() && (hdr->GetAddr1().IsGroup() ||
                                  hdr->GetAddr1() == GetFrameExchangeManager(linkId)->GetAddress()))
        {
            // In the case where the Address 1 field contains a group address, the
            // Address 3 field also is validated to verify that the group addressed
            // frame originated from a STA in the BSS of which the receiving STA is
            // a member (Section 9.3.3.1 of 802.11-2020)
            if (hdr->GetAddr1().IsGroup() && !hdr->GetAddr3().IsBroadcast() &&
                hdr->GetAddr3() != GetFrameExchangeManager(linkId)->GetAddress())
            {
                // not addressed to us
                return;
            }
            MgtProbeRequestHeader probeRequestHeader;
            packet->PeekHeader(probeRequestHeader);
            const auto& ssid = probeRequestHeader.Get<Ssid>();
            if (ssid == GetSsid() || ssid->IsBroadcast())
            {
                NS_LOG_DEBUG("Probe request received from " << from << ": send probe response");
                const auto isReqBcast = hdr->GetAddr1().IsGroup() && hdr->GetAddr3().IsBroadcast();
                // not an ML Probe Request if ADDR1 and ADDR3 are broadcast
                const auto probeResp = GetProbeResp(
                    linkId,
                    isReqBcast ? std::nullopt : probeRequestHeader.Get<MultiLinkElement>());
                EnqueueProbeResp(probeResp, from, linkId);
            }
            return;
        }
        else if (hdr->GetAddr1() == GetFrameExchangeManager(linkId)->GetAddress())
        {
            switch (hdr->GetType())
            {
            case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
            case WIFI_MAC_MGT_REASSOCIATION_REQUEST: {
                NS_LOG_DEBUG(((hdr->IsAssocReq()) ? "Association" : "Reassociation")
                             << " request received from " << from
                             << ((GetNLinks() > 1) ? " on link ID " + std::to_string(linkId) : ""));

                MgtAssocRequestHeader assocReq;
                MgtReassocRequestHeader reassocReq;
                AssocReqRefVariant frame = assocReq;
                if (hdr->IsAssocReq())
                {
                    packet->PeekHeader(assocReq);
                }
                else
                {
                    packet->PeekHeader(reassocReq);
                    frame = reassocReq;
                }
                if (ReceiveAssocRequest(frame, from, linkId) && GetNLinks() > 1)
                {
                    ParseReportedStaInfo(frame, from, linkId);
                }
                SendAssocResp(hdr->GetAddr2(), hdr->IsReassocReq(), linkId);
                return;
            }
            case WIFI_MAC_MGT_DISASSOCIATION: {
                NS_LOG_DEBUG("Disassociation received from " << from);
                GetWifiRemoteStationManager(linkId)->RecordDisassociated(from);
                auto& staList = GetLink(linkId).staList;
                for (auto it = staList.begin(); it != staList.end(); ++it)
                {
                    if (it->second == from)
                    {
                        staList.erase(it);
                        m_deAssocLogger(it->first, it->second);
                        if (GetWifiRemoteStationManager(linkId)->GetDsssSupported(from) &&
                            !GetWifiRemoteStationManager(linkId)->GetErpOfdmSupported(from))
                        {
                            GetLink(linkId).numNonErpStations--;
                        }
                        if (!GetWifiRemoteStationManager(linkId)->GetHtSupported(from) &&
                            !GetWifiRemoteStationManager(linkId)->GetStationHe6GhzCapabilities(
                                from))
                        {
                            GetLink(linkId).numNonHtStations--;
                        }
                        UpdateShortSlotTimeEnabled(linkId);
                        UpdateShortPreambleEnabled(linkId);
                        StaSwitchingToActiveModeOrDeassociated(from, linkId);
                        if (m_gcrManager)
                        {
                            m_gcrManager->NotifyStaDeassociated(from);
                        }
                        break;
                    }
                }
                return;
            }
            case WIFI_MAC_MGT_ACTION: {
                auto pkt = mpdu->GetPacket()->Copy();
                auto [category, action] = WifiActionHeader::Remove(pkt);
                if (category == WifiActionHeader::PROTECTED_EHT &&
                    action.protectedEhtAction ==
                        WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION &&
                    IsAssociated(hdr->GetAddr2()))
                {
                    // received an EML Operating Mode Notification frame from an associated station
                    MgtEmlOmn frame;
                    pkt->RemoveHeader(frame);
                    ReceiveEmlOmn(frame, hdr->GetAddr2(), linkId);
                    return;
                }
                break;
            }
            default:;
                // do nothing
            }
        }
    }

    // Invoke the receive handler of our parent class to deal with any other frames
    WifiMac::Receive(Create<WifiMpdu>(packet, *hdr), linkId);
}

bool
ApWifiMac::ReceiveAssocRequest(const AssocReqRefVariant& assoc,
                               const Mac48Address& from,
                               uint8_t linkId)
{
    NS_LOG_FUNCTION(this << from << +linkId);

    auto remoteStationManager = GetWifiRemoteStationManager(linkId);

    auto failure = [&](const std::string& msg) -> bool {
        NS_LOG_DEBUG("Association Request from " << from << " refused: " << msg);
        remoteStationManager->RecordAssocRefused(from);
        return false;
    };

    // lambda to process received (Re)Association Request
    auto recvAssocRequest = [&](auto&& frameRefWrapper) -> bool {
        const auto& frame = frameRefWrapper.get();

        // first, verify that the the station's supported
        // rate set is compatible with our Basic Rate set
        const CapabilityInformation& capabilities = frame.Capabilities();
        remoteStationManager->AddSupportedPhyPreamble(from, capabilities.IsShortPreamble());
        NS_ASSERT(frame.template Get<SupportedRates>());
        const auto rates = AllSupportedRates{*frame.template Get<SupportedRates>(),
                                             frame.template Get<ExtendedSupportedRatesIE>()};

        if (rates.GetNRates() == 0)
        {
            return failure("STA's supported rate set not compatible with our Basic Rate set");
        }

        if (GetHtSupported(linkId))
        {
            // check whether the HT STA supports all MCSs in Basic MCS Set
            const auto& htCapabilities = frame.template Get<HtCapabilities>();
            if (htCapabilities.has_value() && htCapabilities->IsSupportedMcs(0))
            {
                for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs(); i++)
                {
                    WifiMode mcs = remoteStationManager->GetBasicMcs(i);
                    if (!htCapabilities->IsSupportedMcs(mcs.GetMcsValue()))
                    {
                        return failure("HT STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
        }
        if (GetVhtSupported(linkId))
        {
            // check whether the VHT STA supports all MCSs in Basic MCS Set
            const auto& vhtCapabilities = frame.template Get<VhtCapabilities>();
            if (vhtCapabilities.has_value() && vhtCapabilities->GetVhtCapabilitiesInfo() != 0)
            {
                for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs(); i++)
                {
                    WifiMode mcs = remoteStationManager->GetBasicMcs(i);
                    if (!vhtCapabilities->IsSupportedTxMcs(mcs.GetMcsValue()))
                    {
                        return failure("VHT STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
        }
        if (GetHeSupported())
        {
            // check whether the HE STA supports all MCSs in Basic MCS Set
            const auto& heCapabilities = frame.template Get<HeCapabilities>();
            if (heCapabilities.has_value() && heCapabilities->GetSupportedMcsAndNss() != 0)
            {
                for (uint8_t i = 0; i < remoteStationManager->GetNBasicMcs(); i++)
                {
                    WifiMode mcs = remoteStationManager->GetBasicMcs(i);
                    if (!heCapabilities->IsSupportedTxMcs(mcs.GetMcsValue()))
                    {
                        return failure("HE STA does not support all MCSs in Basic MCS Set");
                    }
                }
            }
            if (Is6GhzBand(linkId))
            {
                if (const auto& he6GhzCapabilities = frame.template Get<He6GhzBandCapabilities>())
                {
                    remoteStationManager->AddStationHe6GhzCapabilities(from, *he6GhzCapabilities);
                }
            }
        }
        if (GetEhtSupported())
        {
            // TODO check whether the EHT STA supports all MCSs in Basic MCS Set
            auto ehtConfig = GetEhtConfiguration();
            NS_ASSERT(ehtConfig);

            if (const auto& tidLinkMapping = frame.template Get<TidToLinkMapping>();
                !tidLinkMapping.empty())
            {
                // non-AP MLD included TID-to-Link Mapping IE(s) in the Association Request.
                // We refuse association if we do not support TID-to-Link mapping negotiation
                // or the non-AP MLD included more than two TID-to-Link Mapping IEs
                // or we support negotiation type 1 but TIDs are mapped onto distinct link sets
                // or there is some TID that is not mapped to any link
                // or the direction(s) is/are not set properly
                if (tidLinkMapping.size() > 2)
                {
                    return failure("More than two TID-to-Link Mapping IEs");
                }

                // if only one Tid-to-Link Mapping element is present, it must be valid for
                // both directions
                bool bothDirIfOneTlm =
                    tidLinkMapping.size() != 1 ||
                    tidLinkMapping[0].m_control.direction == WifiDirection::BOTH_DIRECTIONS;
                // An MLD that includes two TID-To-Link Mapping elements in a (Re)Association
                // Request frame or a (Re)Association Response frame shall set the Direction
                // subfield in one of the TID-To-Link Mapping elements to 0 and the Direction
                // subfield in the other TID-To- Link Mapping element to 1.
                // (Sec. 35.3.7.1.8 of 802.11be D3.1)
                bool distinctDirsIfTwoTlms =
                    tidLinkMapping.size() != 2 ||
                    (tidLinkMapping[0].m_control.direction != WifiDirection::BOTH_DIRECTIONS &&
                     tidLinkMapping[1].m_control.direction != WifiDirection::BOTH_DIRECTIONS &&
                     tidLinkMapping[0].m_control.direction !=
                         tidLinkMapping[1].m_control.direction);

                if (!bothDirIfOneTlm || !distinctDirsIfTwoTlms)
                {
                    return failure("Incorrect directions in TID-to-Link Mapping IEs");
                }

                if (ehtConfig->m_tidLinkMappingSupport ==
                    WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
                {
                    return failure("TID-to-Link Mapping negotiation not supported");
                }

                auto getMapping = [](const TidToLinkMapping& tlmIe, WifiTidLinkMapping& mapping) {
                    if (tlmIe.m_control.defaultMapping)
                    {
                        return;
                    }
                    for (uint8_t tid = 0; tid < 8; tid++)
                    {
                        if (auto linkSet = tlmIe.GetLinkMappingOfTid(tid); !linkSet.empty())
                        {
                            mapping.emplace(tid, std::move(linkSet));
                        }
                    }
                };

                WifiTidLinkMapping dlMapping;
                WifiTidLinkMapping ulMapping;

                switch (tidLinkMapping[0].m_control.direction)
                {
                case WifiDirection::BOTH_DIRECTIONS:
                    getMapping(tidLinkMapping.at(0), dlMapping);
                    ulMapping = dlMapping;
                    break;
                case WifiDirection::DOWNLINK:
                    getMapping(tidLinkMapping.at(0), dlMapping);
                    getMapping(tidLinkMapping.at(1), ulMapping);
                    break;
                case WifiDirection::UPLINK:
                    getMapping(tidLinkMapping.at(0), ulMapping);
                    getMapping(tidLinkMapping.at(1), dlMapping);
                    break;
                }

                if (ehtConfig->m_tidLinkMappingSupport ==
                        WifiTidToLinkMappingNegSupport::SAME_LINK_SET &&
                    !TidToLinkMappingValidForNegType1(dlMapping, ulMapping))
                {
                    return failure("Mapping TIDs to distinct link sets is incompatible with "
                                   "negotiation support of 1");
                }

                // otherwise, we accept the TID-to-link Mapping and store it
                const auto& mle = frame.template Get<MultiLinkElement>();
                NS_ASSERT_MSG(mle,
                              "Multi-Link Element not present in an Association Request including "
                              "TID-to-Link Mapping element(s)");
                auto mldAddr = mle->GetMldMacAddress();

                // The requested link mappings are valid and can be accepted; store them.
                UpdateTidToLinkMapping(mldAddr, WifiDirection::DOWNLINK, dlMapping);
                UpdateTidToLinkMapping(mldAddr, WifiDirection::UPLINK, ulMapping);
            }
        }

        // The association request from the station can be accepted.
        // Record all its supported modes in its associated WifiRemoteStation
        auto phy = GetWifiPhy(linkId);

        for (const auto& mode : phy->GetModeList())
        {
            if (rates.IsSupportedRate(mode.GetDataRate(phy->GetChannelWidth())))
            {
                remoteStationManager->AddSupportedMode(from, mode);
            }
        }
        if (GetErpSupported(linkId) && remoteStationManager->GetErpOfdmSupported(from) &&
            capabilities.IsShortSlotTime())
        {
            remoteStationManager->AddSupportedErpSlotTime(from, true);
        }
        if (GetHtSupported(linkId))
        {
            const auto& htCapabilities = frame.template Get<HtCapabilities>();
            if (htCapabilities.has_value())
            {
                remoteStationManager->AddStationHtCapabilities(from, *htCapabilities);
            }
            const auto& extendedCapabilities = frame.template Get<ExtendedCapabilities>();
            if (extendedCapabilities.has_value())
            {
                remoteStationManager->AddStationExtendedCapabilities(from, *extendedCapabilities);
            }
        }
        if (GetVhtSupported(linkId))
        {
            const auto& vhtCapabilities = frame.template Get<VhtCapabilities>();
            // we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used
            // to check whether it supports VHT
            if (vhtCapabilities.has_value() &&
                vhtCapabilities->GetRxHighestSupportedLgiDataRate() > 0)
            {
                remoteStationManager->AddStationVhtCapabilities(from, *vhtCapabilities);
                for (const auto& mcs : phy->GetMcsList(WIFI_MOD_CLASS_VHT))
                {
                    if (vhtCapabilities->IsSupportedTxMcs(mcs.GetMcsValue()))
                    {
                        remoteStationManager->AddSupportedMcs(from, mcs);
                        // here should add a control to add basic MCS when it is implemented
                    }
                }
            }
        }
        if (GetHeSupported())
        {
            const auto& heCapabilities = frame.template Get<HeCapabilities>();
            if (heCapabilities.has_value() && heCapabilities->GetSupportedMcsAndNss() != 0)
            {
                remoteStationManager->AddStationHeCapabilities(from, *heCapabilities);
                for (const auto& mcs : phy->GetMcsList(WIFI_MOD_CLASS_HE))
                {
                    if (heCapabilities->IsSupportedTxMcs(mcs.GetMcsValue()))
                    {
                        remoteStationManager->AddSupportedMcs(from, mcs);
                        // here should add a control to add basic MCS when it is implemented
                    }
                }
            }
        }
        if (GetEhtSupported())
        {
            if (const auto& ehtCapabilities = frame.template Get<EhtCapabilities>())
            {
                remoteStationManager->AddStationEhtCapabilities(from, *ehtCapabilities);
            }
            for (const auto& mcs : phy->GetMcsList(WIFI_MOD_CLASS_EHT))
            {
                // TODO: Add check whether MCS is supported from the capabilities
                remoteStationManager->AddSupportedMcs(from, mcs);
                // here should add a control to add basic MCS when it is implemented
            }
        }

        NS_LOG_DEBUG("Association Request from " << from << " accepted");
        remoteStationManager->RecordWaitAssocTxOk(from);
        return true;
    };

    return std::visit(recvAssocRequest, assoc);
}

void
ApWifiMac::ParseReportedStaInfo(const AssocReqRefVariant& assoc, Mac48Address from, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << from << +linkId);

    // lambda to process received Multi-Link Element
    auto recvMle = [&](auto&& frame) {
        const auto& mle = frame.get().template Get<MultiLinkElement>();

        if (!mle.has_value())
        {
            return;
        }

        auto mleCommonInfo = std::make_shared<CommonInfoBasicMle>(mle->GetCommonInfoBasic());
        GetWifiRemoteStationManager(linkId)->AddStationMleCommonInfo(from, mleCommonInfo);

        for (std::size_t i = 0; i < mle->GetNPerStaProfileSubelements(); i++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(i);
            if (!perStaProfile.HasStaMacAddress())
            {
                NS_LOG_DEBUG("[i=" << i
                                   << "] Cannot setup a link if the STA MAC address is missing");
                continue;
            }
            uint8_t newLinkId = perStaProfile.GetLinkId();
            if (newLinkId == linkId || newLinkId >= GetNLinks())
            {
                NS_LOG_DEBUG("[i=" << i << "] Link ID " << newLinkId << " not valid");
                continue;
            }
            if (!perStaProfile.HasAssocRequest() && !perStaProfile.HasReassocRequest())
            {
                NS_LOG_DEBUG("[i=" << i << "] No (Re)Association Request frame body present");
                continue;
            }

            ReceiveAssocRequest(perStaProfile.GetAssocRequest(),
                                perStaProfile.GetStaMacAddress(),
                                newLinkId);
            GetWifiRemoteStationManager(newLinkId)->AddStationMleCommonInfo(
                perStaProfile.GetStaMacAddress(),
                mleCommonInfo);
        }
    };

    std::visit(recvMle, assoc);
}

void
ApWifiMac::ReceiveEmlOmn(MgtEmlOmn& frame, const Mac48Address& sender, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << frame << sender << linkId);

    auto ehtConfiguration = GetEhtConfiguration();

    if (!ehtConfiguration || !ehtConfiguration->m_emlsrActivated)
    {
        NS_LOG_DEBUG(
            "Received an EML Operating Mode Notification frame but EMLSR is not activated");
        return;
    }

    if (frame.m_emlControl.emlsrParamUpdateCtrl)
    {
        NS_ASSERT(frame.m_emlsrParamUpdate);
        auto emlCapabilities =
            GetWifiRemoteStationManager(linkId)->GetStationEmlCapabilities(sender);
        NS_ASSERT_MSG(emlCapabilities, "EML Capabilities not stored for STA " << sender);

        // update values stored in remote station manager
        emlCapabilities->get().emlsrPaddingDelay = frame.m_emlsrParamUpdate->paddingDelay;
        emlCapabilities->get().emlsrTransitionDelay = frame.m_emlsrParamUpdate->transitionDelay;
    }

    auto mldAddress = GetWifiRemoteStationManager(linkId)->GetMldAddress(sender);
    NS_ASSERT_MSG(mldAddress, "No MLD address stored for STA " << sender);
    auto emlsrLinks =
        frame.m_emlControl.emlsrMode == 1 ? frame.GetLinkBitmap() : std::list<uint8_t>{};

    // The AP MLD has to consider the changes carried by the received EML Notification frame
    // as effective at the same time as the non-AP MLD. Therefore, we need to start a time
    // when the transmission of the Ack following the received EML Notification frame is
    // completed. For this purpose, we connect a callback to the PHY TX begin trace to catch
    // the Ack transmitted after the EML Notification frame.
    CallbackBase cb = Callback<void, WifiConstPsduMap, WifiTxVector, Watt_u>(
        [=, this](WifiConstPsduMap psduMap, WifiTxVector txVector, Watt_u /* txPower */) {
            NS_ASSERT_MSG(psduMap.size() == 1 && psduMap.begin()->second->GetNMpdus() == 1 &&
                              psduMap.begin()->second->GetHeader(0).IsAck(),
                          "Expected a Normal Ack after EML Notification frame");

            auto ackDuration =
                WifiPhy::CalculateTxDuration(psduMap, txVector, GetLink(linkId).phy->GetPhyBand());

            m_transitionTimeoutEvents[sender] = Simulator::Schedule(
                ackDuration + ehtConfiguration->m_transitionTimeout,
                [=, this]() {
                    for (uint8_t id = 0; id < GetNLinks(); id++)
                    {
                        auto linkAddress =
                            GetWifiRemoteStationManager(id)->GetAffiliatedStaAddress(*mldAddress);
                        if (!linkAddress)
                        {
                            // this link has not been setup by the non-AP MLD
                            continue;
                        }

                        if (!emlsrLinks.empty())
                        {
                            // the non-AP MLD is enabling EMLSR mode
                            /**
                             * After the successful transmission of the EML Operating Mode
                             * Notification frame by the non-AP STA affiliated with the non-AP MLD,
                             * the non-AP MLD shall operate in the EMLSR mode and the other non-AP
                             * STAs operating on the corresponding EMLSR links shall transition to
                             * active mode after the transition delay indicated in the Transition
                             * Timeout subfield in the EML Capabilities subfield of the Basic
                             * Multi-Link element or immediately after receiving an EML Operating
                             * Mode Notification frame from one of the APs operating on the EMLSR
                             * links and affiliated with the AP MLD (Sec. 35.3.17 of 802.11be D3.0)
                             */
                            auto enabled = std::find(emlsrLinks.cbegin(), emlsrLinks.cend(), id) !=
                                           emlsrLinks.cend();
                            if (enabled)
                            {
                                StaSwitchingToActiveModeOrDeassociated(*linkAddress, id);
                            }
                            GetWifiRemoteStationManager(id)->SetEmlsrEnabled(*linkAddress, enabled);
                        }
                        else
                        {
                            // the non-AP MLD is disabling EMLSR mode
                            /**
                             * After the successful transmission of the EML Operating Mode
                             * Notification frame by the non-AP STA affiliated with the non-AP MLD,
                             * the non-AP MLD shall disable the EMLSR mode and the other non-AP
                             * STAs operating on the corresponding EMLSR links shall transition to
                             * power save mode after the transition delay indicated in the
                             * Transition Timeout subfield in the EML Capabilities subfield of the
                             * Basic Multi-Link element or immediately after receiving an EML
                             * Operating Mode Notification frame from one of the APs operating on
                             * the EMLSR links and affiliated with the AP MLD. (Sec. 35.3.17 of
                             * 802.11be D3.0)
                             */
                            if (id != linkId &&
                                GetWifiRemoteStationManager(id)->GetEmlsrEnabled(*linkAddress))
                            {
                                StaSwitchingToPsMode(*linkAddress, id);
                            }
                            GetWifiRemoteStationManager(id)->SetEmlsrEnabled(*linkAddress, false);
                        }
                    }
                });
        });

    // connect the callback to the PHY TX begin trace to catch the Ack and disconnect
    // after its transmission begins
    auto phy = GetLink(linkId).phy;
    phy->TraceConnectWithoutContext("PhyTxPsduBegin", cb);
    Simulator::Schedule(phy->GetSifs() + NanoSeconds(1),
                        [=]() { phy->TraceDisconnectWithoutContext("PhyTxPsduBegin", cb); });

    // An AP MLD with dot11EHTEMLSROptionActivated equal to true sets the EMLSR Mode subfield
    // to the value obtained from the EMLSR Mode subfield of the received EML Operating Mode
    // Notification frame. (Sec. 9.6.35.8 of 802.11be D3.0)

    // When included in a frame sent by an AP affiliated with an AP MLD, the EMLSR Parameter
    // Update Control subfield is set to 0. (Sec. 9.6.35.8 of 802.11be D3.0)
    frame.m_emlControl.emlsrParamUpdateCtrl = 0;

    // An AP MLD with dot11EHTEMLSROptionImplemented equal to true sets the EMLSR Link Bitmap
    // subfield to the value obtained from the EMLSR Link Bitmap subfield of the received
    // EML Operating Mode Notification frame. (Sec. 9.6.35.8 of 802.11be D3.0)

    // The EMLSR Parameter Update field [..] is present if [..] the Action frame is sent by
    // a non-AP STA affiliated with a non-AP MLD (Sec. 9.6.35.8 of 802.11be D3.0)
    frame.m_emlsrParamUpdate.reset();

    auto ehtFem = StaticCast<EhtFrameExchangeManager>(GetFrameExchangeManager(linkId));
    ehtFem->SendEmlOmn(sender, frame);
}

void
ApWifiMac::DeaggregateAmsduAndForward(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    for (auto& i : *PeekPointer(mpdu))
    {
        auto from = i.second.GetSourceAddr();
        auto to = i.second.GetDestinationAddr();

        if (to.IsGroup() || IsAssociated(to))
        {
            NS_LOG_DEBUG("forwarding QoS frame from=" << from << ", to=" << to);
            WifiMac::Enqueue(i.first->Copy(), to, from, mpdu->GetHeader().GetQosTid());
        }

        ForwardUp(i.first, from, to);
    }
}

void
ApWifiMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_beaconTxop->Initialize();

    for (uint8_t linkId = 0; linkId < GetNLinks(); ++linkId)
    {
        GetLink(linkId).beaconEvent.Cancel();
        if (m_enableBeaconGeneration)
        {
            uint64_t jitterUs =
                (m_enableBeaconJitter
                     ? static_cast<uint64_t>(m_beaconJitter->GetValue(0, 1) *
                                             (GetBeaconInterval().GetMicroSeconds()))
                     : 0);
            NS_LOG_DEBUG("Scheduling initial beacon for access point "
                         << GetAddress() << " at time " << jitterUs << "us");
            GetLink(linkId).beaconEvent = Simulator::Schedule(MicroSeconds(jitterUs),
                                                              &ApWifiMac::SendOneBeacon,
                                                              this,
                                                              linkId);
        }
        UpdateShortSlotTimeEnabled(linkId);
        UpdateShortPreambleEnabled(linkId);
    }

    if (m_gcrManager)
    {
        m_gcrManager->Initialize();
    }

    NS_ABORT_IF(!TraceConnectWithoutContext("AckedMpdu", MakeCallback(&ApWifiMac::TxOk, this)));
    NS_ABORT_IF(
        !TraceConnectWithoutContext("DroppedMpdu", MakeCallback(&ApWifiMac::TxFailed, this)));
    WifiMac::DoInitialize();
}

bool
ApWifiMac::GetUseNonErpProtection(uint8_t linkId) const
{
    bool useProtection = (GetLink(linkId).numNonErpStations > 0) && m_enableNonErpProtection;
    GetWifiRemoteStationManager(linkId)->SetUseNonErpProtection(useProtection);
    return useProtection;
}

uint16_t
ApWifiMac::GetNextAssociationId() const
{
    const auto& links = GetLinks();

    // Return the first AID value between 1 and 2007 that is free for all the links
    for (uint16_t nextAid = 1; nextAid <= 2007; ++nextAid)
    {
        if (std::none_of(links.cbegin(), links.cend(), [&](auto&& idLinkPair) {
                return GetStaList(idLinkPair.first).contains(nextAid);
            }))
        {
            return nextAid;
        }
    }
    NS_FATAL_ERROR("No free association ID available!");
    return 0;
}

const std::map<uint16_t, Mac48Address>&
ApWifiMac::GetStaList(uint8_t linkId) const
{
    return GetLink(linkId).staList;
}

uint16_t
ApWifiMac::GetAssociationId(Mac48Address addr, uint8_t linkId) const
{
    return GetWifiRemoteStationManager(linkId)->GetAssociationId(addr);
}

uint8_t
ApWifiMac::GetBufferStatus(uint8_t tid, Mac48Address address) const
{
    auto it = m_bufferStatus.find(WifiAddressTidPair(address, tid));
    if (it == m_bufferStatus.end() || it->second.timestamp + m_bsrLifetime < Simulator::Now())
    {
        return 255;
    }
    return it->second.value;
}

void
ApWifiMac::SetBufferStatus(uint8_t tid, Mac48Address address, uint8_t size)
{
    if (size == 255)
    {
        // no point in storing an unspecified size
        m_bufferStatus.erase(WifiAddressTidPair(address, tid));
    }
    else
    {
        m_bufferStatus[WifiAddressTidPair(address, tid)] = {size, Simulator::Now()};
    }
}

uint8_t
ApWifiMac::GetMaxBufferStatus(Mac48Address address) const
{
    uint8_t maxSize = 0;
    bool found = false;

    for (uint8_t tid = 0; tid < 8; tid++)
    {
        uint8_t size = GetBufferStatus(tid, address);
        if (size != 255)
        {
            maxSize = std::max(maxSize, size);
            found = true;
        }
    }

    if (found)
    {
        return maxSize;
    }
    return 255;
}

bool
ApWifiMac::IsGcrBaAgreementEstablishedWithAllMembers(const Mac48Address& groupAddress,
                                                     uint8_t tid) const
{
    NS_ASSERT(m_gcrManager);
    return GetQosTxop(tid)->GetBaManager()->IsGcrAgreementEstablished(
        groupAddress,
        tid,
        m_gcrManager->GetMemberStasForGroupAddress(groupAddress));
}

} // namespace ns3
