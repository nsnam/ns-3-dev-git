/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-phy.h"

#include "uhr-configuration.h"
#include "uhr-ppdu.h"

#include "ns3/interference-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_PHY_NS_LOG_APPEND_CONTEXT(m_wifiPhy)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrPhy");

/*******************************************************
 *       UHR PHY (P802.11bn/D0.2)
 *******************************************************/

// clang-format off

const PhyEntity::PpduFormats UhrPhy::m_uhrPpduFormats {
    { WIFI_PREAMBLE_UHR_MU, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_U_SIG,         // U-SIG
                              WIFI_PPDU_FIELD_UHR_SIG,       // UHR-SIG
                              WIFI_PPDU_FIELD_TRAINING,      // UHR-STF + UHR-LTFs
                              WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_UHR_TB, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_U_SIG,         // U-SIG
                              WIFI_PPDU_FIELD_TRAINING,      // UHR-STF + UHR-LTFs
                              WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_UHR_ELR, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_U_SIG,         // U-SIG
                              WIFI_PPDU_FIELD_ELR_MARK,      // ELR-MARK
                              WIFI_PPDU_FIELD_TRAINING,      // UHR-STF + UHR-LTFs
                              WIFI_PPDU_FIELD_ELR_SIG,       // ELR-SIG
                              WIFI_PPDU_FIELD_DATA } }
};

// clang-format on

UhrPhy::UhrPhy(bool buildModeList /* = true */)
    : EhtPhy(false) // don't add EHT modes to list
{
    NS_LOG_FUNCTION(this << buildModeList);
    m_bssMembershipSelector = UHR_PHY;
    m_maxMcsIndexPerSs = 13;
    m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
    if (buildModeList)
    {
        BuildModeList();
    }
}

UhrPhy::~UhrPhy()
{
    NS_LOG_FUNCTION(this);
}

void
UhrPhy::BuildModeList()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_modeList.empty());
    NS_ASSERT(m_bssMembershipSelector == UHR_PHY);
    for (uint8_t index = 0; index <= m_maxSupportedMcsIndexPerSs; ++index)
    {
        NS_LOG_LOGIC("Add UhrMcs" << +index << " to list");
        m_modeList.emplace_back(CreateUhrMcs(index));
    }
}

WifiMode
UhrPhy::GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_UHR_SIG:
        return EhtPhy::GetSigMode(WIFI_PPDU_FIELD_EHT_SIG,
                                  txVector); // UHR-SIG is similar to EHT-SIG
    default:
        return EhtPhy::GetSigMode(field, txVector);
    }
}

Time
UhrPhy::GetDuration(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_UHR_SIG:
        return EhtPhy::GetDuration(WIFI_PPDU_FIELD_EHT_SIG,
                                   txVector); // UHR-SIG is similar to EHT-SIG
    case WIFI_PPDU_FIELD_ELR_MARK:
    case WIFI_PPDU_FIELD_ELR_SIG:
        // TODO: ELR not supported yet
    case WIFI_PPDU_FIELD_SIG_A:
    case WIFI_PPDU_FIELD_SIG_B:
    case WIFI_PPDU_FIELD_EHT_SIG:
        // not present in UHR PPDUs
        return NanoSeconds(0);
    default:
        return EhtPhy::GetDuration(field, txVector);
    }
}

Time
UhrPhy::CalculateNonHeDurationForHeMu(const WifiTxVector& txVector) const
{
    Time duration = GetDuration(WIFI_PPDU_FIELD_PREAMBLE, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_NON_HT_HEADER, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_U_SIG, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_UHR_SIG, txVector);
    return duration;
}

const PhyEntity::PpduFormats&
UhrPhy::GetPpduFormats() const
{
    return m_uhrPpduFormats;
}

Ptr<WifiPpdu>
UhrPhy::BuildPpdu(const WifiConstPsduMap& psdus, const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << psdus << txVector << ppduDuration);
    return Create<UhrPpdu>(psdus,
                           txVector,
                           m_wifiPhy->GetOperatingChannel(),
                           ppduDuration,
                           ObtainNextUid(txVector),
                           HePpdu::PSD_NON_HE_PORTION);
}

PhyEntity::PhyFieldRxStatus
UhrPhy::DoEndReceiveField(WifiPpduField field, Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << field << *event);
    switch (field)
    {
    case WIFI_PPDU_FIELD_UHR_SIG:
        return EhtPhy::DoEndReceiveField(WIFI_PPDU_FIELD_EHT_SIG,
                                         event); // UHR-SIG is similar to EHT-SIG
    default:
        return EhtPhy::DoEndReceiveField(field, event);
    }
}

PhyEntity::PhyFieldRxStatus
UhrPhy::ProcessSig(Ptr<Event> event, PhyFieldRxStatus status, WifiPpduField field)
{
    NS_LOG_FUNCTION(this << *event << status << field);
    switch (field)
    {
    case WIFI_PPDU_FIELD_UHR_SIG:
        return EhtPhy::ProcessSig(event,
                                  status,
                                  WIFI_PPDU_FIELD_EHT_SIG); // UHR-SIG is similar to EHT-SIG
    default:
        return EhtPhy::ProcessSig(event, status, field);
    }
    return status;
}

WifiPhyRxfailureReason
UhrPhy::GetFailureReason(WifiPpduField field) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_UHR_SIG:
        return UHR_SIG_FAILURE;
    case WIFI_PPDU_FIELD_ELR_MARK:
        return ELR_MARK_FAILURE;
    case WIFI_PPDU_FIELD_ELR_SIG:
        return ELR_SIG_FAILURE;
    default:
        return EhtPhy::GetFailureReason(field);
    }
}

void
UhrPhy::InitializeModes()
{
    for (uint8_t i = 0; i <= 13; ++i)
    {
        GetUhrMcs(i);
    }
}

WifiMode
UhrPhy::GetUhrMcs(uint8_t index)
{
#define CASE(x)                                                                                    \
    case x:                                                                                        \
        return GetUhrMcs##x();

    switch (index)
    {
        CASE(0)
        CASE(1)
        CASE(2)
        CASE(3)
        CASE(4)
        CASE(5)
        CASE(6)
        CASE(7)
        CASE(8)
        CASE(9)
        CASE(10)
        CASE(11)
        CASE(12)
        CASE(13)
    default:
        NS_ABORT_MSG("Inexistent index (" << +index << ") requested for UHR");
        return WifiMode();
    }
#undef CASE
}

#define GET_UHR_MCS(x)                                                                             \
    WifiMode UhrPhy::GetUhrMcs##x()                                                                \
    {                                                                                              \
        static WifiMode mcs = CreateUhrMcs(x);                                                     \
        return mcs;                                                                                \
    }

GET_UHR_MCS(0)
GET_UHR_MCS(1)
GET_UHR_MCS(2)
GET_UHR_MCS(3)
GET_UHR_MCS(4)
GET_UHR_MCS(5)
GET_UHR_MCS(6)
GET_UHR_MCS(7)
GET_UHR_MCS(8)
GET_UHR_MCS(9)
GET_UHR_MCS(10)
GET_UHR_MCS(11)
GET_UHR_MCS(12)
GET_UHR_MCS(13)
#undef GET_UHR_MCS

WifiMode
UhrPhy::CreateUhrMcs(uint8_t index)
{
    NS_ASSERT_MSG(index <= 13, "UhrMcs index must be <= 13!");
    return WifiModeFactory::CreateWifiMcs("UhrMcs" + std::to_string(index),
                                          index,
                                          WIFI_MOD_CLASS_UHR,
                                          false,
                                          MakeBoundCallback(&GetCodeRate, index),
                                          MakeBoundCallback(&GetConstellationSize, index),
                                          MakeCallback(&GetPhyRateFromTxVector),
                                          MakeCallback(&GetDataRateFromTxVector),
                                          MakeBoundCallback(&GetNonHtReferenceRate, index),
                                          MakeCallback(&IsAllowed));
}

bool
UhrPhy::IsChannelWidthSupported(Ptr<const WifiPpdu> ppdu) const
{
    auto device = m_wifiPhy->GetDevice();
    auto mac = device->GetMac();
    auto uhrConfiguration = device->GetUhrConfiguration();
    const auto& txVector{ppdu->GetTxVector()};
    if ((mac->GetTypeOfStation() == STA) && uhrConfiguration->GetDsoActivated() &&
        IsUhr(txVector.GetPreambleType()) && txVector.IsMu() &&
        (txVector.GetChannelWidth() > m_wifiPhy->GetChannelWidth()))
    {
        // total channel width can be larger than the one supported by the UHR PHY for DSO STA
        return true;
    }
    return EhtPhy::IsChannelWidthSupported(ppdu);
}

MHz_t
UhrPhy::GetChannelWidthForMu(const WifiTxVector& txVector, uint16_t staId) const
{
    auto device = m_wifiPhy->GetDevice();
    auto mac = device->GetMac();
    auto uhrConfiguration = device->GetUhrConfiguration();
    if ((mac->GetTypeOfStation() == STA) && uhrConfiguration->GetDsoActivated() &&
        IsUhr(txVector.GetPreambleType()) && txVector.IsMu() &&
        (txVector.GetChannelWidth() > m_wifiPhy->GetChannelWidth()))
    {
        // channel width to consider for DSO STA is the channel width of the DSO subchannel
        return m_wifiPhy->GetChannelWidth();
    }
    return EhtPhy::GetChannelWidthForMu(txVector, staId);
}

} // namespace ns3

namespace
{

/**
 * Constructor class for UHR modes
 */
class ConstructorUhr
{
  public:
    ConstructorUhr()
    {
        ns3::UhrPhy::InitializeModes();
        ns3::WifiPhy::AddStaticPhyEntity(ns3::WIFI_MOD_CLASS_UHR, ns3::Create<ns3::UhrPhy>());
    }
} g_constructor_uhr; ///< the constructor for UHR modes

} // namespace
