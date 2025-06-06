/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "interference-helper.h"

#include "error-rate-model.h"
#include "wifi-phy-operating-channel.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-utils.h"

#include "ns3/he-ppdu.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("InterferenceHelper");

NS_OBJECT_ENSURE_REGISTERED(InterferenceHelper);

/****************************************************************
 *       PHY event class
 ****************************************************************/

Event::Event(Ptr<const WifiPpdu> ppdu, Time duration, RxPowerWattPerChannelBand&& rxPower)
    : m_ppdu(ppdu),
      m_startTime(Simulator::Now()),
      m_endTime(m_startTime + duration),
      m_rxPowerW(std::move(rxPower))
{
}

Ptr<const WifiPpdu>
Event::GetPpdu() const
{
    return m_ppdu;
}

Time
Event::GetStartTime() const
{
    return m_startTime;
}

Time
Event::GetEndTime() const
{
    return m_endTime;
}

Time
Event::GetDuration() const
{
    return m_endTime - m_startTime;
}

Watt_u
Event::GetRxPower() const
{
    NS_ASSERT(!m_rxPowerW.empty());
    // The total RX power corresponds to the maximum over all the bands
    auto it =
        std::max_element(m_rxPowerW.cbegin(),
                         m_rxPowerW.cend(),
                         [](const auto& p1, const auto& p2) { return p1.second < p2.second; });
    return it->second;
}

Watt_u
Event::GetRxPower(const WifiSpectrumBandInfo& band) const
{
    const auto it = m_rxPowerW.find(band);
    NS_ASSERT(it != m_rxPowerW.cend());
    return it->second;
}

const RxPowerWattPerChannelBand&
Event::GetRxPowerPerBand() const
{
    return m_rxPowerW;
}

void
Event::UpdateRxPowerW(const RxPowerWattPerChannelBand& rxPower)
{
    NS_ASSERT(rxPower.size() == m_rxPowerW.size());
    // Update power band per band
    for (auto& currentRxPowerW : m_rxPowerW)
    {
        auto band = currentRxPowerW.first;
        auto it = rxPower.find(band);
        if (it != rxPower.end())
        {
            currentRxPowerW.second += it->second;
        }
    }
}

void
Event::UpdatePpdu(Ptr<const WifiPpdu> ppdu)
{
    m_ppdu = ppdu;
}

std::ostream&
operator<<(std::ostream& os, const Event& event)
{
    os << "start=" << event.GetStartTime() << ", end=" << event.GetEndTime()
       << ", power=" << event.GetRxPower() << "W"
       << ", PPDU=" << event.GetPpdu();
    return os;
}

/****************************************************************
 *       Class which records SNIR change events for a
 *       short period of time.
 ****************************************************************/

InterferenceHelper::NiChange::NiChange(Watt_u power, Ptr<Event> event)
    : m_power(power),
      m_event(event)
{
}

Watt_u
InterferenceHelper::NiChange::GetPower() const
{
    return m_power;
}

void
InterferenceHelper::NiChange::AddPower(Watt_u power)
{
    m_power += power;
}

Ptr<Event>
InterferenceHelper::NiChange::GetEvent() const
{
    return m_event;
}

/****************************************************************
 *       The actual InterferenceHelper
 ****************************************************************/

InterferenceHelper::InterferenceHelper()
    : m_errorRateModel(nullptr),
      m_numRxAntennas(1)
{
    NS_LOG_FUNCTION(this);
}

InterferenceHelper::~InterferenceHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
InterferenceHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::InterferenceHelper")
                            .SetParent<ns3::Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<InterferenceHelper>();
    return tid;
}

void
InterferenceHelper::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto it : m_niChanges)
    {
        it.second.clear();
    }
    m_niChanges.clear();
    m_firstPowers.clear();
    m_errorRateModel = nullptr;
}

Ptr<Event>
InterferenceHelper::Add(Ptr<const WifiPpdu> ppdu,
                        Time duration,
                        RxPowerWattPerChannelBand& rxPowerW,
                        const FrequencyRange& freqRange,
                        bool isStartHePortionRxing)
{
    Ptr<Event> event = Create<Event>(ppdu, duration, std::move(rxPowerW));
    AppendEvent(event, freqRange, isStartHePortionRxing);
    return event;
}

void
InterferenceHelper::AddForeignSignal(Time duration,
                                     RxPowerWattPerChannelBand& rxPowerW,
                                     const FrequencyRange& freqRange)
{
    // Parameters other than duration and rxPowerW are unused for this type
    // of signal, so we provide dummy versions
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    Ptr<WifiPpdu> fakePpdu = Create<WifiPpdu>(Create<WifiPsdu>(Create<Packet>(0), hdr),
                                              WifiTxVector(),
                                              WifiPhyOperatingChannel());
    Add(fakePpdu, duration, rxPowerW, freqRange);
}

bool
InterferenceHelper::HasBands() const
{
    return !m_niChanges.empty();
}

bool
InterferenceHelper::HasBand(const WifiSpectrumBandInfo& band) const
{
    return m_niChanges.contains(band);
}

void
InterferenceHelper::AddBand(const WifiSpectrumBandInfo& band)
{
    NS_LOG_FUNCTION(this << band);
    NS_ASSERT(!m_niChanges.contains(band));
    NS_ASSERT(!m_firstPowers.contains(band));
    NiChanges niChanges;
    auto result = m_niChanges.insert({band, niChanges});
    NS_ASSERT(result.second);
    // Always have a zero power noise event in the list
    AddNiChangeEvent(Time(0), NiChange(Watt_u{0}, nullptr), result.first);
    m_firstPowers.insert({band, Watt_u{0}});
}

void
InterferenceHelper::RemoveBand(const WifiSpectrumBandInfo& band)
{
    NS_LOG_FUNCTION(this << band);
    NS_ASSERT(m_firstPowers.count(band) != 0);
    m_firstPowers.erase(band);
    auto it = m_niChanges.find(band);
    NS_ASSERT(it != std::end(m_niChanges));
    it->second.clear();
    m_niChanges.erase(it);
}

void
InterferenceHelper::UpdateBands(const std::vector<WifiSpectrumBandInfo>& bands,
                                const FrequencyRange& freqRange)
{
    NS_LOG_FUNCTION(this << freqRange);
    std::vector<WifiSpectrumBandInfo> bandsToRemove{};
    for (auto it = m_niChanges.begin(); it != m_niChanges.end(); ++it)
    {
        if (!IsBandInFrequencyRange(it->first, freqRange))
        {
            continue;
        }
        const auto frequencies = it->first.frequencies;
        const auto found =
            std::find_if(bands.cbegin(), bands.cend(), [frequencies](const auto& item) {
                return frequencies == item.frequencies;
            }) != std::end(bands);
        if (!found)
        {
            // band does not belong to the new bands, erase it
            bandsToRemove.emplace_back(it->first);
        }
    }
    for (const auto& band : bandsToRemove)
    {
        RemoveBand(band);
    }
    for (const auto& band : bands)
    {
        if (!HasBand(band))
        {
            // this is a new band, add it
            AddBand(band);
        }
    }
}

void
InterferenceHelper::SetNoiseFigure(double value)
{
    m_noiseFigure = value;
}

void
InterferenceHelper::SetErrorRateModel(const Ptr<ErrorRateModel> rate)
{
    m_errorRateModel = rate;
}

Ptr<ErrorRateModel>
InterferenceHelper::GetErrorRateModel() const
{
    return m_errorRateModel;
}

void
InterferenceHelper::SetNumberOfReceiveAntennas(uint8_t rx)
{
    m_numRxAntennas = rx;
}

Time
InterferenceHelper::GetEnergyDuration(Watt_u energy, const WifiSpectrumBandInfo& band)
{
    NS_LOG_FUNCTION(this << energy << band);
    Time now = Simulator::Now();
    auto niIt = m_niChanges.find(band);
    NS_ABORT_IF(niIt == m_niChanges.end());
    auto i = GetPreviousPosition(now, niIt);
    Time end = i->first;
    for (; i != niIt->second.end(); ++i)
    {
        const auto noiseInterference = i->second.GetPower();
        end = i->first;
        if (noiseInterference < energy)
        {
            break;
        }
    }
    return end > now ? end - now : Time{0};
}

void
InterferenceHelper::AppendEvent(Ptr<Event> event,
                                const FrequencyRange& freqRange,
                                bool isStartHePortionRxing)
{
    NS_LOG_FUNCTION(this << event << freqRange << isStartHePortionRxing);
    for (const auto& [band, power] : event->GetRxPowerPerBand())
    {
        auto niIt = m_niChanges.find(band);
        NS_ABORT_IF(niIt == m_niChanges.end());
        Watt_u previousPowerStart{0.0};
        Watt_u previousPowerEnd{0.0};
        auto previousPowerPosition = GetPreviousPosition(event->GetStartTime(), niIt);
        previousPowerStart = previousPowerPosition->second.GetPower();
        previousPowerEnd = GetPreviousPosition(event->GetEndTime(), niIt)->second.GetPower();
        if (const auto rxing = (m_rxing.contains(freqRange) && m_rxing.at(freqRange)); !rxing)
        {
            m_firstPowers.find(band)->second = previousPowerStart;
            // Always leave the first zero power noise event in the list
            niIt->second.erase(++(niIt->second.begin()), ++previousPowerPosition);
        }
        else if (isStartHePortionRxing)
        {
            // When the first HE portion is received, we need to set m_firstPowerPerBand
            // so that it takes into account interferences that arrived between the start of the
            // HE TB PPDU transmission and the start of HE TB payload.
            m_firstPowers.find(band)->second = previousPowerStart;
        }
        auto first =
            AddNiChangeEvent(event->GetStartTime(), NiChange(previousPowerStart, event), niIt);
        auto last = AddNiChangeEvent(event->GetEndTime(), NiChange(previousPowerEnd, event), niIt);
        for (auto i = first; i != last; ++i)
        {
            i->second.AddPower(power);
        }
    }
}

void
InterferenceHelper::UpdateEvent(Ptr<Event> event, const RxPowerWattPerChannelBand& rxPower)
{
    NS_LOG_FUNCTION(this << event);
    // This is called for UL MU events, in order to scale power as long as UL MU PPDUs arrive
    for (const auto& [band, power] : rxPower)
    {
        auto niIt = m_niChanges.find(band);
        NS_ABORT_IF(niIt == m_niChanges.end());
        auto first = GetPreviousPosition(event->GetStartTime(), niIt);
        auto last = GetPreviousPosition(event->GetEndTime(), niIt);
        for (auto i = first; i != last; ++i)
        {
            i->second.AddPower(power);
        }
    }
    event->UpdateRxPowerW(rxPower);
}

double
InterferenceHelper::CalculateSnr(Watt_u signal,
                                 Watt_u noiseInterference,
                                 MHz_u channelWidth,
                                 uint8_t nss) const
{
    NS_LOG_FUNCTION(this << signal << noiseInterference << channelWidth << +nss);
    // thermal noise at 290K in J/s = W
    static const double BOLTZMANN = 1.3803e-23;
    // Nt is the power of thermal noise in W
    const auto Nt = BOLTZMANN * 290 * MHzToHz(channelWidth);
    // receiver noise Floor which accounts for thermal noise and non-idealities of the receiver
    Watt_u noiseFloor{m_noiseFigure * Nt};
    Watt_u noise = noiseFloor + noiseInterference;
    auto snr = signal / noise; // linear scale
    NS_LOG_DEBUG("bandwidth=" << channelWidth << "MHz, signal=" << signal << "W, noise="
                              << noiseFloor << "W, interference=" << noiseInterference
                              << "W, snr=" << RatioToDb(snr) << "dB");
    if (m_errorRateModel->IsAwgn())
    {
        double gain = 1;
        if (m_numRxAntennas > nss)
        {
            gain = static_cast<double>(m_numRxAntennas) /
                   nss; // compute gain offered by diversity for AWGN
        }
        NS_LOG_DEBUG("SNR improvement thanks to diversity: " << 10 * std::log10(gain) << "dB");
        snr *= gain;
    }
    return snr;
}

Watt_u
InterferenceHelper::CalculateNoiseInterferenceW(Ptr<Event> event,
                                                NiChangesPerBand& nis,
                                                const WifiSpectrumBandInfo& band) const
{
    NS_LOG_FUNCTION(this << band);
    auto firstPower_it = m_firstPowers.find(band);
    NS_ABORT_IF(firstPower_it == m_firstPowers.end());
    auto noiseInterference = firstPower_it->second;
    auto niIt = m_niChanges.find(band);
    NS_ABORT_IF(niIt == m_niChanges.end());
    const auto now = Simulator::Now();
    auto it = niIt->second.find(event->GetStartTime());
    const auto muMimoPower = (event->GetPpdu()->GetType() == WIFI_PPDU_TYPE_UL_MU)
                                 ? CalculateMuMimoPowerW(event, band)
                                 : Watt_u{0.0};
    for (; it != niIt->second.end() && it->first < now; ++it)
    {
        if (IsSameMuMimoTransmission(event, it->second.GetEvent()) &&
            (event != it->second.GetEvent()))
        {
            // Do not calculate noiseInterferenceW if events belong to the same MU-MIMO transmission
            // unless this is the same event
            continue;
        }
        noiseInterference = it->second.GetPower() - event->GetRxPower(band) - muMimoPower;
        if (std::abs(noiseInterference) < std::numeric_limits<double>::epsilon())
        {
            // fix some possible rounding issues with double values
            noiseInterference = Watt_u{0.0};
        }
    }
    it = niIt->second.find(event->GetStartTime());
    NS_ABORT_IF(it == niIt->second.end());
    for (; it != niIt->second.end() && it->second.GetEvent() != event; ++it)
    {
        ;
    }
    NiChanges ni;
    ni.emplace(event->GetStartTime(), NiChange(Watt_u{0}, event));
    while (++it != niIt->second.end() && it->second.GetEvent() != event)
    {
        ni.insert(*it);
    }
    ni.emplace(event->GetEndTime(), NiChange(Watt_u{0}, event));
    nis.insert({band, ni});
    NS_ASSERT_MSG(noiseInterference >= Watt_u{0.0},
                  "CalculateNoiseInterferenceW returns negative value " << noiseInterference);
    return noiseInterference;
}

Watt_u
InterferenceHelper::CalculateMuMimoPowerW(Ptr<const Event> event,
                                          const WifiSpectrumBandInfo& band) const
{
    auto niIt = m_niChanges.find(band);
    NS_ASSERT(niIt != m_niChanges.end());
    auto it = niIt->second.begin();
    ++it;
    Watt_u muMimoPower{0.0};
    for (; it != niIt->second.end() && it->first < Simulator::Now(); ++it)
    {
        if (IsSameMuMimoTransmission(event, it->second.GetEvent()))
        {
            auto hePpdu = DynamicCast<HePpdu>(it->second.GetEvent()->GetPpdu()->Copy());
            NS_ASSERT(hePpdu);
            HePpdu::TxPsdFlag psdFlag = hePpdu->GetTxPsdFlag();
            if (psdFlag == HePpdu::PSD_HE_PORTION)
            {
                const auto staId =
                    event->GetPpdu()->GetTxVector().GetHeMuUserInfoMap().cbegin()->first;
                const auto otherStaId = it->second.GetEvent()
                                            ->GetPpdu()
                                            ->GetTxVector()
                                            .GetHeMuUserInfoMap()
                                            .cbegin()
                                            ->first;
                if (staId == otherStaId)
                {
                    break;
                }
                muMimoPower += it->second.GetEvent()->GetRxPower(band);
            }
        }
    }
    return muMimoPower;
}

double
InterferenceHelper::CalculateChunkSuccessRate(double snir,
                                              Time duration,
                                              WifiMode mode,
                                              const WifiTxVector& txVector,
                                              WifiPpduField field) const
{
    if (duration.IsZero())
    {
        return 1.0;
    }
    const auto rate = mode.GetDataRate(txVector.GetChannelWidth());
    auto nbits = static_cast<uint64_t>(rate * duration.GetSeconds());
    const auto csr =
        m_errorRateModel->GetChunkSuccessRate(mode, txVector, snir, nbits, m_numRxAntennas, field);
    return csr;
}

double
InterferenceHelper::CalculatePayloadChunkSuccessRate(double snir,
                                                     Time duration,
                                                     const WifiTxVector& txVector,
                                                     uint16_t staId) const
{
    if (duration.IsZero())
    {
        return 1.0;
    }
    const auto mode = txVector.GetMode(staId);
    const auto rate = mode.GetDataRate(txVector, staId);
    auto nbits = static_cast<uint64_t>(rate * duration.GetSeconds());
    nbits /= txVector.GetNss(staId); // divide effective number of bits by NSS to achieve same chunk
                                     // error rate as SISO for AWGN
    double csr = m_errorRateModel->GetChunkSuccessRate(mode,
                                                       txVector,
                                                       snir,
                                                       nbits,
                                                       m_numRxAntennas,
                                                       WIFI_PPDU_FIELD_DATA,
                                                       staId);
    return csr;
}

double
InterferenceHelper::CalculatePayloadPer(Ptr<const Event> event,
                                        MHz_u channelWidth,
                                        NiChangesPerBand* nis,
                                        const WifiSpectrumBandInfo& band,
                                        uint16_t staId,
                                        std::pair<Time, Time> window) const
{
    NS_LOG_FUNCTION(this << channelWidth << band << staId << window.first << window.second);
    double psr = 1.0; /* Packet Success Rate */
    const auto& niIt = nis->find(band)->second;
    auto j = niIt.cbegin();
    auto previous = j->first;
    Watt_u muMimoPower{0.0};
    const auto payloadMode = event->GetPpdu()->GetTxVector().GetMode(staId);
    auto phyPayloadStart = j->first;
    if (event->GetPpdu()->GetType() != WIFI_PPDU_TYPE_UL_MU &&
        event->GetPpdu()->GetType() !=
            WIFI_PPDU_TYPE_DL_MU) // j->first corresponds to the start of the MU payload
    {
        phyPayloadStart = j->first + WifiPhy::CalculatePhyPreambleAndHeaderDuration(
                                         event->GetPpdu()->GetTxVector());
    }
    else
    {
        muMimoPower = CalculateMuMimoPowerW(event, band);
    }
    const auto windowStart = phyPayloadStart + window.first;
    const auto windowEnd = phyPayloadStart + window.second;
    NS_ABORT_IF(!m_firstPowers.contains(band));
    auto noiseInterference = m_firstPowers.at(band);
    auto power = event->GetRxPower(band);
    while (++j != niIt.cend())
    {
        Time current = j->first;
        NS_LOG_DEBUG("previous= " << previous << ", current=" << current);
        NS_ASSERT(current >= previous);
        const auto snr = CalculateSnr(power,
                                      noiseInterference,
                                      channelWidth,
                                      event->GetPpdu()->GetTxVector().GetNss(staId));
        // Case 1: Both previous and current point to the windowed payload
        if (previous >= windowStart)
        {
            psr *= CalculatePayloadChunkSuccessRate(snr,
                                                    Min(windowEnd, current) - previous,
                                                    event->GetPpdu()->GetTxVector(),
                                                    staId);
            NS_LOG_DEBUG("Both previous and current point to the windowed payload: mode="
                         << payloadMode << ", psr=" << psr);
        }
        // Case 2: previous is before windowed payload and current is in the windowed payload
        else if (current >= windowStart)
        {
            psr *= CalculatePayloadChunkSuccessRate(snr,
                                                    Min(windowEnd, current) - windowStart,
                                                    event->GetPpdu()->GetTxVector(),
                                                    staId);
            NS_LOG_DEBUG(
                "previous is before windowed payload and current is in the windowed payload: mode="
                << payloadMode << ", psr=" << psr);
        }
        noiseInterference = j->second.GetPower() - power;
        if (IsSameMuMimoTransmission(event, j->second.GetEvent()))
        {
            muMimoPower += j->second.GetEvent()->GetRxPower(band);
            NS_LOG_DEBUG("PPDU belongs to same MU-MIMO transmission: muMimoPowerW=" << muMimoPower);
        }
        noiseInterference -= muMimoPower;
        previous = j->first;
        if (previous > windowEnd)
        {
            NS_LOG_DEBUG("Stop: new previous=" << previous
                                               << " after time window end=" << windowEnd);
            break;
        }
    }
    const auto per = 1.0 - psr;
    return per;
}

double
InterferenceHelper::CalculatePhyHeaderSectionPsr(
    Ptr<const Event> event,
    NiChangesPerBand* nis,
    MHz_u channelWidth,
    const WifiSpectrumBandInfo& band,
    PhyEntity::PhyHeaderSections phyHeaderSections) const
{
    NS_LOG_FUNCTION(this << band);
    double psr = 1.0; /* Packet Success Rate */
    auto niIt = nis->find(band)->second;
    auto j = niIt.begin();

    NS_ASSERT(!phyHeaderSections.empty());
    Time stopLastSection;
    for (const auto& section : phyHeaderSections)
    {
        stopLastSection = Max(stopLastSection, section.second.first.second);
    }

    auto previous = j->first;
    NS_ABORT_IF(!m_firstPowers.contains(band));
    auto noiseInterference = m_firstPowers.at(band);
    const auto power = event->GetRxPower(band);
    while (++j != niIt.end())
    {
        auto current = j->first;
        NS_LOG_DEBUG("previous= " << previous << ", current=" << current);
        NS_ASSERT(current >= previous);
        const auto snr = CalculateSnr(power, noiseInterference, channelWidth, 1);
        for (const auto& section : phyHeaderSections)
        {
            const auto start = section.second.first.first;
            const auto stop = section.second.first.second;

            if (previous <= stop || current >= start)
            {
                const auto duration = Min(stop, current) - Max(start, previous);
                if (duration.IsStrictlyPositive())
                {
                    psr *= CalculateChunkSuccessRate(snr,
                                                     duration,
                                                     section.second.second,
                                                     event->GetPpdu()->GetTxVector(),
                                                     section.first);
                    NS_LOG_DEBUG("Current NI change in "
                                 << section.first << " [" << start << ", " << stop << "] for "
                                 << duration.As(Time::NS) << ": mode=" << section.second.second
                                 << ", psr=" << psr);
                }
            }
        }
        noiseInterference = j->second.GetPower() - power;
        previous = j->first;
        if (previous > stopLastSection)
        {
            NS_LOG_DEBUG("Stop: new previous=" << previous << " after stop of last section="
                                               << stopLastSection);
            break;
        }
    }
    return psr;
}

double
InterferenceHelper::CalculatePhyHeaderPer(Ptr<const Event> event,
                                          NiChangesPerBand* nis,
                                          MHz_u channelWidth,
                                          const WifiSpectrumBandInfo& band,
                                          WifiPpduField header) const
{
    NS_LOG_FUNCTION(this << band << header);
    auto niIt = nis->find(band)->second;
    auto phyEntity =
        WifiPhy::GetStaticPhyEntity(event->GetPpdu()->GetTxVector().GetModulationClass());

    PhyEntity::PhyHeaderSections sections;
    for (const auto& section :
         phyEntity->GetPhyHeaderSections(event->GetPpdu()->GetTxVector(), niIt.begin()->first))
    {
        if (section.first == header)
        {
            sections[header] = section.second;
        }
    }

    double psr = 1.0;
    if (!sections.empty())
    {
        psr = CalculatePhyHeaderSectionPsr(event, nis, channelWidth, band, sections);
    }
    return 1 - psr;
}

PhyEntity::SnrPer
InterferenceHelper::CalculatePayloadSnrPer(Ptr<Event> event,
                                           MHz_u channelWidth,
                                           const WifiSpectrumBandInfo& band,
                                           uint16_t staId,
                                           std::pair<Time, Time> relativeMpduStartStop) const
{
    NS_LOG_FUNCTION(this << channelWidth << band << staId << relativeMpduStartStop.first
                         << relativeMpduStartStop.second);
    NiChangesPerBand ni;
    const auto noiseInterference = CalculateNoiseInterferenceW(event, ni, band);
    const auto snr = CalculateSnr(event->GetRxPower(band),
                                  noiseInterference,
                                  channelWidth,
                                  event->GetPpdu()->GetTxVector().GetNss(staId));

    /* calculate the SNIR at the start of the MPDU (located through windowing) and accumulate
     * all SNIR changes in the SNIR vector.
     */
    const auto per =
        CalculatePayloadPer(event, channelWidth, &ni, band, staId, relativeMpduStartStop);

    return PhyEntity::SnrPer(snr, per);
}

double
InterferenceHelper::CalculateSnr(Ptr<Event> event,
                                 MHz_u channelWidth,
                                 uint8_t nss,
                                 const WifiSpectrumBandInfo& band) const
{
    NiChangesPerBand ni;
    const auto noiseInterference = CalculateNoiseInterferenceW(event, ni, band);
    return CalculateSnr(event->GetRxPower(band), noiseInterference, channelWidth, nss);
}

PhyEntity::SnrPer
InterferenceHelper::CalculatePhyHeaderSnrPer(Ptr<Event> event,
                                             MHz_u channelWidth,
                                             const WifiSpectrumBandInfo& band,
                                             WifiPpduField header) const
{
    NS_LOG_FUNCTION(this << band << header);
    NiChangesPerBand ni;
    const auto noiseInterference = CalculateNoiseInterferenceW(event, ni, band);
    const auto snr = CalculateSnr(event->GetRxPower(band), noiseInterference, channelWidth, 1);

    /* calculate the SNIR at the start of the PHY header and accumulate
     * all SNIR changes in the SNIR vector.
     */
    const auto per = CalculatePhyHeaderPer(event, &ni, channelWidth, band, header);

    return PhyEntity::SnrPer(snr, per);
}

InterferenceHelper::NiChanges::iterator
InterferenceHelper::GetNextPosition(Time moment, NiChangesPerBand::iterator niIt) const
{
    return niIt->second.upper_bound(moment);
}

InterferenceHelper::NiChanges::iterator
InterferenceHelper::GetPreviousPosition(Time moment, NiChangesPerBand::iterator niIt) const
{
    // This is safe since there is always an NiChange at time 0, before moment.
    return std::prev(GetNextPosition(moment, niIt));
}

InterferenceHelper::NiChanges::iterator
InterferenceHelper::AddNiChangeEvent(Time moment, NiChange change, NiChangesPerBand::iterator niIt)
{
    return niIt->second.insert(GetNextPosition(moment, niIt), {moment, change});
}

void
InterferenceHelper::NotifyRxStart(const FrequencyRange& freqRange)
{
    NS_LOG_FUNCTION(this << freqRange);
    m_rxing[freqRange] = true;
}

void
InterferenceHelper::NotifyRxEnd(Time endTime, const FrequencyRange& freqRange)
{
    NS_LOG_FUNCTION(this << endTime << freqRange);
    m_rxing.at(freqRange) = false;
    // Update m_firstPowers for frame capture
    for (auto niIt = m_niChanges.begin(); niIt != m_niChanges.end(); ++niIt)
    {
        if (!IsBandInFrequencyRange(niIt->first, freqRange))
        {
            continue;
        }
        NS_ASSERT(niIt->second.size() > 1);
        auto it = std::prev(GetPreviousPosition(endTime, niIt));
        m_firstPowers.find(niIt->first)->second = it->second.GetPower();
    }
}

bool
InterferenceHelper::IsBandInFrequencyRange(const WifiSpectrumBandInfo& band,
                                           const FrequencyRange& freqRange) const
{
    return std::all_of(band.frequencies.cbegin(),
                       band.frequencies.cend(),
                       [&freqRange](const auto& freqs) {
                           return ((freqs.second > MHzToHz(freqRange.minFrequency)) &&
                                   (freqs.first < MHzToHz(freqRange.maxFrequency)));
                       });
}

bool
InterferenceHelper::IsSameMuMimoTransmission(Ptr<const Event> currentEvent,
                                             Ptr<const Event> otherEvent) const
{
    if ((currentEvent->GetPpdu()->GetType() == WIFI_PPDU_TYPE_UL_MU) &&
        (otherEvent->GetPpdu()->GetType() == WIFI_PPDU_TYPE_UL_MU) &&
        (currentEvent->GetPpdu()->GetUid() == otherEvent->GetPpdu()->GetUid()))
    {
        const auto currentTxVector = currentEvent->GetPpdu()->GetTxVector();
        const auto otherTxVector = otherEvent->GetPpdu()->GetTxVector();
        NS_ASSERT(currentTxVector.GetHeMuUserInfoMap().size() == 1);
        NS_ASSERT(otherTxVector.GetHeMuUserInfoMap().size() == 1);
        const auto currentUserInfo = currentTxVector.GetHeMuUserInfoMap().cbegin();
        const auto otherUserInfo = otherTxVector.GetHeMuUserInfoMap().cbegin();
        return (currentUserInfo->second.ru == otherUserInfo->second.ru);
    }
    return false;
}

} // namespace ns3
