/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#include "ofdm-phy.h"

#include "ofdm-ppdu.h"

#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#include <array>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_PHY_NS_LOG_APPEND_CONTEXT(m_wifiPhy)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OfdmPhy");

/*******************************************************
 *       OFDM PHY (IEEE 802.11-2016, clause 17)
 *******************************************************/

// clang-format off

const PhyEntity::PpduFormats OfdmPhy::m_ofdmPpduFormats {
    { WIFI_PREAMBLE_LONG, { WIFI_PPDU_FIELD_PREAMBLE,      // STF + LTF
                            WIFI_PPDU_FIELD_NON_HT_HEADER, // SIG
                            WIFI_PPDU_FIELD_DATA } }
};

const PhyEntity::ModulationLookupTable OfdmPhy::m_ofdmModulationLookupTable {
    // Unique name                Code rate           Constellation size
    { "OfdmRate6Mbps",          { WIFI_CODE_RATE_1_2, 2 } },  // 20 MHz
    { "OfdmRate9Mbps",          { WIFI_CODE_RATE_3_4, 2 } },  //  |
    { "OfdmRate12Mbps",         { WIFI_CODE_RATE_1_2, 4 } },  //  V
    { "OfdmRate18Mbps",         { WIFI_CODE_RATE_3_4, 4 } },
    { "OfdmRate24Mbps",         { WIFI_CODE_RATE_1_2, 16 } },
    { "OfdmRate36Mbps",         { WIFI_CODE_RATE_3_4, 16 } },
    { "OfdmRate48Mbps",         { WIFI_CODE_RATE_2_3, 64 } },
    { "OfdmRate54Mbps",         { WIFI_CODE_RATE_3_4, 64 } },
    { "OfdmRate3MbpsBW10MHz",   { WIFI_CODE_RATE_1_2, 2 } },  // 10 MHz
    { "OfdmRate4_5MbpsBW10MHz", { WIFI_CODE_RATE_3_4, 2 } },  //  |
    { "OfdmRate6MbpsBW10MHz",   { WIFI_CODE_RATE_1_2, 4 } },  //  V
    { "OfdmRate9MbpsBW10MHz",   { WIFI_CODE_RATE_3_4, 4 } },
    { "OfdmRate12MbpsBW10MHz",  { WIFI_CODE_RATE_1_2, 16 } },
    { "OfdmRate18MbpsBW10MHz",  { WIFI_CODE_RATE_3_4, 16 } },
    { "OfdmRate24MbpsBW10MHz",  { WIFI_CODE_RATE_2_3, 64 } },
    { "OfdmRate27MbpsBW10MHz",  { WIFI_CODE_RATE_3_4, 64 } },
    { "OfdmRate1_5MbpsBW5MHz",  { WIFI_CODE_RATE_1_2, 2 } },  //  5 MHz
    { "OfdmRate2_25MbpsBW5MHz", { WIFI_CODE_RATE_3_4, 2 } },  //  |
    { "OfdmRate3MbpsBW5MHz",    { WIFI_CODE_RATE_1_2, 4 } },  //  V
    { "OfdmRate4_5MbpsBW5MHz",  { WIFI_CODE_RATE_3_4, 4 } },
    { "OfdmRate6MbpsBW5MHz",    { WIFI_CODE_RATE_1_2, 16 } },
    { "OfdmRate9MbpsBW5MHz",    { WIFI_CODE_RATE_3_4, 16 } },
    { "OfdmRate12MbpsBW5MHz",   { WIFI_CODE_RATE_2_3, 64 } },
    { "OfdmRate13_5MbpsBW5MHz", { WIFI_CODE_RATE_3_4, 64 } }
};

/// OFDM rates in bits per second for each bandwidth
const std::map<MHz_u, std::array<uint64_t, 8> > s_ofdmRatesBpsList =
   {{ MHz_u{20},
     {  6000000,  9000000, 12000000, 18000000,
       24000000, 36000000, 48000000, 54000000 }},
   { MHz_u{10},
     {  3000000,  4500000,  6000000,  9000000,
       12000000, 18000000, 24000000, 27000000 }},
   { MHz_u{5},
     {  1500000,  2250000,  3000000,  4500000,
        6000000,  9000000, 12000000, 13500000 }},
};

// clang-format on

/**
 * Get the array of possible OFDM rates for each bandwidth.
 *
 * @return the OFDM rates in bits per second
 */
const std::map<MHz_u, std::array<uint64_t, 8>>&
GetOfdmRatesBpsList()
{
    return s_ofdmRatesBpsList;
}

OfdmPhy::OfdmPhy(OfdmPhyVariant variant /* = OFDM_PHY_DEFAULT */, bool buildModeList /* = true */)
{
    NS_LOG_FUNCTION(this << variant << buildModeList);

    if (buildModeList)
    {
        auto bwRatesMap = GetOfdmRatesBpsList();

        switch (variant)
        {
        case OFDM_PHY_DEFAULT:
            for (const auto& rate : bwRatesMap.at(MHz_u{20}))
            {
                WifiMode mode = GetOfdmRate(rate, MHz_u{20});
                NS_LOG_LOGIC("Add " << mode << " to list");
                m_modeList.emplace_back(mode);
            }
            break;
        case OFDM_PHY_10_MHZ:
            for (const auto& rate : bwRatesMap.at(MHz_u{10}))
            {
                WifiMode mode = GetOfdmRate(rate, MHz_u{10});
                NS_LOG_LOGIC("Add " << mode << " to list");
                m_modeList.emplace_back(mode);
            }
            break;
        case OFDM_PHY_5_MHZ:
            for (const auto& rate : bwRatesMap.at(MHz_u{5}))
            {
                WifiMode mode = GetOfdmRate(rate, MHz_u{5});
                NS_LOG_LOGIC("Add " << mode << " to list");
                m_modeList.emplace_back(mode);
            }
            break;
        default:
            NS_ABORT_MSG("Unsupported 11a OFDM variant");
        }
    }
}

OfdmPhy::~OfdmPhy()
{
    NS_LOG_FUNCTION(this);
}

WifiMode
OfdmPhy::GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_PREAMBLE: // consider header mode for preamble (useful for
                                   // InterferenceHelper)
    case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetHeaderMode(txVector);
    default:
        return PhyEntity::GetSigMode(field, txVector);
    }
}

WifiMode
OfdmPhy::GetHeaderMode(const WifiTxVector& txVector) const
{
    switch (static_cast<uint16_t>(txVector.GetChannelWidth()))
    {
    case 5:
        return GetOfdmRate1_5MbpsBW5MHz();
    case 10:
        return GetOfdmRate3MbpsBW10MHz();
    case 20:
    default:
        // Section 17.3.2 "PPDU frame format"; IEEE Std 802.11-2016.
        // Actually this is only the first part of the PhyHeader,
        // because the last 16 bits of the PhyHeader are using the
        // same mode of the payload
        return GetOfdmRate6Mbps();
    }
}

const PhyEntity::PpduFormats&
OfdmPhy::GetPpduFormats() const
{
    return m_ofdmPpduFormats;
}

Time
OfdmPhy::GetDuration(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_PREAMBLE:
        return GetPreambleDuration(txVector); // L-STF + L-LTF
    case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return GetHeaderDuration(txVector); // L-SIG
    default:
        return PhyEntity::GetDuration(field, txVector);
    }
}

Time
OfdmPhy::GetPreambleDuration(const WifiTxVector& txVector) const
{
    switch (static_cast<uint16_t>(txVector.GetChannelWidth()))
    {
    case 20:
    default:
        // Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        // also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent
        // parameters"; IEEE Std 802.11-2016
        return MicroSeconds(16);
    case 10:
        // Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        // also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent
        // parameters"; IEEE Std 802.11-2016
        return MicroSeconds(32);
    case 5:
        // Section 17.3.3 "PHY preamble (SYNC)" Figure 17-4 "OFDM training structure"
        // also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent
        // parameters"; IEEE Std 802.11-2016
        return MicroSeconds(64);
    }
}

Time
OfdmPhy::GetHeaderDuration(const WifiTxVector& txVector) const
{
    switch (static_cast<uint16_t>(txVector.GetChannelWidth()))
    {
    case 20:
    default:
        // Section 17.3.3 "PHY preamble (SYNC)" and Figure 17-4 "OFDM training structure"; IEEE Std
        // 802.11-2016 also Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related
        // parameters"; IEEE Std 802.11-2016 We return the duration of the SIGNAL field only, since
        // the SERVICE field (which strictly speaking belongs to the PHY header, see Section 17.3.2
        // and Figure 17-1) is sent using the payload mode.
        return MicroSeconds(4);
    case 10:
        // Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE
        // Std 802.11-2016
        return MicroSeconds(8);
    case 5:
        // Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE
        // Std 802.11-2016
        return MicroSeconds(16);
    }
}

Time
OfdmPhy::GetPayloadDuration(uint32_t size,
                            const WifiTxVector& txVector,
                            WifiPhyBand band,
                            MpduType /* mpdutype */,
                            bool /* incFlag */,
                            uint32_t& /* totalAmpduSize */,
                            double& /* totalAmpduNumSymbols */,
                            uint16_t /* staId */) const
{
    //(Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std
    // 802.11-2016 corresponds to T_{SYM} in the table)
    Time symbolDuration = MicroSeconds(4);

    double numDataBitsPerSymbol =
        txVector.GetMode().GetDataRate(txVector) * symbolDuration.GetNanoSeconds() / 1e9;

    // The number of OFDM symbols in the data field when BCC encoding
    // is used is given in equation 19-32 of the IEEE 802.11-2016 standard.
    double numSymbols = ceil((GetNumberServiceBits() + size * 8.0 + 6.0) / (numDataBitsPerSymbol));

    Time payloadDuration =
        FemtoSeconds(static_cast<uint64_t>(numSymbols * symbolDuration.GetFemtoSeconds()));
    payloadDuration += GetSignalExtension(band);
    return payloadDuration;
}

uint8_t
OfdmPhy::GetNumberServiceBits() const
{
    return 16;
}

Time
OfdmPhy::GetSignalExtension(WifiPhyBand band) const
{
    return (band == WIFI_PHY_BAND_2_4GHZ) ? MicroSeconds(6) : MicroSeconds(0);
}

Ptr<WifiPpdu>
OfdmPhy::BuildPpdu(const WifiConstPsduMap& psdus,
                   const WifiTxVector& txVector,
                   Time /* ppduDuration */)
{
    NS_LOG_FUNCTION(this << psdus << txVector);
    return Create<OfdmPpdu>(
        psdus.begin()->second,
        txVector,
        m_wifiPhy->GetOperatingChannel(),
        m_wifiPhy->GetLatestPhyEntity()->ObtainNextUid(
            txVector)); // use latest PHY entity to handle MU-RTS sent with non-HT rate
}

PhyEntity::PhyFieldRxStatus
OfdmPhy::DoEndReceiveField(WifiPpduField field, Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << field << *event);
    if (field == WIFI_PPDU_FIELD_NON_HT_HEADER)
    {
        return EndReceiveHeader(event); // L-SIG
    }
    return PhyEntity::DoEndReceiveField(field, event);
}

PhyEntity::PhyFieldRxStatus
OfdmPhy::EndReceiveHeader(Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << *event);
    SnrPer snrPer = GetPhyHeaderSnrPer(WIFI_PPDU_FIELD_NON_HT_HEADER, event);
    NS_LOG_DEBUG("L-SIG: SNR(dB)=" << RatioToDb(snrPer.snr) << ", PER=" << snrPer.per);
    PhyFieldRxStatus status(GetRandomValue() > snrPer.per);
    if (status.isSuccess)
    {
        NS_LOG_DEBUG("Received non-HT PHY header");
        if (!IsAllConfigSupported(WIFI_PPDU_FIELD_NON_HT_HEADER, event->GetPpdu()))
        {
            status = PhyFieldRxStatus(false, UNSUPPORTED_SETTINGS, DROP);
        }
    }
    else
    {
        NS_LOG_DEBUG("Abort reception because non-HT PHY header reception failed");
        status.reason = L_SIG_FAILURE;
        status.actionIfFailure = ABORT;
    }
    return status;
}

bool
OfdmPhy::IsChannelWidthSupported(Ptr<const WifiPpdu> ppdu) const
{
    const auto channelWidth = ppdu->GetTxVector().GetChannelWidth();
    if ((channelWidth >= MHz_u{40}) && (channelWidth > m_wifiPhy->GetChannelWidth()))
    {
        NS_LOG_DEBUG("Packet reception could not be started because not enough channel width ("
                     << channelWidth << " vs " << m_wifiPhy->GetChannelWidth() << ")");
        return false;
    }
    return true;
}

bool
OfdmPhy::IsAllConfigSupported(WifiPpduField /* field */, Ptr<const WifiPpdu> ppdu) const
{
    if (!IsChannelWidthSupported(ppdu))
    {
        return false;
    }
    return IsConfigSupported(ppdu);
}

Ptr<SpectrumValue>
OfdmPhy::GetTxPowerSpectralDensity(Watt_u txPower, Ptr<const WifiPpdu> ppdu) const
{
    const auto& centerFrequencies = ppdu->GetTxCenterFreqs();
    const auto& txVector = ppdu->GetTxVector();
    const auto channelWidth = txVector.GetChannelWidth();
    NS_LOG_FUNCTION(this << centerFrequencies.front() << channelWidth << txPower);
    const auto& txMaskRejectionParams = GetTxMaskRejectionParams();
    Ptr<SpectrumValue> v;
    if (txVector.IsNonHtDuplicate())
    {
        v = WifiSpectrumValueHelper::CreateDuplicated20MhzTxPowerSpectralDensity(
            centerFrequencies,
            channelWidth,
            txPower,
            GetGuardBandwidth(channelWidth),
            std::get<0>(txMaskRejectionParams),
            std::get<1>(txMaskRejectionParams),
            std::get<2>(txMaskRejectionParams));
    }
    else
    {
        NS_ASSERT(centerFrequencies.size() == 1);
        v = WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(
            centerFrequencies.front(),
            channelWidth,
            txPower,
            GetGuardBandwidth(channelWidth),
            std::get<0>(txMaskRejectionParams),
            std::get<1>(txMaskRejectionParams),
            std::get<2>(txMaskRejectionParams));
    }
    return v;
}

void
OfdmPhy::InitializeModes()
{
    for (const auto& ratesPerBw : GetOfdmRatesBpsList())
    {
        for (const auto& rate : ratesPerBw.second)
        {
            GetOfdmRate(rate, ratesPerBw.first);
        }
    }
}

WifiMode
OfdmPhy::GetOfdmRate(uint64_t rate, MHz_u bw)
{
    switch (static_cast<uint16_t>(bw))
    {
    case 20:
        switch (rate)
        {
        case 6000000:
            return GetOfdmRate6Mbps();
        case 9000000:
            return GetOfdmRate9Mbps();
        case 12000000:
            return GetOfdmRate12Mbps();
        case 18000000:
            return GetOfdmRate18Mbps();
        case 24000000:
            return GetOfdmRate24Mbps();
        case 36000000:
            return GetOfdmRate36Mbps();
        case 48000000:
            return GetOfdmRate48Mbps();
        case 54000000:
            return GetOfdmRate54Mbps();
        default:
            NS_ABORT_MSG("Inexistent rate (" << rate << " bps) requested for 11a OFDM (default)");
            return WifiMode();
        }
        break;
    case 10:
        switch (rate)
        {
        case 3000000:
            return GetOfdmRate3MbpsBW10MHz();
        case 4500000:
            return GetOfdmRate4_5MbpsBW10MHz();
        case 6000000:
            return GetOfdmRate6MbpsBW10MHz();
        case 9000000:
            return GetOfdmRate9MbpsBW10MHz();
        case 12000000:
            return GetOfdmRate12MbpsBW10MHz();
        case 18000000:
            return GetOfdmRate18MbpsBW10MHz();
        case 24000000:
            return GetOfdmRate24MbpsBW10MHz();
        case 27000000:
            return GetOfdmRate27MbpsBW10MHz();
        default:
            NS_ABORT_MSG("Inexistent rate (" << rate << " bps) requested for 11a OFDM (10 MHz)");
            return WifiMode();
        }
        break;
    case 5:
        switch (rate)
        {
        case 1500000:
            return GetOfdmRate1_5MbpsBW5MHz();
        case 2250000:
            return GetOfdmRate2_25MbpsBW5MHz();
        case 3000000:
            return GetOfdmRate3MbpsBW5MHz();
        case 4500000:
            return GetOfdmRate4_5MbpsBW5MHz();
        case 6000000:
            return GetOfdmRate6MbpsBW5MHz();
        case 9000000:
            return GetOfdmRate9MbpsBW5MHz();
        case 12000000:
            return GetOfdmRate12MbpsBW5MHz();
        case 13500000:
            return GetOfdmRate13_5MbpsBW5MHz();
        default:
            NS_ABORT_MSG("Inexistent rate (" << rate << " bps) requested for 11a OFDM (5 MHz)");
            return WifiMode();
        }
        break;
    default:
        NS_ABORT_MSG("Inexistent bandwidth (" << +bw << " MHz) requested for 11a OFDM");
        return WifiMode();
    }
}

#define GET_OFDM_MODE(x, f)                                                                        \
    WifiMode OfdmPhy::Get##x()                                                                     \
    {                                                                                              \
        static WifiMode mode = CreateOfdmMode(#x, f);                                              \
        return mode;                                                                               \
    }

// 20 MHz channel rates (default)
GET_OFDM_MODE(OfdmRate6Mbps, true)
GET_OFDM_MODE(OfdmRate9Mbps, false)
GET_OFDM_MODE(OfdmRate12Mbps, true)
GET_OFDM_MODE(OfdmRate18Mbps, false)
GET_OFDM_MODE(OfdmRate24Mbps, true)
GET_OFDM_MODE(OfdmRate36Mbps, false)
GET_OFDM_MODE(OfdmRate48Mbps, false)
GET_OFDM_MODE(OfdmRate54Mbps, false)
// 10 MHz channel rates
GET_OFDM_MODE(OfdmRate3MbpsBW10MHz, true)
GET_OFDM_MODE(OfdmRate4_5MbpsBW10MHz, false)
GET_OFDM_MODE(OfdmRate6MbpsBW10MHz, true)
GET_OFDM_MODE(OfdmRate9MbpsBW10MHz, false)
GET_OFDM_MODE(OfdmRate12MbpsBW10MHz, true)
GET_OFDM_MODE(OfdmRate18MbpsBW10MHz, false)
GET_OFDM_MODE(OfdmRate24MbpsBW10MHz, false)
GET_OFDM_MODE(OfdmRate27MbpsBW10MHz, false)
// 5 MHz channel rates
GET_OFDM_MODE(OfdmRate1_5MbpsBW5MHz, true)
GET_OFDM_MODE(OfdmRate2_25MbpsBW5MHz, false)
GET_OFDM_MODE(OfdmRate3MbpsBW5MHz, true)
GET_OFDM_MODE(OfdmRate4_5MbpsBW5MHz, false)
GET_OFDM_MODE(OfdmRate6MbpsBW5MHz, true)
GET_OFDM_MODE(OfdmRate9MbpsBW5MHz, false)
GET_OFDM_MODE(OfdmRate12MbpsBW5MHz, false)
GET_OFDM_MODE(OfdmRate13_5MbpsBW5MHz, false)
#undef GET_OFDM_MODE

WifiMode
OfdmPhy::CreateOfdmMode(std::string uniqueName, bool isMandatory)
{
    // Check whether uniqueName is in lookup table
    const auto it = m_ofdmModulationLookupTable.find(uniqueName);
    NS_ASSERT_MSG(it != m_ofdmModulationLookupTable.end(),
                  "OFDM mode cannot be created because it is not in the lookup table!");

    return WifiModeFactory::CreateWifiMode(uniqueName,
                                           WIFI_MOD_CLASS_OFDM,
                                           isMandatory,
                                           MakeBoundCallback(&GetCodeRate, uniqueName),
                                           MakeBoundCallback(&GetConstellationSize, uniqueName),
                                           MakeCallback(&GetPhyRateFromTxVector),
                                           MakeCallback(&GetDataRateFromTxVector),
                                           MakeCallback(&IsAllowed));
}

WifiCodeRate
OfdmPhy::GetCodeRate(const std::string& name)
{
    return m_ofdmModulationLookupTable.at(name).first;
}

uint16_t
OfdmPhy::GetConstellationSize(const std::string& name)
{
    return m_ofdmModulationLookupTable.at(name).second;
}

uint64_t
OfdmPhy::GetPhyRate(const std::string& name, MHz_u channelWidth)
{
    WifiCodeRate codeRate = GetCodeRate(name);
    uint64_t dataRate = GetDataRate(name, channelWidth);
    return CalculatePhyRate(codeRate, dataRate);
}

uint64_t
OfdmPhy::CalculatePhyRate(WifiCodeRate codeRate, uint64_t dataRate)
{
    return (dataRate / GetCodeRatio(codeRate));
}

uint64_t
OfdmPhy::GetPhyRateFromTxVector(const WifiTxVector& txVector, uint16_t /* staId */)
{
    return GetPhyRate(txVector.GetMode().GetUniqueName(), txVector.GetChannelWidth());
}

double
OfdmPhy::GetCodeRatio(WifiCodeRate codeRate)
{
    switch (codeRate)
    {
    case WIFI_CODE_RATE_3_4:
        return (3.0 / 4.0);
    case WIFI_CODE_RATE_2_3:
        return (2.0 / 3.0);
    case WIFI_CODE_RATE_1_2:
        return (1.0 / 2.0);
    case WIFI_CODE_RATE_UNDEFINED:
    default:
        NS_FATAL_ERROR("trying to get code ratio for undefined coding rate");
        return 0;
    }
}

uint64_t
OfdmPhy::GetDataRateFromTxVector(const WifiTxVector& txVector, uint16_t /* staId */)
{
    return GetDataRate(txVector.GetMode().GetUniqueName(), txVector.GetChannelWidth());
}

uint64_t
OfdmPhy::GetDataRate(const std::string& name, MHz_u channelWidth)
{
    WifiCodeRate codeRate = GetCodeRate(name);
    uint16_t constellationSize = GetConstellationSize(name);
    return CalculateDataRate(codeRate, constellationSize, channelWidth);
}

uint64_t
OfdmPhy::CalculateDataRate(WifiCodeRate codeRate, uint16_t constellationSize, MHz_u channelWidth)
{
    return CalculateDataRate(GetSymbolDuration(channelWidth),
                             GetUsableSubcarriers(),
                             static_cast<uint16_t>(log2(constellationSize)),
                             GetCodeRatio(codeRate));
}

uint64_t
OfdmPhy::CalculateDataRate(Time symbolDuration,
                           uint16_t usableSubCarriers,
                           uint16_t numberOfBitsPerSubcarrier,
                           double codingRate)
{
    double symbolRate = (1e9 / static_cast<double>(symbolDuration.GetNanoSeconds()));
    return ceil(symbolRate * usableSubCarriers * numberOfBitsPerSubcarrier * codingRate);
}

uint16_t
OfdmPhy::GetUsableSubcarriers()
{
    return 48;
}

Time
OfdmPhy::GetSymbolDuration(MHz_u channelWidth)
{
    Time symbolDuration = MicroSeconds(4);
    uint8_t bwFactor = 1;
    if (channelWidth == MHz_u{10})
    {
        bwFactor = 2;
    }
    else if (channelWidth == MHz_u{5})
    {
        bwFactor = 4;
    }
    return bwFactor * symbolDuration;
}

bool
OfdmPhy::IsAllowed(const WifiTxVector& /*txVector*/)
{
    return true;
}

uint32_t
OfdmPhy::GetMaxPsduSize() const
{
    return 4095;
}

MHz_u
OfdmPhy::GetMeasurementChannelWidth(const Ptr<const WifiPpdu> ppdu) const
{
    if (!ppdu)
    {
        return std::min(m_wifiPhy->GetChannelWidth(), MHz_u{20});
    }
    return GetRxChannelWidth(ppdu->GetTxVector());
}

dBm_u
OfdmPhy::GetCcaThreshold(const Ptr<const WifiPpdu> ppdu, WifiChannelListType channelType) const
{
    if (ppdu && ppdu->GetTxVector().GetChannelWidth() < MHz_u{20})
    {
        // scale CCA sensitivity threshold for BW of 5 and 10 MHz
        const auto bw = GetRxChannelWidth(ppdu->GetTxVector());
        const auto thresholdW = DbmToW(m_wifiPhy->GetCcaSensitivityThreshold()) * (bw / MHz_u{20});
        return WToDbm(thresholdW);
    }
    return PhyEntity::GetCcaThreshold(ppdu, channelType);
}

Ptr<const WifiPpdu>
OfdmPhy::GetRxPpduFromTxPpdu(Ptr<const WifiPpdu> ppdu)
{
    const auto txWidth = ppdu->GetTxChannelWidth();
    const auto& txVector = ppdu->GetTxVector();
    // Update channel width in TXVECTOR for non-HT duplicate PPDUs.
    if (txVector.IsNonHtDuplicate() && txWidth > m_wifiPhy->GetChannelWidth())
    {
        // We also do a copy of the PPDU for non-HT duplicate PPDUs since other
        // PHYs might set a different channel width in the reconstructed TXVECTOR.
        auto rxPpdu = ppdu->Copy();
        auto updatedTxVector = txVector;
        updatedTxVector.SetChannelWidth(std::min(txWidth, m_wifiPhy->GetChannelWidth()));
        rxPpdu->UpdateTxVector(updatedTxVector);
        return rxPpdu;
    }
    return PhyEntity::GetRxPpduFromTxPpdu(ppdu);
}

} // namespace ns3

namespace
{

/**
 * Constructor class for OFDM modes
 */
class ConstructorOfdm
{
  public:
    ConstructorOfdm()
    {
        ns3::OfdmPhy::InitializeModes();
        ns3::WifiPhy::AddStaticPhyEntity(ns3::WIFI_MOD_CLASS_OFDM,
                                         ns3::Create<ns3::OfdmPhy>()); // default variant will do
    }
} g_constructor_ofdm; ///< the constructor for OFDM modes

} // namespace
