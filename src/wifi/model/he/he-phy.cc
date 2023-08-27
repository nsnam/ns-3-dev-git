/*
 * Copyright (c) 2020 Orange Labs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy and
 * spectrum-wifi-phy)
 */

#include "he-phy.h"

#include "he-configuration.h"
#include "obss-pd-algorithm.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HePhy");

/*******************************************************
 *       HE PHY (P802.11ax/D4.0, clause 27)
 *******************************************************/

// clang-format off

const PhyEntity::PpduFormats HePhy::m_hePpduFormats { // Ignoring PE (Packet Extension)
    { WIFI_PREAMBLE_HE_SU,    { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                                WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                                WIFI_PPDU_FIELD_SIG_A,         // HE-SIG-A
                                WIFI_PPDU_FIELD_TRAINING,      // HE-STF + HE-LTFs
                                WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_HE_MU,    { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                                WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                                WIFI_PPDU_FIELD_SIG_A,         // HE-SIG-A
                                WIFI_PPDU_FIELD_SIG_B,         // HE-SIG-B
                                WIFI_PPDU_FIELD_TRAINING,      // HE-STF + HE-LTFs
                                WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_HE_TB,    { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                                WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                                WIFI_PPDU_FIELD_SIG_A,         // HE-SIG-A
                                WIFI_PPDU_FIELD_TRAINING,      // HE-STF + HE-LTFs
                                WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_HE_ER_SU, { WIFI_PPDU_FIELD_PREAMBLE,      // L-STF + L-LTF
                                WIFI_PPDU_FIELD_NON_HT_HEADER, // L-SIG + RL-SIG
                                WIFI_PPDU_FIELD_SIG_A,         // HE-SIG-A
                                WIFI_PPDU_FIELD_TRAINING,      // HE-STF + HE-LTFs
                                WIFI_PPDU_FIELD_DATA } }
};

// clang-format on

HePhy::HePhy(bool buildModeList /* = true */)
    : VhtPhy(false), // don't add VHT modes to list
      m_trigVector(std::nullopt),
      m_trigVectorExpirationTime(std::nullopt),
      m_currentTxVector(std::nullopt),
      m_rxHeTbPpdus(0),
      m_lastPer20MHzDurations()
{
    NS_LOG_FUNCTION(this << buildModeList);
    m_bssMembershipSelector = HE_PHY;
    m_maxMcsIndexPerSs = 11;
    m_maxSupportedMcsIndexPerSs = m_maxMcsIndexPerSs;
    m_currentMuPpduUid = UINT64_MAX;
    m_previouslyTxPpduUid = UINT64_MAX;
    if (buildModeList)
    {
        BuildModeList();
    }
}

HePhy::~HePhy()
{
    NS_LOG_FUNCTION(this);
}

void
HePhy::BuildModeList()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_modeList.empty());
    NS_ASSERT(m_bssMembershipSelector == HE_PHY);
    for (uint8_t index = 0; index <= m_maxSupportedMcsIndexPerSs; ++index)
    {
        NS_LOG_LOGIC("Add HeMcs" << +index << " to list");
        m_modeList.emplace_back(CreateHeMcs(index));
    }
}

WifiMode
HePhy::GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_TRAINING: // consider SIG-A (SIG-B) mode for training for the time being
                                   // for SU/ER-SU/TB (MU) (useful for InterferenceHelper)
        if (txVector.IsDlMu())
        {
            NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
            // Training comes after SIG-B
            return GetSigBMode(txVector);
        }
        else
        {
            // Training comes after SIG-A
            return GetSigAMode();
        }
    default:
        return VhtPhy::GetSigMode(field, txVector);
    }
}

WifiMode
HePhy::GetSigAMode() const
{
    return GetVhtMcs0(); // same number of data tones as VHT for 20 MHz (i.e. 52)
}

WifiMode
HePhy::GetSigBMode(const WifiTxVector& txVector) const
{
    NS_ABORT_MSG_IF(!IsDlMu(txVector.GetPreambleType()), "SIG-B only available for DL MU");
    /**
     * Get smallest HE MCS index among station's allocations and use the
     * VHT version of the index. This enables to have 800 ns GI, 52 data
     * tones, and 312.5 kHz spacing while ensuring that MCS will be decoded
     * by all stations.
     */
    uint8_t smallestMcs = 5; // maximum MCS for HE-SIG-B
    for (auto& info : txVector.GetHeMuUserInfoMap())
    {
        smallestMcs = std::min(smallestMcs, info.second.mcs);
    }
    switch (smallestMcs)
    {
    case 0:
        return GetVhtMcs0();
    case 1:
        return GetVhtMcs1();
    case 2:
        return GetVhtMcs2();
    case 3:
        return GetVhtMcs3();
    case 4:
        return GetVhtMcs4();
    case 5:
    default:
        return GetVhtMcs5();
    }
}

const PhyEntity::PpduFormats&
HePhy::GetPpduFormats() const
{
    return m_hePpduFormats;
}

Time
HePhy::GetLSigDuration(WifiPreamble /* preamble */) const
{
    return MicroSeconds(8); // L-SIG + RL-SIG
}

Time
HePhy::GetTrainingDuration(const WifiTxVector& txVector,
                           uint8_t nDataLtf,
                           uint8_t nExtensionLtf /* = 0 */) const
{
    Time ltfDuration = MicroSeconds(8); // TODO extract from TxVector when available
    Time stfDuration;
    if (txVector.IsUlMu())
    {
        NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
        stfDuration = MicroSeconds(8);
    }
    else
    {
        stfDuration = MicroSeconds(4);
    }
    NS_ABORT_MSG_IF(nDataLtf > 8, "Unsupported number of LTFs " << +nDataLtf << " for HE");
    NS_ABORT_MSG_IF(nExtensionLtf > 0, "No extension LTFs expected for HE");
    return stfDuration + ltfDuration * nDataLtf; // HE-STF + HE-LTFs
}

Time
HePhy::GetSigADuration(WifiPreamble preamble) const
{
    return (preamble == WIFI_PREAMBLE_HE_ER_SU)
               ? MicroSeconds(16)
               : MicroSeconds(8); // HE-SIG-A (first and second symbol)
}

uint32_t
HePhy::GetSigBSize(const WifiTxVector& txVector) const
{
    if (ns3::IsDlMu(txVector.GetPreambleType()))
    {
        NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
        return HePpdu::GetSigBFieldSize(
            txVector.GetChannelWidth(),
            txVector.GetRuAllocation(
                m_wifiPhy ? m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20) : 0),
            txVector.IsSigBCompression(),
            txVector.IsSigBCompression() ? txVector.GetHeMuUserInfoMap().size() : 0);
    }
    return 0;
}

Time
HePhy::GetSigBDuration(const WifiTxVector& txVector) const
{
    if (auto sigBSize = GetSigBSize(txVector); sigBSize > 0)
    {
        auto symbolDuration = MicroSeconds(4);
        // Number of data bits per symbol
        auto ndbps =
            GetSigBMode(txVector).GetDataRate(20, 800, 1) * symbolDuration.GetNanoSeconds() / 1e9;
        auto numSymbols = ceil((sigBSize) / ndbps);

        return FemtoSeconds(static_cast<uint64_t>(numSymbols * symbolDuration.GetFemtoSeconds()));
    }
    else
    {
        // no SIG-B
        return MicroSeconds(0);
    }
}

Time
HePhy::GetValidPpduDuration(Time ppduDuration, const WifiTxVector& txVector, WifiPhyBand band)
{
    Time tSymbol = NanoSeconds(12800 + txVector.GetGuardInterval());
    Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
    uint8_t sigExtension = (band == WIFI_PHY_BAND_2_4GHZ ? 6 : 0);
    uint32_t nSymbols =
        floor(static_cast<double>((ppduDuration - preambleDuration).GetNanoSeconds() -
                                  (sigExtension * 1000)) /
              tSymbol.GetNanoSeconds());
    return preambleDuration + (nSymbols * tSymbol) + MicroSeconds(sigExtension);
}

std::pair<uint16_t, Time>
HePhy::ConvertHeTbPpduDurationToLSigLength(Time ppduDuration,
                                           const WifiTxVector& txVector,
                                           WifiPhyBand band)
{
    NS_ABORT_IF(!txVector.IsUlMu() || (txVector.GetModulationClass() < WIFI_MOD_CLASS_HE));
    // update ppduDuration so that it is a valid PPDU duration
    ppduDuration = GetValidPpduDuration(ppduDuration, txVector, band);
    uint8_t sigExtension = (band == WIFI_PHY_BAND_2_4GHZ ? 6 : 0);
    uint8_t m = 2; // HE TB PPDU so m is set to 2
    uint16_t length = ((ceil((static_cast<double>(ppduDuration.GetNanoSeconds() - (20 * 1000) -
                                                  (sigExtension * 1000)) /
                              1000) /
                             4.0) *
                        3) -
                       3 - m);
    return {length, ppduDuration};
}

Time
HePhy::ConvertLSigLengthToHeTbPpduDuration(uint16_t length,
                                           const WifiTxVector& txVector,
                                           WifiPhyBand band)
{
    NS_ABORT_IF(!txVector.IsUlMu() || (txVector.GetModulationClass() < WIFI_MOD_CLASS_HE));
    uint8_t sigExtension = (band == WIFI_PHY_BAND_2_4GHZ ? 6 : 0);
    uint8_t m = 2; // HE TB PPDU so m is set to 2
    // Equation 27-11 of IEEE P802.11ax/D4.0
    Time calculatedDuration =
        MicroSeconds(((ceil(static_cast<double>(length + 3 + m) / 3)) * 4) + 20 + sigExtension);
    return GetValidPpduDuration(calculatedDuration, txVector, band);
}

Time
HePhy::CalculateNonHeDurationForHeTb(const WifiTxVector& txVector) const
{
    Time duration = GetDuration(WIFI_PPDU_FIELD_PREAMBLE, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_NON_HT_HEADER, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_SIG_A, txVector);
    return duration;
}

Time
HePhy::CalculateNonHeDurationForHeMu(const WifiTxVector& txVector) const
{
    Time duration = GetDuration(WIFI_PPDU_FIELD_PREAMBLE, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_NON_HT_HEADER, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_SIG_A, txVector) +
                    GetDuration(WIFI_PPDU_FIELD_SIG_B, txVector);
    return duration;
}

uint8_t
HePhy::GetNumberBccEncoders(const WifiTxVector& /* txVector */) const
{
    return 1; // only 1 BCC encoder for HE since higher rates are obtained using LDPC
}

Time
HePhy::GetSymbolDuration(const WifiTxVector& txVector) const
{
    uint16_t gi = txVector.GetGuardInterval();
    NS_ASSERT(gi == 800 || gi == 1600 || gi == 3200);
    return GetSymbolDuration(NanoSeconds(gi));
}

void
HePhy::SetTrigVector(const WifiTxVector& trigVector, Time validity)
{
    NS_LOG_FUNCTION(this << trigVector << validity);
    NS_ASSERT_MSG(trigVector.GetGuardInterval() > 800,
                  "Invalid guard interval " << trigVector.GetGuardInterval());
    if (auto mac = m_wifiPhy->GetDevice()->GetMac(); mac && mac->GetTypeOfStation() != AP)
    {
        return;
    }
    m_trigVector = trigVector;
    m_trigVectorExpirationTime = Simulator::Now() + validity;
    NS_LOG_FUNCTION(this << m_trigVector.value() << m_trigVectorExpirationTime->As(Time::US));
}

Ptr<WifiPpdu>
HePhy::BuildPpdu(const WifiConstPsduMap& psdus, const WifiTxVector& txVector, Time ppduDuration)
{
    NS_LOG_FUNCTION(this << psdus << txVector << ppduDuration);
    return Create<HePpdu>(psdus,
                          txVector,
                          m_wifiPhy->GetOperatingChannel(),
                          ppduDuration,
                          ObtainNextUid(txVector),
                          HePpdu::PSD_NON_HE_PORTION);
}

void
HePhy::StartReceivePreamble(Ptr<const WifiPpdu> ppdu,
                            RxPowerWattPerChannelBand& rxPowersW,
                            Time rxDuration)
{
    NS_LOG_FUNCTION(this << ppdu << rxDuration);
    const auto& txVector = ppdu->GetTxVector();
    auto hePpdu = DynamicCast<const HePpdu>(ppdu);
    NS_ASSERT(hePpdu);
    const auto psdFlag = hePpdu->GetTxPsdFlag();
    if (psdFlag == HePpdu::PSD_HE_PORTION)
    {
        NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
        if (m_currentMuPpduUid == ppdu->GetUid() && GetCurrentEvent())
        {
            // AP or STA has already received non-HE portion, switch to HE portion, and schedule
            // reception of payload (will be canceled for STAs by StartPayload)
            bool hePortionStarted = !m_beginMuPayloadRxEvents.empty();
            NS_LOG_INFO("Switch to HE portion (already started? "
                        << (hePortionStarted ? "Y" : "N") << ") "
                        << "and schedule payload reception in "
                        << GetDuration(WIFI_PPDU_FIELD_TRAINING, txVector).As(Time::NS));
            auto event = CreateInterferenceEvent(ppdu, rxDuration, rxPowersW, !hePortionStarted);
            uint16_t staId = GetStaId(ppdu);
            NS_ASSERT(m_beginMuPayloadRxEvents.find(staId) == m_beginMuPayloadRxEvents.end());
            m_beginMuPayloadRxEvents[staId] =
                Simulator::Schedule(GetDuration(WIFI_PPDU_FIELD_TRAINING, txVector),
                                    &HePhy::StartReceiveMuPayload,
                                    this,
                                    event);
        }
        else
        {
            // PHY receives the HE portion while having dropped the preamble
            NS_LOG_INFO("Consider HE portion of the PPDU as interference since device dropped the "
                        "preamble");
            CreateInterferenceEvent(ppdu, rxDuration, rxPowersW);
            // the HE portion of the PPDU will be noise _after_ the completion of the current event
            ErasePreambleEvent(ppdu, rxDuration);
        }
    }
    else
    {
        VhtPhy::StartReceivePreamble(
            ppdu,
            rxPowersW,
            ppdu->GetTxDuration()); // The actual duration of the PPDU should be used
    }
}

void
HePhy::CancelAllEvents()
{
    NS_LOG_FUNCTION(this);
    for (auto& beginMuPayloadRxEvent : m_beginMuPayloadRxEvents)
    {
        beginMuPayloadRxEvent.second.Cancel();
    }
    m_beginMuPayloadRxEvents.clear();
    VhtPhy::CancelAllEvents();
}

void
HePhy::DoAbortCurrentReception(WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << reason);
    if (reason != OBSS_PD_CCA_RESET)
    {
        for (auto& endMpduEvent : m_endOfMpduEvents)
        {
            endMpduEvent.Cancel();
        }
        m_endOfMpduEvents.clear();
    }
    else
    {
        VhtPhy::DoAbortCurrentReception(reason);
    }
}

void
HePhy::DoResetReceive(Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << *event);
    if (event->GetPpdu()->GetType() != WIFI_PPDU_TYPE_UL_MU)
    {
        NS_ASSERT(event->GetEndTime() == Simulator::Now());
    }
    for (auto& beginMuPayloadRxEvent : m_beginMuPayloadRxEvents)
    {
        beginMuPayloadRxEvent.second.Cancel();
    }
    m_beginMuPayloadRxEvents.clear();
}

Ptr<Event>
HePhy::DoGetEvent(Ptr<const WifiPpdu> ppdu, RxPowerWattPerChannelBand& rxPowersW)
{
    Ptr<Event> event;
    // We store all incoming preamble events, and a decision is made at the end of the preamble
    // detection window. If a preamble is received after the preamble detection window, it is stored
    // anyway because this is needed for HE TB PPDUs in order to properly update the received power
    // in InterferenceHelper. The map is cleaned anyway at the end of the current reception.
    const auto uidPreamblePair = std::make_pair(ppdu->GetUid(), ppdu->GetPreamble());
    const auto& currentPreambleEvents = GetCurrentPreambleEvents();
    const auto it = currentPreambleEvents.find(uidPreamblePair);
    if (const auto isResponseToTrigger = (m_previouslyTxPpduUid == ppdu->GetUid());
        ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU || isResponseToTrigger)
    {
        const auto& txVector = ppdu->GetTxVector();
        const auto rxDuration =
            (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
                ? CalculateNonHeDurationForHeTb(
                      txVector) // the HE portion of the transmission will be added later on
                : ppdu->GetTxDuration();
        if (it != currentPreambleEvents.cend())
        {
            if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
            {
                NS_LOG_DEBUG("Received another HE TB PPDU for UID "
                             << ppdu->GetUid() << " from STA-ID " << ppdu->GetStaId()
                             << " and BSS color " << +txVector.GetBssColor());
            }
            else
            {
                NS_LOG_DEBUG("Received another response to a trigger frame " << ppdu->GetUid());
            }
            event = it->second;
            HandleRxPpduWithSameContent(event, ppdu, rxPowersW);
            return nullptr;
        }
        else
        {
            if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
            {
                NS_LOG_DEBUG("Received a new HE TB PPDU for UID "
                             << ppdu->GetUid() << " from STA-ID " << ppdu->GetStaId()
                             << " and BSS color " << +txVector.GetBssColor());
            }
            else
            {
                NS_LOG_DEBUG("Received response to a trigger frame for UID " << ppdu->GetUid());
            }
            event = CreateInterferenceEvent(ppdu, rxDuration, rxPowersW);
            AddPreambleEvent(event);
        }
    }
    else if (ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU)
    {
        const auto& txVector = ppdu->GetTxVector();
        Time rxDuration = CalculateNonHeDurationForHeMu(
            txVector); // the HE portion of the transmission will be added later on
        event = CreateInterferenceEvent(ppdu, rxDuration, rxPowersW);
        AddPreambleEvent(event);
    }
    else
    {
        event = VhtPhy::DoGetEvent(ppdu, rxPowersW);
    }
    return event;
}

void
HePhy::HandleRxPpduWithSameContent(Ptr<Event> event,
                                   Ptr<const WifiPpdu> ppdu,
                                   RxPowerWattPerChannelBand& rxPower)
{
    VhtPhy::HandleRxPpduWithSameContent(event, ppdu, rxPower);

    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU && GetCurrentEvent() &&
        (GetCurrentEvent()->GetPpdu()->GetUid() != ppdu->GetUid()))
    {
        NS_LOG_DEBUG("Drop packet because already receiving another HE TB PPDU");
        m_wifiPhy->NotifyRxDrop(GetAddressedPsduInPpdu(ppdu), RXING);
    }
    else if (const auto isResponseToTrigger = (m_previouslyTxPpduUid == ppdu->GetUid());
             isResponseToTrigger && GetCurrentEvent() &&
             (GetCurrentEvent()->GetPpdu()->GetUid() != ppdu->GetUid()))
    {
        NS_LOG_DEBUG("Drop packet because already receiving another response to a trigger frame");
        m_wifiPhy->NotifyRxDrop(GetAddressedPsduInPpdu(ppdu), RXING);
    }
}

Ptr<const WifiPsdu>
HePhy::GetAddressedPsduInPpdu(Ptr<const WifiPpdu> ppdu) const
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU || ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
    {
        auto hePpdu = DynamicCast<const HePpdu>(ppdu);
        NS_ASSERT(hePpdu);
        return hePpdu->GetPsdu(GetBssColor(), GetStaId(ppdu));
    }
    return VhtPhy::GetAddressedPsduInPpdu(ppdu);
}

uint8_t
HePhy::GetBssColor() const
{
    uint8_t bssColor = 0;
    if (m_wifiPhy->GetDevice())
    {
        Ptr<HeConfiguration> heConfiguration = m_wifiPhy->GetDevice()->GetHeConfiguration();
        if (heConfiguration)
        {
            bssColor = heConfiguration->GetBssColor();
        }
    }
    return bssColor;
}

uint16_t
HePhy::GetStaId(const Ptr<const WifiPpdu> ppdu) const
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
    {
        return ppdu->GetStaId();
    }
    else if (ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU)
    {
        auto mac = DynamicCast<StaWifiMac>(m_wifiPhy->GetDevice()->GetMac());
        if (mac && mac->IsAssociated())
        {
            return mac->GetAssociationId();
        }
    }
    return VhtPhy::GetStaId(ppdu);
}

PhyEntity::PhyFieldRxStatus
HePhy::ProcessSig(Ptr<Event> event, PhyFieldRxStatus status, WifiPpduField field)
{
    NS_LOG_FUNCTION(this << *event << status << field);
    NS_ASSERT(event->GetPpdu()->GetTxVector().GetPreambleType() >= WIFI_PREAMBLE_HE_SU);
    switch (field)
    {
    case WIFI_PPDU_FIELD_SIG_A:
        return ProcessSigA(event, status);
    case WIFI_PPDU_FIELD_SIG_B:
        return ProcessSigB(event, status);
    default:
        NS_ASSERT_MSG(false, "Invalid PPDU field");
    }
    return status;
}

PhyEntity::PhyFieldRxStatus
HePhy::ProcessSigA(Ptr<Event> event, PhyFieldRxStatus status)
{
    NS_LOG_FUNCTION(this << *event << status);
    // Notify end of SIG-A (in all cases)
    const auto& txVector = event->GetPpdu()->GetTxVector();
    HeSigAParameters params;
    params.rssiW = GetRxPowerWForPpdu(event);
    params.bssColor = txVector.GetBssColor();
    NotifyEndOfHeSigA(params); // if OBSS_PD CCA_RESET, set power restriction first and wait till
                               // field is processed before switching to IDLE

    if (status.isSuccess)
    {
        // Check if PPDU is filtered based on the BSS color
        uint8_t myBssColor = GetBssColor();
        uint8_t rxBssColor = txVector.GetBssColor();
        if (myBssColor != 0 && rxBssColor != 0 && myBssColor != rxBssColor)
        {
            NS_LOG_DEBUG("The BSS color of this PPDU ("
                         << +rxBssColor << ") does not match the device's (" << +myBssColor
                         << "). The PPDU is filtered.");
            return PhyFieldRxStatus(false, FILTERED, DROP);
        }

        // When SIG-A is decoded, we know the type of frame being received. If we stored a
        // valid TRIGVECTOR and we are not receiving a TB PPDU, we drop the frame.
        Ptr<const WifiPpdu> ppdu = event->GetPpdu();
        if (m_trigVectorExpirationTime.has_value() &&
            (m_trigVectorExpirationTime.value() >= Simulator::Now()) &&
            (ppdu->GetType() != WIFI_PPDU_TYPE_UL_MU))
        {
            NS_LOG_DEBUG("Expected an HE TB PPDU, receiving a " << txVector.GetPreambleType());
            return PhyFieldRxStatus(false, FILTERED, DROP);
        }

        if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
        {
            NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
            // check that the stored TRIGVECTOR is still valid
            if (!m_trigVectorExpirationTime.has_value() ||
                (m_trigVectorExpirationTime < Simulator::Now()))
            {
                NS_LOG_DEBUG("No valid TRIGVECTOR, the PHY was not expecting a TB PPDU");
                return PhyFieldRxStatus(false, FILTERED, DROP);
            }
            // We expected a TB PPDU and we are receiving a TB PPDU. However, despite
            // the previous check on BSS Color, we may be receiving a TB PPDU from an
            // OBSS, as BSS Colors are not guaranteed to be different for all APs in
            // range (an example is when BSS Color is 0). We can detect this situation
            // by comparing the TRIGVECTOR with the TXVECTOR of the TB PPDU being received
            NS_ABORT_IF(!m_trigVector.has_value());
            if (m_trigVector->GetChannelWidth() != txVector.GetChannelWidth())
            {
                NS_LOG_DEBUG("Received channel width different than in TRIGVECTOR");
                return PhyFieldRxStatus(false, FILTERED, DROP);
            }
            if (m_trigVector->GetLength() != txVector.GetLength())
            {
                NS_LOG_DEBUG("Received UL Length (" << txVector.GetLength()
                                                    << ") different than in TRIGVECTOR ("
                                                    << m_trigVector->GetLength() << ")");
                return PhyFieldRxStatus(false, FILTERED, DROP);
            }
            uint16_t staId = ppdu->GetStaId();
            if (m_trigVector->GetHeMuUserInfoMap().find(staId) ==
                m_trigVector->GetHeMuUserInfoMap().end())
            {
                NS_LOG_DEBUG("TB PPDU received from un unexpected STA ID");
                return PhyFieldRxStatus(false, FILTERED, DROP);
            }

            NS_ASSERT(txVector.GetGuardInterval() == m_trigVector->GetGuardInterval());
            NS_ASSERT(txVector.GetMode(staId) == m_trigVector->GetMode(staId));
            NS_ASSERT(txVector.GetNss(staId) == m_trigVector->GetNss(staId));
            NS_ASSERT(txVector.GetHeMuUserInfo(staId) == m_trigVector->GetHeMuUserInfo(staId));

            m_currentMuPpduUid =
                ppdu->GetUid(); // to be able to correctly schedule start of MU payload
        }

        if (ppdu->GetType() != WIFI_PPDU_TYPE_DL_MU &&
            !GetAddressedPsduInPpdu(ppdu)) // Final decision on STA-ID correspondence of DL MU is
                                           // delayed to end of SIG-B
        {
            NS_ASSERT(ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU);
            NS_LOG_DEBUG(
                "No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
            return PhyFieldRxStatus(false, FILTERED, DROP);
        }
    }
    return status;
}

void
HePhy::SetObssPdAlgorithm(const Ptr<ObssPdAlgorithm> algorithm)
{
    m_obssPdAlgorithm = algorithm;
}

void
HePhy::SetEndOfHeSigACallback(EndOfHeSigACallback callback)
{
    m_endOfHeSigACallback = callback;
}

void
HePhy::NotifyEndOfHeSigA(HeSigAParameters params)
{
    if (!m_endOfHeSigACallback.IsNull())
    {
        m_endOfHeSigACallback(params);
    }
}

PhyEntity::PhyFieldRxStatus
HePhy::ProcessSigB(Ptr<Event> event, PhyFieldRxStatus status)
{
    NS_LOG_FUNCTION(this << *event << status);
    NS_ASSERT(IsDlMu(event->GetPpdu()->GetTxVector().GetPreambleType()));
    if (status.isSuccess)
    {
        // Check if PPDU is filtered only if the SIG-B content is supported (not explicitly stated
        // but assumed based on behavior for SIG-A)
        if (!GetAddressedPsduInPpdu(event->GetPpdu()))
        {
            NS_LOG_DEBUG(
                "No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
            return PhyFieldRxStatus(false, FILTERED, DROP);
        }
    }
    m_currentMuPpduUid =
        event->GetPpdu()->GetUid(); // to be able to correctly schedule start of MU payload

    return status;
}

bool
HePhy::IsConfigSupported(Ptr<const WifiPpdu> ppdu) const
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
    {
        return true; // evaluated in ProcessSigA
    }

    const auto& txVector = ppdu->GetTxVector();
    uint16_t staId = GetStaId(ppdu);
    WifiMode txMode = txVector.GetMode(staId);
    uint8_t nss = txVector.GetNssMax();
    if (txVector.IsDlMu())
    {
        NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
        for (auto info : txVector.GetHeMuUserInfoMap())
        {
            if (info.first == staId)
            {
                nss = info.second.nss; // no need to look at other PSDUs
                break;
            }
        }
    }

    if (nss > m_wifiPhy->GetMaxSupportedRxSpatialStreams())
    {
        NS_LOG_DEBUG("Packet reception could not be started because not enough RX antennas");
        return false;
    }
    if (!IsModeSupported(txMode))
    {
        NS_LOG_DEBUG("Drop packet because it was sent using an unsupported mode ("
                     << txVector.GetMode() << ")");
        return false;
    }
    return true;
}

Time
HePhy::DoStartReceivePayload(Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << *event);
    const auto ppdu = event->GetPpdu();
    const auto& txVector = ppdu->GetTxVector();

    if (!txVector.IsMu())
    {
        return VhtPhy::DoStartReceivePayload(event);
    }

    NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);

    if (txVector.IsDlMu())
    {
        Time payloadDuration =
            ppdu->GetTxDuration() - CalculatePhyPreambleAndHeaderDuration(txVector);
        NotifyPayloadBegin(txVector, payloadDuration);
        return payloadDuration;
    }

    // TX duration is determined by the Length field of TXVECTOR
    Time payloadDuration = ConvertLSigLengthToHeTbPpduDuration(txVector.GetLength(),
                                                               txVector,
                                                               m_wifiPhy->GetPhyBand()) -
                           CalculatePhyPreambleAndHeaderDuration(txVector);
    // This method is called when we start receiving the first MU payload. To
    // compute the time to the reception end of the last TB PPDU, we need to add the
    // offset of the last TB PPDU to the payload duration (same for all TB PPDUs)
    Time maxOffset{0};
    for (const auto& beginMuPayloadRxEvent : m_beginMuPayloadRxEvents)
    {
        maxOffset = Max(maxOffset, Simulator::GetDelayLeft(beginMuPayloadRxEvent.second));
    }
    Time timeToEndRx = payloadDuration + maxOffset;

    if (m_wifiPhy->GetDevice()->GetMac()->GetTypeOfStation() != AP)
    {
        NS_LOG_DEBUG("Ignore HE TB PPDU payload received by STA but keep state in Rx");
        NotifyPayloadBegin(txVector, timeToEndRx);
        m_endRxPayloadEvents.push_back(
            Simulator::Schedule(timeToEndRx, &HePhy::ResetReceive, this, event));
        // Cancel all scheduled events for MU payload reception
        NS_ASSERT(!m_beginMuPayloadRxEvents.empty() &&
                  m_beginMuPayloadRxEvents.begin()->second.IsRunning());
        for (auto& beginMuPayloadRxEvent : m_beginMuPayloadRxEvents)
        {
            beginMuPayloadRxEvent.second.Cancel();
        }
        m_beginMuPayloadRxEvents.clear();
    }
    else
    {
        NS_LOG_DEBUG("Receiving PSDU in HE TB PPDU");
        uint16_t staId = GetStaId(ppdu);
        m_signalNoiseMap.insert({std::make_pair(ppdu->GetUid(), staId), SignalNoiseDbm()});
        m_statusPerMpduMap.insert({std::make_pair(ppdu->GetUid(), staId), std::vector<bool>()});
        // for HE TB PPDUs, ScheduleEndOfMpdus and EndReceive are scheduled by
        // StartReceiveMuPayload
        NS_ASSERT(!m_beginMuPayloadRxEvents.empty());
        for (auto& beginMuPayloadRxEvent : m_beginMuPayloadRxEvents)
        {
            NS_ASSERT(beginMuPayloadRxEvent.second.IsRunning());
        }
    }

    return timeToEndRx;
}

void
HePhy::RxPayloadSucceeded(Ptr<const WifiPsdu> psdu,
                          RxSignalInfo rxSignalInfo,
                          const WifiTxVector& txVector,
                          uint16_t staId,
                          const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);
    m_state->NotifyRxPsduSucceeded(psdu, rxSignalInfo, txVector, staId, statusPerMpdu);
    if (!IsUlMu(txVector.GetPreambleType()))
    {
        m_state->SwitchFromRxEndOk();
    }
    else
    {
        m_rxHeTbPpdus++;
    }
}

void
HePhy::RxPayloadFailed(Ptr<const WifiPsdu> psdu, double snr, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << txVector << snr);
    m_state->NotifyRxPsduFailed(psdu, snr);
    if (!txVector.IsUlMu())
    {
        m_state->SwitchFromRxEndError();
    }
}

void
HePhy::DoEndReceivePayload(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this << ppdu);
    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
    {
        for (auto it = m_endRxPayloadEvents.begin(); it != m_endRxPayloadEvents.end();)
        {
            if (it->IsExpired())
            {
                it = m_endRxPayloadEvents.erase(it);
            }
            else
            {
                it++;
            }
        }
        if (m_endRxPayloadEvents.empty())
        {
            // We've got the last PPDU of the UL-MU transmission.
            // Indicate a successful reception is terminated if at least one HE TB PPDU
            // has been successfully received, otherwise indicate a unsuccessful reception is
            // terminated.
            if (m_rxHeTbPpdus > 0)
            {
                m_state->SwitchFromRxEndOk();
            }
            else
            {
                m_state->SwitchFromRxEndError();
            }
            NotifyInterferenceRxEndAndClear(true); // reset WifiPhy
            m_rxHeTbPpdus = 0;
        }
    }
    else
    {
        NS_ASSERT(m_wifiPhy->GetLastRxEndTime() == Simulator::Now());
        VhtPhy::DoEndReceivePayload(ppdu);
    }
    // we are done receiving the payload, we can reset the current MU PPDU UID
    m_currentMuPpduUid = UINT64_MAX;
}

void
HePhy::StartReceiveMuPayload(Ptr<Event> event)
{
    NS_LOG_FUNCTION(this << event);
    Ptr<const WifiPpdu> ppdu = event->GetPpdu();
    const RxPowerWattPerChannelBand& rxPowersW = event->GetRxPowerWPerBand();
    // The total RX power corresponds to the maximum over all the bands.
    // Only perform this computation if the result needs to be logged.
    auto it = rxPowersW.end();
    if (g_log.IsEnabled(ns3::LOG_FUNCTION))
    {
        it = std::max_element(rxPowersW.cbegin(),
                              rxPowersW.cend(),
                              [](const auto& p1, const auto& p2) { return p1.second < p2.second; });
    }
    NS_LOG_FUNCTION(this << *event << it->second);
    NS_ASSERT(GetCurrentEvent());
    NS_ASSERT(m_rxHeTbPpdus == 0);
    auto itEvent = m_beginMuPayloadRxEvents.find(GetStaId(ppdu));
    /**
     * m_beginMuPayloadRxEvents should still be running only for APs, since canceled in
     * StartReceivePayload for STAs. This is because SpectrumWifiPhy does not have access to the
     * device type and thus blindly schedules things, letting the parent WifiPhy class take into
     * account device type.
     */
    NS_ASSERT(itEvent != m_beginMuPayloadRxEvents.end() && itEvent->second.IsExpired());
    m_beginMuPayloadRxEvents.erase(itEvent);

    Time payloadDuration =
        ppdu->GetTxDuration() - CalculatePhyPreambleAndHeaderDuration(ppdu->GetTxVector());
    Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu(ppdu);
    ScheduleEndOfMpdus(event);
    m_endRxPayloadEvents.push_back(
        Simulator::Schedule(payloadDuration, &HePhy::EndReceivePayload, this, event));
    uint16_t staId = GetStaId(ppdu);
    m_signalNoiseMap.insert({std::make_pair(ppdu->GetUid(), staId), SignalNoiseDbm()});
    m_statusPerMpduMap.insert({std::make_pair(ppdu->GetUid(), staId), std::vector<bool>()});
    // Notify the MAC about the start of a new HE TB PPDU, so that it can reschedule the timeout
    NotifyPayloadBegin(ppdu->GetTxVector(), payloadDuration);
}

std::pair<uint16_t, WifiSpectrumBandInfo>
HePhy::GetChannelWidthAndBand(const WifiTxVector& txVector, uint16_t staId) const
{
    if (txVector.IsMu())
    {
        return {HeRu::GetBandwidth(txVector.GetRu(staId).GetRuType()),
                GetRuBandForRx(txVector, staId)};
    }
    else
    {
        return VhtPhy::GetChannelWidthAndBand(txVector, staId);
    }
}

WifiSpectrumBandInfo
HePhy::GetRuBandForTx(const WifiTxVector& txVector, uint16_t staId) const
{
    NS_ASSERT(txVector.IsMu());
    HeRu::RuSpec ru = txVector.GetRu(staId);
    uint16_t channelWidth = txVector.GetChannelWidth();
    NS_ASSERT(channelWidth <= m_wifiPhy->GetChannelWidth());
    HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup(
        channelWidth,
        ru.GetRuType(),
        ru.GetPhyIndex(channelWidth, m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20)));
    HeRu::SubcarrierRange subcarrierRange =
        std::make_pair(group.front().first, group.back().second);
    // for a TX spectrum, the guard bandwidth is a function of the transmission channel width
    // and the spectrum width equals the transmission channel width (hence bandIndex equals 0)
    auto indices = ConvertHeRuSubcarriers(channelWidth,
                                          GetGuardBandwidth(channelWidth),
                                          m_wifiPhy->GetSubcarrierSpacing(),
                                          subcarrierRange,
                                          0);
    auto frequencies = m_wifiPhy->ConvertIndicesToFrequencies(indices);
    return {indices, frequencies};
}

WifiSpectrumBandInfo
HePhy::GetRuBandForRx(const WifiTxVector& txVector, uint16_t staId) const
{
    NS_ASSERT(txVector.IsMu());
    HeRu::RuSpec ru = txVector.GetRu(staId);
    uint16_t channelWidth = txVector.GetChannelWidth();
    NS_ASSERT(channelWidth <= m_wifiPhy->GetChannelWidth());
    HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup(
        channelWidth,
        ru.GetRuType(),
        ru.GetPhyIndex(channelWidth, m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20)));
    HeRu::SubcarrierRange subcarrierRange =
        std::make_pair(group.front().first, group.back().second);
    // for an RX spectrum, the guard bandwidth is a function of the operating channel width
    // and the spectrum width equals the operating channel width
    auto indices = ConvertHeRuSubcarriers(
        channelWidth,
        GetGuardBandwidth(m_wifiPhy->GetChannelWidth()),
        m_wifiPhy->GetSubcarrierSpacing(),
        subcarrierRange,
        m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(channelWidth));
    auto frequencies = m_wifiPhy->ConvertIndicesToFrequencies(indices);
    return {indices, frequencies};
}

WifiSpectrumBandInfo
HePhy::GetNonOfdmaBand(const WifiTxVector& txVector, uint16_t staId) const
{
    NS_ASSERT(txVector.IsUlMu() && (txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE));
    uint16_t channelWidth = txVector.GetChannelWidth();
    NS_ASSERT(channelWidth <= m_wifiPhy->GetChannelWidth());

    HeRu::RuSpec ru = txVector.GetRu(staId);
    uint16_t nonOfdmaWidth = GetNonOfdmaWidth(ru);

    // Find the RU that encompasses the non-OFDMA part of the HE TB PPDU for the STA-ID
    HeRu::RuSpec nonOfdmaRu =
        HeRu::FindOverlappingRu(channelWidth, ru, HeRu::GetRuType(nonOfdmaWidth));

    HeRu::SubcarrierGroup groupPreamble = HeRu::GetSubcarrierGroup(
        channelWidth,
        nonOfdmaRu.GetRuType(),
        nonOfdmaRu.GetPhyIndex(channelWidth,
                               m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20)));
    HeRu::SubcarrierRange subcarrierRange =
        std::make_pair(groupPreamble.front().first, groupPreamble.back().second);
    auto indices = ConvertHeRuSubcarriers(
        channelWidth,
        GetGuardBandwidth(m_wifiPhy->GetChannelWidth()),
        m_wifiPhy->GetSubcarrierSpacing(),
        subcarrierRange,
        m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(channelWidth));
    auto frequencies = m_wifiPhy->ConvertIndicesToFrequencies(indices);
    return {indices, frequencies};
}

uint16_t
HePhy::GetNonOfdmaWidth(HeRu::RuSpec ru) const
{
    if (ru.GetRuType() == HeRu::RU_26_TONE && ru.GetIndex() == 19)
    {
        // the center 26-tone RU in an 80 MHz channel is not fully covered by
        // any 20 MHz channel, but only by an 80 MHz channel
        return 80;
    }
    return std::max<uint16_t>(HeRu::GetBandwidth(ru.GetRuType()), 20);
}

uint64_t
HePhy::GetCurrentHeTbPpduUid() const
{
    return m_currentMuPpduUid;
}

uint16_t
HePhy::GetMeasurementChannelWidth(const Ptr<const WifiPpdu> ppdu) const
{
    uint16_t channelWidth = OfdmPhy::GetMeasurementChannelWidth(ppdu);
    /**
     * The PHY shall not issue a PHY-RXSTART.indication primitive in response to a PPDU that does
     * not overlap the primary channel unless the PHY at an AP receives the HE TB PPDU solicited by
     * the AP. For the HE TB PPDU solicited by the AP, the PHY shall issue a PHY-RXSTART.indication
     * primitive for a PPDU received in the primary or at the secondary 20 MHz channel, the
     * secondary 40 MHz channel, or the secondary 80 MHz channel.
     */
    if (channelWidth >= 40 && ppdu->GetUid() != m_previouslyTxPpduUid)
    {
        channelWidth = 20;
    }
    return channelWidth;
}

double
HePhy::GetCcaThreshold(const Ptr<const WifiPpdu> ppdu, WifiChannelListType channelType) const
{
    if (!ppdu)
    {
        return VhtPhy::GetCcaThreshold(ppdu, channelType);
    }

    if (!m_obssPdAlgorithm)
    {
        return VhtPhy::GetCcaThreshold(ppdu, channelType);
    }

    if (channelType == WIFI_CHANLIST_PRIMARY)
    {
        return VhtPhy::GetCcaThreshold(ppdu, channelType);
    }

    const uint16_t ppduBw = ppdu->GetTxVector().GetChannelWidth();
    double obssPdLevel = m_obssPdAlgorithm->GetObssPdLevel();
    uint16_t bw = ppduBw;
    while (bw > 20)
    {
        obssPdLevel += 3;
        bw /= 2;
    }

    return std::max(VhtPhy::GetCcaThreshold(ppdu, channelType), obssPdLevel);
}

void
HePhy::SwitchMaybeToCcaBusy(const Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this);
    const auto ccaIndication = GetCcaIndication(ppdu);
    const auto per20MHzDurations = GetPer20MHzDurations(ppdu);
    if (ccaIndication.has_value())
    {
        NS_LOG_DEBUG("CCA busy for " << ccaIndication.value().second << " during "
                                     << ccaIndication.value().first.As(Time::S));
        NotifyCcaBusy(ccaIndication.value().first, ccaIndication.value().second, per20MHzDurations);
        return;
    }
    if (ppdu)
    {
        SwitchMaybeToCcaBusy(nullptr);
        return;
    }
    if (per20MHzDurations != m_lastPer20MHzDurations)
    {
        /*
         * 8.3.5.12.3: For Clause 27 PHYs, this primitive is generated when (...) the per20bitmap
         * parameter changes.
         */
        NS_LOG_DEBUG("per-20MHz CCA durations changed");
        NotifyCcaBusy(Seconds(0), WIFI_CHANLIST_PRIMARY, per20MHzDurations);
    }
}

void
HePhy::NotifyCcaBusy(const Ptr<const WifiPpdu> ppdu, Time duration, WifiChannelListType channelType)
{
    NS_LOG_FUNCTION(this << duration << channelType);
    NS_LOG_DEBUG("CCA busy for " << channelType << " during " << duration.As(Time::S));
    const auto per20MHzDurations = GetPer20MHzDurations(ppdu);
    NotifyCcaBusy(duration, channelType, per20MHzDurations);
}

void
HePhy::NotifyCcaBusy(Time duration,
                     WifiChannelListType channelType,
                     const std::vector<Time>& per20MHzDurations)
{
    NS_LOG_FUNCTION(this << duration << channelType);
    m_state->SwitchMaybeToCcaBusy(duration, channelType, per20MHzDurations);
    m_lastPer20MHzDurations = per20MHzDurations;
}

std::vector<Time>
HePhy::GetPer20MHzDurations(const Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this);

    /**
     * 27.3.20.6.5 Per 20 MHz CCA sensitivity:
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
        double ccaThresholdDbm = -62;
        Time delayUntilCcaEnd = GetDelayUntilCcaEnd(ccaThresholdDbm, band);

        if (ppdu)
        {
            const uint16_t subchannelMinFreq =
                m_wifiPhy->GetFrequency() - (m_wifiPhy->GetChannelWidth() / 2) + (index * 20);
            const uint16_t subchannelMaxFreq = subchannelMinFreq + 20;
            const uint16_t ppduBw = ppdu->GetTxVector().GetChannelWidth();

            if (ppduBw <= m_wifiPhy->GetChannelWidth() &&
                ppdu->DoesOverlapChannel(subchannelMinFreq, subchannelMaxFreq))
            {
                std::optional<double> obssPdLevel{std::nullopt};
                if (m_obssPdAlgorithm)
                {
                    obssPdLevel = m_obssPdAlgorithm->GetObssPdLevel();
                }
                switch (ppduBw)
                {
                case 20:
                case 22:
                    /**
                     * A 20 MHz non-HT, HT_MF, HT_GF, VHT, or HE PPDU at or above max(–72 dBm, OBSS_
                     * PDlevel) at the receiver's antenna(s) is present on the 20 MHz subchannel.
                     * The PHY shall indicate that the 20 MHz subchannel is busy with > 90%
                     * probability within a period aCCAMidTime.
                     */
                    ccaThresholdDbm =
                        obssPdLevel.has_value() ? std::max(-72.0, obssPdLevel.value()) : -72.0;
                    band = m_wifiPhy->GetBand(20, index);
                    break;
                case 40:
                    /**
                     * The 20 MHz subchannel is in a channel on which a 40 MHz non-HT duplicate,
                     * HT_MF, HT_GF, VHT or HE PPDU at or above max(–72 dBm, OBSS_PDlevel + 3 dB) at
                     * the receiver's antenna(s) is present. The PHY shall indicate that the 20 MHz
                     * subchannel is busy with > 90% probability within a period aCCAMidTime.
                     */
                    ccaThresholdDbm =
                        obssPdLevel.has_value() ? std::max(-72.0, obssPdLevel.value() + 3) : -72.0;
                    band = m_wifiPhy->GetBand(40, std::floor(index / 2));
                    break;
                case 80:
                    /**
                     * The 20 MHz subchannel is in a channel on which an 80 MHz non-HT duplicate,
                     * VHT or HE PPDU at or above max(–69 dBm, OBSS_PDlevel + 6 dB) at the
                     * receiver's antenna(s) is present. The PHY shall indicate that the 20 MHz
                     * subchannel is busy with > 90% probability within a period aCCAMidTime.
                     */
                    ccaThresholdDbm =
                        obssPdLevel.has_value() ? std::max(-69.0, obssPdLevel.value() + 6) : -69.0;
                    band = m_wifiPhy->GetBand(80, std::floor(index / 4));
                    break;
                case 160:
                    // Not defined in the standard: keep -62 dBm
                    break;
                default:
                    NS_ASSERT_MSG(false, "Invalid channel width: " << ppduBw);
                }
            }
            Time ppduCcaDuration = GetDelayUntilCcaEnd(ccaThresholdDbm, band);
            delayUntilCcaEnd = std::max(delayUntilCcaEnd, ppduCcaDuration);
        }
        per20MhzDurations.push_back(delayUntilCcaEnd);
    }

    return per20MhzDurations;
}

uint64_t
HePhy::ObtainNextUid(const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << txVector);
    uint64_t uid;
    if (txVector.IsUlMu() || txVector.IsTriggerResponding())
    {
        // Use UID of PPDU containing trigger frame to identify resulting HE TB PPDUs, since the
        // latter should immediately follow the former
        uid = m_wifiPhy->GetPreviouslyRxPpduUid();
        NS_ASSERT(uid != UINT64_MAX);
    }
    else
    {
        uid = m_globalPpduUid++;
    }
    m_previouslyTxPpduUid = uid; // to be able to identify solicited HE TB PPDUs
    return uid;
}

Time
HePhy::GetMaxDelayPpduSameUid(const WifiTxVector& txVector)
{
    auto heConfiguration = m_wifiPhy->GetDevice()->GetHeConfiguration();
    NS_ASSERT(heConfiguration);
    // DoStartReceivePayload(), which is called when we start receiving the Data field,
    // computes the max offset among TB PPDUs based on the begin MU payload RX events,
    // which are scheduled by StartReceivePreamble() when starting the reception of the
    // HE portion. Therefore, the maximum delay cannot exceed the duration of the
    // training fields that are between the start of the HE portion and the start
    // of the Data field.
    auto maxDelay = GetDuration(WIFI_PPDU_FIELD_TRAINING, txVector);
    if (heConfiguration->GetMaxTbPpduDelay().IsStrictlyPositive())
    {
        maxDelay = Min(maxDelay, heConfiguration->GetMaxTbPpduDelay());
    }
    return maxDelay;
}

Ptr<SpectrumValue>
HePhy::GetTxPowerSpectralDensity(double txPowerW, Ptr<const WifiPpdu> ppdu) const
{
    auto hePpdu = DynamicCast<const HePpdu>(ppdu);
    NS_ASSERT(hePpdu);
    HePpdu::TxPsdFlag flag = hePpdu->GetTxPsdFlag();
    return GetTxPowerSpectralDensity(txPowerW, ppdu, flag);
}

Ptr<SpectrumValue>
HePhy::GetTxPowerSpectralDensity(double txPowerW,
                                 Ptr<const WifiPpdu> ppdu,
                                 HePpdu::TxPsdFlag flag) const
{
    const auto& txVector = ppdu->GetTxVector();
    uint16_t centerFrequency = GetCenterFrequencyForChannelWidth(txVector);
    uint16_t channelWidth = txVector.GetChannelWidth();
    NS_LOG_FUNCTION(this << centerFrequency << channelWidth << txPowerW << txVector);
    const auto& puncturedSubchannels = txVector.GetInactiveSubchannels();
    if (!puncturedSubchannels.empty())
    {
        const auto p20Index = m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20);
        const auto& indices =
            m_wifiPhy->GetOperatingChannel().GetAll20MHzChannelIndicesInPrimary(channelWidth);
        const auto p20IndexInBitmap = p20Index - *(indices.cbegin());
        NS_ASSERT(
            !puncturedSubchannels.at(p20IndexInBitmap)); // the primary channel cannot be punctured
    }
    const auto& txMaskRejectionParams = GetTxMaskRejectionParams();
    switch (ppdu->GetType())
    {
    case WIFI_PPDU_TYPE_UL_MU: {
        if (flag == HePpdu::PSD_NON_HE_PORTION)
        {
            // non-HE portion is sent only on the 20 MHz channels covering the RU
            const uint16_t staId = GetStaId(ppdu);
            centerFrequency = GetCenterFrequencyForNonHePart(txVector, staId);
            const uint16_t ruWidth = HeRu::GetBandwidth(txVector.GetRu(staId).GetRuType());
            channelWidth = (ruWidth < 20) ? 20 : ruWidth;
            return WifiSpectrumValueHelper::CreateDuplicated20MhzTxPowerSpectralDensity(
                centerFrequency,
                channelWidth,
                txPowerW,
                GetGuardBandwidth(channelWidth),
                std::get<0>(txMaskRejectionParams),
                std::get<1>(txMaskRejectionParams),
                std::get<2>(txMaskRejectionParams),
                puncturedSubchannels);
        }
        else
        {
            const auto band = GetRuBandForTx(txVector, GetStaId(ppdu)).indices;
            return WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity(
                centerFrequency,
                channelWidth,
                txPowerW,
                GetGuardBandwidth(channelWidth),
                band);
        }
    }
    case WIFI_PPDU_TYPE_DL_MU: {
        if (flag == HePpdu::PSD_NON_HE_PORTION)
        {
            return WifiSpectrumValueHelper::CreateDuplicated20MhzTxPowerSpectralDensity(
                centerFrequency,
                channelWidth,
                txPowerW,
                GetGuardBandwidth(channelWidth),
                std::get<0>(txMaskRejectionParams),
                std::get<1>(txMaskRejectionParams),
                std::get<2>(txMaskRejectionParams),
                puncturedSubchannels);
        }
        else
        {
            return WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
                centerFrequency,
                channelWidth,
                txPowerW,
                GetGuardBandwidth(channelWidth),
                std::get<0>(txMaskRejectionParams),
                std::get<1>(txMaskRejectionParams),
                std::get<2>(txMaskRejectionParams),
                puncturedSubchannels);
        }
    }
    case WIFI_PPDU_TYPE_SU:
    default: {
        NS_ASSERT(puncturedSubchannels.empty());
        return WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
            centerFrequency,
            channelWidth,
            txPowerW,
            GetGuardBandwidth(channelWidth),
            std::get<0>(txMaskRejectionParams),
            std::get<1>(txMaskRejectionParams),
            std::get<2>(txMaskRejectionParams));
    }
    }
}

uint16_t
HePhy::GetCenterFrequencyForNonHePart(const WifiTxVector& txVector, uint16_t staId) const
{
    NS_LOG_FUNCTION(this << txVector << staId);
    NS_ASSERT(txVector.IsUlMu() && (txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE));
    uint16_t centerFrequency = GetCenterFrequencyForChannelWidth(txVector);
    uint16_t currentWidth = txVector.GetChannelWidth();

    HeRu::RuSpec ru = txVector.GetRu(staId);
    uint16_t nonOfdmaWidth = GetNonOfdmaWidth(ru);
    if (nonOfdmaWidth != currentWidth)
    {
        // Obtain the index of the non-OFDMA portion
        HeRu::RuSpec nonOfdmaRu =
            HeRu::FindOverlappingRu(currentWidth, ru, HeRu::GetRuType(nonOfdmaWidth));

        uint16_t startingFrequency = centerFrequency - (currentWidth / 2);
        centerFrequency =
            startingFrequency +
            nonOfdmaWidth * (nonOfdmaRu.GetPhyIndex(
                                 currentWidth,
                                 m_wifiPhy->GetOperatingChannel().GetPrimaryChannelIndex(20)) -
                             1) +
            nonOfdmaWidth / 2;
    }
    return centerFrequency;
}

void
HePhy::StartTx(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(this << ppdu);
    const auto& txVector = ppdu->GetTxVector();
    if (auto mac = m_wifiPhy->GetDevice()->GetMac(); mac && (mac->GetTypeOfStation() == AP))
    {
        m_currentTxVector = txVector;
    }
    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU || ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU)
    {
        auto nonHeTxPowerDbm = m_wifiPhy->GetTxPowerForTransmission(ppdu) + m_wifiPhy->GetTxGain();

        // temporarily set WifiPpdu flag to PSD_HE_PORTION for correct calculation of TX power for
        // the HE portion
        auto hePpdu = DynamicCast<const HePpdu>(ppdu);
        NS_ASSERT(hePpdu);
        hePpdu->SetTxPsdFlag(HePpdu::PSD_HE_PORTION);
        auto heTxPowerDbm = m_wifiPhy->GetTxPowerForTransmission(ppdu) + m_wifiPhy->GetTxGain();
        hePpdu->SetTxPsdFlag(HePpdu::PSD_NON_HE_PORTION);

        // non-HE portion
        auto nonHePortionDuration = ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU
                                        ? CalculateNonHeDurationForHeTb(txVector)
                                        : CalculateNonHeDurationForHeMu(txVector);
        auto nonHeTxPowerSpectrum =
            GetTxPowerSpectralDensity(DbmToW(nonHeTxPowerDbm), ppdu, HePpdu::PSD_NON_HE_PORTION);
        Transmit(nonHePortionDuration,
                 ppdu,
                 nonHeTxPowerDbm,
                 nonHeTxPowerSpectrum,
                 "non-HE portion transmission");

        // HE portion
        auto hePortionDuration = ppdu->GetTxDuration() - nonHePortionDuration;
        auto heTxPowerSpectrum =
            GetTxPowerSpectralDensity(DbmToW(heTxPowerDbm), ppdu, HePpdu::PSD_HE_PORTION);
        Simulator::Schedule(nonHePortionDuration,
                            &HePhy::StartTxHePortion,
                            this,
                            ppdu,
                            heTxPowerDbm,
                            heTxPowerSpectrum,
                            hePortionDuration);
    }
    else
    {
        VhtPhy::StartTx(ppdu);
    }
}

void
HePhy::StartTxHePortion(Ptr<const WifiPpdu> ppdu,
                        double txPowerDbm,
                        Ptr<SpectrumValue> txPowerSpectrum,
                        Time hePortionDuration)
{
    NS_LOG_FUNCTION(this << ppdu << txPowerDbm << hePortionDuration);
    auto hePpdu = DynamicCast<const HePpdu>(ppdu);
    NS_ASSERT(hePpdu);
    hePpdu->SetTxPsdFlag(HePpdu::PSD_HE_PORTION);
    Transmit(hePortionDuration, ppdu, txPowerDbm, txPowerSpectrum, "HE portion transmission");
}

Time
HePhy::CalculateTxDuration(WifiConstPsduMap psduMap,
                           const WifiTxVector& txVector,
                           WifiPhyBand band) const
{
    if (txVector.IsUlMu())
    {
        NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
        return ConvertLSigLengthToHeTbPpduDuration(txVector.GetLength(), txVector, band);
    }

    Time maxDuration = Seconds(0);
    for (auto& staIdPsdu : psduMap)
    {
        if (txVector.IsDlMu())
        {
            NS_ASSERT(txVector.GetModulationClass() >= WIFI_MOD_CLASS_HE);
            WifiTxVector::HeMuUserInfoMap userInfoMap = txVector.GetHeMuUserInfoMap();
            NS_ABORT_MSG_IF(userInfoMap.find(staIdPsdu.first) == userInfoMap.end(),
                            "STA-ID in psduMap (" << staIdPsdu.first
                                                  << ") should be referenced in txVector");
        }
        Time current = WifiPhy::CalculateTxDuration(staIdPsdu.second->GetSize(),
                                                    txVector,
                                                    band,
                                                    staIdPsdu.first);
        if (current > maxDuration)
        {
            maxDuration = current;
        }
    }
    NS_ASSERT(maxDuration.IsStrictlyPositive());
    return maxDuration;
}

void
HePhy::InitializeModes()
{
    for (uint8_t i = 0; i < 12; ++i)
    {
        GetHeMcs(i);
    }
}

WifiMode
HePhy::GetHeMcs(uint8_t index)
{
#define CASE(x)                                                                                    \
    case x:                                                                                        \
        return GetHeMcs##x();

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
    default:
        NS_ABORT_MSG("Inexistent index (" << +index << ") requested for HE");
        return WifiMode();
    }
#undef CASE
}

#define GET_HE_MCS(x)                                                                              \
    WifiMode HePhy::GetHeMcs##x()                                                                  \
    {                                                                                              \
        static WifiMode mcs = CreateHeMcs(x);                                                      \
        return mcs;                                                                                \
    };

GET_HE_MCS(0)
GET_HE_MCS(1)
GET_HE_MCS(2)
GET_HE_MCS(3)
GET_HE_MCS(4)
GET_HE_MCS(5)
GET_HE_MCS(6)
GET_HE_MCS(7)
GET_HE_MCS(8)
GET_HE_MCS(9)
GET_HE_MCS(10)
GET_HE_MCS(11)
#undef GET_HE_MCS

WifiMode
HePhy::CreateHeMcs(uint8_t index)
{
    NS_ASSERT_MSG(index <= 11, "HeMcs index must be <= 11!");
    return WifiModeFactory::CreateWifiMcs("HeMcs" + std::to_string(index),
                                          index,
                                          WIFI_MOD_CLASS_HE,
                                          false,
                                          MakeBoundCallback(&GetCodeRate, index),
                                          MakeBoundCallback(&GetConstellationSize, index),
                                          MakeCallback(&GetPhyRateFromTxVector),
                                          MakeCallback(&GetDataRateFromTxVector),
                                          MakeBoundCallback(&GetNonHtReferenceRate, index),
                                          MakeCallback(&IsAllowed));
}

WifiCodeRate
HePhy::GetCodeRate(uint8_t mcsValue)
{
    switch (mcsValue)
    {
    case 10:
        return WIFI_CODE_RATE_3_4;
    case 11:
        return WIFI_CODE_RATE_5_6;
    default:
        return VhtPhy::GetCodeRate(mcsValue);
    }
}

uint16_t
HePhy::GetConstellationSize(uint8_t mcsValue)
{
    switch (mcsValue)
    {
    case 10:
    case 11:
        return 1024;
    default:
        return VhtPhy::GetConstellationSize(mcsValue);
    }
}

uint64_t
HePhy::GetPhyRate(uint8_t mcsValue, uint16_t channelWidth, uint16_t guardInterval, uint8_t nss)
{
    WifiCodeRate codeRate = GetCodeRate(mcsValue);
    uint64_t dataRate = GetDataRate(mcsValue, channelWidth, guardInterval, nss);
    return HtPhy::CalculatePhyRate(codeRate, dataRate);
}

uint64_t
HePhy::GetPhyRateFromTxVector(const WifiTxVector& txVector, uint16_t staId /* = SU_STA_ID */)
{
    uint16_t bw = txVector.GetChannelWidth();
    if (txVector.IsMu())
    {
        bw = HeRu::GetBandwidth(txVector.GetRu(staId).GetRuType());
    }
    return HePhy::GetPhyRate(txVector.GetMode(staId).GetMcsValue(),
                             bw,
                             txVector.GetGuardInterval(),
                             txVector.GetNss(staId));
}

uint64_t
HePhy::GetDataRateFromTxVector(const WifiTxVector& txVector, uint16_t staId /* = SU_STA_ID */)
{
    uint16_t bw = txVector.GetChannelWidth();
    if (txVector.IsMu())
    {
        bw = HeRu::GetBandwidth(txVector.GetRu(staId).GetRuType());
    }
    return HePhy::GetDataRate(txVector.GetMode(staId).GetMcsValue(),
                              bw,
                              txVector.GetGuardInterval(),
                              txVector.GetNss(staId));
}

uint64_t
HePhy::GetDataRate(uint8_t mcsValue, uint16_t channelWidth, uint16_t guardInterval, uint8_t nss)
{
    NS_ASSERT(guardInterval == 800 || guardInterval == 1600 || guardInterval == 3200);
    NS_ASSERT(nss <= 8);
    return HtPhy::CalculateDataRate(GetSymbolDuration(NanoSeconds(guardInterval)),
                                    GetUsableSubcarriers(channelWidth),
                                    static_cast<uint16_t>(log2(GetConstellationSize(mcsValue))),
                                    HtPhy::GetCodeRatio(GetCodeRate(mcsValue)),
                                    nss);
}

uint16_t
HePhy::GetUsableSubcarriers(uint16_t channelWidth)
{
    switch (channelWidth)
    {
    case 2: // 26-tone RU
        return 24;
    case 4: // 52-tone RU
        return 48;
    case 8: // 106-tone RU
        return 102;
    case 20:
    default:
        return 234;
    case 40:
        return 468;
    case 80:
        return 980;
    case 160:
        return 1960;
    }
}

Time
HePhy::GetSymbolDuration(Time guardInterval)
{
    return NanoSeconds(12800) + guardInterval;
}

uint64_t
HePhy::GetNonHtReferenceRate(uint8_t mcsValue)
{
    WifiCodeRate codeRate = GetCodeRate(mcsValue);
    uint16_t constellationSize = GetConstellationSize(mcsValue);
    return CalculateNonHtReferenceRate(codeRate, constellationSize);
}

uint64_t
HePhy::CalculateNonHtReferenceRate(WifiCodeRate codeRate, uint16_t constellationSize)
{
    uint64_t dataRate;
    switch (constellationSize)
    {
    case 1024:
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
        dataRate = VhtPhy::CalculateNonHtReferenceRate(codeRate, constellationSize);
    }
    return dataRate;
}

bool
HePhy::IsAllowed(const WifiTxVector& /*txVector*/)
{
    return true;
}

WifiConstPsduMap
HePhy::GetWifiConstPsduMap(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) const
{
    uint16_t staId = SU_STA_ID;

    if (IsUlMu(txVector.GetPreambleType()))
    {
        NS_ASSERT(txVector.GetHeMuUserInfoMap().size() == 1);
        staId = txVector.GetHeMuUserInfoMap().begin()->first;
    }

    return WifiConstPsduMap({std::make_pair(staId, psdu)});
}

uint32_t
HePhy::GetMaxPsduSize() const
{
    return 6500631;
}

bool
HePhy::CanStartRx(Ptr<const WifiPpdu> ppdu) const
{
    /*
     * The PHY shall not issue a PHY-RXSTART.indication primitive in response to a PPDU
     * that does not overlap the primary channel, unless the PHY at an AP receives the
     * HE TB PPDU solicited by the AP. For the HE TB PPDU solicited by the AP, the PHY
     * shall issue a PHY-RXSTART.indication primitive for a PPDU received in the primary
     * or at the secondary 20 MHz channel, the secondary 40 MHz channel, or the secondary
     * 80 MHz channel.
     */
    Ptr<WifiMac> mac = m_wifiPhy->GetDevice() ? m_wifiPhy->GetDevice()->GetMac() : nullptr;
    if (ppdu->GetTxVector().IsUlMu() && mac && mac->GetTypeOfStation() == AP)
    {
        return true;
    }
    return VhtPhy::CanStartRx(ppdu);
}

Ptr<const WifiPpdu>
HePhy::GetRxPpduFromTxPpdu(Ptr<const WifiPpdu> ppdu)
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_UL_MU)
    {
        Ptr<const WifiPpdu> rxPpdu;
        if ((m_trigVectorExpirationTime.has_value()) &&
            (Simulator::Now() <= m_trigVectorExpirationTime.value()))
        {
            // We only copy if the AP that is expecting a HE TB PPDU, since the content
            // of the TXVECTOR is reconstructed from the TRIGVECTOR, hence the other RX
            // PHYs should not have this information.
            rxPpdu = ppdu->Copy();
        }
        else
        {
            rxPpdu = ppdu;
        }
        auto hePpdu = DynamicCast<const HePpdu>(rxPpdu);
        NS_ASSERT(hePpdu);
        hePpdu->UpdateTxVectorForUlMu(m_trigVector);
        return rxPpdu;
    }
    return VhtPhy::GetRxPpduFromTxPpdu(ppdu);
}

WifiSpectrumBandIndices
HePhy::ConvertHeRuSubcarriers(uint16_t bandWidth,
                              uint16_t guardBandwidth,
                              uint32_t subcarrierSpacing,
                              HeRu::SubcarrierRange subcarrierRange,
                              uint8_t bandIndex)
{
    WifiSpectrumBandIndices convertedSubcarriers;
    auto nGuardBands =
        static_cast<uint32_t>(((2 * guardBandwidth * 1e6) / subcarrierSpacing) + 0.5);
    uint32_t centerFrequencyIndex = 0;
    switch (bandWidth)
    {
    case 20:
        centerFrequencyIndex = (nGuardBands / 2) + 6 + 122;
        break;
    case 40:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 244;
        break;
    case 80:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 500;
        break;
    case 160:
        centerFrequencyIndex = (nGuardBands / 2) + 12 + 1012;
        break;
    default:
        NS_FATAL_ERROR("ChannelWidth " << bandWidth << " unsupported");
        break;
    }

    auto numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / subcarrierSpacing);
    centerFrequencyIndex += numBandsInBand * bandIndex;

    convertedSubcarriers.first = centerFrequencyIndex + subcarrierRange.first;
    convertedSubcarriers.second = centerFrequencyIndex + subcarrierRange.second;
    return convertedSubcarriers;
}

} // namespace ns3

namespace
{

/**
 * Constructor class for HE modes
 */
class ConstructorHe
{
  public:
    ConstructorHe()
    {
        ns3::HePhy::InitializeModes();
        ns3::WifiPhy::AddStaticPhyEntity(ns3::WIFI_MOD_CLASS_HE, ns3::Create<ns3::HePhy>());
    }
} g_constructor_he; ///< the constructor for HE modes

} // namespace
