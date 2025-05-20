/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-phy.h"

#include "eht-configuration.h"
#include "eht-ppdu.h"

#include "ns3/interference-helper.h"
#include "ns3/obss-pd-algorithm.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_PHY_NS_LOG_APPEND_CONTEXT(m_wifiPhy)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtPhy");

/*******************************************************
 *       EHT PHY (P802.11be/D1.5)
 *******************************************************/

// clang-format off

const PhyEntity::PpduFormats EhtPhy::m_ehtPpduFormats {
    { WIFI_PREAMBLE_EHT_MU, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_U_SIG,         // U-SIG
                              WIFI_PPDU_FIELD_EHT_SIG,       // EHT-SIG
                              WIFI_PPDU_FIELD_TRAINING,      // EHT-STF + EHT-LTFs
                              WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_EHT_TB, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                              WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                              WIFI_PPDU_FIELD_U_SIG,         // U-SIG
                              WIFI_PPDU_FIELD_TRAINING,      // EHT-STF + EHT-LTFs
                              WIFI_PPDU_FIELD_DATA } }
};

/**
 * \brief map a given secondary channel width to its channel list type
 */
const std::map<MHz_u, WifiChannelListType> ehtSecondaryChannels {
    {20, WIFI_CHANLIST_SECONDARY},
    {40, WIFI_CHANLIST_SECONDARY40},
    {80, WIFI_CHANLIST_SECONDARY80},
    {160, WIFI_CHANLIST_SECONDARY160},
};

// clang-format on

EhtPhy::EhtPhy(bool buildModeList /* = true */)
    : HePhy(false) // don't add HE modes to list
{
    NS_LOG_FUNCTION(this << buildModeList);
    m_bssMembershipSelector = EHT_PHY;
    m_maxMcsIndexPerSs = 13;
    m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
    if (buildModeList)
    {
        BuildModeList();
    }
}

EhtPhy::~EhtPhy()
{
    NS_LOG_FUNCTION(this);
}

void
EhtPhy::BuildModeList()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_modeList.empty());
    NS_ASSERT(m_bssMembershipSelector == EHT_PHY);
    for (uint8_t index = 0; index <= m_maxSupportedMcsIndexPerSs; ++index)
    {
        NS_LOG_LOGIC("Add EhtMcs" << +index << " to list");
        m_modeList.emplace_back(CreateEhtMcs(index));
    }
}

uint16_t
EhtPhy::GetUsableSubcarriers(MHz_u channelWidth)
{
    if (channelWidth == MHz_u{320})
    {
        return 3920;
    }
    return HePhy::GetUsableSubcarriers(channelWidth);
}

WifiMode
EhtPhy::GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_U_SIG:
        return GetSigAMode(); // U-SIG is similar to SIG-A
    case WIFI_PPDU_FIELD_EHT_SIG:
        return GetSigBMode(txVector); // EHT-SIG is similar to SIG-B
    default:
        return HePhy::GetSigMode(field, txVector);
    }
}

WifiMode
EhtPhy::GetSigBMode(const WifiTxVector& txVector) const
{
    if (txVector.IsDlMu())
    {
        return HePhy::GetSigBMode(txVector);
    }
    // we get here in case of EHT SU transmission
    // TODO fix the MCS used for EHT-SIG
    auto smallestMcs = std::min<uint8_t>(5, txVector.GetMode().GetMcsValue());
    return VhtPhy::GetVhtMcs(smallestMcs);
}

Time
EhtPhy::GetDuration(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_U_SIG:
        return GetSigADuration(txVector.GetPreambleType()); // U-SIG is similar to SIG-A
    case WIFI_PPDU_FIELD_EHT_SIG:
        return GetSigBDuration(txVector); // EHT-SIG is similar to SIG-B
    case WIFI_PPDU_FIELD_SIG_A:
        [[fallthrough]];
    case WIFI_PPDU_FIELD_SIG_B:
        return NanoSeconds(0);
    default:
        return HePhy::GetDuration(field, txVector);
    }
}

uint32_t
EhtPhy::GetSigBSize(const WifiTxVector& txVector) const
{
    if (ns3::IsDlMu(txVector.GetPreambleType()) && ns3::IsEht(txVector.GetPreambleType()))
    {
        return EhtPpdu::GetEhtSigFieldSize(
            txVector.GetChannelWidth(),
            txVector.GetRuAllocation(
                m_wifiPhy ? m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(MHz_u{20}) : 0),
            txVector.GetEhtPpduType(),
            txVector.IsSigBCompression(),
            txVector.IsSigBCompression() ? txVector.GetHeMuUserInfoMap().size() : 0);
    }
    return HePhy::GetSigBSize(txVector);
}

Time
EhtPhy::CalculateNonHeDurationForHeTb(const WifiTxVector& txVector) const
{
    Time duration = GetDuration(WIFI_PPDU_FIELD_PREAMBLE, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_NON_HT_HEADER, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_U_SIG, txVector);
    return duration;
}

Time
EhtPhy::CalculateNonHeDurationForHeMu(const WifiTxVector& txVector) const
{
    Time duration = GetDuration(WIFI_PPDU_FIELD_PREAMBLE, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_NON_HT_HEADER, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_U_SIG, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_EHT_SIG, txVector);
    return duration;
}

const PhyEntity::PpduFormats&
EhtPhy::GetPpduFormats() const
{
    return m_ehtPpduFormats;
}

Ptr<WifiPpdu>
EhtPhy::BuildPpdu(const WifiConstPsduMap& psdus, const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << psdus << txVector << ppduDuration);
    return Create<EhtPpdu>(psdus,
                           txVector,
                           m_wifiPhy->GetOperatingChannel(),
                           ppduDuration,
                           ObtainNextUid(txVector),
                           HePpdu::PSD_NON_HE_PORTION);
}

PhyEntity::PhyFieldRxStatus
EhtPhy::DoEndReceiveField(WifiPpduField field, Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << field << *event);
    switch (field)
    {
    case WIFI_PPDU_FIELD_U_SIG:
        [[fallthrough]];
    case WIFI_PPDU_FIELD_EHT_SIG:
        return EndReceiveSig(event, field);
    default:
        return HePhy::DoEndReceiveField(field, event);
    }
}

PhyEntity::PhyFieldRxStatus
EhtPhy::ProcessSig(Ptr<Event> event, PhyFieldRxStatus status, WifiPpduField field)
{
    NS_LOG_FUNCTION(this << *event << status << field);
    switch (field)
    {
    case WIFI_PPDU_FIELD_U_SIG:
        return ProcessSigA(event, status); // U-SIG is similar to SIG-A
    case WIFI_PPDU_FIELD_EHT_SIG:
        return ProcessSigB(event, status); // EHT-SIG is similar to SIG-B
    default:
        return HePhy::ProcessSig(event, status, field);
    }
    return status;
}

WifiPhyRxfailureReason
EhtPhy::GetFailureReason(WifiPpduField field) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_U_SIG:
        return U_SIG_FAILURE;
    case WIFI_PPDU_FIELD_EHT_SIG:
        return EHT_SIG_FAILURE;
    default:
        return HePhy::GetFailureReason(field);
    }
}

void
EhtPhy::InitializeModes()
{
    for (uint8_t i = 0; i <= 13; ++i)
    {
        GetEhtMcs(i);
    }
}

WifiMode
EhtPhy::GetEhtMcs(uint8_t index)
{
#define CASE(x)                                                                                    \
    case x:                                                                                        \
        return GetEhtMcs##x();

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
        NS_ABORT_MSG("Inexistent index (" << +index << ") requested for EHT");
        return WifiMode();
    }
#undef CASE
}

#define GET_EHT_MCS(x)                                                                             \
    WifiMode EhtPhy::GetEhtMcs##x()                                                                \
    {                                                                                              \
        static WifiMode mcs = CreateEhtMcs(x);                                                     \
        return mcs;                                                                                \
    }

GET_EHT_MCS(0)
GET_EHT_MCS(1)
GET_EHT_MCS(2)
GET_EHT_MCS(3)
GET_EHT_MCS(4)
GET_EHT_MCS(5)
GET_EHT_MCS(6)
GET_EHT_MCS(7)
GET_EHT_MCS(8)
GET_EHT_MCS(9)
GET_EHT_MCS(10)
GET_EHT_MCS(11)
GET_EHT_MCS(12)
GET_EHT_MCS(13)
#undef GET_EHT_MCS

WifiMode
EhtPhy::CreateEhtMcs(uint8_t index)
{
    NS_ASSERT_MSG(index <= 13, "EhtMcs index must be <= 13!");
    return WifiModeFactory::CreateWifiMcs("EhtMcs" + std::to_string(index),
                                          index,
                                          WIFI_MOD_CLASS_EHT,
                                          false,
                                          MakeBoundCallback(&GetCodeRate, index),
                                          MakeBoundCallback(&GetConstellationSize, index),
                                          MakeCallback(&GetPhyRateFromTxVector),
                                          MakeCallback(&GetDataRateFromTxVector),
                                          MakeBoundCallback(&GetNonHtReferenceRate, index),
                                          MakeCallback(&IsAllowed));
}

WifiCodeRate
EhtPhy::GetCodeRate(uint8_t mcsValue)
{
    switch (mcsValue)
    {
    case 12:
        return WIFI_CODE_RATE_3_4;
    case 13:
        return WIFI_CODE_RATE_5_6;
    default:
        return HePhy::GetCodeRate(mcsValue);
    }
}

uint16_t
EhtPhy::GetConstellationSize(uint8_t mcsValue)
{
    switch (mcsValue)
    {
    case 12:
        [[fallthrough]];
    case 13:
        return 4096;
    default:
        return HePhy::GetConstellationSize(mcsValue);
    }
}

uint64_t
EhtPhy::GetPhyRate(uint8_t mcsValue, MHz_u channelWidth, Time guardInterval, uint8_t nss)
{
    const auto codeRate = GetCodeRate(mcsValue);
    const auto dataRate = GetDataRate(mcsValue, channelWidth, guardInterval, nss);
    return HtPhy::CalculatePhyRate(codeRate, dataRate);
}

uint64_t
EhtPhy::GetPhyRateFromTxVector(const WifiTxVector& txVector, uint16_t staId /* = SU_STA_ID */)
{
    auto bw = txVector.GetChannelWidth();
    if (txVector.IsMu())
    {
        bw = WifiRu::GetBandwidth(WifiRu::GetRuType(txVector.GetRu(staId)));
    }
    return EhtPhy::GetPhyRate(txVector.GetMode(staId).GetMcsValue(),
                              bw,
                              txVector.GetGuardInterval(),
                              txVector.GetNss(staId));
}

uint64_t
EhtPhy::GetDataRateFromTxVector(const WifiTxVector& txVector, uint16_t staId /* = SU_STA_ID */)
{
    auto bw = txVector.GetChannelWidth();
    if (txVector.IsMu())
    {
        bw = WifiRu::GetBandwidth(WifiRu::GetRuType(txVector.GetRu(staId)));
    }
    return EhtPhy::GetDataRate(txVector.GetMode(staId).GetMcsValue(),
                               bw,
                               txVector.GetGuardInterval(),
                               txVector.GetNss(staId));
}

uint64_t
EhtPhy::GetDataRate(uint8_t mcsValue, MHz_u channelWidth, Time guardInterval, uint8_t nss)
{
    [[maybe_unused]] const auto gi = guardInterval.GetNanoSeconds();
    NS_ASSERT((gi == 800) || (gi == 1600) || (gi == 3200));
    NS_ASSERT(nss <= 8);
    return HtPhy::CalculateDataRate(GetSymbolDuration(guardInterval),
                                    GetUsableSubcarriers(channelWidth),
                                    static_cast<uint16_t>(log2(GetConstellationSize(mcsValue))),
                                    HtPhy::GetCodeRatio(GetCodeRate(mcsValue)),
                                    nss);
}

uint64_t
EhtPhy::GetNonHtReferenceRate(uint8_t mcsValue)
{
    WifiCodeRate codeRate = GetCodeRate(mcsValue);
    uint16_t constellationSize = GetConstellationSize(mcsValue);
    return CalculateNonHtReferenceRate(codeRate, constellationSize);
}

uint64_t
EhtPhy::CalculateNonHtReferenceRate(WifiCodeRate codeRate, uint16_t constellationSize)
{
    uint64_t dataRate;
    switch (constellationSize)
    {
    case 4096:
        if (codeRate == WIFI_CODE_RATE_3_4 || codeRate == WIFI_CODE_RATE_5_6)
        {
            dataRate = 54000000;
        }
        else
        {
            NS_FATAL_ERROR("Trying to get reference rate for a MCS with wrong combination of "
                           "coding rate and modulation");
        }
        break;
    default:
        dataRate = HePhy::CalculateNonHtReferenceRate(codeRate, constellationSize);
    }
    return dataRate;
}

dBm_u
EhtPhy::Per20MHzCcaThreshold(const Ptr<const WifiPpdu> ppdu) const
{
    if (!ppdu)
    {
        /**
         * A signal is present on the 20 MHz subchannel at or above a threshold of –62 dBm at the
         * receiver's antenna(s). The PHY shall indicate that the 20 MHz subchannel is busy a period
         * aCCATime after the signal starts and shall continue to indicate the 20 MHz subchannel is
         * busy while the threshold continues to be exceeded (Sec. 36.3.21.6.4 - Per 20 MHz CCA
         * sensitivity - of 802.11be D7.0).
         */
        return m_wifiPhy->GetCcaEdThreshold();
    }

    /**
     * A non-HT, HT_MF, HT_GF, VHT, HE, or EHT PPDU for which the power measured within
     * this 20 MHz subchannel is at or above max(–72 dBm, OBSS_PD level) at the
     * receiver’s antenna(s). The PHY shall indicate that the 20 MHz subchannel is busy
     * with greater than 90% probability within a period aCCAMidTime (Sec. 36.3.21.6.4 - Per 20 MHz
     * CCA sensitivity - of 802.11be D7.0).
     */
    auto ehtConfiguration = m_wifiPhy->GetDevice()->GetEhtConfiguration();
    NS_ASSERT(ehtConfiguration);
    const auto ccaThresholdNonObss = ehtConfiguration->m_per20CcaSensitivityThreshold;
    return GetObssPdAlgorithm()
               ? std::max(ccaThresholdNonObss, GetObssPdAlgorithm()->GetObssPdLevel())
               : ccaThresholdNonObss;
}

dBm_u
EhtPhy::GetCcaThreshold(const Ptr<const WifiPpdu> ppdu, WifiChannelListType channelType) const
{
    if (channelType != WIFI_CHANLIST_PRIMARY)
    {
        return Per20MHzCcaThreshold(ppdu);
    }
    return HePhy::GetCcaThreshold(ppdu, channelType);
}

const std::map<MHz_u, WifiChannelListType>&
EhtPhy::GetCcaSecondaryChannels() const
{
    return ehtSecondaryChannels;
}

PhyEntity::CcaIndication
EhtPhy::GetCcaIndicationOnSecondary(const Ptr<const WifiPpdu> ppdu)
{
    const auto secondaryWidthsToCheck = GetCcaSecondaryWidths(ppdu);

    for (auto secondaryWidth : secondaryWidthsToCheck)
    {
        const auto channelType = ehtSecondaryChannels.at(secondaryWidth);
        const auto ccaThreshold = GetCcaThreshold(ppdu, channelType);
        const auto indices =
            m_wifiPhy->GetOperatingChannel().GetAll20MHzChannelIndicesInSecondary(secondaryWidth);
        for (auto index : indices)
        {
            const auto band = m_wifiPhy->GetBand(20, index);
            if (const auto delayUntilCcaEnd = GetDelayUntilCcaEnd(ccaThreshold, band);
                delayUntilCcaEnd.IsStrictlyPositive())
            {
                return std::make_pair(delayUntilCcaEnd, channelType);
            }
        }
    }

    return std::nullopt;
}

std::vector<Time>
EhtPhy::GetPer20MHzDurations(const Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this);

    /**
     * 36.3.21.6.4 Per 20 MHz CCA sensitivity:
     * If the operating channel width is greater than 20 MHz and the PHY issues a PHY-CCA.indication
     * primitive, the PHY shall set the per20bitmap to indicate the busy/idle status of each 20 MHz
     * subchannel.
     */
    if (m_wifiPhy->GetChannelWidth() < 40)
    {
        return {};
    }

    std::vector<Time> per20MhzDurations{};
    const auto indices = m_wifiPhy->GetOperatingChannel().GetAll20MHzChannelIndicesInPrimary(
        m_wifiPhy->GetChannelWidth());
    for (auto index : indices)
    {
        auto band = m_wifiPhy->GetBand(20, index);
        /**
         * A signal is present on the 20 MHz subchannel at or above a threshold of –62 dBm at the
         * receiver's antenna(s). The PHY shall indicate that the 20 MHz subchannel is busy a period
         * aCCATime after the signal starts and shall continue to indicate the 20 MHz subchannel is
         * busy while the threshold continues to be exceeded.
         */
        dBm_u ccaThreshold = -62;
        auto delayUntilCcaEnd = GetDelayUntilCcaEnd(ccaThreshold, band);

        if (ppdu)
        {
            const MHz_u subchannelMinFreq =
                m_wifiPhy->GetFrequency() - (m_wifiPhy->GetChannelWidth() / 2) + (index * 20);
            const MHz_u subchannelMaxFreq = subchannelMinFreq + 20;
            const auto ppduBw = ppdu->GetTxVector().GetChannelWidth();

            if ((ppduBw <= m_wifiPhy->GetChannelWidth()) &&
                ppdu->DoesOverlapChannel(subchannelMinFreq, subchannelMaxFreq))
            {
                ccaThreshold = Per20MHzCcaThreshold(ppdu);
                const auto ppduCcaDuration = GetDelayUntilCcaEnd(ccaThreshold, band);
                delayUntilCcaEnd = std::max(delayUntilCcaEnd, ppduCcaDuration);
            }
        }
        per20MhzDurations.push_back(delayUntilCcaEnd);
    }

    return per20MhzDurations;
}

} // namespace ns3

namespace
{

/**
 * Constructor class for EHT modes
 */
class ConstructorEht
{
  public:
    ConstructorEht()
    {
        ns3::EhtPhy::InitializeModes();
        ns3::WifiPhy::AddStaticPhyEntity(ns3::WIFI_MOD_CLASS_EHT, std::make_shared<ns3::EhtPhy>());
    }
} g_constructor_eht; ///< the constructor for EHT modes

} // namespace
